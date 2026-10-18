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

#ifndef FILECONCATER_H
#define FILECONCATER_H
#include "Unpacker.h"
#include <QDir>

// Concatenates a .001/.002/... split back into one file. No passwords, and
// (as before) piping a split file isn't supported.
class FileConcater : public Unpacker
{
public:
	FileConcater(const QString& file, UnpackUi& ui);

	static Unpacker* create(const QString& file, UnpackUi& ui) { return new FileConcater(file, ui); }
	static bool supported(const QString& file);

	QList<FileEntry> list() override;
	void extract(const QList<bool>& files, const QString& where) override;
	void pipe(int fileIndex, QIODevice* device) override;
private:
	QString m_strTargetName, m_strBareName;
	QDir m_dirDestination;
	qint64 m_total = 0;
	int m_extSize = 0;
};

#endif
