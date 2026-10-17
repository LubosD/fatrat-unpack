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

// The command we pipe into may consume data slower than we decompress it
static const qint64 MAX_PIPE_BUFFER = 4*1024*1024;

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

// The setters below are called from the extraction thread, so they hand the
// work over to the GUI thread.
void Unpacker::setProgress(int nProgress)
{
	QMetaObject::invokeMethod(m_widget->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, nProgress));
}

void Unpacker::setToolTip(QString toolTip)
{
	QProgressBar* bar = m_widget->progressBar;
	QMetaObject::invokeMethod(bar, [bar, toolTip]() { bar->setToolTip(toolTip); }, Qt::QueuedConnection);
}

void Unpacker::showError(QString error)
{
	QMessageBox::critical(getMainWindow(), tr("Unpack an archive"), error);
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
	QList<bool> files = m_dialog.getStates();
	if(!files.contains(true))
		return;
	
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
	
	connect(prs, SIGNAL(finished(int, QProcess::ExitStatus)), prs, SLOT(deleteLater()));
	this->pipe(files.indexOf(true), prs);
}

void Unpacker::minimize()
{
	m_dialog.hide();
}

void Unpacker::setFileProgress(int index, int nProgress)
{
	UnpackDlg* dlg = &m_dialog;
	QMetaObject::invokeMethod(dlg, [dlg, index, nProgress]() { dlg->setFileProgress(index, nProgress); }, Qt::QueuedConnection);
}

// Returns false when the command has terminated or the extraction was aborted
bool Unpacker::writeToPipe(QPointer<QProcess> process, const char* data, qint64 length)
{
	QByteArray chunk(data, length);
	bool running = false;
	
	QMetaObject::invokeMethod(this, [process, chunk, &running]() {
		running = process && process->state() == QProcess::Running;
		if(running)
			process->write(chunk);
	}, Qt::BlockingQueuedConnection);
	
	if(!running)
		return false;
	
	while(true)
	{
		qint64 pending = 0;
		QMetaObject::invokeMethod(this, [process, &pending]() {
			if(process)
				pending = process->bytesToWrite();
		}, Qt::BlockingQueuedConnection);
		
		if(pending < MAX_PIPE_BUFFER)
			return true;
		if(m_bAbort)
			return false;
		msleep(20);
	}
}

void Unpacker::closePipe(QPointer<QProcess> process)
{
	QMetaObject::invokeMethod(this, [process]() {
		if(process)
			process->closeWriteChannel();
	}, Qt::QueuedConnection);
}
