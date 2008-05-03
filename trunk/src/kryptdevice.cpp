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

#include <kconfig.h>
#include <kdebug.h>

#include "kryptglobal.h"
#include "halbackend.h"
#include "kryptapp.h"
#include "kryptdialog.h"
#include "kryptdevconf.h"
#include "kryptdevice.h"
#include "kryptdevice.moc"

// Lets choose 'exotic' base :)
#define OBFUSCATE_BASE 35

int KryptDevice::_lastDevID = 0;

KryptDevice::KryptDevice ( KryptApp *kryptApp, const QString & udi ) :
    _kryptApp ( kryptApp ), _udi ( udi )
{
  _devID = ++_lastDevID;
  _halBackend = HALBackend::get();
  _cfg = kryptApp->getConfig();
  _passDialog = 0;
  _confDialog = 0;
  _cUdi = 0;

  _hasPassword = false;

  _isPresent = false;
  _isDecrypted = false;
  _isMounted = false;
  _isIgnored = false;

  _encryptOnUmount = false;

  _storePass = false;

  _password = "";

  slotLoadConfig();
}

KryptDevice::~KryptDevice()
{
  if ( _passDialog != 0 )
  {
    delete _passDialog;
    _passDialog = 0;
  }

  if ( _cUdi != 0 )
  {
    delete[] _cUdi;
    _cUdi = 0;
  }

  if ( _confDialog != 0 )
  {
    delete _confDialog;
    _confDialog = 0;
  }
}

void KryptDevice::recreateCUdi()
{
  if ( _cUdi != 0 )
  {
    delete[] _cUdi;
    _cUdi = 0;
  }

  _cUdi = new char[_udi.length() + 1];

  memcpy ( _cUdi, _udi.ascii(), _udi.length() + 1 );
}

int KryptDevice::getID() const
{
  return _devID;
}

const QString & KryptDevice::getUDI() const
{
  return _udi;
}

QString KryptDevice::getDesc( ) const
{
  return QString ( "%1 %2 (%3)" ).arg ( _vendor ).arg ( _product ).arg ( _blockDev );
}

QString KryptDevice::getName( ) const
{
  return QString ( "%1 %2" ).arg ( _vendor ).arg ( _product );
}

const QString & KryptDevice::getType() const
{
  return _type;
}

const QString & KryptDevice::getProduct() const
{
  return _product;
}

const QString & KryptDevice::getVendor() const
{
  return _vendor;
}

const QString & KryptDevice::getBlockDev() const
{
  return _blockDev;
}

QPixmap KryptDevice::getIcon ( KIcon::StdSizes size ) const
{
  QString deviceIcon;

  if ( _type == "memory_stick"
       || _type == "cdrom"
       || _type == "sd_mmc"
       || _type == "compact_flash"
       || _type == "smart_media"
       || _type == "zip" )
  {

    deviceIcon = QString ( "%1_unmount" ).arg ( _type );
  }
  else
  {
    deviceIcon = QString ( "hdd_unmount" );
  }

  return KGlobal::iconLoader()->loadIcon ( deviceIcon, KIcon::NoGroup, size );
}

QString KryptDevice::getConfigGroup ( const QString & forUdi )
{
  return QString ( KRYPT_CONF_UDI_PREFIX "%1" ).arg ( forUdi );
}

QString KryptDevice::getConfigGroup() const
{
  return getConfigGroup ( _udi );
}

bool KryptDevice::isDecrypted() const
{
  return _isDecrypted;
}

bool KryptDevice::isMounted() const
{
  return _isMounted;
}

bool KryptDevice::isIgnored() const
{
  return _isIgnored;
}

bool KryptDevice::isPresent() const
{
  return _isPresent;
}

bool KryptDevice::showMount() const
{
  if ( _showMount == KryptDevice::OptionOn ) return true;

  if ( _showMount == KryptDevice::OptionOff ) return false;

  return _globShowMount;
}

