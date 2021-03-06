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

#include "RarUnpacker.h"
#include "PasswordDlg.h"

#include <QMessageBox>
#include <QRegExp>
#include <QFile>
#include <fatrat/fatrat.h>
#include <QtDebug>

#define _UNIX
#define LPARAM unsigned long
#include "unrar/dll.hpp"

static QRegExp g_re("\\.part\\d+");

RarUnpacker::RarUnpacker(QString file, QString cmt)
	: Unpacker(processFileName(file)), m_nTotal(0), m_nDone(0), m_nPercents(0),
		   m_strCommentTransfer(cmt), m_pipe(0), m_nFileIndex(-1)
{
	processArchive();
}

RarUnpacker::~RarUnpacker()
{
	//if(m_pipe)
	//	m_pipe->deleteLater();
}

bool RarUnpacker::supported(QString file)
{
	int x = g_re.indexIn(file);
	if(x < 0)
		return true;
	else
		return g_re.cap(1).toInt() == 1;
}

QString RarUnpacker::processFileName(QString file)
{
	int x = g_re.indexIn(file);
	if(x != -1)
	{
		int i;
		for(i=x+5;file[i].isDigit();i++)
			file[i] = '0';
		file[i-1] = '1';
	}
	return file;
}

void RarUnpacker::extract(QList<bool> files, QString where)
{
	m_dirDestination = where;
	m_filesValues = files;
	for(int i=0;i<m_files.size();i++)
	{
		if(m_filesValues[i])
			m_nTotal += m_files[i].size;
	}
	
	start();
}

void RarUnpacker::pipe(int fileIndex, QProcess* process)
{
	m_pipe = process;
	m_nFileIndex = fileIndex;
	m_nTotal = m_files[fileIndex].size;
	
	start();
}

void RarUnpacker::processArchive()
{
	HANDLE handle;
	RAROpenArchiveData oa;

	try
	{
		memset(&oa, 0, sizeof(oa));
		
		QByteArray ba = m_strFile.toUtf8();
		char comment[4*1024] = "";
		
		oa.ArcName = (char*) ba.constData();
		oa.OpenMode = RAR_OM_LIST;
		oa.CmtBuf = comment;
		oa.CmtBufSize = sizeof(comment);
		
		handle = RAROpenArchive(&oa);
		if(!handle)
			throw tr("Unable to open the archive.");
		
		RARSetCallback(handle, rarCallback, (LPARAM) this);
		
		while(true)
		{
			int r;
                        RARHeaderDataEx hd;
			memset(&hd, 0, sizeof(hd));
			
			r = RARReadHeaderEx(handle, &hd);
			
			if(r == ERAR_END_ARCHIVE)
				break;
			else if(r == ERAR_BAD_DATA)
				throw tr("The archive is corrupt.");
			else if(r != 0)
				throw tr("Cannot read the archive: %1.").arg(r);
			
			FileEntry file;
			file.name = QString::fromWCharArray(hd.FileNameW);
			file.size = hd.UnpSize;
			file.size += qint64(hd.UnpSizeHigh) << 32;
			
			m_files << file;
			
			RARProcessFile(handle, RAR_SKIP, 0, 0);
		}
		
		if(oa.CmtState == 1 || oa.CmtState == ERAR_SMALL_BUF)
			m_strCommentArchive = QString::fromUtf8(comment, oa.CmtSize);
		
		RARCloseArchive(handle);
		runDialog(m_files);
	}
	catch(QString error)
	{
		RARCloseArchive(handle);
		
		if(!error.isEmpty())
			showError(error);
		deleteLater();
	}
}

void RarUnpacker::askPassword(QByteArray* out)
{
	PasswordDlg dlg;
	dlg.m_strCommentArchive = m_strCommentArchive;
	dlg.m_strCommentTransfer = m_strCommentTransfer;
	
	if(dlg.exec() == QDialog::Accepted)
		*out = dlg.m_strPassword.toUtf8();
}

