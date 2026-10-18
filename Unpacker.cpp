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

#include "Unpacker.h"
#include <QJsonArray>
#include <QJsonObject>

// The command we pipe into may consume data slower than we decompress it.
static const qint64 MAX_PIPE_BUFFER = 4 * 1024 * 1024;

QByteArray Unpacker::askPassword(const QString& archiveComment, bool wrongBefore)
{
	QJsonArray fields;

	if (!m_strTransferComment.isEmpty())
		fields.append(QJsonObject{ { "type", "label" }, { "title", m_strTransferComment } });
	if (!archiveComment.isEmpty())
		fields.append(QJsonObject{ { "type", "label" }, { "title", archiveComment } });

	QJsonObject password{ { "type", "password" }, { "key", "password" }, { "title", tr("Password") } };
	if (!m_strTransferComment.isEmpty() && !m_strTransferComment.contains('\n'))
		password.insert("default", m_strTransferComment);
	fields.append(password);

	QJsonObject form{ { "title", tr("Password needed") }, { "fields", fields } };
	if (wrongBefore)
		form.insert("description", tr("The password is incorrect."));

	std::optional<QJsonObject> answer = m_ui.ask(form, 600000);
	if (!answer)
		throw NoAnswer{};
	return answer->value("password").toString().toUtf8();
}

bool Unpacker::writeToPipe(QIODevice* device, const char* data, qint64 length)
{
	qint64 written = 0;

	while (written < length)
	{
		qint64 n = device->write(data + written, length - written);
		if (n < 0)
			return false;
		written += n;
	}

	while (device->bytesToWrite() > MAX_PIPE_BUFFER)
	{
		if (cancelled())
			return false;
		device->waitForBytesWritten(20);
	}
	return true;
}
