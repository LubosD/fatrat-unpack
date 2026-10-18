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

#ifndef ARCHIVATORS_H
#define ARCHIVATORS_H
#include <QRegularExpression>
#include <QString>

class Unpacker;
class UnpackUi;

/* The one table of archive kinds this plugin knows, shared by the cheap
 * appliesTo() check in fr_unpack.cpp and the directory search + construction
 * in UnpackTask.cpp, so the two never drift apart. */
struct UnpackerEntry
{
	QRegularExpression re;
	Unpacker* (*pCreate)(const QString& file, UnpackUi& ui);
	bool (*pSupported)(const QString& file);
};

namespace Archivators
{
	// The entry whose pattern matches `name` and whose pSupported() agrees, or
	// nullptr. Cheap: no file I/O beyond what pSupported() itself does (none of
	// today's do).
	const UnpackerEntry* isArchive(const QString& name);
}

#endif
