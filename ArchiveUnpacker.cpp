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
#include <QFile>
#include <QFileInfo>
#include <archive.h>
#include <archive_entry.h>

ArchiveUnpacker::ArchiveUnpacker(const QString& file, UnpackUi& ui)
	: Unpacker(file, ui)
{
}

bool ArchiveUnpacker::supported(const QString& file)
{
	Q_UNUSED(file);
	return true;
}

QString ArchiveUnpacker::archiveError(struct archive* a)
{
	const char* str = archive_error_string(a);
	if (!str)
		return tr("Cannot read the archive.");
	return QString::fromLocal8Bit(str);
}

QString ArchiveUnpacker::entryName(struct archive_entry* entry)
{
	const char* name = archive_entry_pathname_utf8(entry);
	if (name)
		return QString::fromUtf8(name);
	return QFile::decodeName(archive_entry_pathname(entry));
}

// Refuse names that would end up outside of the destination directory.
bool ArchiveUnpacker::isSafePath(const QString& name)
{
	if (name.isEmpty() || QDir::isAbsolutePath(name))
		return false;

	const QStringList parts = name.split('/');
	for (const QString& part : parts)
	{
		if (part == "..")
			return false;
	}
	return true;
}

struct archive* ArchiveUnpacker::openArchive(bool askPasswords)
{
	struct archive* a = archive_read_new();
	archive_read_support_format_7zip(a);
	archive_read_support_format_zip_seekable(a);

	if (askPasswords)
		archive_read_set_passphrase_callback(a, this, passphraseCallback);

	if (archive_read_open_filename(a, QFile::encodeName(m_strFile).constData(), 64 * 1024) != ARCHIVE_OK)
	{
		QString error = archiveError(a);
		archive_read_free(a);
		throw error;
	}
	return a;
}

QList<FileEntry> ArchiveUnpacker::list()
{
	struct archive* a = openArchive(false);

	try
	{
		for (int index = 0;; index++)
		{
			struct archive_entry* entry;
			int r = archive_read_next_header(a, &entry);

			if (r == ARCHIVE_EOF)
				break;
			else if (r < ARCHIVE_WARN)
				throw archiveError(a);

			if (archive_entry_filetype(entry) != AE_IFREG)
				continue;

			FileEntry file;
			file.name = entryName(entry);
			file.size = archive_entry_size_is_set(entry) ? archive_entry_size(entry) : 0;

			if (!isSafePath(file.name))
				continue; // an archive entry with an unsafe name

			// libarchive can list encrypted 7z archives, but not extract them.
			if (archive_entry_is_data_encrypted(entry) && archive_format(a) == ARCHIVE_FORMAT_7ZIP)
				throw tr("Encrypted 7z archives are not supported.");

			m_files << file;
			m_entryIndexes << index;
			m_mtimes << (archive_entry_mtime_is_set(entry) ? QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry)) : QDateTime());
		}
	}
	catch (...)
	{
		archive_read_free(a);
		throw;
	}
	archive_read_free(a);

	if (m_files.isEmpty())
		throw tr("The archive contains no files.");
	return m_files;
}

// Called by libarchive, possibly more than once per open (it retries the
// passphrase itself on a wrong one). May not throw: see RarUnpacker::rarCallback.
const char* ArchiveUnpacker::passphraseCallback(struct archive* a, void* data)
{
	Q_UNUSED(a);
	ArchiveUnpacker* This = static_cast<ArchiveUnpacker*>(data);

	if (This->cancelled())
		return nullptr;

	This->m_nPasswordAttempts++;
	try
	{
		This->m_strPassword = This->askPassword(QString(), This->m_nPasswordAttempts > 1);
	}
	catch (const NoAnswer&)
	{
		This->m_bNoAnswer = true;
		return nullptr;
	}

	if (This->m_strPassword.isEmpty())
		return nullptr;
	return This->m_strPassword.constData();
}

