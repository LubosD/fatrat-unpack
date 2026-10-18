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

#include "TaskUi.h"
#include "plugins/Task.h"

std::optional<QJsonObject> TaskUi::ask(const QJsonObject& form, int timeoutMs)
{
	return m_task->askBlocking(form, timeoutMs);
}

void TaskUi::setProgress(int percent)
{
	m_task->setProgress(percent);
}

void TaskUi::setStatus(const QString& text)
{
	m_task->setStatus(text);
}

bool TaskUi::cancelled() const
{
	return m_task->cancelRequested();
}
