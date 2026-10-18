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
#include "FakeUi.h"
#include "UnpackTask.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <functional>

namespace
{
	QStringList fieldKeys(const QJsonObject& form)
	{
		QStringList keys;
		for (const QJsonValue& f : form.value("fields").toArray())
			keys << f.toObject().value("key").toString();
		return keys;
	}

	QJsonObject fieldByKey(const QJsonObject& form, const QString& key)
	{
		for (const QJsonValue& f : form.value("fields").toArray())
		{
			QJsonObject field = f.toObject();
			if (field.value("key").toString() == key)
				return field;
		}
		return QJsonObject();
	}

	QStringList rowValues(const QJsonObject& form, const QString& key)
	{
		QStringList values;
		for (const QJsonValue& r : fieldByKey(form, key).value("rows").toArray())
			values << r.toObject().value("value").toString();
		return values;
	}

	bool isPasswordForm(const QJsonObject& form)
	{
		return fieldKeys(form).contains("password");
	}

	bool isFilesForm(const QJsonObject& form)
	{
		return fieldKeys(form).contains("files");
	}

	// A "choose files" answer that checks exactly `files` (all of them if
	// empty), extracting to `dest`, with the pipe fields set from the rest.
	QJsonObject filesAnswer(const QJsonObject& form, QStringList files, const QString& dest,
	                        const QString& mode = "extract", const QString& preset = "",
	                        const QString& command = "")
	{
		if (files.isEmpty())
			files = rowValues(form, "files");

		QJsonObject answer{ { "files", QJsonArray::fromStringList(files) }, { "dest", dest } };
		if (fieldKeys(form).contains("mode"))
		{
			answer.insert("mode", mode);
			answer.insert("preset", preset);
			answer.insert("command", command);
		}
		return answer;
	}

	// Copies the whole fixtures tree into a fresh temp dir (fixtures with
	// siblings -- a RAR volume set, a .NNN split, a directory of archives --
	// need those siblings alongside them) and returns a path under it.
	void copyTree(const QString& srcDir, const QString& dstDir)
	{
		QDir().mkpath(dstDir);
		QDir d(srcDir);
		const QFileInfoList entries = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
		for (const QFileInfo& e : entries)
		{
			if (e.isDir())
				copyTree(e.filePath(), dstDir + "/" + e.fileName());
			else
				QFile::copy(e.filePath(), dstDir + "/" + e.fileName());
		}
	}

	QString fixture(QTemporaryDir& tmp, const QString& relativeName)
	{
		copyTree(QStringLiteral(FIXTURES_DIR), tmp.path());
		return tmp.filePath(relativeName);
	}

	QByteArray readFile(const QString& path)
	{
		QFile f(path);
		f.open(QIODevice::ReadOnly);
		return f.readAll();
	}
}

class tst_unpacktask : public QObject
{
	Q_OBJECT
private slots:
	void extractAllFiles();
	void extractSubsetOnly();
	void chooseBetweenArchivesInADirectory();
	void encryptedHeaderAsksPasswordBeforeListing();
	void wrongPasswordThenRightOne();
	void wrongPasswordThenNoAnswer();
	void noAnswerToFileFormFails();
	void cancelDuringExtractionLeavesNoPartialFile();
	void pipeNotOfferedWhenDisallowed();
	void pipeIntoACommand();
	void pipeWithTwoFilesIsRejectedAndReasked();
	void corruptArchiveSurfacesItsOwnError();
	void missingDataPathFails();
	void splitFileConcatenates();
#ifdef WITH_LIBARCHIVE
	void sevenZPlainExtracts();
	void zipPasswordWrongThenRight();
#endif
};

void tst_unpacktask::extractAllFiles()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QCOMPARE(readFile(dest + "/one.txt"), QByteArray("Hello from file one.\n"));
	QVERIFY(QFile::exists(dest + "/two.txt"));
}

void tst_unpacktask::extractSubsetOnly()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, { "two.txt" }, dest); });

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QVERIFY(!QFile::exists(dest + "/one.txt"));
	QVERIFY(QFile::exists(dest + "/two.txt"));
}

void tst_unpacktask::chooseBetweenArchivesInADirectory()
{
	QTemporaryDir tmp;
	QString dir = fixture(tmp, "dir_two_archives");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	// First question: which archive (one.rar, plain and unencrypted).
	ui.queueResponder([&](const QJsonObject& form) {
		QStringList choices = rowValues(form, "archive");
		QString pick;
		for (const QString& c : choices)
		{
			if (c.endsWith("one.rar"))
				pick = c;
		}
		return std::optional<QJsonObject>(QJsonObject{ { "archive", pick } });
	});
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });

	UnpackResult r = runUnpackTask(ui, dir, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QVERIFY(QFile::exists(dest + "/one.txt"));
	QCOMPARE(ui.forms().size(), 2);
}

void tst_unpacktask::encryptedHeaderAsksPasswordBeforeListing()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "encrypted_headers.rar");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([](const QJsonObject& form) {
		return isPasswordForm(form) ? std::optional<QJsonObject>(QJsonObject{ { "password", "secret123" } }) : std::nullopt;
	});
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QVERIFY(isPasswordForm(ui.forms().first()));
	QVERIFY(isFilesForm(ui.forms().at(1)));
	QVERIFY(QFile::exists(dest + "/one.txt"));
}

