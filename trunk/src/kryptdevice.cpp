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

#include <kconfig.h>
#include <kdebug.h>
#include <kwallet.h>
#include <klocale.h>
#include <kiconloader.h>

#include "halbackend.h"
#include "kryptapp.h"
#include "kryptglobal.h"
#include "kryptdialog.h"
#include "kryptdevconf.h"
#include "kryptdevice.h"
#include "kryptdevice.moc"

KryptDevice::KryptDevice ( KryptApp *kryptApp, const QString & udi ) :
    _kryptApp ( kryptApp ), _udi ( udi )
{
  _passDialog = 0;
  _confDialog = 0;
  _cUdi = 0;

  _devID = ++_lastDevID;
  _halBackend = HALBackend::get();
  _cfg = kryptApp->getConfig();

  _isPresent = false;
  _isDecrypted = false;
  _isMounted = false;
  _isIgnored = false;

  _manualDecrypt = false;
  _manualEncrypt = false;
  _manualEncryptOfMounted = false;

  _saveToKWallet = false;
  _waitingToDecrypt = false;

  _storePass = false;

  _password = "";

  slotLoadConfig();
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

  bool encDev = false;
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

      // Volume is decrypted

    case KRYPT_HAL_DEV_EVENT_MAPPED:
      _isDecrypted = true;
      _isMounted = false;
      removePass = true;

      // This is manual decryption and we want manual decryption notifications
      // or this is automatic one, and we want notifications for those

      if ( ( _manualDecrypt && _kryptApp->notifyManualDecrypt() )
           || ( !_manualDecrypt && _kryptApp->notifyAutoDecrypt() ) )
      {
        showTrayMessage ( i18n ( "Volume is now: Decrypted" ), UserIcon ( "decrypt_48" ) );
      }

      break;

    case KRYPT_HAL_DEV_EVENT_UNMAPPED:
      _isDecrypted = false;
      _isMounted = false;

      // This is manual encryption and we want manual encryption notifications
      // or this is automatic one, and we want notifications for those

      if ( ( _manualEncrypt && _kryptApp->notifyManualEncrypt() )
           || ( !_manualEncrypt && _kryptApp->notifyAutoEncrypt() ) )
      {
        showTrayMessage ( i18n ( "Volume is now: Encrypted" ), UserIcon ( "encrypt_48" ) );
      }

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

      if ( _manualEncryptOfMounted )
      {
        // This umount is because user clicked 'encrypt' on mounted device
        encDev = true;
      }
      else if ( autoEncrypt() )
      {
        // This umount is normal, but we want to do auto-encryption on umount.
        // So lets mark it as automatic one:
        _manualEncrypt = false;
        encDev = true;
      }

      break;

    default:
      kdDebug() << "Unknown HAL Device EventID: '" << eventID << "'\n";

      break;
  }

  _manualEncryptOfMounted = false;

  if ( removePass && _passDialog != 0 )
  {
    delete _passDialog;
    _passDialog = 0;
  }

  if ( newDev )
  {
    checkNewDevice();
  }

  if ( encDev )
  {
    doEncrypt();
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

  // It is possible that there is no password dialog - when there was a password
  // available. Krypt didn't show password dialog
  // and tried to decrypt the volume with the password it knows, but, for some reason,
  // it failed. Sometimes we want to show the password dialog on error, sometimes we don't.
  if ( !_passDialog )
  {
    if ( _manualDecrypt )
    {
      // User clicked 'decrypt' button - for sure he wants to see if something is wrong!
      showPassDialog();
    }
    else if ( !_manualDecrypt && showPopup() )
    {
      // User didn't click anything - automatic decryption is used,
      // and he wants to see the password pop-up for this device - so lets show it to him
      // (with an error)
      showPassDialog();
    }
  }

  if ( _passDialog != 0 )
  {
    _passDialog->slotPassError ( errorName, errorMsg );
  }
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

  if ( _storePass && !_kryptApp->useKWallet() )
  {
    // User wants to store the password in config file

    // It doesn't even try to encrypt the password,
    // but makes it a little bit less readable if someone
    // opens configuration file.
    _cfg->writeEntry ( "password", obfuscate ( _password ) );
  }
  else
  {
    // In any other case - remove the password from config file!

    _cfg->deleteEntry ( "password" );

    if ( _kryptApp->useKWallet() )
    {
      // If the wallet is used, no matter if we want to store the password or not
      // This will also remove saved password if _kryptApp->useKWallet() = true, but _storePass = false
      // If slotSaveConfig is called by global configuration dialog, there will
      // be signal walletReady emited by KryptApp, if we just started using KDE Wallet.
      // Otherwise, checkKWallet will be called by KryptDevConf - we just mark that we want to update KDE Wallet
      _saveToKWallet = true;
    }
  }

  _cfg->sync();

  emit signalConfigChanged();
}

