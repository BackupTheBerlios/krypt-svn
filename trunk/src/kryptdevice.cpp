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
#include "kryptdevice.h"
#include "kryptdevice.moc"

int KryptDevice::_lastDevID = 0;

KryptDevice::KryptDevice ( KryptApp *kryptApp, const QString & udi ) :
    _kryptApp ( kryptApp ), _udi ( udi )
{
  _devID = ++_lastDevID;
  _halBackend = HALBackend::get();
  _cfg = kryptApp->getConfig();
  _passDialog = 0;
  _cUdi = 0;

  _isPresent = false;
  _isDecrypted = false;
  _isMounted = false;
  _isIgnored = false;

  _encryptOnUmount = false;

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

QPixmap KryptDevice::getIcon () const
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

  return KGlobal::iconLoader()->loadIcon ( deviceIcon, KIcon::NoGroup, KIcon::SizeLarge );
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
  _cfg->setGroup ( _udi );

  _cfg->writeEntry ( "dev_vendor", _vendor );

  _cfg->writeEntry ( "dev_product", _product );

  _cfg->writeEntry ( "dev_block_dev", _blockDev );

  _cfg->writeEntry ( "dev_type", _type );

  _cfg->writeEntry ( "dev_mount_point", _mountPoint );

  _cfg->writeEntry ( "is_ignored", _isIgnored );

  saveOption ( KRYPT_CONF_SHOW_MOUNT, _showMount );
  saveOption ( KRYPT_CONF_SHOW_UMOUNT, _showUMount );
  saveOption ( KRYPT_CONF_SHOW_ENCRYPT, _showEncrypt );
  saveOption ( KRYPT_CONF_SHOW_DECRYPT, _showDecrypt );
  saveOption ( KRYPT_CONF_SHOW_OPTIONS, _showOptions );
  saveOption ( KRYPT_CONF_AUTO_ENCRYPT, _autoEncrypt );
  saveOption ( KRYPT_CONF_AUTO_DECRYPT, _autoDecrypt );
  saveOption ( KRYPT_CONF_SHOW_POPUP, _showPopup );

  _cfg->sync();
}

KryptDevice::OptionType KryptDevice::loadOption ( const char *opt )
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

void KryptDevice::saveOption ( const char *opt, KryptDevice::OptionType optVal )
{
  const char *val = KRYPT_CONF_OPT_DEFAULT;

  if ( optVal == OptionOn ) val = KRYPT_CONF_OPT_ON;
  else if ( optVal == OptionOff ) val = KRYPT_CONF_OPT_OFF;

  _cfg->writeEntry ( opt, val );
}

void KryptDevice::slotLoadConfig()
{
  _cfg->setGroup ( _udi );

  if ( !_isPresent )
  {
    _vendor = _cfg->readEntry ( "dev_vendor", "" );
    _product = _cfg->readEntry ( "dev_product", "" );
    _blockDev = _cfg->readEntry ( "dev_block_dev", "" );
    _type = _cfg->readEntry ( "dev_type", "" );
    _mountPoint = _cfg->readEntry ( "dev_mount_point", "" );
  }

  _isIgnored = _cfg->readBoolEntry ( "is_ignored", false );

  _showMount = loadOption ( KRYPT_CONF_SHOW_MOUNT );
  _showUMount = loadOption ( KRYPT_CONF_SHOW_UMOUNT );
  _showEncrypt = loadOption ( KRYPT_CONF_SHOW_ENCRYPT );
  _showDecrypt = loadOption ( KRYPT_CONF_SHOW_DECRYPT );
  _showOptions = loadOption ( KRYPT_CONF_SHOW_OPTIONS );
  _autoEncrypt = loadOption ( KRYPT_CONF_AUTO_ENCRYPT );
  _autoDecrypt = loadOption ( KRYPT_CONF_AUTO_DECRYPT );
  _showPopup = loadOption ( KRYPT_CONF_SHOW_POPUP );

  _cfg->setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  _globShowMount = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_MOUNT, true );
  _globShowUMount = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_UMOUNT, true );
  _globShowEncrypt = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_ENCRYPT, true );
  _globShowDecrypt = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_DECRYPT, true );
  _globShowOptions = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_OPTIONS, true );

  _globAutoEncrypt = _cfg->readBoolEntry ( KRYPT_CONF_AUTO_ENCRYPT, true );
  _globAutoDecrypt = _cfg->readBoolEntry ( KRYPT_CONF_AUTO_DECRYPT, false );

  _globShowPopup = _cfg->readBoolEntry ( KRYPT_CONF_SHOW_POPUP, true );
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
  // TODO
}

void KryptDevice::slotPassDecrypt ( const QString &password )
{
  recreateCUdi();

  _halBackend->slotSendPassword ( _cUdi, password.ascii() );
}

void KryptDevice::passDialogCanceled()
{
  // We just set the pointer to 0. This should be called from within the KryptDialog,
  // which should remove itself (with deleteLater)
  _passDialog = 0;
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

  _passDialog->show();

  // TODO - wallet?
}
