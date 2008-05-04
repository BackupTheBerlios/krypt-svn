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

#include <qpixmap.h>
#include <kiconloader.h>
#include <kglobal.h>

#include "kryptdevice.h"
#include "kryptdevitem.h"

KryptDevItem::KryptDevItem ( QListView * parent, KryptDevice *kryptDev ) :
    QListViewItem ( parent ), _kryptDev ( kryptDev )
{
  doSetup();
}

KryptDevItem::~KryptDevItem()
{
}

void KryptDevItem::doSetup()
{
  setPixmap ( KRYPT_DEV_ITEM_COL_NAME, _kryptDev->getIcon ( KIcon::SizeSmall ) );
  setText ( KRYPT_DEV_ITEM_COL_NAME, _kryptDev->getName() );
  setText ( KRYPT_DEV_ITEM_COL_BLOCK_DEV, _kryptDev->getBlockDev() );
  void configureKryptDev();

  _ignored = false;

  if ( _kryptDev->isIgnored() )
  {
    toggleIgnored();
  }
}

bool KryptDevItem::isIgnored()
{
  return _ignored;
}

void KryptDevItem::toggleIgnored()
{
  _ignored = !_ignored;

  if ( _ignored )
  {
    //setPixmap ( KRYPT_DEV_ITEM_COL_IGNORED, KGlobal::iconLoader()->loadIcon ( "button_cancel", KIcon::NoGroup, KIcon::SizeSmall ) );
    setPixmap ( KRYPT_DEV_ITEM_COL_IGNORED, UserIcon ( "ignore" ) );
  }
  else
  {
    setPixmap ( KRYPT_DEV_ITEM_COL_IGNORED, 0 );
  }
}

KryptDevice *KryptDevItem::getKryptDevice()
{
  return _kryptDev;
}
