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

#include <qtooltip.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>

#include "kryptglobal.h"
#include "kryptapp.h"
#include "kryptsystray.h"
#include "kryptsystray.moc"

KryptSystemTray::KryptSystemTray ( KryptApp *kryptApp, QWidget* parent, const char *name )
    : KSystemTray ( parent, name ), _kryptApp ( kryptApp )
{
  _mountMenu = new KPopupMenu ( this->contextMenu (), "mountmenu" );
  _umountMenu = new KPopupMenu ( this->contextMenu (), "umountmenu" );
  _encryptMenu = new KPopupMenu ( this->contextMenu (), "encryptmenu" );
  _decryptMenu = new KPopupMenu ( this->contextMenu (), "decryptmenu" );
  _optionMenu = new KPopupMenu ( this->contextMenu (), "optionmenu" );
  _helpMenu    = new KHelpMenu ( this, KGlobal::instance ()->aboutData (), false );

  _mountMenu->setCheckable ( false );
  _umountMenu->setCheckable ( false );
  _encryptMenu->setCheckable ( false );
  _decryptMenu->setCheckable ( false );
  _optionMenu->setCheckable ( false );

  _helpMenu->menu ()->removeItemAt ( KHelpMenu::menuHelpContents );
  /* once the help menu is gone, remove the separator which is at position KHelpMenu::menuHelpContents now */
  _helpMenu->menu ()->removeItemAt ( KHelpMenu::menuHelpContents );

  setPixmap ( KSystemTray::loadIcon ( "krypt" ) );
  setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
  connect ( this, SIGNAL ( quitSelected() ), kapp, SLOT ( quit() ) );
  QToolTip::add ( this, i18n ( "Control LUKS encrypted volumes" ) );
}

KryptSystemTray::~KryptSystemTray()
{
  delete _mountMenu;
  delete _umountMenu;
  delete _encryptMenu;
  delete _decryptMenu;
  delete _optionMenu;
  delete _helpMenu;

  QMap<KryptDevice*, KPopupMenu*>::ConstIterator it;

  for ( it =  _devMenus.begin(); it != _devMenus.end(); ++it )
  {
    delete it.data();
  }

  _devMenus.clear();
}

void KryptSystemTray::mousePressEvent ( QMouseEvent* e )
{
  // Popup the context menu with left-click
  if ( e->button() == LeftButton )
  {
    // Show the same context menu, but without help and configuration options
    recreateMenu ( contextMenu(), false );
    contextMenu()->popup ( e->globalPos() );
    e->accept();
    return;
  }

  KSystemTray::mousePressEvent ( e );
}

void KryptSystemTray::removeUnneeded ( QValueList<KryptDevice*> & devices )
{
  // Removes devices from the list to be added to the menu
  // as well as menu objects that are not needed anymore
  QValueList<KryptDevice*>::Iterator it = devices.begin();
  QValueList<KryptDevice*>::Iterator itEnd = devices.end();

  while ( it != itEnd )
  {
    KryptDevice *dev = *it;

    // Menu entry for this device is not needed...

    if ( _kryptApp->flatMenu() || _kryptApp->groupByCategory() || !dev->isPresent() || dev->isIgnored() )
    {
      // ... but it exists
      if ( _devMenus.contains ( dev ) )
      {
        delete _devMenus[dev];
        _devMenus.remove ( dev );
      }
    }

    // This device is not needed
    if ( !dev->isPresent() || dev->isIgnored() )
    {
      it = devices.remove ( it );
    }
    else
    {
      ++it;
    }
  }
}

