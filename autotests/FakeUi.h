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

#ifndef FAKEUI_H
#define FAKEUI_H
#include "UnpackUi.h"
#include <QList>
#include <QStringList>
#include <functional>

/* A scripted UnpackUi for the tests. Each ask() pops the next queued
 * responder and calls it with the form it was asked; a test queues either a
 * fixed answer (queueAnswer()/queueNoAnswer()) or a function that inspects
 * the form to decide (queueResponder()), so it can assert on the form (its
 * title, description, field list, a checklist's actual rows) as well as on
 * the final result.
 */
class FakeUi : public UnpackUi
{
public:
	using Responder = std::function<std::optional<QJsonObject>(const QJsonObject& form)>;

	void queueResponder(Responder r) { m_responders << r; }
	void queueAnswer(QJsonObject answer) { queueResponder([answer](const QJsonObject&) { return std::optional<QJsonObject>(answer); }); }
	void queueNoAnswer() { queueResponder([](const QJsonObject&) { return std::optional<QJsonObject>(); }); }

	std::optional<QJsonObject> ask(const QJsonObject& form, int timeoutMs) override
	{
		Q_UNUSED(timeoutMs);
		m_forms << form;
		if (m_responders.isEmpty())
			return std::nullopt; // the test forgot to queue enough answers
		Responder r = m_responders.takeFirst();
		return r(form);
	}

	void setProgress(int percent) override { m_progress << percent; }
	void setStatus(const QString& text) override { m_status << text; }
	bool cancelled() const override { return m_cancelled; }

	void setCancelled(bool c) { m_cancelled = c; }

	const QList<QJsonObject>& forms() const { return m_forms; }
	const QList<int>& progress() const { return m_progress; }
	const QStringList& status() const { return m_status; }
	int pendingAnswers() const { return m_responders.size(); }
private:
	QList<Responder> m_responders;
	QList<QJsonObject> m_forms;
	QList<int> m_progress;
	QStringList m_status;
	bool m_cancelled = false;
};

#endif
