/*
FatRat download manager
http://fatrat.dolezel.info

Copyright (C) 2006-2008 Lubos Dolezel <lubos a dolezel.info>

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
#include "ProgressWidget.h"
#include <fatrat/fatrat.h>

Unpacker::Unpacker()
{
	m_widget = new ProgressWidget;
	addStatusWidget(m_widget, false);
	m_widget->show();
}

Unpacker::~Unpacker()
{
	removeStatusWidget(m_widget);
	delete m_widget;
}

void Unpacker::setProgress(int nProgress)
{
	QMetaObject::invokeMethod(m_widget->progressBar, "setValue", Q_ARG(int, nProgress));
}

void Unpacker::setToolTip(QString toolTip)
{
	m_widget->progressBar->setToolTip(toolTip);
	//QMetaObject::invokeMethod(m_widget->progressBar, "setToolTip", Q_ARG(QString, toolTip));
}