int KryptSystemTray::createCategoryEntries ( KPopupMenu* menu, const QValueList<KryptDevice*> & devices, bool full )
{
  int lastIndex = 0;
  QValueList<KryptDevice*>::ConstIterator itBeg = devices.begin();
  QValueList<KryptDevice*>::ConstIterator itEnd = devices.end();
  QValueList<KryptDevice*>::ConstIterator it;
  KryptDevice *dev;
  bool added;

  added = false;

  // UMOUNT Category

  for ( it = itBeg; it != itEnd; ++it )
  {
    dev = *it;

    if ( dev->showUMount() && dev->isMounted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( UserIcon ( "umount" ), i18n ( "Umount Volume" ) );
          added = true;
        }

        lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickUMount() ) );
      }

      else
      {
        if ( !added )
        {
          _umountMenu->clear();
          lastIndex = menu->insertItem ( UserIcon ( "umount" ), i18n ( "Umount Volume" ), _umountMenu, -1 );
          added = true;
        }

        _umountMenu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickUMount() ) );
      }
    }
  }

  added = false;

  // MOUNT Category

  for ( it = itBeg; it != itEnd; ++it )
  {
    dev = *it;

    if ( dev->showMount() && !dev->isMounted() && dev->isDecrypted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( UserIcon ( "mount" ), i18n ( "Mount Volume" ) );
          added = true;
        }

        lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickMount() ) );
      }

      else
      {
        if ( !added )
        {
          _mountMenu->clear();
          lastIndex = menu->insertItem ( UserIcon ( "mount" ), i18n ( "Mount Volume" ), _mountMenu, -1 );
          added = true;
        }

        _mountMenu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickMount() ) );
      }
    }
  }

  added = false;

  // ENCRYPT Category

  for ( it = itBeg; it != itEnd; ++it )
  {
    dev = *it;

    if ( dev->showEncrypt() && dev->isDecrypted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( UserIcon ( "encrypt" ), i18n ( "Encrypt Volume" ) );
          added = true;
        }

        lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickEncrypt() ) );
      }

      else
      {
        if ( !added )
        {
          _encryptMenu->clear();
          lastIndex = menu->insertItem ( UserIcon ( "encrypt" ), i18n ( "Encrypt Volume" ), _encryptMenu, -1 );
          added = true;
        }

        _encryptMenu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickEncrypt() ) );
      }
    }
  }

  added = false;

  // DECRYPT Category

  for ( it = itBeg; it != itEnd; ++it )
  {
    dev = *it;

    if ( dev->showDecrypt() && !dev->isDecrypted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( UserIcon ( "decrypt" ), i18n ( "Decrypt Volume" ) );
          added = true;
        }

        lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickDecrypt() ) );
      }

      else
      {
        if ( !added )
        {
          _decryptMenu->clear();
          lastIndex = menu->insertItem ( UserIcon ( "decrypt" ), i18n ( "Decrypt Volume" ), _decryptMenu, -1 );
          added = true;
        }

        _decryptMenu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickDecrypt() ) );
      }
    }
  }

  added = false;

  if ( full )
  {
    // OPTIONS Category

    for ( it = itBeg; it != itEnd; ++it )
    {
      dev = *it;

      if ( dev->showOptions() )
      {
        if ( _kryptApp->flatMenu() )
        {
          if ( !added )
          {
            lastIndex = menu->insertTitle ( SmallIcon ( "configure" ), i18n ( "Volume Options" ) );
            added = true;
          }

          lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickOptions() ) );
        }

        else
        {
          if ( !added )
          {
            _optionMenu->clear();
            lastIndex = menu->insertItem ( SmallIconSet ( "configure" ), i18n ( "Volume Options" ), _optionMenu, -1 );
            added = true;
          }

          _optionMenu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), dev, SLOT ( slotClickOptions() ) );
        }
      }
    }
  }

  return lastIndex;
}

KPopupMenu *KryptSystemTray::getDevMenu ( KPopupMenu *menu, KryptDevice *dev )
{
  if ( !_devMenus.contains ( dev ) )
  {
    _devMenus[dev] = new KPopupMenu ( menu, dev->getUDI() );
  }

  return _devMenus[dev];
}

