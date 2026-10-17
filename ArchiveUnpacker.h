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
#include <QPointer>
#include <QProcess>

struct archive;
struct archive_entry;

// 7z and ZIP archives, read through libarchive
class ArchiveUnpacker : public Unpacker
{
Q_OBJECT
public:
	ArchiveUnpacker(QString file, QString transferComment);
	virtual ~ArchiveUnpacker();

	static QThread* create(QString file, QString transferComment) { return new ArchiveUnpacker(file, transferComment); }
	static bool supported(QString file);

	virtual void run();
	virtual void extract(QList<bool> files, QString where);
	virtual void pipe(int fileIndex, QProcess* process);
private slots:
	void askPassword(QByteArray* out);
private:
	void processArchive();
	struct archive* openArchive(bool askPasswords);
	void extractEntry(struct archive* a, int i);
	static QString archiveError(struct archive* a);
	static QString entryName(struct archive_entry* entry);
	static bool isSafePath(const QString& name);
	static const char* passphraseCallback(struct archive* a, void* data);
	void writeToPipe(const char* data, qint64 length);
	bool piping() const { return m_nFileIndex != -1; }
private:
	QString m_strCommentTransfer;
	QDir m_dirDestination;
	QByteArray m_strPassword;

	QList<FileEntry> m_files;
	// For each file in m_files, its position among all the archive's entries
	QList<int> m_entryIndexes;
	QList<QDateTime> m_mtimes;
	QList<bool> m_filesValues;
	qint64 m_nTotal, m_nDone;
	int m_nPercents;

	// for piping only
	// QProcess deletes itself when the command exits
	QPointer<QProcess> m_pipe;
	int m_nFileIndex;
};

#endif
