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

#include <klocale.h>
#include <qmessagebox.h>

#include "kryptdebug.h"
#include "kryptglobal.h"
#include "kryptapp.h"
#include "kryptapp.moc"

#include "halbackend.h"
#include "kryptsystray.h"
#include "kryptdialog.h"
#include "kryptconf.h"

KryptApp::KryptApp() : _cfg ( "kryptrc" )
{
  _confDlg = 0;
  _kwallet = 0;

  checkConfig();
  loadConfig();

  _tray = new KryptSystemTray ( this, 0L, "KryptSysTray" );

  _tray->show();

  HALBackend *halBackend = HALBackend::get();

  if ( !halBackend->isOK() ) return;

  connect ( halBackend, SIGNAL ( sigHALEvent ( int, const QString& ) ),
            this, SLOT ( slotHALEvent ( int, const QString& ) ) );

  connect ( halBackend, SIGNAL ( sigNewInfo ( const QString& ) ),
            this, SLOT ( slotNewInfo ( const QString& ) ) );

  connect ( halBackend, SIGNAL ( sigError ( const QString&, const QString&, const QString& ) ),
            this, SLOT ( slotError ( const QString&, const QString&, const QString& ) ) );

  connect ( halBackend, SIGNAL ( sigPassError ( const QString&, const QString&, const QString& ) ),
            this, SLOT ( slotPassError ( const QString&, const QString&, const QString& ) ) );

  connect ( _tray, SIGNAL ( signalClickConfig() ),
            this, SLOT ( slotShowConfig() ) );

  halBackend->initScan();
}

KryptApp::~KryptApp()
{
  QMap<QString, KryptDevice*>::ConstIterator it;

  for ( it =  _udi2Dev.begin(); it != _udi2Dev.end(); ++it )
  {
    delete it.data();
  }

  _udi2Dev.clear();

  _id2Dev.clear();

  // TODO - Needed? Probably Qt does that anyway...

  if ( _confDlg != 0 )
  {
    delete _confDlg;
    _confDlg = 0;
  }
}

QString KryptApp::getHalDevEventDesc ( int eventID ) const
{
  switch ( eventID )
  {

    case KRYPT_HAL_DEV_EVENT_NEW:
      return "NEW";
      break;

    case KRYPT_HAL_DEV_EVENT_REMOVED:
      return "REMOVED";
      break;

    case KRYPT_HAL_DEV_EVENT_MAPPED:
      return "MAPPED";
      break;

    case KRYPT_HAL_DEV_EVENT_UNMAPPED:
      return "UNMAPPED";
      break;

    case KRYPT_HAL_DEV_EVENT_MOUNTED:
      return "MOUNTED";
      break;

    case KRYPT_HAL_DEV_EVENT_UMOUNTED:
      return "UMOUNTED";
      break;
  }

  return "?????";
}

void KryptApp::slotHALEvent ( int eventID, const QString& udi )
{
  KryptDevice *dev = 0;

#ifdef KRYPT_DEBUG
  kdDebug() << "HAL Device Event: ID: '" << getHalDevEventDesc ( eventID ) << "'; UDI: '" << udi << "'\n";
#endif

  if ( eventID == KRYPT_HAL_DEV_EVENT_REMOVED )
  {
    // Don't create this device if it doesn't exist for REMOVED events!
    dev = getDevice ( udi, false );
  }
  else
  {
    dev = getDevice ( udi, true );
  }

  if ( dev != 0 )
  {
    dev->slotHALEvent ( eventID, udi );
  }
}

void KryptApp::slotPassError ( const QString &udi, const QString &errorName, const QString &errorMsg )
{
  // Don't create this device if it doesn't exist
  KryptDevice *dev = getDevice ( udi, false );

  if ( dev != 0 )
  {
    dev->slotPassError ( udi, errorName, errorMsg );
  }
}

void KryptApp::slotError ( const QString &, const QString &, const QString &errorMsg )
{
  // If we use KMessageBox it is modal dialog - we don't want it,
  // as it breaks dbus/hal communication...

  QMessageBox* mb = new QMessageBox ( "Krypt: HAL Error", QString ( "%1" ).arg ( errorMsg ),
                                      QMessageBox::Warning,
                                      QMessageBox::Ok | QMessageBox::Default | QMessageBox::Escape,
                                      0, 0, 0, 0, false );

  mb->show();
}