void ArchiveUnpacker::extract(const QList<bool>& files, const QString& where)
{
	m_filesValues = files;
	m_dirDestination = QDir(where);
	m_nFileIndex = -1;
	runExtraction();
}

void ArchiveUnpacker::pipe(int fileIndex, QIODevice* device)
{
	m_pipeDevice = device;
	m_nFileIndex = fileIndex;
	runExtraction();
}

void ArchiveUnpacker::extractEntry(struct archive* a, int i)
{
	QFile file;
	char buffer[64 * 1024];

	if (!piping())
	{
		QString fpath = m_dirDestination.filePath(m_files[i].name);
		m_dirDestination.mkpath(QFileInfo(m_files[i].name).path());

		file.setFileName(fpath);
		if (!file.open(QIODevice::WriteOnly))
			throw tr("Unable to open %1 for writing.").arg(fpath);

		setStatus(tr("%1 (%2 of %3)").arg(m_files[i].name).arg(++m_nSelectedDone).arg(m_nSelectedTotal));
	}
	else
		setStatus(tr("Piping %1...").arg(m_files[i].name));

	try
	{
		while (true)
		{
			la_ssize_t rd = archive_read_data(a, buffer, sizeof(buffer));
			if (rd < 0)
				throw archiveError(a);
			if (rd == 0)
				break;
			if (cancelled())
				throw QString();

			if (piping())
			{
				if (!writeToPipe(m_pipeDevice, buffer, rd))
					throw tr("The command has terminated.");
			}
			else if (file.write(buffer, rd) != rd)
				throw tr("Unable to write the extracted data.");

			m_nDone += rd;

			if (m_nTotal)
			{
				int pcts = 100 * m_nDone / m_nTotal;
				if (pcts > m_nPercents)
					setProgress(m_nPercents = pcts);
			}
		}
	}
	catch (...)
	{
		if (file.isOpen())
			file.remove();
		throw;
	}

	if (!piping())
	{
		file.flush();
		if (m_mtimes[i].isValid())
			file.setFileTime(m_mtimes[i], QFileDevice::FileModificationTime);
	}
}

void ArchiveUnpacker::runExtraction()
{
	int last = -1;
	m_nSelectedTotal = 0;
	for (int i = 0; i < m_files.size(); i++)
	{
		if (piping() ? i == m_nFileIndex : m_filesValues[i])
		{
			last = i;
			m_nSelectedTotal++;
		}
	}

	// Skipping over a file in a solid 7z archive means decompressing it, so
	// the files skipped on the way to the last selected one count too.
	m_nTotal = 0;
	for (int i = 0; i <= last; i++)
		m_nTotal += m_files[i].size;
	m_nDone = 0;
	m_nPercents = -1;
	m_nSelectedDone = 0;

	m_nPasswordAttempts = 0;
	m_bNoAnswer = false;

	struct archive* a = openArchive(true);

	try
	{
		for (int index = 0, i = 0; i <= last; index++)
		{
			struct archive_entry* entry;
			int r = archive_read_next_header(a, &entry);

			if (r == ARCHIVE_EOF)
				throw tr("The archive is truncated or has changed.");
			else if (r < ARCHIVE_WARN)
			{
				if (m_bNoAnswer)
					throw NoAnswer{};
				throw archiveError(a);
			}

			if (index != m_entryIndexes[i])
				continue;

			if (piping() ? i == m_nFileIndex : m_filesValues[i])
				extractEntry(a, i);
			else
			{
				if (archive_read_data_skip(a) < ARCHIVE_WARN)
				{
					if (m_bNoAnswer)
						throw NoAnswer{};
					throw archiveError(a);
				}
				m_nDone += m_files[i].size;
			}

			i++;
		}
	}
	catch (...)
	{
		archive_read_free(a);
		throw;
	}

	archive_read_free(a);
	if (m_bNoAnswer)
		throw NoAnswer{};
}
