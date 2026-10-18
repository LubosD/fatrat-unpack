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

#ifndef RARUNPACKER_H
#define RARUNPACKER_H
#include "Unpacker.h"
#include <QByteArray>
#include <QDir>
#include <QFile>

class RarUnpacker : public Unpacker
{
public:
	RarUnpacker(const QString& file, UnpackUi& ui);

	static Unpacker* create(const QString& file, UnpackUi& ui) { return new RarUnpacker(file, ui); }
	static bool supported(const QString& file);

	QList<FileEntry> list() override;
	void extract(const QList<bool>& files, const QString& where) override;
	void pipe(int fileIndex, QIODevice* device) override;
private:
	static QString processFileName(QString name);
	static int rarCallback(unsigned msg, long tthis, long p1, long p2);
	static QString rarError(int code);
	int findFile(const QString& file) const;
	void runExtraction();
	bool piping() const { return m_nFileIndex != -1; }
private:
	QByteArray m_strPassword;
	QDir m_dirDestination;

	QList<FileEntry> m_files;
	QList<bool> m_filesValues;
	qint64 m_nTotal = 0, m_nDone = 0;
	int m_nPercents = -1;
	int m_nSelectedTotal = 0, m_nSelectedDone = 0;
	QFile m_file;
	QString m_strCommentArchive;

	// Set by rarCallback(), consumed once control is back in normal (non-callback) code.
	bool m_bNoAnswer = false;

	// for piping only
	QIODevice* m_pipeDevice = nullptr;
	int m_nFileIndex = -1;
};

#endif