void KryptApp::slotNewInfo ( const QString &info )
{
#ifdef KRYPT_DEBUG
  kdDebug() << info << endl;
#endif
}

KryptDevice * KryptApp::getDevice ( const QString &udi, bool create )
{
  if ( !_udi2Dev.contains ( udi ) )
  {
    if ( !create ) return 0;

    KryptDevice * nDev = new KryptDevice ( this, udi );

    _udi2Dev.insert ( udi, nDev );

    _id2Dev.insert ( nDev->getID(), nDev );

    connect ( this, SIGNAL ( signalConfigChanged() ),
              nDev, SLOT ( slotLoadConfig() ) );

    connect ( this, SIGNAL ( signalKWalletReady ( bool ) ),
              nDev, SLOT ( slotKWalletReady ( bool ) ) );
  }

  if ( _udi2Dev.contains ( udi ) )
  {
    return _udi2Dev[udi];
  }

  return 0;
}

KryptDevice * KryptApp::getDevice ( int id )
{
  if ( !_id2Dev.contains ( id ) )
  {
    return 0;
  }

  return _id2Dev[id];
}

KConfig * KryptApp::getConfig()
{
  return &_cfg;
}

QValueList<KryptDevice*> KryptApp::getDevices() const
{
  return _udi2Dev.values();
}

void KryptApp::slotShowConfig()
{
  if ( _confDlg )
  {
    delete _confDlg;
    _confDlg = 0;
  }

  createAllKnownDevices();

  _confDlg = new KryptConf ( this, getDevices() );

  connect ( _confDlg, SIGNAL ( signalConfigChanged() ),
            this, SLOT ( slotConfigChanged() ) );

  connect ( _confDlg, SIGNAL ( signalClosed() ),
            this, SLOT ( slotConfigClosed() ) );

  _confDlg->show();

}

void KryptApp::createAllKnownDevices()
{
  QStringList groups = _cfg.groupList();
  QStringList::Iterator it = groups.begin();
  QStringList::Iterator itEnd = groups.end();
  int prefLen = strlen ( KRYPT_CONF_UDI_PREFIX );

  for ( ; it != itEnd; ++it )
  {
    QString vol = *it;
    int volLen = vol.length();

    if ( volLen > prefLen && vol.startsWith ( KRYPT_CONF_UDI_PREFIX ) )
    {
      QString udi = vol.right ( volLen - prefLen );

      if ( !_udi2Dev.contains ( udi ) ) getDevice ( udi, true );
    }
  }
}

void KryptApp::slotConfigClosed()
{
  if ( _confDlg )
  {
    _confDlg->deleteLater();
    _confDlg = 0;
  }
}

KryptSystemTray *KryptApp::getKryptTray()
{
  return _tray;
}

void KryptApp::slotWalletOpened ( bool success )
{
  if ( !success || !_kwallet || !_kwallet->isOpen() )
  {
    if ( _kwallet != 0 )
    {
      delete _kwallet;
      _kwallet = 0;
    }

    emit signalKWalletReady ( false );

    return;
  }

  if ( !_kwallet->hasFolder ( KRYPT_KWALLET_FOLDER ) )
  {
    if ( !_kwallet->createFolder ( KRYPT_KWALLET_FOLDER ) )
    {
      delete _kwallet;
      _kwallet = 0;

      emit signalKWalletReady ( false );
      return;
    }
  }

  emit signalKWalletReady ( true );
}

void KryptApp::checkKWallet()
{
  if ( !_useKWallet ) return;

  if ( !_kwallet || !_kwallet->isOpen() )
  {
    if ( _kwallet != 0 )
    {
      delete _kwallet;
      _kwallet = 0;
    }

    _kwallet = KWallet::Wallet::openWallet ( KWallet::Wallet::LocalWallet(), 0, KWallet::Wallet::Asynchronous );

    connect ( _kwallet, SIGNAL ( walletOpened ( bool ) ), this, SLOT ( slotWalletOpened ( bool ) ) );
    return;
  }

  slotWalletOpened ( true );
}

KWallet::Wallet *KryptApp::getKWallet()
{
  return _kwallet;
}

