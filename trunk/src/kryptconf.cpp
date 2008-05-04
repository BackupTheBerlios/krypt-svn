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
#include <qcheckbox.h>
#include <kactionselector.h>
#include <qradiobutton.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kryptdevitem.h"
#include "kryptglobal.h"
#include "kryptconf.h"
#include "kryptconf.moc"

KryptConf::KryptConf ( KConfig *cfg, QValueList<KryptDevice*> devices ) :
    KDialogBase ( 0, "Conf", true, "Configure Krypt", Ok | Cancel, Ok, false )
    , _dlg ( 0 ), _cfg ( cfg ), _devices ( devices ), _isLeaving ( false )
{
  _dlg = new ConfDialog ( this );

  connect ( this, SIGNAL ( cancelClicked() ),
            this, SLOT ( slotCancel() ) );

  connect ( this, SIGNAL ( okClicked() ),
            this, SLOT ( slotOk() ) );

  setMainWidget ( _dlg );

  _cfg->setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  _dlg->cMount->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_SHOW_MOUNT, true ) );
  _dlg->cUmount->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_SHOW_UMOUNT, true ) );
  _dlg->cEncrypt->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_SHOW_ENCRYPT, true ) );
  _dlg->cDecrypt->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_SHOW_DECRYPT, true ) );
  _dlg->cOptions->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_SHOW_OPTIONS, true ) );

  _dlg->cAutoEncrypt->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_AUTO_ENCRYPT, true ) );
  _dlg->cAutoDecrypt->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_AUTO_DECRYPT, false ) );

  _dlg->cNotifyAutoEncrypt->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_NOTIFY_AUTO_ENCRYPT, true ) );
  _dlg->cNotifyAutoDecrypt->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_NOTIFY_AUTO_DECRYPT, true ) );

  _dlg->cPopUp->setChecked ( _cfg->readBoolEntry ( KRYPT_CONF_SHOW_POPUP, true ) );

  if ( _cfg->readBoolEntry ( KRYPT_CONF_GROUP_BY_CAT, false ) )
  {
    _dlg->rGroupActions->setChecked ( true );
  }
  else
  {
    _dlg->rGroupDevices->setChecked ( true );
  }

  if ( _cfg->readBoolEntry ( KRYPT_CONF_FLAT_MENU, false ) )
  {
    _dlg->rFlat->setChecked ( true );
  }
  else
  {
    _dlg->rDropDown->setChecked ( true );
  }

  if ( _cfg->readBoolEntry ( KRYPT_CONF_PASS_IN_WALLET, true ) )
  {
    _dlg->rStoreKWallet->setChecked ( true );
  }
  else
  {
    _dlg->rStoreConfig->setChecked ( true );
  }

  slotRegenerateDeviceList();

  QValueList<KryptDevice*>::const_iterator it = _devices.begin();
  QValueList<KryptDevice*>::const_iterator itEnd = _devices.end();

  for ( ; it != itEnd; ++it )
  {
    connect ( ( *it ), SIGNAL ( signalConfigChanged() ),
              this, SLOT ( slotRegenerateDeviceList() ) );
  }

  connect ( _dlg->listKnown, SIGNAL ( doubleClicked ( QListViewItem *, const QPoint &, int ) ),

            this, SLOT ( slotDoubleClicked ( QListViewItem *, const QPoint &, int ) ) );
}

KryptConf::~KryptConf()
{
  delete _dlg;
}

void KryptConf::slotRegenerateDeviceList()
{
  if ( _isLeaving ) return;

  _dlg->listKnown->clear();

  QValueList<KryptDevice*>::const_iterator it = _devices.begin();

  QValueList<KryptDevice*>::const_iterator itEnd = _devices.end();

  for ( ; it != itEnd; ++it )
  {
    new KryptDevItem ( _dlg->listKnown, ( *it ) );
  }
}

void KryptConf::slotDoubleClicked ( QListViewItem *item, const QPoint &, int col )
{
  if ( col < 0 ) return;

  KryptDevItem *kItem = ( KryptDevItem* ) item;

  if ( col == KRYPT_DEV_ITEM_COL_IGNORED )
  {
    kItem->toggleIgnored();
  }
  else if ( col == KRYPT_DEV_ITEM_COL_NAME || col == KRYPT_DEV_ITEM_COL_BLOCK_DEV )
  {
    kItem->getKryptDevice()->slotClickOptions();
  }
}

void KryptConf::slotOk()
{
  if ( !_dlg->rStoreKWallet->isChecked() )
  {
    // Warning. We want to transfer all plaintext passwords from configuration file
    // to KDE Wallet (and remove them from configuration file), but not the other
    // way. So it is impossible to dump passwords stored in the wallet into the configuration
    // file.
    int ret = KMessageBox::messageBox ( this, KMessageBox::WarningContinueCancel,
                                        i18n ( "You have enabled storing unencrypted passwords in configuration file, which is unsafe!\n"
                                               "You are strongly encouraged to use KDE Wallet for storing passwords.\n"
                                               "Also, if there are any volume passwords stored in KDE Wallet, they will not be transfered"
                                               "to the configuration file - you will have to enter them again.\n" ),
                                        QString::null, KStdGuiItem::cont() );

    if ( ret != KMessageBox::Continue ) return;
  }
  else
  {
    // Transfer password for each known device from the config file to the wallet
    // TODO
    // Try to open the wallet once, here!
  }

  _cfg->setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  _cfg->writeEntry ( KRYPT_CONF_SHOW_MOUNT, _dlg->cMount->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_SHOW_UMOUNT, _dlg->cUmount->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_SHOW_ENCRYPT, _dlg->cEncrypt->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_SHOW_DECRYPT, _dlg->cDecrypt->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_SHOW_OPTIONS, _dlg->cOptions->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_AUTO_ENCRYPT, _dlg->cAutoEncrypt->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_AUTO_DECRYPT, _dlg->cAutoDecrypt->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_NOTIFY_AUTO_ENCRYPT, _dlg->cNotifyAutoEncrypt->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_NOTIFY_AUTO_DECRYPT, _dlg->cNotifyAutoDecrypt->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_SHOW_POPUP, _dlg->cPopUp->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_GROUP_BY_CAT, _dlg->rGroupActions->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_FLAT_MENU, _dlg->rFlat->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_PASS_IN_WALLET, _dlg->rStoreKWallet->isChecked() );

  // So we don't update item list in slotRegenerateDeviceList
  _isLeaving = true;

  QListViewItem * item = _dlg->listKnown->firstChild();

  while ( item != 0 )
  {
    KryptDevItem *kItem = ( KryptDevItem* ) item;
    KryptDevice *kDev = kItem->getKryptDevice();

    kDev->setIgnored ( kItem->isIgnored() );
    kDev->slotSaveConfig();

    item = item->nextSibling();
  }

  hide();

  emit signalConfigChanged();

  _cfg->sync();

  emit signalClosed();
}

void KryptConf::slotCancel()
{
  hide();
  emit signalClosed();
}
