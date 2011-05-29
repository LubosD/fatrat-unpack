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

#include "FileConcater.h"
#include <fatrat/fatrat.h>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QtDebug>

FileConcater::FileConcater(QString file)
: Unpacker(file), m_strFile(file)
{
	processArchive();
}

bool FileConcater::supported(QString file)
{
	int pos = file.lastIndexOf(".");
	if (pos == -1)
		return false;
	QString ext = file.mid(pos+1);
	
	if (ext.size() < 3 || ext.size() > 4)
		return false;
	
	bool ok;
	ext.toInt(&ok);
	
	return ok;
}

void FileConcater::processArchive()
{
	try
	{
		int pos = m_strFile.lastIndexOf(".");
		if (pos == -1)
			throw tr("Unsupported file.");
		QString ext = m_strFile.mid(pos+1);
		FileEntry file;
		
		m_extSize = ext.size();
		m_strBareName = m_strFile.left(m_strFile.size()-m_extSize-1);
		
		file.size = 0;
		
		int poss = m_strFile.lastIndexOf("/");
		file.name = m_strTargetName = m_strFile.mid(poss+1, m_strFile.size()-poss-m_extSize-2);
		
		for (int i = 1;; i++)
		{
			QString name = QString("%1.%2").arg(m_strBareName).arg(i, m_extSize, 10, QChar('0'));
			QFileInfo info(name);
			
			qDebug() << "Trying" << name;
			
			if (!info.exists())
				break;
				//throw tr("%1 doesn't exist.").arg(name);
			
			file.size += info.size();
		}
		
		m_total = file.size;
		
		runDialog(QList<FileEntry>() << file);
	}
	catch (QString err)
	{
		QMessageBox::critical(getMainWindow(), tr("Unpack an archive"), err);
	}
}

void FileConcater::run()
{
	try
	{
		QFile target(m_dirDestination.filePath(m_strTargetName));
		
		if (!target.open(QIODevice::WriteOnly))
			throw tr("Unable to open %1 for writing.").arg(m_strTargetName);
		
		for (int i = 1;; i++)
		{
			QString name = QString("%1.%2").arg(m_strBareName).arg(i, m_extSize, 10, QChar('0'));
			QFile file(name);
			
			if (!file.open(QIODevice::ReadOnly))
				break;
			
			char buffer[4096];
			
			while (!file.atEnd())
			{
				qint64 rd = file.read(buffer, sizeof buffer);
				if (rd == -1)
					throw file.errorString();
				
				rd = target.write(buffer, rd);
				if (rd == -1)
					throw target.errorString();
				
				int pcts;
				
				pcts = 100*target.pos()/m_total;
				
				setProgress(pcts);
				setFileProgress(0, pcts);
			}
		}
	}
	catch (QString errString)
	{
		QMessageBox::critical(getMainWindow(), tr("Unpack an archive"), errString);
	}
	
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void FileConcater::extract(QList<bool> files, QString where)
{
	m_dirDestination = QDir(where);
	start();
}

void FileConcater::pipe(int fileIndex, QProcess* process)
{
}
