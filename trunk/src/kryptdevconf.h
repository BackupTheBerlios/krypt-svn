/***************************************************************************
 *   Copyright (C) 2007, 2008 by Jakub Schmidtke                           *
 *   sjakub@users.berlios.de                                               *
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

#ifndef _KRYPT_DEV_CONF_H_
#define _KRYPT_DEV_CONF_H_

#include <kdialogbase.h>

#include "devconfdialog.h"
#include "kryptdevice.h"

class QRadioButton;

class KryptDevConf : public KDialogBase
{
  Q_OBJECT

public:
  KryptDevConf ( KryptDevice *kDev );
  ~KryptDevConf();

signals:
  void signalClosed();
  void signalConfigChanged();

protected slots:
  void slotCancel();
  void slotOk();
  void slotDefault();

private:
  KryptDevice *_kryptDev;
  DevConfDialog* _dlg;

  void setButtonGroup ( KryptDevice::OptionType oVal, QRadioButton *on, QRadioButton *off, QRadioButton *def );
  KryptDevice::OptionType getGroupVal ( QRadioButton *on, QRadioButton *off, QRadioButton *def ) const;
};

#endif // _KRYPT_DEV_CONF_H_
