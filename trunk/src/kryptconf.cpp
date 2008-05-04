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
#include "kryptapp.h"
#include "kryptconf.h"
#include "kryptconf.moc"

KryptConf::KryptConf ( KryptApp *kryptApp, QValueList<KryptDevice*> devices ) :
    KDialogBase ( 0, "Conf", true, "Configure Krypt", Ok | Cancel, Ok, false )
    , _kryptApp ( kryptApp ), _dlg ( 0 ), _cfg ( 0 ), _devices ( devices ), _isLeaving ( false )
{
  _cfg = _kryptApp->getConfig();
  _dlg = new ConfDialog ( this );

  connect ( this, SIGNAL ( cancelClicked() ),
            this, SLOT ( slotCancel() ) );

  connect ( this, SIGNAL ( okClicked() ),
            this, SLOT ( slotOk() ) );

  setMainWidget ( _dlg );

  _cfg->setGroup ( KRYPT_CONF_GLOBAL_GROUP );

  _dlg->cMount->setChecked ( _kryptApp->showMount() );
  _dlg->cUmount->setChecked ( _kryptApp->showUMount() );
  _dlg->cEncrypt->setChecked ( _kryptApp->showEncrypt() );
  _dlg->cDecrypt->setChecked ( _kryptApp->showDecrypt() );
  _dlg->cOptions->setChecked ( _kryptApp->showOptions() );

  _dlg->cPopUp->setChecked ( _kryptApp->showPopup() );

  _dlg->cAutoEncrypt->setChecked ( _kryptApp->autoEncrypt() );
  _dlg->cAutoDecrypt->setChecked ( _kryptApp->autoDecrypt() );

  _dlg->cNotifyAutoEncrypt->setChecked ( _kryptApp->notifyAutoEncrypt() );
  _dlg->cNotifyAutoDecrypt->setChecked ( _kryptApp->notifyAutoDecrypt() );
  _dlg->cNotifyManualEncrypt->setChecked ( _kryptApp->notifyManualEncrypt() );
  _dlg->cNotifyManualDecrypt->setChecked ( _kryptApp->notifyManualDecrypt() );

  if ( _kryptApp->groupByCategory() )
  {
    _dlg->rGroupActions->setChecked ( true );
  }
  else
  {
    _dlg->rGroupDevices->setChecked ( true );
  }

  if ( _kryptApp->flatMenu() )
  {
    _dlg->rFlat->setChecked ( true );
  }
  else
  {
    _dlg->rDropDown->setChecked ( true );
  }

  if ( _kryptApp->useKWallet() )
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
                                        i18n ( "Storing UNENCRYPTED passwords "
                                               "in configuration file is unsafe!\n"
                                               "You are strongly encouraged to use KDE Wallet "
                                               "for storing passwords.\n"
                                               "Also, if there are any volume passwords stored "
                                               "in KDE Wallet, they will not be transfered"
                                               "to the configuration file - you will have to "
                                               "enter them again.\n" ),
                                        QString::null, KStdGuiItem::cont() );

    if ( ret != KMessageBox::Continue ) return;
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
  _cfg->writeEntry ( KRYPT_CONF_NOTIFY_MANUAL_ENCRYPT, _dlg->cNotifyManualEncrypt->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_NOTIFY_MANUAL_DECRYPT, _dlg->cNotifyManualDecrypt->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_SHOW_POPUP, _dlg->cPopUp->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_GROUP_BY_CAT, _dlg->rGroupActions->isChecked() );
  _cfg->writeEntry ( KRYPT_CONF_FLAT_MENU, _dlg->rFlat->isChecked() );

  _cfg->writeEntry ( KRYPT_CONF_USE_KWALLET, _dlg->rStoreKWallet->isChecked() );

  // So we don't update item list in slotRegenerateDeviceList
  _isLeaving = true;

  // Lets make devices set their new ignore status (no matter if it changed or not)
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

  _cfg->sync();

  // Everything should update the configuration
  emit signalConfigChanged();

  emit signalClosed();
}

void KryptConf::slotCancel()
{
  hide();
  emit signalClosed();
}
