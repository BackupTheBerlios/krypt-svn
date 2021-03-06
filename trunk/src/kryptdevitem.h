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

#ifndef _KRYPT_DEV_ITEM_H_
#define _KRYPT_DEV_ITEM_H_

#define KRYPT_DEV_ITEM_COL_NAME       0
#define KRYPT_DEV_ITEM_COL_BLOCK_DEV  1
#define KRYPT_DEV_ITEM_COL_IGNORED    2

#include <qlistview.h>

class KryptDevice;

class KryptDevItem : public QListViewItem
{

public:
  KryptDevItem ( QListView * parent, KryptDevice *kryptDev );
  virtual ~KryptDevItem();

  bool isIgnored();
  void toggleIgnored();
  KryptDevice *getKryptDevice();

private:
  KryptDevice *_kryptDev;
  bool _ignored;

  void doSetup();
};

#endif // _KRYPT_DEV_ITEM_H_
