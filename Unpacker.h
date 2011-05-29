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

#ifndef UNPACKER_H
#define UNPACKER_H
#include <QThread>
#include <QList>
#include <QProcess>
#include <QRegExp>
#include "UnpackDlg.h"

class ProgressWidget;

class Unpacker : public QThread
{
Q_OBJECT
public:
	Unpacker(QString file);
	virtual ~Unpacker();
protected:
	void setProgress(int nProgress);
	void setFileProgress(int index, int nProgress);
	void setToolTip(QString toolTip);
	
	void runDialog(const QList<FileEntry>& files);
	virtual void extract(QList<bool> files, QString where) = 0;
	virtual void pipe(int fileIndex, QProcess* process) = 0;
protected slots:
	void unpack();
	void abort();
	void minimize();
	void pipe();
private:
	ProgressWidget* m_widget;
	UnpackDlg m_dialog;
protected:
	bool m_bAbort;
	QString m_strFile;
};

struct UnpackerEntry
{
	QRegExp re;
	QThread* (*pCreate)(QString /*file*/, QString /*comment*/);
	bool (*pSupported)(QString /*file*/);
};

#endif

