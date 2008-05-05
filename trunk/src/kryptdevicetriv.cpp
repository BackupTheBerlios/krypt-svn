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
#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>

#include <kpassivepopup.h>

#include "halbackend.h"
#include "kryptapp.h"
#include "kryptsystray.h"
#include "kryptdialog.h"
#include "kryptglobal.h"
#include "kryptdevconf.h"
#include "kryptdevice.h"

// Lets choose 'exotic' base :)
#define OBFUSCATE_BASE 35

int KryptDevice::_lastDevID = 0;

/* The trivial parts of KryptDevice - so it's easier to read the main part */

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

void KryptDevice::updateDeviceInfo()
{
  if ( _isPresent ) return;

  if ( !_halBackend->isDevicePresent ( _udi ) ) return;

  if ( !_halBackend->getDeviceInfo ( _udi, _vendor, _product, _blockDev, _type, _mountPoint ) ) return;

  _isPresent = true;

  slotSaveConfig();
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

  return _kryptApp->showMount();
}

bool KryptDevice::showUMount() const
{
  if ( _showUMount == KryptDevice::OptionOn ) return true;

  if ( _showUMount == KryptDevice::OptionOff ) return false;

  return _kryptApp->showUMount();
}

bool KryptDevice::showEncrypt() const
{
  if ( _showEncrypt == KryptDevice::OptionOn ) return true;

  if ( _showEncrypt == KryptDevice::OptionOff ) return false;

  return _kryptApp->showEncrypt();
}

bool KryptDevice::showDecrypt() const
{
  if ( _showDecrypt == KryptDevice::OptionOn ) return true;

  if ( _showDecrypt == KryptDevice::OptionOff ) return false;

  return _kryptApp->showDecrypt();
}

bool KryptDevice::showOptions() const
{
  if ( _showOptions == KryptDevice::OptionOn ) return true;

  if ( _showOptions == KryptDevice::OptionOff ) return false;

  return _kryptApp->showOptions();
}

bool KryptDevice::showPopup() const
{
  if ( _showPopup == KryptDevice::OptionOn ) return true;

  if ( _showPopup == KryptDevice::OptionOff ) return false;

  return _kryptApp->showPopup();
}

bool KryptDevice::autoEncrypt() const
{
  if ( _autoEncrypt == KryptDevice::OptionOn ) return true;

  if ( _autoEncrypt == KryptDevice::OptionOff ) return false;

  return _kryptApp->autoEncrypt();
}

bool KryptDevice::autoDecrypt() const
{
  if ( _autoDecrypt == KryptDevice::OptionOn ) return true;

  if ( _autoDecrypt == KryptDevice::OptionOff ) return false;

  return _kryptApp->autoDecrypt();
}

void KryptDevice::showTrayMessage ( const QString & msg, const QPixmap & pixmap )
{
  if ( !_kryptApp ) return;

  if ( !_kryptApp->getKryptTray() ) return;

  KPassivePopup *pop = new KPassivePopup ( _kryptApp->getKryptTray() );

  pop->setAutoDelete ( true );

  QHBox *hb = new QHBox ( pop );

  hb->setSpacing ( KDialog::spacingHint() + 10 );

  QLabel * labIcon = new QLabel ( hb );

  labIcon->setPixmap ( getIcon() );

  QVBox *vb = new QVBox ( hb );

  QLabel *labName = new QLabel ( QString ( "<b>%1</b>" ).arg ( getName() ), vb );

  labName->setAlignment ( Qt::AlignHCenter );

  QLabel *labMsg = new QLabel ( msg, vb );

  labMsg->setAlignment ( AlignHCenter );

  if ( !pixmap.isNull() )
  {
    labIcon = new QLabel ( hb );

    labIcon->setPixmap ( pixmap );
  }

  pop->setView ( hb );

  pop->show();
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

bool KryptDevice::usesKWallet() const
{
  return _kryptApp->useKWallet();
}

bool KryptDevice::shouldAutoDecrypt() const
{
  if ( _autoDecrypt == OptionOn ) return true;

  if ( _autoDecrypt == OptionOff ) return false;

  // So it's default
  return _kryptApp->autoDecrypt();
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
