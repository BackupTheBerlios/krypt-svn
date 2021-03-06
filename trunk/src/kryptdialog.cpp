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

#include <kmessagebox.h>

#include <qcheckbox.h>

#include "kryptglobal.h"
#include "kryptdevice.h"
#include "kryptdialog.h"
#include "kryptdialog.moc"
#include "decryptdialog.h"

KryptDialog::KryptDialog ( KryptDevice *kryptDev ) :
    KDialogBase ( NULL, "Dialog", true, i18n ( "Decrypt Storage Device" ), ( Cancel | User1 ),
                  User1, false, KGuiItem ( i18n ( "Decrypt" ), "decrypted" ) ),
    _kryptDev ( kryptDev ), _dlg ( 0 )
{
  _dlg = new DecryptDialog ( this );

  _dlg->errorBox->hide();

  _dlg->descLabel->setText ( _dlg->descLabel->text().
                             arg ( _kryptDev->getVendor() ).
                             arg ( _kryptDev->getProduct() ).
                             arg ( _kryptDev->getBlockDev() ) );

  _dlg->descLabel->adjustSize();
  _dlg->adjustSize();

  _dlg->encryptedIcon->setPixmap ( _kryptDev->getIcon() );

  _dlg->cStorePass->setChecked ( _kryptDev->getStorePass() );

  _dlg->cAutoDecrypt->setChecked ( _kryptDev->shouldAutoDecrypt() );

  _dlg->passwordEdit->setText ( _kryptDev->getPassword() );

  if ( _dlg->passwordEdit->text().isEmpty() )
  {
    enableButton ( User1, false );
  }
  else
  {
    enableButton ( User1, true );
  }

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
  delete _dlg;
}

QString KryptDialog::getPassword()
{
  return _dlg->passwordEdit->text();
}

void KryptDialog::slotPassError ( const QString &errName, const QString &errMsg )
{
  QString error = QString::null;

  if ( errName == "org.freedesktop.Hal.Device.Volume.Crypto.SetupPasswordError" )
  {
    error = QString ( i18n ( "Wrong password!" ) );
  }
  else if ( errName == "org.freedesktop.Hal.Device.Volume.Crypto.CryptSetupMissing" )
  {
    error = QString ( i18n ( "Decryption failed! Application \"cryptsetup\" not found. "
                             "Is package with \"cryptsetup\" program installed?" ) );
  }
  else if ( errName == "org.freedesktop.Hal.Device.Volume.Crypto.SetupError" )
  {
    // TODO - maybe show the original error message here?
    error = QString ( i18n ( "%1 is already decrypted!" ).arg ( _kryptDev->getBlockDev() ) );
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
  _dlg->errorLabel->setText ( "" );

  if ( _dlg->passwordEdit->text().isEmpty() ) return;

  if ( !_kryptDev->usesKWallet() && _dlg->cStorePass->isChecked() && !_kryptDev->getStorePass() )
  {
    // Show this only if KryptDevice didn't have password storage enabled before

    int ret = KMessageBox::messageBox ( this, KMessageBox::WarningContinueCancel,
                                        i18n ( "You have selected to store the password. "
                                               "However, use of KDE Wallet is disabled, so unencrypted password will be "
                                               "saved in configuration file. This is unsafe!\n"
                                               "You are strongly encouraged to enable KDE Wallet in Krypt's global configuration." ),
                                        QString::null, KStdGuiItem::cont() );

    if ( ret != KMessageBox::Continue ) return;
  }

  // We want to modify 'auto decrypt' property only if it was modified
  // Otherwise it would always be changed from the 'default' value to
  // either 'on' or 'off'
  if ( _kryptDev->shouldAutoDecrypt() != _dlg->cAutoDecrypt->isChecked() )
  {
    if ( _dlg->cAutoDecrypt->isChecked() )
    {
      _kryptDev->setOptAutoDecrypt ( KryptDevice::OptionOn );
    }
    else
    {
      _kryptDev->setOptAutoDecrypt ( KryptDevice::OptionOff );
    }
  }

  _kryptDev->setStorePass ( _dlg->cStorePass->isChecked() );

  _kryptDev->setPassword ( _dlg->passwordEdit->text() );

  _kryptDev->slotSaveConfig();

  _kryptDev->checkKWallet();

  _kryptDev->slotPassDecrypt ( );
}

void KryptDialog::slotCancel()
{
  emit signalClosed();
}
