/***************************************************************************
 *   Copyright (C) 2007, 2008 by Jakub Schmidtke                                 *
 *   sjakub@users.berlios.de                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef _KRYPT_DIALOG_H_
#define _KRYPT_DIALOG_H_

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kiconloader.h>

#include <qlineedit.h>
#include <qlabel.h>
#include <qgroupbox.h>

class DecryptDialog;

class KryptDialog : public KDialogBase
{
	Q_OBJECT

public:
	KryptDialog ( const QString &udi, const QString &vendor,
				  const QString &product, const QString &dev, const QString &devType );
	~KryptDialog();

	QString getPassword();

signals:
	void sigPassword ( char *udi, const char *password );

public slots:
	void slotPassError ( const QString& udi, const QString &errName, const QString &errMsg );
	void slotDevRemoved ( const QString &udi );
	void slotDevMapped ( const QString &udi );
	void slotPasswordChanged ( const QString &text );
	void slotCancel();
	void slotDecrypt();

private:
	DecryptDialog* _dlg;
	QString _udi;
	QString _device;
	char* _cUDI;

	void setDeviceIcon ( QString deviceType );
};

#endif // _KRYPT_DIALOG_H_
