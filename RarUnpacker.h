/*
FatRat download manager
http://fatrat.dolezel.info

Copyright (C) 2006-2008 Lubos Dolezel <lubos a dolezel.info>

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

class RarUnpacker : public Unpacker
{
Q_OBJECT
public:
	RarUnpacker(QString file, QString destination);
	void run();
private slots:
	void askPassword(QByteArray* out);
	void showError(QString error);
private:
	static int rarCallback(unsigned msg, unsigned long tthis, unsigned long p1, unsigned long p2);
private:
	QByteArray m_strFile, m_strPassword;
	QDir m_dirDestination;
	
	struct File
	{
		QString name;
		qint64 size;
	};
	QList<File> m_files;
	qint64 m_nTotal, m_nDone;
	int m_nPercents;
	QFile m_file;
};

#endif