bool KryptDevice::showUMount() const
{
  if ( _showUMount == KryptDevice::OptionOn ) return true;

  if ( _showUMount == KryptDevice::OptionOff ) return false;

  return _globShowUMount;
}

bool KryptDevice::showEncrypt() const
{
  if ( _showEncrypt == KryptDevice::OptionOn ) return true;

  if ( _showEncrypt == KryptDevice::OptionOff ) return false;

  return _globShowEncrypt;
}

bool KryptDevice::showDecrypt() const
{
  if ( _showDecrypt == KryptDevice::OptionOn ) return true;

  if ( _showDecrypt == KryptDevice::OptionOff ) return false;

  return _globShowDecrypt;
}

bool KryptDevice::showOptions() const
{
  if ( _showOptions == KryptDevice::OptionOn ) return true;

  if ( _showOptions == KryptDevice::OptionOff ) return false;

  return _globShowOptions;
}

bool KryptDevice::showPopup() const
{
  if ( _showPopup == KryptDevice::OptionOn ) return true;

  if ( _showPopup == KryptDevice::OptionOff ) return false;

  return _globShowPopup;
}

bool KryptDevice::autoEncrypt() const
{
  if ( _autoEncrypt == KryptDevice::OptionOn ) return true;

  if ( _autoEncrypt == KryptDevice::OptionOff ) return false;

  return _globAutoEncrypt;
}

bool KryptDevice::autoDecrypt() const
{
  if ( _autoDecrypt == KryptDevice::OptionOn ) return true;

  if ( _autoDecrypt == KryptDevice::OptionOff ) return false;

  return _globAutoDecrypt;
}

void KryptDevice::slotHALEvent ( int eventID, const QString& udi )
{
  if ( udi != _udi )
  {
    kdDebug() << "KryptDevice with UDI: " << _udi << " received event for UDI: " << udi << endl;
    return;
  }

  if ( !_isPresent )
  {
    updateDeviceInfo();

    if ( !_isPresent )
    {
      return;
    }
  }

  bool removePass = false;

  bool doEncrypt = false;
  bool newDev = false;

  switch ( eventID )
  {

    case KRYPT_HAL_DEV_EVENT_NEW:
      _isDecrypted = false;
      _isMounted = false;
      newDev = true;
      break;

    case KRYPT_HAL_DEV_EVENT_REMOVED:
      _isDecrypted = false;
      _isMounted = false;
      _isPresent = false;
      removePass = true;
      break;

    case KRYPT_HAL_DEV_EVENT_MAPPED:
      _isDecrypted = true;
      _isMounted = false;
      removePass = true;
      break;

    case KRYPT_HAL_DEV_EVENT_UNMAPPED:
      _isDecrypted = false;
      _isMounted = false;
      break;

    case KRYPT_HAL_DEV_EVENT_MOUNTED:
      _isDecrypted = true;
      _isMounted = true;
      removePass = true;
      break;

    case KRYPT_HAL_DEV_EVENT_UMOUNTED:
      _isDecrypted = true;
      _isMounted = false;
      removePass = true;

      if ( _encryptOnUmount || autoEncrypt() )
      {
        doEncrypt = true;
      }

      break;

    default:
      kdDebug() << "Unknown HAL Device EventID: '" << eventID << "'\n";

      break;
  }

  _encryptOnUmount = false;

  if ( removePass && _passDialog != 0 )
  {
    delete _passDialog;
    _passDialog = 0;
  }

  if ( newDev )
  {
    checkNewDevice();
  }

  if ( doEncrypt )
  {
    _encryptOnUmount = false;
    slotClickEncrypt();
  }
}

void KryptDevice::slotPassError ( const QString &udi, const QString &errorName, const QString &errorMsg )
{
  if ( udi != _udi )
  {
    kdDebug() << "KryptDevice with UDI: " << _udi << " received pass error for UDI: " << udi << endl;
    return;
  }

  if ( !_isPresent )
  {
    updateDeviceInfo();
  }

  _passDialog->slotPassError ( errorName, errorMsg );
}

