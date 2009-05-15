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

#ifndef PIPEDLG_H
#define PIPEDLG_H
#include <QDialog>
#include <QFile>
#include "ui_PipeDlg.h"
#include <fatrat/fatrat.h>

class PipeDlg : public QDialog, Ui_PipeDlg
{
Q_OBJECT
public:
	PipeDlg(QWidget* parent)
	: QDialog(parent)
	{
		setupUi(this);
		
		comboPredefined->addItem(QString());
		
		QFile file;
		if(openDataFile(&file, "/data/plugins/fatrat-unpack/pipecmds"))
		{
			QString line;
			while(true)
			{
				line = QString::fromUtf8(file.readLine());
				if(line.isEmpty())
					break;
				
				QStringList items = line.split('|');
				if(items.size() != 2)
					continue;
				
				comboPredefined->addItem(items[0], items[1].trimmed());
			}
		}
		
		connect(comboPredefined, SIGNAL(currentIndexChanged(int)), this, SLOT(cmdChanged(int)));
	}
	int exec()
	{
		lineCommand->setText(m_strCommand);
		int r = QDialog::exec();
		if(r == QDialog::Accepted)
			m_strCommand = lineCommand->text();
		return r;
	}
protected slots:
	void cmdChanged(int index)
	{
		lineCommand->setText(comboPredefined->itemData(index).toString());
	}
public:
	QString m_strCommand;
};

#endif
