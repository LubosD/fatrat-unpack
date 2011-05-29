/*
FatRat download manager
http://fatrat.dolezel.info

Copyright (C) 2006-2011 Lubos Dolezel <lubos a dolezel.info>

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

#ifndef FILECONCATER_H
#define FILECONCATER_H
#include "Unpacker.h"
#include <QDir>

class FileConcater : public Unpacker
{
public:
	FileConcater(QString file);
	virtual ~FileConcater() {}
	
	static QThread* create(QString file, QString transferComment) { return new FileConcater(file); }
	static bool supported(QString file);
	
	virtual void run();
	virtual void extract(QList<bool> files, QString where);
	virtual void pipe(int fileIndex, QProcess* process);
private:
	void processArchive();
private:
	QString m_strFile, m_strTargetName, m_strBareName;
	QDir m_dirDestination;
	qint64 m_total;
	int m_extSize;
};

#endif
