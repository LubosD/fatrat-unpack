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

#ifndef UNPACKDLG_H
#define UNPACKDLG_H
#include <QDialog>
#include <QList>
#include "ui_UnpackDlg.h"

struct FileEntry
{
	QString name;
	qint64 size;
};

class UnpackDlg : public QDialog, public Ui_UnpackDlg
{
Q_OBJECT
public:
	UnpackDlg(QWidget* parent = 0);
	void load(const QList<FileEntry>& files);
	int exec();
	QList<bool> getStates() const;
	void setFileProgress(int index, int progress);
protected:
	void recursiveUpdate(QTreeWidgetItem* item);
	qint64 recursiveUpdateDown(QTreeWidgetItem* item);
	void recursiveCheck(QTreeWidgetItem* item, Qt::CheckState state);
	virtual void closeEvent(QCloseEvent* event);
protected slots:
	void fileItemChanged(QTreeWidgetItem* item, int column);
private:
	QList<QTreeWidgetItem*> m_files;
	bool m_bUpdating;
};

#endif
