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

#include "unpack_config.h"
#include "Archivators.h"
#include <QTest>

/* What fr_unpack.cpp's appliesTo() relies on: whether a path looks like
 * something this plugin can unpack. isArchive() is pure string matching (plus
 * FileConcater/RarUnpacker's cheap, IO-free supported() checks), so these
 * don't need any real files on disk. */
class tst_archivators : public QObject
{
	Q_OBJECT
private slots:
	void rarFileIsAnArchive();
	void nonFirstRarVolumeIsNotListedAlone();
	void splitFileIsAnArchive();
	void plainFileIsNotAnArchive();
	void directoryLikePathIsNotAnArchive();
	void missingPathStillMatchesByName();
#ifdef WITH_LIBARCHIVE
	void zipAndSevenZAreArchives();
#endif
};

void tst_archivators::rarFileIsAnArchive()
{
	QVERIFY(Archivators::isArchive("/tmp/movie.rar") != nullptr);
	QVERIFY(Archivators::isArchive("/tmp/MOVIE.RAR") != nullptr); // case-insensitive
}

void tst_archivators::nonFirstRarVolumeIsNotListedAlone()
{
	// RarUnpacker::supported() only accepts .part1.rar of a multi-volume set.
	QVERIFY(Archivators::isArchive("/tmp/movie.part1.rar") != nullptr);
	QVERIFY(Archivators::isArchive("/tmp/movie.part2.rar") == nullptr);
}

void tst_archivators::splitFileIsAnArchive()
{
	QVERIFY(Archivators::isArchive("/tmp/movie.001") != nullptr);
	QVERIFY(Archivators::isArchive("/tmp/movie.0001") != nullptr);
}

void tst_archivators::plainFileIsNotAnArchive()
{
	QVERIFY(Archivators::isArchive("/tmp/readme.txt") == nullptr);
	QVERIFY(Archivators::isArchive("/tmp/noextension") == nullptr);
}

void tst_archivators::directoryLikePathIsNotAnArchive()
{
	QVERIFY(Archivators::isArchive("/tmp/some/directory") == nullptr);
}

void tst_archivators::missingPathStillMatchesByName()
{
	// isArchive() never touches the filesystem; appliesTo() itself is what
	// checks existence before calling it.
	QVERIFY(Archivators::isArchive("/does/not/exist.rar") != nullptr);
}

#ifdef WITH_LIBARCHIVE
void tst_archivators::zipAndSevenZAreArchives()
{
	QVERIFY(Archivators::isArchive("/tmp/archive.zip") != nullptr);
	QVERIFY(Archivators::isArchive("/tmp/archive.7z") != nullptr);
}
#endif

QTEST_GUILESS_MAIN(tst_archivators)
#include "tst_archivators.moc"
