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
#include <QTranslator>
#include <QLocale>
#include <QFileInfo>
#include <QMessageBox>

#include "config.h"
#include "RarUnpacker.h"
#include "FilesDlg.h"
#ifdef WITH_ZIP
#	include "ZipUnpacker.h"
#endif

UnpackerEntry g_archivators[] = {
		{ ".rar", RarUnpacker::create, RarUnpacker::supported },
#ifdef WITH_ZIP
		{ ".zip", ZipUnpacker::create, ZipUnpacker::supported },
#endif
};

struct ArchiveEntry
{
	QString path;
	UnpackerEntry* unpacker;
};

void unpackArchive(Transfer* t, Queue* q);
static void searchDirectory(QString absolute, QString relative, QList<ArchiveEntry>& out);
static UnpackerEntry* isArchive(QString name);

__attribute__ ((constructor)) void init()
{
	//Q_INIT_RESOURCE(unpack);
	
#ifdef WITH_NLS
	static QTranslator translator;
	{
		QString fname = QString("fatrat-unpack_") + QLocale::system().name();
		translator.load(fname, getDataFileDir("/lang", fname));
		QCoreApplication::installTranslator(&translator);
	}
#endif
	
	MenuAction action;
	
	action.strName = QObject::tr("Unpack archive(s)...");
	action.lpfnTriggered = unpackArchive;
	addMenuAction(action);
}

extern "C" __attribute__((visibility("default"))) PluginInfo getInfo()
{
	PluginInfo info;
	
	info.version = "1.1.1";
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
	UnpackerEntry* unpacker = 0;
	
	if(!finfo.exists())
		return;
	
	if(!finfo.isDir())
	{
		for(size_t i=0;i<sizeof(g_archivators)/sizeof(g_archivators[0]);i++)
		{
			if(transferPath.endsWith(g_archivators[i].suffix, Qt::CaseInsensitive))
			{
				unpacker = &g_archivators[i];
				break;
			}
		}
		
		if(!unpacker)
		{
			QMessageBox::critical(getMainWindow(), QObject::tr("Unpack an archive"), QObject::tr("This file is not a supported archive!"));
			return;
		}
	}
	else
	{
		QList<ArchiveEntry> eligible;
		searchDirectory(transferPath+'/', "/", eligible);
		
		if(eligible.isEmpty())
		{
			QMessageBox::warning(getMainWindow(), QObject::tr("Unpack an archive"), QObject::tr("No supported archives found."));
			return;
		}
		else if(eligible.size() == 1)
		{
			transferPath = eligible[0].path;
			unpacker = eligible[0].unpacker;
		}
		else
		{
			FilesDlg dlg(getMainWindow());
			QDir dir(transferPath);
			
			for(int i=0;i<eligible.size();i++)
				dlg.listArchives->addItem(dir.relativeFilePath(eligible[i].path));
			
			if(dlg.exec() != QDialog::Accepted || dlg.listArchives->currentRow() < 0)
				return;
			
			int ix = dlg.listArchives->currentRow();
			transferPath = eligible[ix].path;
			unpacker = eligible[ix].unpacker;
		}
	}
	
	unpacker->pCreate(transferPath, t->comment());
}

void searchDirectory(QString absolute, QString relative, QList<ArchiveEntry>& out)
{
	QDir dir(absolute);
	QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
	
	foreach(QFileInfo info, list)
	{
		if(info.isDir())
			searchDirectory(absolute+info.fileName()+'/', relative+info.fileName()+'/', out);
		else if(!info.isSymLink())
		{
			UnpackerEntry* ue = isArchive(info.absoluteFilePath());
			if(!ue)
				continue;
			
			ArchiveEntry ae;
			ae.path = info.absoluteFilePath();
			ae.unpacker = ue;
			out << ae;
		}
	}
}

UnpackerEntry* isArchive(QString name)
{
	for(size_t i=0;i<sizeof(g_archivators)/sizeof(g_archivators[0]);i++)
	{
		if(name.endsWith(g_archivators[i].suffix, Qt::CaseInsensitive) && g_archivators[i].pSupported(name))
			return &g_archivators[i];
	}
	return 0;
}