int RarUnpacker::rarCallback(unsigned msg, unsigned long tthis, unsigned long p1, unsigned long p2)
{
	char* target = (char*) p1;
	RarUnpacker* This = (RarUnpacker*) tthis;
	
	if(This->m_bAbort)
		return -1;
	
	if(msg == UCM_NEEDPASSWORD)
	{
		QByteArray pass;
		
		QMetaObject::invokeMethod(This, "askPassword", (This->m_filesValues.isEmpty()) ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
					  Q_ARG(QByteArray*, &pass));
		This->m_strPassword = pass;
		
		if(pass.isEmpty())
			return -1;
		else
		{
			strncpy(target, pass.constData(), p2-1);
			target[p2] = 0;
			
		}
	}
	else if(msg == UCM_PROCESSDATA)
	{
		if(!This->m_pipe)
			This->m_file.write(target, p2);
		else
		{
			This->m_pipe->write(target, p2);
			This->m_pipe->waitForBytesWritten(-1);
		}
		This->m_nDone += p2;
		
		if(This->m_nTotal)
		{
			int pcts = 100*This->m_nDone/This->m_nTotal;
			if(pcts > This->m_nPercents)
				This->setProgress(This->m_nPercents = pcts);
		}
		if(This->m_nTotalFile)
		{
			int pcts = 100*This->m_file.pos()/This->m_nTotalFile;
			if(pcts > This->m_nPercentsFile)
				This->setFileProgress(This->m_nCurrentFile, This->m_nPercentsFile = pcts);
		}
		return 1;
	}
	else if(msg == UCM_CHANGEVOLUME)
	{
		if(p2 == RAR_VOL_ASK)
		{
			QString error = tr("Volume %1 not found").arg(target);
			QMetaObject::invokeMethod(This, "showError", Qt::QueuedConnection, Q_ARG(QString, error));
			return -1;
		}
		else if(p2 == RAR_VOL_NOTIFY)
		{
			qDebug() << "UCM_CHANGEVOLUME notify: " << target;
		}
	}
	
	return 0;
}

int RarUnpacker::findFile(QString file)
{
	for (int i = 0; i < m_files.size(); i++)
	{
		if (file == m_files[i].name)
			return i;
	}
	return -1;
}

void RarUnpacker::run()
{
	HANDLE handle;
	RAROpenArchiveData oa;
	RARHeaderDataEx hd;

	try
	{
		qDebug() << "Extraction of " << m_strFile;
		
		memset(&oa, 0, sizeof(oa));
		QByteArray ba = m_strFile.toUtf8();
		oa.ArcName = (char*) ba.constData();
		oa.OpenMode = RAR_OM_EXTRACT;
		
		handle = RAROpenArchive(&oa);
		if(!handle)
			throw tr("Unable to open the archive.");
		
		RARSetPassword(handle, m_strPassword.data());
		RARSetCallback(handle, rarCallback, (LPARAM) this);
		
		if(m_pipe == 0)
		{
			while (true)
			{
				int e, i;
				
				e = RARReadHeaderEx(handle, &hd);
				if(e == ERAR_END_ARCHIVE)
					break;
				i = findFile(QString::fromWCharArray(hd.FileNameW));
				if (i == -1)
					continue;
				
				if(!m_filesValues[i] || !m_files[i].size)
				{
					RARProcessFile(handle, RAR_SKIP, 0, 0);
					continue;
				}
				
				QString fpath = m_dirDestination.filePath(m_files[i].name);
				m_file.setFileName(fpath);
				
				int l = m_files[i].name.lastIndexOf('/');
				if(l != -1)
					m_dirDestination.mkpath(m_files[i].name.left(l));
				
				if(!m_file.open(QIODevice::WriteOnly))
					throw tr("Unable to open %1 for writing.").arg(fpath);
				
				setToolTip(tr("Extracting %1...").arg(m_files[i].name));
				m_nPercentsFile = -1;
				m_nTotalFile = m_files[i].size;
				m_nCurrentFile = i;
				
				e = RARProcessFile(handle, RAR_TEST, 0, 0);
				m_file.close();
				
				if(m_bAbort)
					throw QString();
				else if(e)
					throw tr("Cannot read the archive: %1.").arg(e);
			}
		}
		else
		{
			for(int i=0;i<m_files.size();i++)
			{
				int e;
				
				e = RARReadHeaderEx(handle, &hd);
				if(i != m_nFileIndex)
				{
					RARProcessFile(handle, RAR_SKIP, 0, 0);
					continue;
				}
				
				setToolTip(tr("Piping %1...").arg(m_files[i].name));
				m_nPercentsFile = -1;
				m_nTotalFile = m_files[i].size;
				m_nCurrentFile = i;
				
				e = RARProcessFile(handle, RAR_TEST, 0, 0);
				
				if(m_bAbort)
					throw QString();
				else if(e)
					throw tr("Cannot read the archive: %1.").arg(e);
				break;
			}
		}
		
		RARCloseArchive(handle);
	}
	catch(QString error)
	{
		m_file.close();
		RARCloseArchive(handle);
		
		if(!error.isEmpty())
			QMetaObject::invokeMethod(this, "showError", Qt::QueuedConnection, Q_ARG(QString, error));
	}
	
	qDebug() << "Extraction complete";
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void RarUnpacker::showError(QString error)
{
	QMessageBox::critical(getMainWindow(), tr("Unpack an archive"), error);
}
