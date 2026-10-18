/*
FatRat download manager
http://fatrat.dolezel.info

Copyright (C) 2006-2026 Lubos Dolezel <lubos a dolezel.info>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "RarUnpacker.h"

#include <QRegularExpression>
#include <cstring>

#define _UNIX
#include "unrar/dll.hpp"

static const QRegularExpression g_re("\\.part(\\d+)");

RarUnpacker::RarUnpacker(const QString& file, UnpackUi& ui)
	: Unpacker(processFileName(file), ui)
{
}

bool RarUnpacker::supported(const QString& file)
{
	QRegularExpressionMatch m = g_re.match(file);
	if (!m.hasMatch())
		return true;
	else
		return m.captured(1).toInt() == 1;
}

QString RarUnpacker::processFileName(QString file)
{
	int x = g_re.match(file).capturedStart();
	if (x != -1)
	{
		int i;
		for (i = x + 5; file[i].isDigit(); i++)
			file[i] = '0';
		file[i - 1] = '1';
	}
	return file;
}

QString RarUnpacker::rarError(int code)
{
	switch (code)
	{
	case ERAR_NO_MEMORY:
		return tr("Not enough memory.");
	case ERAR_BAD_DATA:
		return tr("The archive is corrupt.");
	case ERAR_BAD_ARCHIVE:
	case ERAR_UNKNOWN_FORMAT:
		return tr("This is not a supported RAR archive.");
	case ERAR_EOPEN:
		return tr("Unable to open the archive.");
	case ERAR_ECREATE:
	case ERAR_EWRITE:
		return tr("Unable to write the extracted data.");
	case ERAR_MISSING_PASSWORD:
		return tr("The archive is encrypted and no password was given.");
	case ERAR_BAD_PASSWORD:
		return tr("The password is incorrect.");
	case ERAR_LARGE_DICT:
		return tr("The archive needs a dictionary too large for this system.");
	default:
		return tr("Cannot read the archive: %1.").arg(code);
	}
}

// Called by the unrar dll. May not throw: it runs on top of the dll's own
// call stack, which doesn't expect C++ exceptions crossing it. Anything it
// can't handle on the spot (no password given) is recorded on This and
// re-raised once control is back in ordinary code.
int RarUnpacker::rarCallback(unsigned msg, long tthis, long p1, long p2)
{
	char* target = (char*) p1;
	RarUnpacker* This = (RarUnpacker*) tthis;

	if (This->cancelled())
		return -1;

	if (msg == UCM_NEEDPASSWORD)
	{
		QByteArray pass;
		try
		{
			pass = This->askPassword(This->m_strCommentArchive, !This->m_strPassword.isEmpty());
		}
		catch (const NoAnswer&)
		{
			This->m_bNoAnswer = true;
			return -1;
		}

		This->m_strPassword = pass;
		if (pass.isEmpty())
			return -1;

		strncpy(target, pass.constData(), p2 - 1);
		target[p2 - 1] = 0;
	}
	else if (msg == UCM_PROCESSDATA)
	{
		bool ok;
		if (!This->piping())
		{
			ok = This->m_file.write(target, p2) == p2;
		}
		else
			ok = This->writeToPipe(This->m_pipeDevice, target, p2);

		if (!ok)
			return -1;

		This->m_nDone += p2;
		if (This->m_nTotal)
		{
			int pcts = 100 * This->m_nDone / This->m_nTotal;
			if (pcts > This->m_nPercents)
				This->setProgress(This->m_nPercents = pcts);
		}
		return 1;
	}
	else if (msg == UCM_CHANGEVOLUME)
	{
		if (p2 == RAR_VOL_ASK)
			return -1; // The volume is missing; fail with ERAR_EOPEN or similar.
	}

	return 0;
}

int RarUnpacker::findFile(const QString& file) const
{
	for (int i = 0; i < m_files.size(); i++)
	{
		if (file == m_files[i].name)
			return i;
	}
	return -1;
}

QList<FileEntry> RarUnpacker::list()
{
	while (true)
	{
		RAROpenArchiveData oa;
		memset(&oa, 0, sizeof(oa));

		QByteArray ba = m_strFile.toUtf8();
		char comment[4 * 1024] = "";

		oa.ArcName = (char*) ba.constData();
		oa.OpenMode = RAR_OM_LIST;
		oa.CmtBuf = comment;
		oa.CmtBufSize = sizeof(comment);

		HANDLE handle = RAROpenArchive(&oa);
		if (!handle)
			throw rarError(oa.OpenResult);

		RARSetCallback(handle, rarCallback, (LPARAM) this);
		if (!m_strPassword.isEmpty())
			RARSetPassword(handle, m_strPassword.data());

		m_bNoAnswer = false;
		QList<FileEntry> files;
		int err = ERAR_SUCCESS;

		while (true)
		{
			RARHeaderDataEx hd;
			memset(&hd, 0, sizeof(hd));

			int r = RARReadHeaderEx(handle, &hd);
			if (r == ERAR_END_ARCHIVE)
				break;
			if (r == ERAR_BAD_PASSWORD || r == ERAR_MISSING_PASSWORD)
			{
				err = r;
				break;
			}
			if (r != 0)
			{
				RARCloseArchive(handle);
				if (m_bNoAnswer)
					throw NoAnswer{};
				throw rarError(r);
			}

			FileEntry file;
			file.name = QString::fromWCharArray(hd.FileNameW);
			file.size = hd.UnpSize + (qint64(hd.UnpSizeHigh) << 32);
			files << file;

			RARProcessFile(handle, RAR_SKIP, 0, 0);
		}

		if (err == ERAR_SUCCESS && (oa.CmtState == 1 || oa.CmtState == ERAR_SMALL_BUF))
			m_strCommentArchive = QString::fromUtf8(comment, oa.CmtSize);

		RARCloseArchive(handle);

		if (m_bNoAnswer)
			throw NoAnswer{};

		if (err == ERAR_SUCCESS)
		{
			m_files = files;
			return files;
		}

		// Reaching this line at all means a password was already tried (via
		// the callback above, or a previous spin of this loop) and was wrong.
		m_strPassword = askPassword(m_strCommentArchive, true);
	}
}

void RarUnpacker::extract(const QList<bool>& files, const QString& where)
{
	m_dirDestination = QDir(where);
	m_filesValues = files;
	m_nFileIndex = -1;

	m_nTotal = 0;
	m_nSelectedTotal = 0;
	for (int i = 0; i < m_files.size(); i++)
	{
		if (m_filesValues[i])
		{
			m_nTotal += m_files[i].size;
			m_nSelectedTotal++;
		}
	}
	m_nDone = 0;
	m_nPercents = -1;
	m_nSelectedDone = 0;

	runExtraction();
}

void RarUnpacker::pipe(int fileIndex, QIODevice* device)
{
	m_pipeDevice = device;
	m_nFileIndex = fileIndex;
	m_nTotal = m_files[fileIndex].size;
	m_nDone = 0;
	m_nPercents = -1;

	runExtraction();
}

void RarUnpacker::runExtraction()
{
	while (true)
	{
		RAROpenArchiveData oa;
		memset(&oa, 0, sizeof(oa));
		QByteArray ba = m_strFile.toUtf8();
		oa.ArcName = (char*) ba.constData();
		oa.OpenMode = RAR_OM_EXTRACT;

		HANDLE handle = RAROpenArchive(&oa);
		if (!handle)
			throw rarError(oa.OpenResult);

		// An empty password would count as a given one and suppress the prompt.
		if (!m_strPassword.isEmpty())
			RARSetPassword(handle, m_strPassword.data());
		RARSetCallback(handle, rarCallback, (LPARAM) this);
		m_bNoAnswer = false;

		int e = ERAR_SUCCESS;
		try
		{
			if (!piping())
			{
				while (true)
				{
					RARHeaderDataEx hd;
					memset(&hd, 0, sizeof(hd));
					e = RARReadHeaderEx(handle, &hd);
					if (e == ERAR_END_ARCHIVE)
					{
						e = ERAR_SUCCESS;
						break;
					}
					else if (e)
						break;

					int i = findFile(QString::fromWCharArray(hd.FileNameW));
					if (i == -1 || !m_filesValues[i] || !m_files[i].size)
					{
						RARProcessFile(handle, RAR_SKIP, 0, 0);
						continue;
					}

					QString fpath = m_dirDestination.filePath(m_files[i].name);
					m_file.setFileName(fpath);

					int l = m_files[i].name.lastIndexOf('/');
					if (l != -1)
						m_dirDestination.mkpath(m_files[i].name.left(l));

					if (!m_file.open(QIODevice::WriteOnly))
						throw tr("Unable to open %1 for writing.").arg(fpath);

					setStatus(tr("%1 (%2 of %3)").arg(m_files[i].name).arg(++m_nSelectedDone).arg(m_nSelectedTotal));

					e = RARProcessFile(handle, RAR_TEST, 0, 0);
					m_file.close();

					if (cancelled())
					{
						QFile::remove(fpath);
						throw QString();
					}
					if (e == ERAR_BAD_PASSWORD || e == ERAR_MISSING_PASSWORD)
					{
						QFile::remove(fpath);
						break;
					}
					if (e)
					{
						QFile::remove(fpath);
						throw rarError(e);
					}
				}
			}
			else
			{
				for (int i = 0; i < m_files.size(); i++)
				{
					RARHeaderDataEx hd;
					memset(&hd, 0, sizeof(hd));
					e = RARReadHeaderEx(handle, &hd);
					if (e)
						break;
					if (i != m_nFileIndex)
					{
						RARProcessFile(handle, RAR_SKIP, 0, 0);
						continue;
					}

					setStatus(tr("Piping %1...").arg(m_files[i].name));
					e = RARProcessFile(handle, RAR_TEST, 0, 0);

					if (cancelled())
						throw QString();
					if (e && e != ERAR_BAD_PASSWORD && e != ERAR_MISSING_PASSWORD)
						throw rarError(e);
					break;
				}
			}
		}
		catch (...)
		{
			RARCloseArchive(handle);
			throw;
		}

		RARCloseArchive(handle);

		if (m_bNoAnswer)
			throw NoAnswer{};

		if (e == ERAR_SUCCESS)
			return;
		if (e != ERAR_BAD_PASSWORD && e != ERAR_MISSING_PASSWORD)
			throw rarError(e);

		// Reaching this line at all means a password was already tried and was wrong.
		m_strPassword = askPassword(m_strCommentArchive, true);
		if (!piping())
		{
			m_nDone = 0;
			m_nPercents = -1;
			m_nSelectedDone = 0;
		}
	}
}
