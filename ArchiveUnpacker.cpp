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

#include "ArchiveUnpacker.h"
#include "PasswordDlg.h"
#include <QFile>
#include <QFileInfo>
#include <QtDebug>
#include <archive.h>
#include <archive_entry.h>

ArchiveUnpacker::ArchiveUnpacker(QString file, QString transferComment)
	: Unpacker(file), m_strCommentTransfer(transferComment), m_nTotal(0), m_nDone(0),
		m_nPercents(0), m_nFileIndex(-1)
{
	processArchive();
}

ArchiveUnpacker::~ArchiveUnpacker()
{
}

bool ArchiveUnpacker::supported(QString file)
{
	return true;
}

QString ArchiveUnpacker::archiveError(struct archive* a)
{
	const char* str = archive_error_string(a);
	if(!str)
		return tr("Cannot read the archive.");
	return QString::fromLocal8Bit(str);
}

QString ArchiveUnpacker::entryName(struct archive_entry* entry)
{
	const char* name = archive_entry_pathname_utf8(entry);
	if(name)
		return QString::fromUtf8(name);
	return QFile::decodeName(archive_entry_pathname(entry));
}

// Refuse names that would end up outside of the destination directory
bool ArchiveUnpacker::isSafePath(const QString& name)
{
	if(name.isEmpty() || QDir::isAbsolutePath(name))
		return false;

	const QStringList parts = name.split('/');
	for(const QString& part : parts)
	{
		if(part == "..")
			return false;
	}
	return true;
}

struct archive* ArchiveUnpacker::openArchive(bool askPasswords)
{
	struct archive* a = archive_read_new();
	archive_read_support_format_7zip(a);
	archive_read_support_format_zip_seekable(a);

	if(askPasswords)
		archive_read_set_passphrase_callback(a, this, passphraseCallback);

	if(archive_read_open_filename(a, QFile::encodeName(m_strFile).constData(), 64*1024) != ARCHIVE_OK)
	{
		QString error = archiveError(a);
		archive_read_free(a);
		throw error;
	}
	return a;
}

void ArchiveUnpacker::processArchive()
{
	struct archive* a = 0;

	try
	{
		a = openArchive(false);

		for(int index = 0; ; index++)
		{
			struct archive_entry* entry;
			int r = archive_read_next_header(a, &entry);

			if(r == ARCHIVE_EOF)
				break;
			else if(r < ARCHIVE_WARN)
				throw archiveError(a);

			if(archive_entry_filetype(entry) != AE_IFREG)
				continue;

			FileEntry file;
			file.name = entryName(entry);
			file.size = archive_entry_size_is_set(entry) ? archive_entry_size(entry) : 0;

			if(!isSafePath(file.name))
			{
				qWarning() << "Skipping an archive entry with an unsafe name:" << file.name;
				continue;
			}

			// libarchive can list encrypted 7z archives, but not extract them
			if(archive_entry_is_data_encrypted(entry) && archive_format(a) == ARCHIVE_FORMAT_7ZIP)
				throw tr("Encrypted 7z archives are not supported.");

			m_files << file;
			m_entryIndexes << index;
			m_mtimes << (archive_entry_mtime_is_set(entry) ? QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry)) : QDateTime());
		}

		archive_read_free(a);
		a = 0;

		if(m_files.isEmpty())
			throw tr("The archive contains no files.");

		runDialog(m_files);
	}
	catch(QString error)
	{
		if(a)
			archive_read_free(a);

		showError(error);
		deleteLater();
	}
}

void ArchiveUnpacker::askPassword(QByteArray* out)
{
	PasswordDlg dlg;
	dlg.m_strCommentTransfer = m_strCommentTransfer;

	if(dlg.exec() == QDialog::Accepted)
		*out = dlg.m_strPassword.toUtf8();
}