void KryptApp::checkConfig()
{
  _cfg.setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  QString confVer = _cfg.readEntry ( KRYPT_CONF_VERSION, "0" );

  bool doSync = false;

  if ( confVer == "0" )
  {
    // Remove options from older version. Sorry ;)
    _cfg.deleteGroup ( "app" );
    _cfg.deleteGroup ( "device_desc" );
    _cfg.deleteGroup ( "tray" );
    _cfg.deleteGroup ( "devices" );
    doSync = true;
  }

  if ( confVer != KRYPT_VERSION_STRING )
  {
    _cfg.writeEntry ( KRYPT_CONF_VERSION, KRYPT_VERSION_STRING );
    doSync = true;
  }

  if ( doSync )
  {
    _cfg.sync();
  }
}

void KryptApp::slotConfigChanged()
{
  bool oldUseKWallet = _useKWallet;

  // We HAVE TO load global config BEFORE we signal that there have been some changes!
  loadConfig();

  emit signalConfigChanged();

  // We just started using KDE Wallet!
  // AFTER all devices updated their config, we want to try
  // to open the wallet - so they can store their passwords in it!

  if ( !oldUseKWallet && _useKWallet ) checkKWallet();
}

void KryptApp::loadConfig()
{
  _cfg.setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  // Defaults should be kept only in this file:
  _showMount = _cfg.readBoolEntry ( KRYPT_CONF_SHOW_MOUNT, true );

  _showUMount = _cfg.readBoolEntry ( KRYPT_CONF_SHOW_UMOUNT, true );
  _showEncrypt = _cfg.readBoolEntry ( KRYPT_CONF_SHOW_ENCRYPT, true );
  _showDecrypt = _cfg.readBoolEntry ( KRYPT_CONF_SHOW_DECRYPT, true );
  _showOptions = _cfg.readBoolEntry ( KRYPT_CONF_SHOW_OPTIONS, true );
  _showPopup = _cfg.readBoolEntry ( KRYPT_CONF_SHOW_POPUP, true );

  _autoEncrypt = _cfg.readBoolEntry ( KRYPT_CONF_AUTO_ENCRYPT, true );
  _autoDecrypt = _cfg.readBoolEntry ( KRYPT_CONF_AUTO_DECRYPT, false );

  _notifyAutoEncrypt = _cfg.readBoolEntry ( KRYPT_CONF_NOTIFY_AUTO_ENCRYPT, true );
  _notifyAutoDecrypt = _cfg.readBoolEntry ( KRYPT_CONF_NOTIFY_AUTO_DECRYPT, true );
  _notifyManualEncrypt = _cfg.readBoolEntry ( KRYPT_CONF_NOTIFY_MANUAL_ENCRYPT, false );
  _notifyManualDecrypt = _cfg.readBoolEntry ( KRYPT_CONF_NOTIFY_MANUAL_DECRYPT, false );

  _useKWallet = _cfg.readBoolEntry ( KRYPT_CONF_USE_KWALLET, true );

  _groupByCategory = _cfg.readBoolEntry ( KRYPT_CONF_GROUP_BY_CAT, false );
  _flatMenu = _cfg.readBoolEntry ( KRYPT_CONF_FLAT_MENU, false );
}

bool KryptApp::showMount() const
{
  return _showMount;
}

bool KryptApp::showUMount() const
{
  return _showUMount;
}

bool KryptApp::showEncrypt() const
{
  return _showEncrypt;
}

bool KryptApp::showDecrypt() const
{
  return _showDecrypt;
}

bool KryptApp::showOptions() const
{
  return _showOptions;
}

bool KryptApp::showPopup() const
{
  return _showPopup;
}

bool KryptApp::autoEncrypt() const
{
  return _autoEncrypt;
}

bool KryptApp::autoDecrypt() const
{
  return _autoDecrypt;
}

bool KryptApp::notifyAutoEncrypt() const
{
  return _notifyAutoEncrypt;
}

bool KryptApp::notifyAutoDecrypt() const
{
  return _notifyAutoDecrypt;
}

bool KryptApp::notifyManualEncrypt() const
{
  return _notifyManualEncrypt;
}

bool KryptApp::notifyManualDecrypt() const
{
  return _notifyManualDecrypt;
}

bool KryptApp::useKWallet() const
{
  return _useKWallet;
}

bool KryptApp::groupByCategory() const
{
  return _groupByCategory;
}

bool KryptApp::flatMenu() const
{
  return _flatMenu;
}
