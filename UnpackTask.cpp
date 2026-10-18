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

#include "UnpackTask.h"
#include "Archivators.h"
#include "Unpacker.h"
#include "UnpackUi.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <algorithm>
#include <memory>

namespace
{
	struct Tr
	{
		Q_DECLARE_TR_FUNCTIONS(UnpackTask)
	};

	QString formatSize(qint64 bytes)
	{
		static const char* const units[] = { "B", "KB", "MB", "GB", "TB" };
		double size = bytes;
		int unit = 0;

		while (size >= 1024.0 && unit < 4)
		{
			size /= 1024.0;
			unit++;
		}
		return unit == 0 ? QString("%1 %2").arg(bytes).arg(units[0])
		                  : QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unit]);
	}

	struct ArchiveEntry
	{
		QString path;
		const UnpackerEntry* unpacker;
	};

	void searchDirectory(const QString& absolute, QList<ArchiveEntry>& out)
	{
		QDir dir(absolute);
		const QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

		for (const QFileInfo& info : list)
		{
			if (info.isDir())
				searchDirectory(absolute + info.fileName() + '/', out);
			else if (!info.isSymLink())
			{
				const UnpackerEntry* ue = Archivators::isArchive(info.absoluteFilePath());
				if (ue)
					out << ArchiveEntry{ info.absoluteFilePath(), ue };
			}
		}
	}

	// Empty if `dataPath` doesn't exist, isn't an archive and (if a directory)
	// contains none either.
	QList<ArchiveEntry> findArchives(const QString& dataPath)
	{
		QFileInfo info(dataPath);
		QList<ArchiveEntry> out;

		if (!info.exists())
			return out;

		if (!info.isDir())
		{
			const UnpackerEntry* ue = Archivators::isArchive(dataPath);
			if (ue)
				out << ArchiveEntry{ dataPath, ue };
		}
		else
			searchDirectory(dataPath + '/', out);

		return out;
	}

	QString pipeCommandFor(const QString& presetName, const QList<PipePreset>& presets)
	{
		for (const PipePreset& p : presets)
		{
			if (p.name == presetName)
				return p.command;
		}
		return QString();
	}

	// Throws NoAnswer instead of returning std::nullopt.
	QJsonObject askOrNoAnswer(UnpackUi& ui, const QJsonObject& form, int timeoutMs)
	{
		std::optional<QJsonObject> answer = ui.ask(form, timeoutMs);
		if (!answer)
			throw NoAnswer{};
		return *answer;
	}

	QJsonObject chooseArchiveForm(const QList<ArchiveEntry>& archives, const QDir& baseDir)
	{
		QJsonArray rows;
		for (const ArchiveEntry& e : archives)
		{
			rows.append(QJsonObject{
				{ "value", e.path },
				{ "cells", QJsonArray{ baseDir.relativeFilePath(e.path), formatSize(QFileInfo(e.path).size()) } }
			});
		}

		return QJsonObject{
			{ "title", Tr::tr("Choose an archive") },
			{ "fields", QJsonArray{ QJsonObject{
				{ "type", "list" }, { "key", "archive" }, { "title", Tr::tr("Archive") },
				{ "columns", QJsonArray{ Tr::tr("Archive"), Tr::tr("Size") } },
				{ "rows", rows }
			} } }
		};
	}

	struct FilesChoice
	{
		QStringList checkedFiles;
		QString dest;
		bool pipe = false;
		QString pipeCommand;
	};

	FilesChoice chooseFiles(UnpackUi& ui, const QList<FileEntry>& files, const QString& archivePath,
	                        bool allowPipe, const QList<PipePreset>& presets)
	{
		QStringList checkedFiles;
		for (const FileEntry& f : files)
			checkedFiles << f.name;

		QString dest = QFileInfo(archivePath).absolutePath();
		QString mode = "extract", preset, command;
		QString formError;

		while (true)
		{
			QJsonArray rows;
			for (const FileEntry& f : files)
			{
				rows.append(QJsonObject{
					{ "value", f.name },
					{ "cells", QJsonArray{ f.name, formatSize(f.size) } },
					{ "checked", checkedFiles.contains(f.name) }
				});
			}

			QJsonArray fields;
			fields.append(QJsonObject{
				{ "type", "checklist" }, { "key", "files" }, { "title", Tr::tr("Files") },
				{ "columns", QJsonArray{ Tr::tr("Name"), Tr::tr("Size") } }, { "rows", rows }
			});
			fields.append(QJsonObject{
				{ "type", "directory" }, { "key", "dest" }, { "title", Tr::tr("Destination") }, { "default", dest }
			});

			if (allowPipe)
			{
				fields.append(QJsonObject{
					{ "type", "combo" }, { "key", "mode" }, { "title", Tr::tr("Action") },
					{ "default", mode },
					{ "options", QJsonArray{
						QJsonObject{ { "value", "extract" }, { "title", Tr::tr("Extract to the destination") } },
						QJsonObject{ { "value", "pipe" }, { "title", Tr::tr("Pipe one file into a command") } }
					} }
				});

				QJsonArray presetOptions;
				presetOptions.append(QJsonObject{ { "value", "" }, { "title", Tr::tr("Custom") } });
				for (const PipePreset& p : presets)
					presetOptions.append(QJsonObject{ { "value", p.name }, { "title", p.name } });

				fields.append(QJsonObject{
					{ "type", "combo" }, { "key", "preset" }, { "title", Tr::tr("Command") },
					{ "default", preset }, { "options", presetOptions }
				});
				fields.append(QJsonObject{
					{ "type", "text" }, { "key", "command" }, { "title", Tr::tr("Custom command") }, { "default", command }
				});
			}

			QJsonObject form{ { "title", Tr::tr("Choose files") }, { "fields", fields } };
			if (!formError.isEmpty())
				form.insert("description", formError);

			QJsonObject answer = askOrNoAnswer(ui, form, 1800000);

			checkedFiles.clear();
			for (const QJsonValue& v : answer.value("files").toArray())
				checkedFiles << v.toString();
			dest = answer.value("dest").toString();

			if (allowPipe)
			{
				mode = answer.value("mode").toString();
				preset = answer.value("preset").toString();
				command = answer.value("command").toString();
			}

			if (mode == "pipe")
			{
				if (checkedFiles.size() != 1)
				{
					formError = Tr::tr("Pick exactly one file to pipe.");
					continue;
				}
				if (preset.isEmpty() && command.trimmed().isEmpty())
				{
					formError = Tr::tr("Enter a command.");
					continue;
				}
			}
			break;
		}

		FilesChoice choice;
		choice.checkedFiles = checkedFiles;
		choice.dest = dest;
		choice.pipe = (mode == "pipe");
		if (choice.pipe)
			choice.pipeCommand = preset.isEmpty() ? command : pipeCommandFor(preset, presets);
		return choice;
	}
}

