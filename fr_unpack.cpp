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

#include "fatrat.h"
#include "Settings.h"
#include "Transfer.h"
#include "plugins/PluginApi.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QLocale>
#include <QPointer>
#include <QThread>
#include <QTranslator>

#include "unpack_config.h"
#include "Archivators.h"
#include "TaskUi.h"
#include "UnpackTask.h"

namespace
{
	// Worker threads still running. A plugin that starts threads must wait for
	// them before FatRat exits: the process is about to unmap this plugin's code.
	QList<QPointer<QThread>> s_workers;

	void joinWorkers()
	{
		for (const QPointer<QThread>& w : s_workers)
		{
			if (w)
				w->wait();
		}
	}

	QList<PipePreset> loadPipePresets()
	{
		QList<PipePreset> presets;
		QFile file;

		if (openDataFile(&file, "/data/plugins/fatrat-unpack/pipecmds"))
		{
			while (!file.atEnd())
			{
				QString line = QString::fromUtf8(file.readLine());
				QStringList items = line.split('|');
				if (items.size() != 2)
					continue;
				presets << PipePreset{ items[0].trimmed(), items[1].trimmed() };
			}
		}
		return presets;
	}

	bool unpackAppliesTo(Transfer* t)
	{
		QString path = t->dataPath(true);
		QFileInfo info(path);

		if (!info.exists())
			return false;
		return info.isDir() || Archivators::isArchive(path) != nullptr;
	}

	void unpackArchive(Transfer* t, TaskHost& host)
	{
		QString dataPath = t->dataPath(true);
		QString comment = t->comment();
		bool allowPipe = getSettingsValue("unpack/allow_pipe", false).toBool();
		QList<PipePreset> presets = allowPipe ? loadPipePresets() : QList<PipePreset>();

		Task* task = host.createTask(QObject::tr("Unpack %1").arg(t->name()));

		QThread* worker = QThread::create([task, dataPath, comment, allowPipe, presets] {
			TaskUi ui(task);
			UnpackResult result = runUnpackTask(ui, dataPath, comment, allowPipe, presets);
			task->finish(result.ok, result.message);
		});

		QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
		QObject::connect(worker, &QThread::finished, worker, [worker] { s_workers.removeAll(worker); });
		s_workers << worker;
		worker->start();
	}
}

extern "C" __attribute__((visibility("default"))) PluginInfo getInfo()
{
	PluginInfo info;

	// FatRat warns about plugins whose version differs from its own.
	info.version = VERSION;
	info.name = QObject::tr("Unpack plugin");
	info.author = QString::fromUtf8("Luboš Doležel");
	info.website = "http://fatrat.dolezel.info";

	return info;
}

FATRAT_PLUGIN_EXPORT int getApiVersion()
{
	return FATRAT_PLUGIN_API_VERSION;
}

FATRAT_PLUGIN_EXPORT bool requiresGui()
{
	return false;
}

FATRAT_PLUGIN_EXPORT void initPlugin()
{
#ifdef WITH_NLS
	static QTranslator translator;
	QString fname = QString("fatrat-unpack_") + QLocale::system().name();
	if (translator.load(fname, getDataFileDir("/lang", fname)))
		QCoreApplication::installTranslator(&translator);
#endif

	if (QCoreApplication::instance())
	{
		QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
		                  QCoreApplication::instance(), joinWorkers, Qt::DirectConnection);
	}

	TransferAction action;
	action.id = "unpack.unpack";
	action.label = QObject::tr("Unpack archive(s)...");
	action.icon = "package-x-generic";
	action.appliesTo = unpackAppliesTo;
	action.trigger = unpackArchive;
	registerTransferAction(action);
}