void KryptDevice::updateDeviceInfo()
{
  if ( _isPresent ) return;

  if ( !_halBackend->isDevicePresent ( _udi ) ) return;

  if ( !_halBackend->getDeviceInfo ( _udi, _vendor, _product, _blockDev, _type, _mountPoint ) ) return;

  _isPresent = true;

  slotSaveConfig();
}

void KryptDevice::slotSaveConfig()
{
  _cfg->setGroup ( getConfigGroup() );

  _cfg->writeEntry ( "dev_vendor", _vendor );

  _cfg->writeEntry ( "dev_product", _product );

  _cfg->writeEntry ( "dev_block_dev", _blockDev );

  _cfg->writeEntry ( "dev_type", _type );

  _cfg->writeEntry ( "dev_mount_point", _mountPoint );

  _cfg->writeEntry ( "is_ignored", _isIgnored );

  _cfg->writeEntry ( "store_password", _storePass );

  saveOption ( KRYPT_CONF_SHOW_MOUNT, _showMount );
  saveOption ( KRYPT_CONF_SHOW_UMOUNT, _showUMount );
  saveOption ( KRYPT_CONF_SHOW_ENCRYPT, _showEncrypt );
  saveOption ( KRYPT_CONF_SHOW_DECRYPT, _showDecrypt );
  saveOption ( KRYPT_CONF_SHOW_OPTIONS, _showOptions );
  saveOption ( KRYPT_CONF_AUTO_ENCRYPT, _autoEncrypt );
  saveOption ( KRYPT_CONF_AUTO_DECRYPT, _autoDecrypt );
  saveOption ( KRYPT_CONF_SHOW_POPUP, _showPopup );

  if ( _storePass && !_globPassInKWallet )
  {
    // It doesn't even try to encrypt the password,
    // but makes it a little bit less readable if someone
    // opens configuration file.
    _cfg->writeEntry ( "password", obfuscate ( _password ) );
  }
  else
  {
    _cfg->deleteEntry ( "password" );
  }

  _cfg->sync();

  emit signalConfigChanged();
}

QStringList KryptDevice::obfuscate ( const QString & str )
{
  QStringList ret;

  for ( unsigned int i = 0; i < str.length(); ++i )
  {
    ret.push_back ( QString::number ( str[i].unicode(), OBFUSCATE_BASE ) );
  }

  return ret;
}

QString KryptDevice::deobfuscate ( const QStringList & list )
{
  QString ret;
  bool ok;

  for ( unsigned int i = 0; i < list.size(); ++i )
  {
    QChar c = QChar ( list[i].toUShort ( &ok, OBFUSCATE_BASE ) );

    if ( ok ) ret.append ( c );
  }

  return ret;
}

KryptDevice::KryptDevice::OptionType KryptDevice::loadOption ( const char *opt )
{
  QString entry = _cfg->readEntry ( opt, KRYPT_CONF_OPT_DEFAULT ).lower();

  if ( entry == KRYPT_CONF_OPT_ON )
  {
    return OptionOn;
  }
  else if ( entry == KRYPT_CONF_OPT_OFF )
  {
    return OptionOff;
  }

  return OptionDefault;
}

void KryptDevice::saveOption ( const char *opt, KryptDevice::KryptDevice::OptionType optVal )
{
  const char *val = KRYPT_CONF_OPT_DEFAULT;

  if ( optVal == OptionOn ) val = KRYPT_CONF_OPT_ON;
  else if ( optVal == OptionOff ) val = KRYPT_CONF_OPT_OFF;

  _cfg->writeEntry ( opt, val );
}

