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

#ifndef UNPACKTASK_H
#define UNPACKTASK_H
#include <QList>
#include <QString>

class UnpackUi;

struct UnpackResult
{
	bool ok;
	QString message;
};

// One entry of data/pipecmds: a named, predefined command to pipe a file into.
struct PipePreset
{
	QString name, command;
};

/* The whole unpacking flow for one transfer's data: find the archive(s),
 * ask what to unpack and where (or what to pipe into), do it, and report the
 * result. Runs entirely on the calling thread -- meant to be a worker thread
 * of the plugin's own -- and talks to the outside world only through `ui`,
 * so it needs no FatRat binary to test.
 *
 * `pipePresets` is read from data/pipecmds by the caller (that file lookup
 * does need FatRat's openDataFile()); empty is fine, and is ignored unless
 * `allowPipe` is true.
 */
UnpackResult runUnpackTask(UnpackUi& ui, const QString& dataPath, const QString& transferComment,
                           bool allowPipe, const QList<PipePreset>& pipePresets = {});

#endif
