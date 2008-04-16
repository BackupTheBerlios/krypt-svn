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

#include <string.h>

#include "kryptdialog.h"
#include "kryptdialog.moc"
#include "decryptdialog.h"

KryptDialog::KryptDialog ( const QString &udi, const QString &vendor,
                           const QString &product, const QString &dev, const QString &devType ) :
    KDialogBase ( NULL, "Dialog", true, "Decrypt Storage Device", ( Cancel | User1 ),
                  User1, false, KGuiItem ( i18n ( "Decrypt" ), "decrypted" ) )
    , _dlg ( 0 ), _udi ( udi ), _device ( dev ), _cUDI ( 0 )
{
  _cUDI = new char[_udi.length() + 1];
  memcpy ( _cUDI, _udi.ascii(), _udi.length() + 1 );

  _dlg = new DecryptDialog ( this );

  _dlg->errorBox->hide();
  _dlg->descLabel->setText ( _dlg->descLabel->text().arg ( vendor ).arg ( product ).arg ( _device ) );
  _dlg->descLabel->adjustSize();
  _dlg->adjustSize();

  setDeviceIcon ( devType );

  enableButton ( User1, false );

  connect ( _dlg->passwordEdit, SIGNAL ( textChanged ( const QString & ) ),
            this, SLOT ( slotPasswordChanged ( const QString & ) ) );

  connect ( this, SIGNAL ( cancelClicked() ),
            this, SLOT ( slotCancel() ) );

  connect ( this, SIGNAL ( user1Clicked() ),
            this, SLOT ( slotDecrypt() ) );

  setMainWidget ( _dlg );
}

KryptDialog::~KryptDialog()
{
  if ( _cUDI ) delete[] _cUDI;

  delete _dlg;
}

void KryptDialog::setDeviceIcon ( QString deviceType )
{
  QString deviceIcon;

  if ( deviceType == "memory_stick"
       || deviceType == "cdrom"
       || deviceType == "sd_mmc"
       || deviceType == "compact_flash"
       || deviceType == "smart_media"
       || deviceType == "zip" )
  {

    deviceIcon = QString ( "%1_unmount" ).arg ( deviceType );
  }
  else
  {
    deviceIcon = QString ( "hdd_unmount" );
  }

  QPixmap pixmap = KGlobal::iconLoader()->loadIcon ( deviceIcon, KIcon::NoGroup, KIcon::SizeLarge );

  _dlg->encryptedIcon->setPixmap ( pixmap );
}

QString KryptDialog::getPassword()
{
  return _dlg->passwordEdit->text();
}

void KryptDialog::slotDevRemoved ( const QString &udi )
{
  if ( udi == _udi )
  {
    this->deleteLater();
  }
}

void KryptDialog::slotDevMapped ( const QString &udi )
{
  // Same action - we close the dialog!
  slotDevRemoved ( udi );
}

void KryptDialog::slotPassError ( const QString& udi, const QString &errName, const QString &errMsg )
{
  if ( udi != _udi ) return;

  QString error = QString::null;

  if ( errName == "org.freedesktop.Hal.Device.Volume.Crypto.SetupPasswordError" )
  {
    error = QString ( i18n ( "Wrong password!" ) );
  }
  else if ( errName == "org.freedesktop.Hal.Device.Volume.Crypto.CryptSetupMissing" )
  {
    error = QString ( i18n ( "Decryption failed! Application \"cryptsetup\" not found. "
                             "Is package \"util-linux-crypto\" installed?" ) );
  }
  else if ( errName == "org.freedesktop.Hal.Device.Volume.Crypto.SetupError" )
  {
    error = QString ( i18n ( "%1 is already decrypted!" ).arg ( _device ) );
  }
  else
  {
    error = errMsg;
  }

  _dlg->errorLabel->setText ( QString ( "<b>%1</b>" ).arg ( error ) );

  _dlg->errorBox->show();
}

void KryptDialog::slotPasswordChanged ( const QString &text )
{
  enableButton ( User1, !text.isEmpty() );
}

void KryptDialog::slotDecrypt()
{
  if ( _dlg->passwordEdit->text().isEmpty() ) return;

  emit sigPassword ( _cUDI, _dlg->passwordEdit->text().ascii() );
}

void KryptDialog::slotCancel()
{
  this->deleteLater();
}
