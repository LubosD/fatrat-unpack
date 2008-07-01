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

#include <fatrat/fatrat.h>
#include <fatrat/Transfer.h>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include "RarUnpacker.h"

const char* g_archiveSuffixes[] = { ".rar", 0 };

void unpackArchive(Transfer* t, Queue* q);
static void searchDirectory(QString absolute, QString relative, QList<QString>& out);
static bool isArchive(QString name);

__attribute__ ((constructor)) void init()
{
	//Q_INIT_RESOURCE(unpack);
	
	MenuAction action;
	
	action.strName = QObject::tr("Unpack archive(s)...");
	action.lpfnTriggered = unpackArchive;
	addMenuAction(action);
}

extern "C" __attribute__((visibility("default"))) PluginInfo getInfo()
{
	PluginInfo info;
	
	info.version = VERSION;
	info.name = QObject::tr("Unpack plugin");
	info.author = QString::fromUtf8("Luboš Doležel");
	info.website = "http://fatrat.dolezel.info";
	
	return info;
}

void unpackArchive(Transfer* t, Queue* q)
{
	QString transferPath = t->dataPath(true);
	QString dest;
	QFileInfo finfo(transferPath);
	QThread* unpacker;
	
	if(!finfo.exists())
		return;
	
	if(!finfo.isDir())
	{
		bool bIsArchive = false;
		for(size_t i=0;g_archiveSuffixes[i];i++)
		{
			if(transferPath.endsWith(g_archiveSuffixes[0], Qt::CaseInsensitive))
			{
				bIsArchive = true;
				break;
			}
		}
		
		if(!bIsArchive)
		{
			QMessageBox::critical(getMainWindow(), QObject::tr("Unpack an archive"), QObject::tr("This file is not a supported archive!"));
			return;
		}
	}
	else
	{
		QMessageBox::critical(getMainWindow(), QObject::tr("Unpack an archive"), "This plugin is a work in progress!");
		return;
	}
	
	unpacker = new RarUnpacker(transferPath);
}

void searchDirectory(QString absolute, QString relative, QList<QString>& out)
{
	QDir dir(absolute);
	QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
	
	foreach(QFileInfo info, list)
	{
		if(info.isDir())
			searchDirectory(absolute+info.fileName()+'/', relative+info.fileName()+'/', out);
		else if(!info.isSymLink())
		{
			if(isArchive(info.fileName()))
				out << QString(relative+info.fileName());
		}
	}
}

bool isArchive(QString name)
{
	for(size_t i=0;g_archiveSuffixes[i];i++)
	{
		if(name.endsWith(g_archiveSuffixes[0], Qt::CaseInsensitive))
			return true;
	}
	return false;
}