void KryptDevice::checkKWallet()
{
  _kryptApp->checkKWallet();
}

void KryptDevice::slotLoadConfig()
{
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

  if ( _storePass && !_kryptApp->useKWallet() )
  {
    // We store password in config file - lets read it!
    // We don't check if it exists in config file - this is on purpose.
    // If user just changed password storage, from Wallet to config file,
    // and the password has been already read from the wallet, we don't
    // want it to be stored in config file (for security reasons)
    // So if we just started using config file for passwords,
    // this will remove password read before from the Wallet
    _password = deobfuscate ( _cfg->readListEntry ( "password" ) );
  }
  else
  {
    // In any other case - either we don't want to store the password,
    // or it should be stored in KDE Wallet - we want to make sure it is not
    // left in the config file!
    _cfg->deleteEntry ( "password" );

    _cfg->sync();
  }

  if ( _kryptApp->useKWallet() )
  {
    // No matter if we want to store the password or not, we want
    // to update KDE Wallet at next possibility - either remove existing
    // password, or store it. If this is executed because of global
    // configuration change, and we just started using KDE Wallet,
    // the 'checkWallet' will be executed by
    // KryptApp, and signal walletReady will be emited.
    // If there are many devices, we don't want each of them to call
    // checkKWallet - so we just mark that the password should be saved if
    // opportunity arises.

    _saveToKWallet = true;
  }
}

void KryptDevice::slotClickMount()
{
  _manualDecrypt = false;
  _manualEncrypt = false;
  _manualEncryptOfMounted = false;
  _halBackend->slotMountDevice ( _udi );
}

void KryptDevice::slotClickUMount()
{
  _manualDecrypt = false;
  _manualEncrypt = false;
  _manualEncryptOfMounted = false;
  doUMount();
}

void KryptDevice::doUMount()
{
  _halBackend->slotUmountDevice ( _udi );
}

void KryptDevice::doEncrypt()
{
  _halBackend->slotRemoveDevice ( _udi );
}

void KryptDevice::slotClickEncrypt()
{
  _manualDecrypt = false;
  _manualEncrypt = true;

  if ( !_isMounted )
  {
    _manualEncryptOfMounted = false;
    doEncrypt();
  }
  else
  {
    _manualEncryptOfMounted = true;
    doUMount();
  }
}

void KryptDevice::slotClickDecrypt()
{
  // Manual decryption
  _manualDecrypt = true;
  _manualEncrypt = false;
  _manualEncryptOfMounted = false;

  // We have the password. Just try to decrypt the device.

  if ( _password.length() > 0 )
  {
    slotPassDecrypt();
    return;
  }

  // We don't have the password

  if ( _storePass && _kryptApp->useKWallet() )
  {
    // But we store the password for this device, and we are using KDE Wallet
    // Try to get the password from there!

    // Mark that we are waiting for the password to decrypt this device
    _waitingToDecrypt = true;

    // And check the wallet!
    checkKWallet();
    return;
  }

  // We don't have the password and we don't use KDE Wallet - show the pass dialog!
  showPassDialog();
}

