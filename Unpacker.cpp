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
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include "PipeDlg.h"

Unpacker::Unpacker(QString file)
	: m_dialog(getMainWindow()), m_bAbort(false), m_strFile(file)
{
	m_widget = new ProgressWidget;
	addStatusWidget(m_widget, false);
	m_widget->show();
	connect(m_widget->toolRestore, SIGNAL(clicked()), &m_dialog, SLOT(show()));
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

void Unpacker::runDialog(const QList<FileEntry>& files)
{
	m_dialog.load(files);
	connect(m_dialog.pushUnpack, SIGNAL(clicked()), this, SLOT(unpack()));
	connect(m_dialog.pushAbort, SIGNAL(clicked()), this, SLOT(abort()));
	connect(m_dialog.pushHide, SIGNAL(clicked()), this, SLOT(minimize()));
	connect(m_dialog.pushPipe, SIGNAL(clicked()), this, SLOT(pipe()));
	
	m_dialog.show();
}

void Unpacker::unpack()
{
	QString dest = QFileDialog::getExistingDirectory(getMainWindow(), QObject::tr("Unpack an archive"), m_strFile);
	if(dest.isEmpty())
		return;
	
	m_dialog.pushUnpack->setEnabled(false);
	m_dialog.pushAbort->setEnabled(true);
	extract(m_dialog.getStates(), dest);
}

void Unpacker::abort()
{
	if(isRunning())
		m_bAbort = true;
	else
		deleteLater();
}

void Unpacker::pipe()
{
	PipeDlg dlg(&m_dialog);
	if(dlg.exec() != QDialog::Accepted)
		return;
	
	QProcess* prs = new QProcess;
	prs->start("sh", QStringList() << "-c" << dlg.m_strCommand);
	
	if(!prs->waitForStarted(3000))
	{
		QMessageBox::critical(&m_dialog, "FatRat", tr("Failed to execute the command."));
		delete prs;
		return;
	}
	
	m_dialog.pushUnpack->setEnabled(false);
	m_dialog.pushAbort->setEnabled(true);
	
	QList<bool> files = m_dialog.getStates();
	
	connect(prs, SIGNAL(finished(int, QProcess::ExitStatus)), prs, SLOT(deleteLater()));
	this->pipe(files.indexOf(true), prs);
}

void Unpacker::minimize()
{
	m_dialog.hide();
}

void Unpacker::setFileProgress(int index, int nProgress)
{
	m_dialog.setFileProgress(index, nProgress);
}

