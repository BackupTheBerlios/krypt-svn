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

#ifndef _KRYPT_SYS_TRAY_H_
#define _KRYPT_SYS_TRAY_H_

#include <qstringlist.h>
#include <ksystemtray.h>
#include <kconfig.h>

#include "kryptdevice.h"

class KryptApp;

class KPopupMenu;

class KHelpMenu;

class KryptSystemTray :  public KSystemTray
{
  Q_OBJECT

public:
  KryptSystemTray ( KryptApp *kryptApp, QWidget *parent = 0, const char *name = 0 );
  ~KryptSystemTray();

  virtual void contextMenuAboutToShow ( KPopupMenu *menu );

signals:
  void signalClickConfig();

public slots:
  void slotLoadConfig();

protected:
  void mousePressEvent ( QMouseEvent *e );

private:
  KryptApp *_kryptApp;

  KPopupMenu *_mountMenu;
  KPopupMenu *_umountMenu;
  KPopupMenu *_encryptMenu;
  KPopupMenu *_decryptMenu;
  KPopupMenu *_optionMenu;
  KHelpMenu *_helpMenu;

  QMap<KryptDevice*, KPopupMenu*> _devMenus;

  bool _groupByCategory;
  bool _flatMenu;

  KConfig* _cfg;

  void recreateMenu ( KPopupMenu* menu, bool full );
  void removeUnneeded ( QValueList<KryptDevice*> & devices );
  int createCategoryEntries ( KPopupMenu* menu, const QValueList<KryptDevice*> & devices, bool full );
  int createDevEntries ( KPopupMenu* menu, const QValueList<KryptDevice*> & devices, bool full );
  KPopupMenu *getDevMenu ( KPopupMenu *menu, KryptDevice *dev );
};

#endif // _KRYPT_SYS_TRAY_H_
