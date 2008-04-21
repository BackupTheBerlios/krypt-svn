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

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <kactionselector.h>
#include <klocale.h>

#include "kryptdevice.h"
#include "kryptglobal.h"
#include "kryptdevconf.h"
#include "kryptdevconf.moc"

KryptDevConf::KryptDevConf ( KryptDevice *kDev ) :
    KDialogBase ( 0, "DevConf", true, "Device Options", Default | Ok | Cancel, Ok, false ),
    _kryptDev ( kDev ), _dlg ( 0 )
{
  _dlg = new DevConfDialog ( this );

  connect ( this, SIGNAL ( cancelClicked() ),
            this, SLOT ( slotCancel() ) );

  connect ( this, SIGNAL ( okClicked() ),
            this, SLOT ( slotOk() ) );

  connect ( this , SIGNAL ( defaultClicked() ),
            this, SLOT ( slotDefault() ) );

  setMainWidget ( _dlg );

  setButtonGroup ( _kryptDev->getOptShowMount(), _dlg->rMountShow, _dlg->rMountHide, _dlg->rMountDefault );
  setButtonGroup ( _kryptDev->getOptShowUMount(), _dlg->rUmountShow, _dlg->rUmountHide, _dlg->rUmountDefault );
  setButtonGroup ( _kryptDev->getOptShowDecrypt(), _dlg->rDecryptShow, _dlg->rDecryptHide, _dlg->rDecryptDefault );
  setButtonGroup ( _kryptDev->getOptShowEncrypt(), _dlg->rEncryptShow, _dlg->rEncryptHide, _dlg->rEncryptDefault );
  setButtonGroup ( _kryptDev->getOptShowOptions(), _dlg->rOptionsShow, _dlg->rOptionsHide, _dlg->rOptionsDefault );

  setButtonGroup ( _kryptDev->getOptShowPopup(), _dlg->rShowPopupYes, _dlg->rShowPopupNo, _dlg->rShowPopupDefault );
  setButtonGroup ( _kryptDev->getOptAutoDecrypt(), _dlg->rAutoDecryptYes, _dlg->rAutoDecryptNo, _dlg->rAutoDecryptDefault );
  setButtonGroup ( _kryptDev->getOptAutoEncrypt(), _dlg->rAutoEncryptYes, _dlg->rAutoEncryptNo, _dlg->rAutoEncryptDefault );

  _dlg->lDevIcon->setPixmap ( _kryptDev->getIcon() );
  _dlg->lName->setText ( _kryptDev->getName() );
  _dlg->lDevice->setText ( _kryptDev->getBlockDev() );

  if ( _kryptDev->isPresent() )
  {
    _dlg->lPresent->setText ( i18n ( "Yes" ) );
  }
  else
  {
    _dlg->lPresent->setText ( i18n ( "No" ) );
  }

  _dlg->cIgnore->setChecked ( _kryptDev->isIgnored() );
}

KryptDevConf::~KryptDevConf()
{
  delete _dlg;
}

void KryptDevConf::slotOk()
{
  _kryptDev->setIgnored ( _dlg->cIgnore->isChecked() );

  _kryptDev->setOptShowMount ( getGroupVal ( _dlg->rMountShow, _dlg->rMountHide, _dlg->rMountDefault ) );
  _kryptDev->setOptShowUMount ( getGroupVal ( _dlg->rUmountShow, _dlg->rUmountHide, _dlg->rUmountDefault ) );
  _kryptDev->setOptShowDecrypt ( getGroupVal ( _dlg->rDecryptShow, _dlg->rDecryptHide, _dlg->rDecryptDefault ) );
  _kryptDev->setOptShowEncrypt ( getGroupVal ( _dlg->rEncryptShow, _dlg->rEncryptHide, _dlg->rEncryptDefault ) );
  _kryptDev->setOptShowOptions ( getGroupVal ( _dlg->rOptionsShow, _dlg->rOptionsHide, _dlg->rOptionsDefault ) );
  _kryptDev->setOptShowPopup ( getGroupVal ( _dlg->rShowPopupYes, _dlg->rShowPopupNo, _dlg->rShowPopupDefault ) );
  _kryptDev->setOptAutoEncrypt ( getGroupVal ( _dlg->rAutoEncryptYes, _dlg->rAutoEncryptNo, _dlg->rAutoEncryptDefault ) );
  _kryptDev->setOptAutoDecrypt ( getGroupVal ( _dlg->rAutoDecryptYes, _dlg->rAutoDecryptNo, _dlg->rAutoDecryptDefault ) );

  hide();

  emit signalConfigChanged();
}

void KryptDevConf::slotCancel()
{
  hide();
  emit signalClosed();
}

void KryptDevConf::slotDefault()
{
  _dlg->cIgnore->setChecked ( false );

  _dlg->rMountDefault->setChecked ( true );
  _dlg->rUmountDefault->setChecked ( true );
  _dlg->rDecryptDefault->setChecked ( true );
  _dlg->rEncryptDefault->setChecked ( true );
  _dlg->rOptionsDefault->setChecked ( true );

  _dlg->rShowPopupDefault->setChecked ( true );
  _dlg->rAutoDecryptDefault->setChecked ( true );
  _dlg->rAutoEncryptDefault->setChecked ( true );
}

KryptDevice::OptionType KryptDevConf::getGroupVal ( QRadioButton *on, QRadioButton *off, QRadioButton * ) const
{
  if ( on->isChecked() ) return KryptDevice::OptionOn;

  if ( off->isChecked() ) return KryptDevice::OptionOff;

  return KryptDevice::OptionDefault;
}

void KryptDevConf::setButtonGroup ( KryptDevice::OptionType oVal, QRadioButton *on, QRadioButton *off, QRadioButton *def )
{
  if ( oVal == KryptDevice::OptionOn )
  {
    on->setChecked ( true );
  }
  else if ( oVal == KryptDevice::OptionOff )
  {
    off->setChecked ( true );
  }
  else
  {
    def->setChecked ( true );
  }
}
