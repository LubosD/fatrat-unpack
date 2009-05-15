/*
FatRat download manager
http://fatrat.dolezel.info

Copyright (C) 2006-2009 Lubos Dolezel <lubos a dolezel.info>

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

#ifndef _ZIPUNPACKER_H
#define _ZIPUNPACKER_H
#include "Unpacker.h"
#include "config.h"

#ifndef WITH_ZIP
#	error This file is not supported to be included!
#endif

#include <zip.h>
#include <QDir>

class ZipUnpacker : public Unpacker
{
public:
	ZipUnpacker(QString file, QString transferComment);
	virtual ~ZipUnpacker();
	
	static QThread* create(QString file, QString transferComment) { return new ZipUnpacker(file, transferComment); }
	static bool supported(QString file);
	
	virtual void run();
	virtual void extract(QList<bool> files, QString where);
	virtual void pipe(int fileIndex, QProcess* process);
private:
	void processArchive();
	static QString zipError(int code, int scode);
private:
	QString m_strTransferComment, m_strArchiveComment;
	QDir m_dirDestination;
	qint64 m_nTotal;
	struct zip* m_zip;
	QList<FileEntry> m_files;
	QList<bool> m_filesValues;
};

#endif
