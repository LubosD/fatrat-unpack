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

#ifndef PASSWORDDLG_H
#define PASSWORDDLG_H
#include <QDialog>
#include "ui_PasswordDlg.h"

class PasswordDlg : public QDialog, public Ui_PasswordDlg
{
public:
	PasswordDlg(QWidget* parent = 0) : QDialog(parent)
	{
		setupUi(this);
	}
	int exec()
	{
		int r;
		linePassword->setText(m_strPassword);
		textArchive->setText(m_strCommentArchive);
		textTransfer->setText(m_strCommentTransfer);
		
		r = QDialog::exec();
		
		if(r == QDialog::Accepted)
			m_strPassword = linePassword->text();
		
		return r;
	}
	
	QString m_strPassword, m_strCommentArchive, m_strCommentTransfer;
};

#endif