void KryptDevice::slotKWalletReady ( bool isReady )
{
  KWallet::Wallet *w = 0;

  // We don't use KDE Wallet... strange!

  if ( !_kryptApp->useKWallet() ) return;

  if ( isReady )
  {
    w = _kryptApp->getKWallet();

    if ( !w || !w->isOpen() || !w->hasFolder ( KRYPT_KWALLET_FOLDER ) )
    {
      isReady = false;
    }
  }

  if ( isReady && _saveToKWallet )
  {
    // The wallet is ready, and we want to update password information

    if ( w->setFolder ( KRYPT_KWALLET_FOLDER ) )
    {

      // We managed to open the folder

      if ( _storePass )
      {
        // We want to store the password - save the password, but only
        // if it exists (is longer than 0)

        if ( _password.length() > 0 )
        {
          // Returns 0 on success!
          if ( !w->writePassword ( _udi, _password ) )
          {
            _saveToKWallet = false;
          }
        }
      }
      else
      {
        // We don't want to store the password - remove it if it exists in the wallet!

        if ( w->hasEntry ( _udi ) )
        {
          if ( !w->removeEntry ( _udi ) )
          {
            // We have managed to update the password information
            // so there is no need to do that again
            _saveToKWallet = false;
          }
        }
        else
        {
          // The password didn't exist, so we don't have to remove it again.
          _saveToKWallet = false;
        }
      }
    }
  }

  // The Wallet is ready, and we want to store the password - so it's possible that it's already there!
  // Lets read the password. Might be needed later. But only if we don't have the password read already.
  if ( isReady && _storePass && _password.length() < 1 )
  {
    if ( w->setFolder ( KRYPT_KWALLET_FOLDER ) )
    {
      if ( w->readPassword ( _udi, _password ) != 0 )
      {
        _password = "";
      }
    }
  }

  // We are waiting to decrypt the device
  if ( _waitingToDecrypt )
  {
    _waitingToDecrypt = false;

    if ( _password.length() > 0 )
    {
      // We now have the password - lets try to decrypt the device with the
      // new password

      slotPassDecrypt();
      return;
    }
    else
    {
      // We still don't have the password. This is not good.
      // We want to display password pop-up, but only if the user wants to see it!
      if ( showPopup() ) showPassDialog();

      return;
    }
  }
}

void KryptDevice::checkNewDevice()
{
  // New, encrypted, device detected

  // We want to ignore it - do nothing!
  if ( isIgnored() ) return;

  // AutoDecryption is active - try to decrypt it
  if ( autoDecrypt() )
  {
    // This is not manual decryption
    _manualDecrypt = false;

    // We have the password (no matter from where)

    if ( _password.length() > 0 )
    {
      // Just try to decrypt it
      slotPassDecrypt();
      return;
    }

    // We don't have the password, but we want to store it (so it's possible
    // that it's stored), and we want to use KDE Wallet
    if ( _kryptApp->useKWallet() && _storePass )
    {
      // Mark that we are waiting for the password to decrypt this device
      _waitingToDecrypt = true;

      // And check the wallet!
      checkKWallet();
      return;
    }
  }

  // We don't want auto decryption at this point.

  // And we don't want any pop-ups - just exit.
  if ( !showPopup() ) return;

  // Otherwise, show password dialog!
  showPassDialog();
}

void KryptDevice::showPassDialog()
{
  // Destroy existing pass dialog (if any)
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

  // Decryption from this point will be manual, since we displayed the pass dialog!
  _manualDecrypt = true;

  _passDialog = new KryptDialog ( this );

  connect ( _passDialog, SIGNAL ( signalClosed() ),
            this, SLOT ( slotClosedPassDialog() ) );

  _passDialog->show();
}

QString KryptDevice::getPassword()
{
  return _password;
}

void KryptDevice::setPassword ( const QString & pass )
{
  _password = pass;

  if ( _kryptApp->useKWallet() )
  {
    // We don't call checkKWallet here - KryptDevConf calls checkKWallet
    // by itself. We only want to mark here that there is something new to store
    _saveToKWallet = true;
  }
}