int KryptSystemTray::createDevEntries ( KPopupMenu* menu, const QValueList<KryptDevice*> & devices, bool full )
{
  int lastIndex = 0;
  QValueList<KryptDevice*>::ConstIterator itBeg = devices.begin();
  QValueList<KryptDevice*>::ConstIterator itEnd = devices.end();
  QValueList<KryptDevice*>::ConstIterator it;
  KryptDevice *dev;

  for ( it = itBeg; it != itEnd; ++it )
  {
    bool added = false;
    KPopupMenu *devMenu = 0;
    dev = *it;

    // UMOUNT

    if ( dev->showUMount() && dev->isMounted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc() );
          added = true;
        }

        lastIndex = menu->insertItem ( UserIcon ( "umount" ), i18n ( "Umount Volume" ) , dev, SLOT ( slotClickUMount() ) );
      }
      else
      {
        if ( !devMenu ) devMenu = getDevMenu ( menu, dev );

        if ( !added )
        {
          devMenu->clear();
          lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), devMenu, -1 );
          added = true;
        }

        devMenu->insertItem ( UserIcon ( "umount" ), i18n ( "Umount Volume" ) , dev, SLOT ( slotClickUMount() ) );
      }
    }

    // MOUNT
    if ( dev->showMount() && !dev->isMounted() && dev->isDecrypted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc() );
          added = true;
        }

        lastIndex = menu->insertItem ( UserIcon ( "mount" ), i18n ( "Mount Volume" ) , dev, SLOT ( slotClickMount() ) );
      }
      else
      {
        if ( !devMenu ) devMenu = getDevMenu ( menu, dev );

        if ( !added )
        {
          devMenu->clear();
          lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), devMenu, -1 );
          added = true;
        }

        devMenu->insertItem ( UserIcon ( "mount" ), i18n ( "Mount Volume" ) , dev, SLOT ( slotClickMount() ) );
      }
    }

    // ENCRYPT
    if ( dev->showEncrypt() && dev->isDecrypted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc() );
          added = true;
        }

        lastIndex = menu->insertItem ( UserIcon ( "encrypt" ), i18n ( "Encrypt Volume" ) , dev, SLOT ( slotClickEncrypt() ) );
      }
      else
      {
        if ( !devMenu ) devMenu = getDevMenu ( menu, dev );

        if ( !added )
        {
          devMenu->clear();
          lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), devMenu, -1 );
          added = true;
        }

        devMenu->insertItem ( UserIcon ( "encrypt" ), i18n ( "Encrypt Volume" ) , dev, SLOT ( slotClickEncrypt() ) );
      }
    }

    // DECRYPT
    if ( dev->showDecrypt() && !dev->isDecrypted() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc() );
          added = true;
        }

        lastIndex = menu->insertItem ( UserIcon ( "decrypt" ), i18n ( "Decrypt Volume" ) , dev, SLOT ( slotClickDecrypt() ) );
      }
      else
      {
        if ( !devMenu ) devMenu = getDevMenu ( menu, dev );

        if ( !added )
        {
          devMenu->clear();
          lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), devMenu, -1 );
          added = true;
        }

        devMenu->insertItem ( UserIcon ( "decrypt" ), i18n ( "Decrypt Volume" ) , dev, SLOT ( slotClickDecrypt() ) );
      }
    }

    // OPTIONS
    if ( full && dev->showOptions() )
    {
      if ( _kryptApp->flatMenu() )
      {
        if ( !added )
        {
          lastIndex = menu->insertTitle ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc() );
          added = true;
        }

        lastIndex = menu->insertItem ( SmallIconSet ( "configure" ), i18n ( "Volume Options" ) , dev, SLOT ( slotClickOptions() ) );
      }
      else
      {
        if ( !devMenu ) devMenu = getDevMenu ( menu, dev );

        if ( !added )
        {
          devMenu->clear();
          lastIndex = menu->insertItem ( dev->getIcon ( KIcon::SizeSmall ), dev->getDesc(), devMenu, -1 );
          added = true;
        }

        devMenu->insertItem ( SmallIconSet ( "configure" ), i18n ( "Volume Options" ) , dev, SLOT ( slotClickOptions() ) );
      }
    }
  }

  return lastIndex;
}

void KryptSystemTray::recreateMenu ( KPopupMenu* menu, bool full )
{
  int lastIndex = 0;

  menu->clear();

  QValueList<KryptDevice*> devices = _kryptApp->getDevices();

  // This will remove devices that should be ignored, or are no longer
  // present. It will also clean any unnecessary _devMenus entries
  // and delete their KPopupMenu objects.
  removeUnneeded ( devices );

  if ( _kryptApp->groupByCategory() )
  {
    lastIndex = createCategoryEntries ( menu, devices, full );
  }
  else
  {
    lastIndex = createDevEntries ( menu, devices, full );
  }

  if ( !lastIndex )
  {
    lastIndex = menu->insertItem ( i18n ( "Nothing to display..." ) );
    menu->setItemEnabled ( lastIndex, false );
  }

  if ( full )
  {
    menu->insertSeparator();

    KAction *actPrefs = new KAction ( i18n ( "Configure Krypt..." ),
                                      SmallIconSet ( "configure" ), KShortcut(), this, SIGNAL ( signalClickConfig() ),
                                      actionCollection() );
    actPrefs->plug ( menu );

    /* Help menu */
    menu->insertItem ( SmallIcon ( "help" ), i18n ( "&Help" ), _helpMenu->menu () );
    menu->insertSeparator ();

    KAction *quitAction = actionCollection()->action ( KStdAction::name ( KStdAction::Quit ) );
    quitAction->plug ( menu );
  }
}

void KryptSystemTray::contextMenuAboutToShow ( KPopupMenu* menu )
{
  recreateMenu ( menu, true );
}