void KryptDevice::slotLoadConfig()
{
  loadGlobalOptions();

  _cfg->setGroup ( getConfigGroup() );

  if ( !_isPresent )
  {
    _vendor = _cfg->readEntry ( "dev_vendor", "" );
    _product = _cfg->readEntry ( "dev_product", "" );
    _blockDev = _cfg->readEntry ( "dev_block_dev", "" );
    _type = _cfg->readEntry ( "dev_type", "" );
    _mountPoint = _cfg->readEntry ( "dev_mount_point", "" );
  }

  if ( _cfg->hasKey ( "is_ignored" ) )
  {
    _isIgnored = _cfg->readBoolEntry ( "is_ignored", false );
  }
  else
  {
    if ( _halBackend->isDeviceHotpluggable ( _udi ) == HALBackend::VolNotHotplug )
    {
      _isIgnored = true;
    }
    else
    {
      _isIgnored = false;
    }
  }

  _storePass = _cfg->readBoolEntry ( "store_password", false );

  _showMount = loadOption ( KRYPT_CONF_SHOW_MOUNT );

  _showUMount = loadOption ( KRYPT_CONF_SHOW_UMOUNT );
  _showEncrypt = loadOption ( KRYPT_CONF_SHOW_ENCRYPT );
  _showDecrypt = loadOption ( KRYPT_CONF_SHOW_DECRYPT );
  _showOptions = loadOption ( KRYPT_CONF_SHOW_OPTIONS );
  _autoEncrypt = loadOption ( KRYPT_CONF_AUTO_ENCRYPT );
  _autoDecrypt = loadOption ( KRYPT_CONF_AUTO_DECRYPT );
  _showPopup = loadOption ( KRYPT_CONF_SHOW_POPUP );

  if ( _storePass && !_globPassInKWallet )
  {
    _password = deobfuscate ( _cfg->readListEntry ( "password" ) );
  }
  else
  {
    _cfg->deleteEntry ( "password" );

    _cfg->sync();
  }
}

void KryptDevice::loadGlobalOptions()
{
  _cfg->setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  _globShowMount = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_MOUNT, true );
  _globShowUMount = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_UMOUNT, true );
  _globShowEncrypt = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_ENCRYPT, true );
  _globShowDecrypt = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_DECRYPT, true );
  _globShowOptions = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_OPTIONS, true );

  _globAutoEncrypt = _cfg->readBoolEntry ( KRYPT_CONF_AUTO_ENCRYPT, true );
  _globAutoDecrypt = _cfg->readBoolEntry ( KRYPT_CONF_AUTO_DECRYPT, false );

  _globShowPopup = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_POPUP, true );

  _globPassInKWallet = _cfg->readBoolEntry ( KRYPT_CONF_PASS_IN_WALLET, true );
}

void KryptDevice::slotClickMount()
{
  _encryptOnUmount = false;
  _halBackend->slotMountDevice ( _udi );
}

void KryptDevice::slotClickUMount()
{
  _encryptOnUmount = false;
  _halBackend->slotUmountDevice ( _udi );
}

void KryptDevice::slotClickEncrypt()
{
  if ( !_isMounted )
  {
    _encryptOnUmount = false;
    _halBackend->slotRemoveDevice ( _udi );
  }
  else
  {
    _encryptOnUmount = true;
    slotClickUMount();
  }
}

void KryptDevice::slotClickDecrypt()
{
  _encryptOnUmount = false;
  popupPassDialog();
}

void KryptDevice::slotClickOptions()
{
  if ( _confDialog != 0 )
  {
    delete _confDialog;
    _confDialog = 0;
  }

  _confDialog = new KryptDevConf ( this );

  connect ( _confDialog, SIGNAL ( signalClosed() ),
            this, SLOT ( slotClosedConfDialog() ) );

  connect ( _confDialog, SIGNAL ( signalConfigChanged() ),
            this, SLOT ( slotSaveConfig() ) );

  _confDialog->show();
}

void KryptDevice::slotPassDecrypt ( )
{
  if ( _password.length() < 1 ) return;

  recreateCUdi();

  _halBackend->slotSendPassword ( _cUdi, _password.ascii() );
}

