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

#ifndef ARCHIVEUNPACKER_H
#define ARCHIVEUNPACKER_H
#include "Unpacker.h"
#include "unpack_config.h"

#ifndef WITH_LIBARCHIVE
#	error This file is not supported to be included!
#endif

#include <QByteArray>
#include <QDateTime>
#include <QDir>

struct archive;
struct archive_entry;

// 7z and ZIP archives, read through libarchive.
class ArchiveUnpacker : public Unpacker
{
public:
	ArchiveUnpacker(const QString& file, UnpackUi& ui);

	static Unpacker* create(const QString& file, UnpackUi& ui) { return new ArchiveUnpacker(file, ui); }
	static bool supported(const QString& file);

	QList<FileEntry> list() override;
	void extract(const QList<bool>& files, const QString& where) override;
	void pipe(int fileIndex, QIODevice* device) override;
private:
	struct archive* openArchive(bool askPasswords);
	void runExtraction();
	void extractEntry(struct archive* a, int i);
	static QString archiveError(struct archive* a);
	static QString entryName(struct archive_entry* entry);
	static bool isSafePath(const QString& name);
	static const char* passphraseCallback(struct archive* a, void* data);
	bool piping() const { return m_nFileIndex != -1; }
private:
	QDir m_dirDestination;
	QByteArray m_strPassword;

	QList<FileEntry> m_files;
	// For each file in m_files, its position among all the archive's entries.
	QList<int> m_entryIndexes;
	QList<QDateTime> m_mtimes;
	QList<bool> m_filesValues;
	qint64 m_nTotal = 0, m_nDone = 0;
	int m_nPercents = -1;
	int m_nSelectedTotal = 0, m_nSelectedDone = 0;
	// How many passphraseCallback() calls happened for the current open; > 1
	// means the previous one was wrong.
	int m_nPasswordAttempts = 0;
	bool m_bNoAnswer = false;

	// for piping only
	QIODevice* m_pipeDevice = nullptr;
	int m_nFileIndex = -1;
};

#endif
