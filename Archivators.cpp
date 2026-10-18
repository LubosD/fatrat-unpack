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

#include "Archivators.h"
#include "unpack_config.h"
#include "RarUnpacker.h"
#include "FileConcater.h"
#ifdef WITH_LIBARCHIVE
#	include "ArchiveUnpacker.h"
#endif

namespace
{
	const UnpackerEntry g_archivators[] = {
		{ QRegularExpression(QRegularExpression::anchoredPattern(".+\\.rar"), QRegularExpression::CaseInsensitiveOption),
		  RarUnpacker::create, RarUnpacker::supported },
#ifdef WITH_LIBARCHIVE
		{ QRegularExpression(QRegularExpression::anchoredPattern(".+\\.(7z|zip)"), QRegularExpression::CaseInsensitiveOption),
		  ArchiveUnpacker::create, ArchiveUnpacker::supported },
#endif
		{ QRegularExpression(QRegularExpression::anchoredPattern(".+\\.\\d{3,4}")),
		  FileConcater::create, FileConcater::supported },
	};
}

const UnpackerEntry* Archivators::isArchive(const QString& name)
{
	for (const UnpackerEntry& e : g_archivators)
	{
		if (e.re.match(name).hasMatch() && e.pSupported(name))
			return &e;
	}
	return nullptr;
}
