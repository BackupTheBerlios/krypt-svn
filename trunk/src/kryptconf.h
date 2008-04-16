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

#ifndef _KRYPT_CONF_H_
#define _KRYPT_CONF_H_

#include <kconfig.h>
#include <kdialogbase.h>

#include "confdialog.h"

class KryptConf : public KDialogBase
{
  Q_OBJECT

public:
  KryptConf ( KConfig *cfg );
  ~KryptConf();

signals:
  void sigConfChanged();

protected slots:
  void slotCancel();
  void slotOk();

private:
  ConfDialog* _dlg;
  KConfig *_cfg;
};

#endif // _KRYPT_CONF_H_
