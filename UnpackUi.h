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

#ifndef UNPACKUI_H
#define UNPACKUI_H
#include <QJsonObject>
#include <QString>
#include <optional>

/* What the unpacking flow (UnpackTask, Unpacker and its subclasses) needs from
 * whatever is running it. The real implementation (TaskUi) wraps a plugin Task;
 * the tests use a scripted fake instead, so the flow itself never depends on
 * FatRat's Task class, a Transfer, or any particular thread.
 *
 * Every call blocks the calling (worker) thread until it has an answer: ask()
 * is expected to behave like Task::askBlocking() -- the implementation hops to
 * the main thread and back on its own.
 */
class UnpackUi
{
public:
	virtual ~UnpackUi() = default;

	// Blocks until answered, timed out, dismissed or the task is cancelled.
	// std::nullopt covers all three of the latter; the caller can't tell them
	// apart and treats them the same way (see Task::askBlocking()).
	virtual std::optional<QJsonObject> ask(const QJsonObject& form, int timeoutMs) = 0;

	// 0..100, or -1 for "unknown".
	virtual void setProgress(int percent) = 0;
	virtual void setStatus(const QString& text) = 0;

	// Checked between chunks of work so a cancelled task stops promptly.
	virtual bool cancelled() const = 0;
};

/* Thrown by anything in the unpacking flow (Unpacker subclasses, UnpackTask
 * itself) when a required question got no answer: the user dismissed it, it
 * timed out, or the task was cancelled while it was open -- ask() reports all
 * three the same way (std::nullopt). The catcher tells cancellation apart from
 * a genuine timeout/dismissal with UnpackUi::cancelled(). */
struct NoAnswer {};

#endif
