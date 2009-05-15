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

#include "UnpackDlg.h"
#include <fatrat/fatrat.h>

UnpackDlg::UnpackDlg(QWidget* parent)
	: QDialog(parent), m_bUpdating(false)
{
	setupUi(this);
	
	QTreeWidgetItem* hdr = treeFiles->headerItem();
	hdr->setText(0, tr("Name"));
	hdr->setText(1, tr("Size"));
	hdr->setText(2, tr("Progress"));
	treeFiles->setColumnWidth(0, 300);
	
	connect(treeFiles, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(fileItemChanged(QTreeWidgetItem*,int)));
}

void UnpackDlg::closeEvent(QCloseEvent* event)
{
	pushAbort->animateClick();
	QDialog::closeEvent(event);
}

void UnpackDlg::setFileProgress(int index, int progress)
{
	m_files[index]->setText(2, QString("%1%").arg(progress));
}

void UnpackDlg::load(const QList<FileEntry>& files)
{
	for(int i=0;i<files.size();i++)
	{
		QStringList elems = files[i].name.split('/');
		QTreeWidgetItem* item = 0;
		
		for(int x=0;x<elems.size();x++)
		{
			if(item != 0)
			{
				bool bFound = false;
				for(int i=0;i<item->childCount();i++)
				{
					QTreeWidgetItem* c = item->child(i);
					if(c->text(0) == elems[x])
					{
						bFound = true;
						item = c;
					}
				}
				
				if(!bFound)
					item = new QTreeWidgetItem(item, QStringList( elems[x] ));
			}
			else
			{
				bool bFound = false;
				for(int i=0;i<treeFiles->topLevelItemCount();i++)
				{
					QTreeWidgetItem* c = treeFiles->topLevelItem(i);
					if(c->text(0) == elems[x])
					{
						bFound = true;
						item = c;
					}
				}
				
				if(!bFound)
					item = new QTreeWidgetItem(treeFiles, QStringList( elems[x] ));
			}
		}
		
		// fill in info
		if(item->text(0).isEmpty())
			item->setHidden(true);
		item->setText(1, formatSize(files[i].size));
		item->setData(1, Qt::UserRole, qint64(files[i].size));
		m_files << item;
	}
	
	for(int i=0;i<m_files.size();i++)
	{
		m_files[i]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		m_files[i]->setCheckState(0, Qt::Checked);
	}
	
	recursiveUpdateDown(treeFiles->invisibleRootItem());
	treeFiles->expandAll();
	updatePipeButton();
}

QList<bool> UnpackDlg::getStates() const
{
	QList<bool> retval;
	for(int i=0;i<m_files.size();i++)
	{
		retval << (m_files[i]->checkState(0) == Qt::Checked);
		m_files[i]->setData(2, Qt::UserRole, (int) m_files[i]->checkState(0));
	}
	return retval;
}

void UnpackDlg::fileItemChanged(QTreeWidgetItem* item, int column)
{
	if(!pushUnpack->isEnabled())
	{
		item->setCheckState(0, (Qt::CheckState) item->data(2, Qt::UserRole).toInt());
		return;
	}
	
	if(column != 0 || m_bUpdating)
		return;
	
	m_bUpdating = true;
	
	if(item->childCount())
	{
		// directory
		recursiveCheck(item, item->checkState(0));
	}
	
	if(QTreeWidgetItem* parent = item->parent())
		recursiveUpdate(parent);
	
	m_bUpdating = false;
	
	updatePipeButton();
}

void UnpackDlg::updatePipeButton()
{
	QList<bool> data = getStates();
	pushPipe->setEnabled(data.count(true) == 1);
}

void UnpackDlg::recursiveUpdate(QTreeWidgetItem* item)
{
	int yes = 0, no = 0;
	int total = item->childCount();
	
	for(int i=0;i<total;i++)
	{
		int state = item->child(i)->checkState(0);
		if(state == Qt::Checked)
			yes++;
		else if(state == Qt::Unchecked)
			no++;
	}
	
	if(yes == total)
		item->setCheckState(0, Qt::Checked);
	else if(no == total)
		item->setCheckState(0, Qt::Unchecked);
	else
		item->setCheckState(0, Qt::PartiallyChecked);
	
	if(QTreeWidgetItem* parent = item->parent())
		recursiveUpdate(parent);
}

qint64 UnpackDlg::recursiveUpdateDown(QTreeWidgetItem* item)
{
	int yes = 0, no = 0;
	int total = item->childCount();
	qint64 size = 0;
	
	for(int i=0;i<total;i++)
	{
		QTreeWidgetItem* child = item->child(i);
		
		if(child->childCount())
			size += recursiveUpdateDown(child);
		
		int state = child->checkState(0);
		if(state == Qt::Checked)
			yes++;
		else if(state == Qt::Unchecked)
			no++;
		
		size += child->data(1, Qt::UserRole).toLongLong();
	}
	
	if(yes == total)
		item->setCheckState(0, Qt::Checked);
	else if(no == total)
		item->setCheckState(0, Qt::Unchecked);
	else
		item->setCheckState(0, Qt::PartiallyChecked);
	
	item->setText(1, formatSize(size));
	
	return size;
}

void UnpackDlg::recursiveCheck(QTreeWidgetItem* item, Qt::CheckState state)
{
	item->setCheckState(0, state);
	for(int i=0;i<item->childCount();i++)
		recursiveCheck(item->child(i), state);
}

