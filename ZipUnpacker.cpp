/*
FatRat download manager
http://fatrat.dolezel.info

Copyright (C) 2006-2009 Lubos Dolezel <lubos a dolezel.info>

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

#include "ZipUnpacker.h"
#include <QMessageBox>
#include <fatrat/fatrat.h>
#include <errno.h>
#include <QFile>

ZipUnpacker::ZipUnpacker(QString file, QString transferComment)
	: Unpacker(file), m_strTransferComment(transferComment), m_zip(0), m_nTotal(0)
{
	processArchive();
}

ZipUnpacker::~ZipUnpacker()
{
	if(m_zip)
		zip_close(m_zip);
}

bool ZipUnpacker::supported(QString file)
{
	return true;
}

QString ZipUnpacker::zipError(int code, int scode)
{
	char buffer[255];
	zip_error_to_str(buffer, sizeof(buffer), code, scode);
	return QString::fromUtf8(buffer);
}

void ZipUnpacker::processArchive()
{
	int err;
	QByteArray ba = m_strFile.toUtf8();
	
	try
	{
		m_zip = zip_open(ba.constData(), 0, &err);
		
		if(!m_zip)
			throw zipError(err, errno);
		
		m_strArchiveComment = QString::fromUtf8(zip_get_archive_comment(m_zip, 0, 0));
		
		int nfiles = zip_get_num_files(m_zip);
		
		for(int i=0;i<nfiles;i++)
		{
			FileEntry file;
			struct zip_stat zstat;
			
			if(zip_stat_index(m_zip, i, 0, &zstat) < 0)
			{
				int code, scode;
				zip_error_get(m_zip, &code, &scode);
				throw zipError(code, scode);
			}
			
			file.name = QString::fromUtf8(zstat.name);
			file.size = zstat.size;
			
			m_files << file;
		}
		
		runDialog(m_files);
	}
	catch(QString errString)
	{
		QMessageBox::critical(getMainWindow(), tr("Unpack an archive"), errString);
	}
}

void ZipUnpacker::run()
{
	char buffer[4096];
	qint64 totalDone = 0;
	int totalPercents = 0;
	
	try
	{
		for(int i=0;i<m_files.size();i++)
		{
			if(!m_filesValues[i] || !m_files[i].size)
				continue;
			
			QFile file;
			QString fpath = m_dirDestination.filePath(m_files[i].name);
			struct zip_file* zfile;
			int percents = 0;
			
			zfile = zip_fopen_index(m_zip, i, 0);
			if(!zfile)
			{
				int code, scode;
				zip_error_get(m_zip, &code, &scode);
				throw zipError(code, scode);
			}
			
			try
			{
				file.setFileName(fpath);
				
				int l = m_files[i].name.lastIndexOf('/');
				if(l != -1)
					m_dirDestination.mkpath(m_files[i].name.left(l));
				
				if(!file.open(QIODevice::WriteOnly))
					throw tr("Unable to open %1 for writing.").arg(fpath);
				
				setToolTip(tr("Extracting %1...").arg(m_files[i].name));
				
				for(unsigned int pos=0;pos<m_files[i].size;)
				{
					int read = zip_fread(zfile, buffer, sizeof(buffer));
					if(read < 0)
						throw tr("Read error");
					file.write(buffer, read);
					totalDone += read;
					pos += read;
					
					if(m_nTotal)
					{
						int pcts = 100*totalDone/m_nTotal;
						if(pcts > totalPercents)
							setProgress(totalPercents = pcts);
					}
					if(m_files[i].size)
					{
						int pcts = 100*file.pos()/m_files[i].size;
						if(pcts > percents)
							setFileProgress(i, percents = pcts);
					}
				}
			}
			catch(...)
			{
				zip_fclose(zfile);
				throw;
			}
			zip_fclose(zfile);
		}
	}
	catch(QString errString)
	{
		QMessageBox::critical(getMainWindow(), tr("Unpack an archive"), errString);
	}
	
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void ZipUnpacker::extract(QList<bool> files, QString where)
{
	m_filesValues = files;
	m_dirDestination = QDir(where);
	
	for(int i=0;i<m_files.size();i++)
	{
		if(m_filesValues[i])
			m_nTotal += m_files[i].size;
	}
	
	start();
}

