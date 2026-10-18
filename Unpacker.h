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

#ifndef UNPACKER_H
#define UNPACKER_H
#include <QCoreApplication>
#include <QIODevice>
#include <QList>
#include <QString>
#include <optional>

#include "UnpackUi.h"

struct FileEntry
{
	QString name;
	qint64 size;
};

/* Base of the unpackers (RarUnpacker, ArchiveUnpacker, FileConcater). An
 * Unpacker is driven synchronously, start to finish, by UnpackTask on its own
 * worker thread: list() to see what is inside, then extract() or pipe().
 *
 * Every method may throw a QString (an error message for Task::finish()) or
 * NoAnswer (a required question went unanswered, see UnpackUi.h).
 */
class Unpacker
{
	Q_DECLARE_TR_FUNCTIONS(Unpacker)
public:
	Unpacker(const QString& file, UnpackUi& ui) : m_ui(ui), m_strFile(file) {}
	virtual ~Unpacker() = default;

	virtual QList<FileEntry> list() = 0;
	virtual void extract(const QList<bool>& files, const QString& where) = 0;
	// fileIndex is an index into list()'s result. The device is really a
	// QProcess's stdin, handed over as a QIODevice so Unpacker subclasses
	// don't need to know about QProcess at all.
	virtual void pipe(int fileIndex, QIODevice* device) = 0;

	// UnpackTask calls this right after construction, before list(). Kept out
	// of the constructor (and so out of UnpackerEntry::pCreate) because it's
	// only ever used for the password form.
	void setTransferComment(const QString& comment) { m_strTransferComment = comment; }

protected:
	bool cancelled() const { return m_ui.cancelled(); }
	void setProgress(int percent) { m_ui.setProgress(percent); }
	void setStatus(const QString& text) { m_ui.setStatus(text); }

	// Asks for a password. `wrongBefore` shows "The password is incorrect."
	// (the caller already tried one). Throws NoAnswer if the user gives none.
	QByteArray askPassword(const QString& archiveComment, bool wrongBefore);

	// Writes `length` bytes to `device` (really the pipe command's stdin),
	// blocking while more than 4MB is still unwritten. Returns false once
	// writing fails or the task is cancelled while waiting. UnpackTask owns
	// the QProcess itself and closes its write channel once pipe() returns.
	bool writeToPipe(QIODevice* device, const char* data, qint64 length);

	UnpackUi& m_ui;
	QString m_strFile;
	QString m_strTransferComment;
};

#endif