void tst_unpacktask::wrongPasswordThenRightOne()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "encrypted_headers.rar");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueAnswer(QJsonObject{ { "password", "wrong-one" } });
	ui.queueAnswer(QJsonObject{ { "password", "secret123" } });
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QCOMPARE(ui.forms().size(), 3);
	QVERIFY(ui.forms().at(1).value("description").toString().contains("incorrect"));
}

void tst_unpacktask::wrongPasswordThenNoAnswer()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "encrypted_headers.rar");

	FakeUi ui;
	ui.queueAnswer(QJsonObject{ { "password", "wrong-one" } });
	ui.queueNoAnswer();

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY(!r.ok);
	QCOMPARE(r.message, QString("No answer"));
}

void tst_unpacktask::noAnswerToFileFormFails()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");

	FakeUi ui;
	ui.queueNoAnswer();

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY(!r.ok);
	QCOMPARE(r.message, QString("No answer"));
}

void tst_unpacktask::cancelDuringExtractionLeavesNoPartialFile()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) {
		ui.setCancelled(true); // flips right before extract() starts working
		return filesAnswer(form, {}, dest);
	});

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY(!r.ok);
	QCOMPARE(r.message, QString("Cancelled"));
	QVERIFY(!QFile::exists(dest + "/one.txt"));
	QVERIFY(!QFile::exists(dest + "/two.txt"));
}

void tst_unpacktask::pipeNotOfferedWhenDisallowed()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	bool sawModeField = false;
	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) {
		sawModeField = sawModeField || fieldKeys(form).contains("mode");
		return filesAnswer(form, {}, dest);
	});

	UnpackResult r = runUnpackTask(ui, archive, QString(), /*allowPipe=*/false);
	QVERIFY2(r.ok, qPrintable(r.message));
	QVERIFY(!sawModeField);
}

void tst_unpacktask::pipeIntoACommand()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");
	QString target = tmp.filePath("piped_out.txt");

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) {
		return filesAnswer(form, { "one.txt" }, tmp.path(), "pipe", "", QString("cat > %1").arg(target));
	});

	UnpackResult r = runUnpackTask(ui, archive, QString(), /*allowPipe=*/true);

	QVERIFY2(r.ok, qPrintable(r.message));
	QCOMPARE(readFile(target), QByteArray("Hello from file one.\n"));
}

void tst_unpacktask::pipeWithTwoFilesIsRejectedAndReasked()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.rar");

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) {
		return filesAnswer(form, { "one.txt", "two.txt" }, tmp.path(), "pipe", "", "cat");
	});
	ui.queueResponder([&](const QJsonObject& form) {
		return filesAnswer(form, { "one.txt" }, tmp.path(), "pipe", "", QString("cat > %1").arg(tmp.filePath("x")));
	});

	UnpackResult r = runUnpackTask(ui, archive, QString(), /*allowPipe=*/true);

	QVERIFY2(r.ok, qPrintable(r.message));
	QCOMPARE(ui.forms().size(), 2);
	QVERIFY(ui.forms().at(1).value("description").toString().contains("exactly one"));
}

void tst_unpacktask::corruptArchiveSurfacesItsOwnError()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "corrupt.rar");

	FakeUi ui;
	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY(!r.ok);
	QVERIFY(!r.message.isEmpty());
	QVERIFY(r.message != "No answer");
	QVERIFY(r.message != "Cancelled");
}

void tst_unpacktask::missingDataPathFails()
{
	FakeUi ui;
	UnpackResult r = runUnpackTask(ui, "/does/not/exist/at/all", QString(), false);
	QVERIFY(!r.ok);
}

void tst_unpacktask::splitFileConcatenates()
{
	QTemporaryDir tmp;
	QString part1 = fixture(tmp, "split_concat.001");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });

	UnpackResult r = runUnpackTask(ui, part1, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QByteArray result = readFile(dest + "/split_concat");
	QCOMPARE(result.size(), 500);
	QCOMPARE(result, QByteArray("0123456789").repeated(50));
}

#ifdef WITH_LIBARCHIVE
void tst_unpacktask::sevenZPlainExtracts()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "plain.7z");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QCOMPARE(readFile(dest + "/one.txt"), QByteArray("Hello from file one.\n"));
	QVERIFY(QFile::exists(dest + "/two.txt"));
}

void tst_unpacktask::zipPasswordWrongThenRight()
{
	QTemporaryDir tmp;
	QString archive = fixture(tmp, "encrypted.zip");
	QString dest = tmp.filePath("out");
	QDir().mkpath(dest);

	FakeUi ui;
	ui.queueResponder([&](const QJsonObject& form) { return filesAnswer(form, {}, dest); });
	ui.queueAnswer(QJsonObject{ { "password", "wrong-one" } });
	ui.queueAnswer(QJsonObject{ { "password", "secret123" } });

	UnpackResult r = runUnpackTask(ui, archive, QString(), false);

	QVERIFY2(r.ok, qPrintable(r.message));
	QVERIFY(ui.forms().at(2).value("description").toString().contains("incorrect"));
	QVERIFY(QFile::exists(dest + "/one.txt"));
}
#endif

QTEST_GUILESS_MAIN(tst_unpacktask)
#include "tst_unpacktask.moc"