UnpackResult runUnpackTask(UnpackUi& ui, const QString& dataPath, const QString& transferComment,
                           bool allowPipe, const QList<PipePreset>& pipePresets)
{
	try
	{
		if (!QFileInfo::exists(dataPath))
			return { false, Tr::tr("The downloaded data is gone.") };

		QList<ArchiveEntry> archives = findArchives(dataPath);
		if (archives.isEmpty())
			return { false, Tr::tr("No supported archives found.") };

		ArchiveEntry chosen = archives.first();
		if (archives.size() > 1)
		{
			QDir baseDir(dataPath);
			QJsonObject answer = askOrNoAnswer(ui, chooseArchiveForm(archives, baseDir), 600000);
			QString picked = answer.value("archive").toString();

			auto it = std::find_if(archives.begin(), archives.end(), [&](const ArchiveEntry& e) { return e.path == picked; });
			if (it == archives.end())
				return { false, Tr::tr("No answer") };
			chosen = *it;
		}

		std::unique_ptr<Unpacker> unpacker(chosen.unpacker->pCreate(chosen.path, ui));
		unpacker->setTransferComment(transferComment);

		QList<FileEntry> files = unpacker->list();
		FilesChoice choice = chooseFiles(ui, files, chosen.path, allowPipe, pipePresets);

		if (choice.pipe)
		{
			int index = -1;
			for (int i = 0; i < files.size(); i++)
			{
				if (files[i].name == choice.checkedFiles.first())
				{
					index = i;
					break;
				}
			}

			QProcess process;
			process.start("sh", QStringList() << "-c" << choice.pipeCommand);
			if (!process.waitForStarted(3000))
				return { false, Tr::tr("Failed to execute the command.") };

			try
			{
				unpacker->pipe(index, &process);
			}
			catch (...)
			{
				process.closeWriteChannel();
				if (!process.waitForFinished(3000))
					process.kill();
				throw;
			}

			process.closeWriteChannel();
			process.waitForFinished(-1);

			if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
				return { false, Tr::tr("The command exited with status %1.").arg(process.exitCode()) };
			return { true, Tr::tr("Piped %1 into the command.").arg(files[index].name) };
		}
		else
		{
			QList<bool> selected;
			int count = 0;
			for (const FileEntry& f : files)
			{
				bool on = choice.checkedFiles.contains(f.name);
				selected << on;
				if (on)
					count++;
			}

			unpacker->extract(selected, choice.dest);
			return { true, Tr::tr("Unpacked %n file(s) to %1.", nullptr, count).arg(choice.dest) };
		}
	}
	catch (const NoAnswer&)
	{
		return { false, ui.cancelled() ? Tr::tr("Cancelled") : Tr::tr("No answer") };
	}
	catch (const QString& msg)
	{
		if (ui.cancelled())
			return { false, Tr::tr("Cancelled") };
		return { false, msg.isEmpty() ? Tr::tr("Cancelled") : msg };
	}
}