void KryptDevice::slotClosedPassDialog()
{
  if ( _passDialog != 0 )
  {
    _passDialog->deleteLater();
    _passDialog = 0;
  }
}

void KryptDevice::slotClosedConfDialog()
{
  if ( _confDialog != 0 )
  {
    _confDialog->deleteLater();
    _confDialog = 0;
  }
}

void KryptDevice::checkNewDevice()
{
  if ( isIgnored() ) return;

  if ( !showPopup() ) return;

  // TODO - Wallet integration

  popupPassDialog();
}

void KryptDevice::popupPassDialog()
{
  if ( _passDialog != 0 )
  {
    delete _passDialog;
    _passDialog = 0;
  }

  if ( !_halBackend->isDevicePresent ( _udi ) ) return;

  if ( !_isPresent )
  {
    updateDeviceInfo();

    if ( !_isPresent ) return;
  }

  _passDialog = new KryptDialog ( this );

  connect ( _passDialog, SIGNAL ( signalClosed() ),
            this, SLOT ( slotClosedPassDialog() ) );

  _passDialog->show();

  // TODO - wallet?
}

QString KryptDevice::getPassword()
{
  // TODO - wallet/config
  return _password;
}

void KryptDevice::setPassword ( const QString & pass )
{
  // TODO - wallet/config
  _password = pass;
}

bool KryptDevice::usesKWallet() const
{
  return _globPassInKWallet;
}

bool KryptDevice::shouldAutoDecrypt() const
{
  if ( _autoDecrypt == OptionOn ) return true;

  if ( _autoDecrypt == OptionOff ) return false;

  // So it's default
  return _globAutoDecrypt;
}

KryptDevice::OptionType KryptDevice::getOptShowMount() const
{
  return _showMount;
}

KryptDevice::OptionType KryptDevice::getOptShowUMount() const
{
  return _showUMount;
}

KryptDevice::OptionType KryptDevice::getOptShowDecrypt() const
{
  return _showDecrypt;
}

KryptDevice::OptionType KryptDevice::getOptShowEncrypt() const
{
  return _showEncrypt;
}

KryptDevice::OptionType KryptDevice::getOptShowOptions() const
{
  return _showOptions;
}

KryptDevice::OptionType KryptDevice::getOptShowPopup() const
{
  return _showPopup;
}

KryptDevice::OptionType KryptDevice::getOptAutoDecrypt() const
{
  return _autoDecrypt;
}

KryptDevice::OptionType KryptDevice::getOptAutoEncrypt() const
{
  return _autoEncrypt;
}

bool KryptDevice::getStorePass() const
{
  return _storePass;
}

void KryptDevice::setOptShowMount ( KryptDevice::OptionType nOpt )
{
  _showMount = nOpt;
}

void KryptDevice::setOptShowUMount ( KryptDevice::OptionType nOpt )
{
  _showUMount = nOpt;
}

void KryptDevice::setOptShowDecrypt ( KryptDevice::OptionType nOpt )
{
  _showDecrypt = nOpt;
}

void KryptDevice::setOptShowEncrypt ( KryptDevice::OptionType nOpt )
{
  _showEncrypt = nOpt;
}

void KryptDevice::setOptShowOptions ( KryptDevice::OptionType nOpt )
{
  _showOptions = nOpt;
}

void KryptDevice::setOptShowPopup ( KryptDevice::OptionType nOpt )
{
  _showPopup = nOpt;
}

void KryptDevice::setOptAutoDecrypt ( KryptDevice::OptionType nOpt )
{
  _autoDecrypt = nOpt;
}

void KryptDevice::setOptAutoEncrypt ( KryptDevice::OptionType nOpt )
{
  _autoEncrypt = nOpt;
}

void KryptDevice::setStorePass ( bool nVal )
{
  _storePass = nVal;
}

void KryptDevice::setIgnored ( bool newIgnored )
{
  _isIgnored = newIgnored;
}