// Called from the extraction thread whenever libarchive needs another password
// to try; libarchive keeps the ones it was given for the remaining entries
const char* ArchiveUnpacker::passphraseCallback(struct archive* a, void* data)
{
	ArchiveUnpacker* This = static_cast<ArchiveUnpacker*>(data);
	QByteArray pass;

	if(This->m_bAbort)
		return 0;

	QMetaObject::invokeMethod(This, "askPassword", Qt::BlockingQueuedConnection, Q_ARG(QByteArray*, &pass));
	if(pass.isEmpty())
		return 0;

	This->m_strPassword = pass;
	return This->m_strPassword.constData();
}

void ArchiveUnpacker::extract(QList<bool> files, QString where)
{
	m_filesValues = files;
	m_dirDestination = QDir(where);
	start();
}

void ArchiveUnpacker::pipe(int fileIndex, QProcess* process)
{
	m_pipe = process;
	m_nFileIndex = fileIndex;
	start();
}

void ArchiveUnpacker::extractEntry(struct archive* a, int i)
{
	QFile file;
	char buffer[64*1024];
	int percents = -1;
	qint64 pos = 0;

	if(!piping())
	{
		QString fpath = m_dirDestination.filePath(m_files[i].name);
		m_dirDestination.mkpath(QFileInfo(m_files[i].name).path());

		file.setFileName(fpath);
		if(!file.open(QIODevice::WriteOnly))
			throw tr("Unable to open %1 for writing.").arg(fpath);

		setToolTip(tr("Extracting %1...").arg(m_files[i].name));
	}
	else
		setToolTip(tr("Piping %1...").arg(m_files[i].name));

	try
	{
		while(true)
		{
			la_ssize_t rd = archive_read_data(a, buffer, sizeof(buffer));
			if(rd < 0)
				throw archiveError(a);
			if(rd == 0)
				break;
			if(m_bAbort)
				throw QString();

			if(piping())
			{
				if(!writeToPipe(m_pipe, buffer, rd))
					throw m_bAbort ? QString() : tr("The command has terminated.");
			}
			else if(file.write(buffer, rd) != rd)
				throw tr("Unable to write the extracted data.");

			pos += rd;
			m_nDone += rd;

			if(m_nTotal)
			{
				int pcts = 100*m_nDone/m_nTotal;
				if(pcts > m_nPercents)
					setProgress(m_nPercents = pcts);
			}
			if(m_files[i].size)
			{
				int pcts = 100*pos/m_files[i].size;
				if(pcts > percents)
					setFileProgress(i, percents = pcts);
			}
		}
	}
	catch(...)
	{
		// Don't leave a truncated file behind
		if(file.isOpen())
			file.remove();
		throw;
	}

	if(!piping())
	{
		file.flush();
		if(m_mtimes[i].isValid())
			file.setFileTime(m_mtimes[i], QFileDevice::FileModificationTime);
	}
}

void ArchiveUnpacker::run()
{
	struct archive* a = 0;

	try
	{
		int last = -1;
		for(int i=0;i<m_files.size();i++)
		{
			if(piping() ? i == m_nFileIndex : m_filesValues[i])
				last = i;
		}

		// Skipping over a file in a solid 7z archive means decompressing it,
		// so the files skipped on the way to the last selected one count too
		m_nTotal = 0;
		for(int i=0;i<=last;i++)
			m_nTotal += m_files[i].size;

		a = openArchive(true);

		for(int index = 0, i = 0; i <= last; index++)
		{
			struct archive_entry* entry;
			int r = archive_read_next_header(a, &entry);

			if(r == ARCHIVE_EOF)
				throw tr("The archive is truncated or has changed.");
			else if(r < ARCHIVE_WARN)
				throw archiveError(a);

			if(index != m_entryIndexes[i])
				continue;

			if(piping() ? i == m_nFileIndex : m_filesValues[i])
				extractEntry(a, i);
			else
			{
				if(archive_read_data_skip(a) < ARCHIVE_WARN)
					throw archiveError(a);
				m_nDone += m_files[i].size;
			}

			if(m_bAbort)
				throw QString();
			i++;
		}

		setProgress(100);
	}
	catch(QString error)
	{
		if(!error.isEmpty())
			QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection, Q_ARG(QString, error));
	}

	if(a)
		archive_read_free(a);

	if(piping())
		closePipe(m_pipe);

	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}
