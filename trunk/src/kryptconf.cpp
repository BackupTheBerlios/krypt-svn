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

#include "kryptdevitem.h"
#include "kryptglobal.h"
#include "kryptconf.h"
#include "kryptconf.moc"

KryptConf::KryptConf (KConfig *cfg) :
		KDialogBase( 0, "Conf", true, "Configure Krypt", Ok|Cancel, Ok, false )
		, _dlg(0), _cfg(cfg)
{
	_dlg = new ConfDialog ( this );

	connect ( this, SIGNAL ( cancelClicked() ),
		  this, SLOT ( slotCancel() ) );

	connect ( this, SIGNAL(okClicked()),
		  this, SLOT(slotOk()));

	setMainWidget ( _dlg );

	_cfg->setGroup(KRYPT_CONF_APP_GROUP);
	_dlg->cPopUp->setChecked(_cfg->readBoolEntry(KRYPT_CONF_APP_SHOW_POPUP, true));

	_cfg->setGroup(KRYPT_CONF_TRAY_GROUP);
	_dlg->cUmount->setChecked(_cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_UMOUNT, true));
	_dlg->cMount->setChecked(_cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_MOUNT, true));
	_dlg->cEncrypt->setChecked(_cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_ENCRYPT, true));
	_dlg->cDecrypt->setChecked(_cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_DECRYPT, true));
	_dlg->cAutoEncrypt->setChecked(_cfg->readBoolEntry(KRYPT_CONF_TRAY_AUTO_ENCRYPT, true));

	_cfg->setGroup(KRYPT_CONF_DEVICES_GROUP);

	QStringList known = _cfg->readListEntry(KRYPT_CONF_DEVICES_KNOWN);
	QStringList ignored = _cfg->readListEntry(KRYPT_CONF_DEVICES_IGNORED);

	_cfg->setGroup(KRYPT_CONF_DEVICE_DESC_GROUP);

	QListBox *list = _dlg->selIgnored->selectedListBox();

	for ( QStringList::Iterator it = ignored.begin(); it != ignored.end(); ++it )
	{
		known.remove(*it);
		new KryptDevItem(*it, list, _cfg->readEntry(*it, "unknown"));
	}

	list = _dlg->selIgnored->availableListBox();

	for ( QStringList::Iterator it = known.begin(); it != known.end(); ++it )
	{
		new KryptDevItem(*it, list, _cfg->readEntry(*it, "unknown"));
	}
}

KryptConf::~KryptConf()
{
	delete _dlg;
}

void KryptConf::slotOk()
{
	_cfg->setGroup(KRYPT_CONF_APP_GROUP);
	_cfg->writeEntry(KRYPT_CONF_APP_SHOW_POPUP, _dlg->cPopUp->isChecked());

	_cfg->setGroup(KRYPT_CONF_TRAY_GROUP);
	_cfg->writeEntry(KRYPT_CONF_TRAY_SHOW_UMOUNT, _dlg->cUmount->isChecked());
	_cfg->writeEntry(KRYPT_CONF_TRAY_SHOW_MOUNT, _dlg->cMount->isChecked());
	_cfg->writeEntry(KRYPT_CONF_TRAY_SHOW_ENCRYPT, _dlg->cEncrypt->isChecked());
	_cfg->writeEntry(KRYPT_CONF_TRAY_SHOW_DECRYPT, _dlg->cDecrypt->isChecked());
	_cfg->writeEntry(KRYPT_CONF_TRAY_AUTO_ENCRYPT, _dlg->cAutoEncrypt->isChecked());

	QStringList ignored;

	QListBox *list = _dlg->selIgnored->selectedListBox();
	uint count = list->count();

	for (uint i = 0; i < count; ++i)
	{
		QListBoxItem *item = list->item(i);

		if (item)
		{
			ignored.append(((KryptDevItem*) item)->udi);
		}
	}

	_cfg->setGroup(KRYPT_CONF_DEVICES_GROUP);
	_cfg->writeEntry(KRYPT_CONF_DEVICES_IGNORED, ignored);

	_cfg->sync();
	hide();

	emit sigConfChanged();
}

void KryptConf::slotCancel()
{
	hide();
}
