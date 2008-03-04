/***************************************************************************
 *   Copyright (C) 2007, 2008 by Jakub Schmidtke                                 *
 *   sjakub@users.berlios.de                                                      *
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

#include <qtooltip.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
//#include <khelpmenu.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>

#include "kryptglobal.h"
#include "kryptconf.h"
#include "kryptsystray.h"
#include "kryptsystray.moc"

KryptSystemTray::KryptSystemTray(KConfig *cfg, QWidget* parent, const char *name)
	: KSystemTray(parent, name), _confDlg(0), _cfg(cfg), _nextID(1)
//	, _help(new KHelpMenu(this, KGlobal::instance()->aboutData(), false, actionCollection()))
{
	loadConfig();

	setPixmap(KSystemTray::loadIcon("krypt"));
	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(this, SIGNAL(quitSelected()), kapp, SLOT(quit()));
	QToolTip::add(this, i18n("Control LUKS encrypted volumes"));
}

void KryptSystemTray::mousePressEvent(QMouseEvent* e)
{
	// Popup the context menu with left-click
	if (e->button() == LeftButton) {
		contextMenuAboutToShow(contextMenu());
		contextMenu()->popup(e->globalPos());
		e->accept();
		return;
	}

	KSystemTray::mousePressEvent(e);
}

void KryptSystemTray::contextMenuAboutToShow(KPopupMenu* menu)
{
	int lastIndex = 0;

	menu->clear();

	QValueList<int>::Iterator it;

	if (_showUmount)
	{
		bool addedMenu = false;

		for ( it = _devsMounted.begin(); it != _devsMounted.end(); ++it )
		{
			if (!_devsIgnored.contains(*it))
			{
				if (!addedMenu)
				{
					addedMenu = true;

					menu->insertTitle(UserIcon("umount"), i18n("Umount Volume"));
				}

				lastIndex = menu->insertItem(_id2Desc[*it]);

				menu->setItemParameter(lastIndex, *it);
				menu->connectItem(lastIndex, this, SLOT(slotUmountClicked(int)));
			}
		}
	}

	if (_showMount)
	{
		bool addedMenu = false;

		for ( it = _devsUmounted.begin(); it != _devsUmounted.end(); ++it )
		{
			if (!_devsIgnored.contains(*it))
			{
				if (!addedMenu)
				{
					addedMenu = true;

					menu->insertTitle(UserIcon("mount"), i18n("Mount Volume"));
				}

				lastIndex = menu->insertItem(_id2Desc[*it]);

				menu->setItemParameter(lastIndex, *it);
				menu->connectItem(lastIndex, this, SLOT(slotMountClicked(int)));
			}
		}
	}

	if (_showEncrypt)
	{
		bool addedMenu = false;

		for ( it = _devsUmounted.begin(); it != _devsUmounted.end(); ++it )
		{
			if (!_devsIgnored.contains(*it))
			{
				if (!addedMenu)
				{
					addedMenu = true;

					menu->insertTitle(UserIcon("encrypt"), i18n("Encrypt Volume"));
				}

				lastIndex = menu->insertItem(_id2Desc[*it]);

				menu->setItemParameter(lastIndex, *it);
				menu->connectItem(lastIndex, this, SLOT(slotEncryptClicked(int)));
			}
		}

		// No addedMenu reset!

		for ( it = _devsMounted.begin(); it != _devsMounted.end(); ++it )
		{
			if (!_devsIgnored.contains(*it))
			{
				if (!addedMenu)
				{
					addedMenu = true;

					menu->insertTitle(UserIcon("encrypt"), i18n("Encrypt Volume"));
				}

				lastIndex = menu->insertItem(_id2Desc[*it]);

				menu->setItemParameter(lastIndex, *it);
				menu->connectItem(lastIndex, this, SLOT(slotEncryptClicked(int)));
			}
		}
	}

	if (_showDecrypt)
	{
		bool addedMenu = false;

		for ( it = _devsEncrypted.begin(); it != _devsEncrypted.end(); ++it )
		{
			if (!_devsIgnored.contains(*it))
			{
				if (!addedMenu)
				{
					addedMenu = true;

					menu->insertTitle(UserIcon("decrypt"), i18n("Decrypt Volume"));
				}

				lastIndex = menu->insertItem(_id2Desc[*it]);

				menu->setItemParameter(lastIndex, *it);
				menu->connectItem(lastIndex, this, SLOT(slotDecryptClicked(int)));
			}
		}
	}

	if (!lastIndex)
	{
		lastIndex = menu->insertItem(i18n("No LUKS volumes to display"));
		menu->setItemEnabled(lastIndex, false);
	}

	menu->insertSeparator();

	KAction *actPrefs = new KAction( i18n( "Configure Krypt..." ),
					 SmallIconSet( "configure" ), KShortcut(), this, SLOT( slotPrefs() ),
							 actionCollection() );
	actPrefs->plug( menu );

// 	menu->insertItem(SmallIcon("help"),KStdGuiItem::help().text(), m_help->menu());

	KAction *quitAction = actionCollection()->action(KStdAction::name(KStdAction::Quit));
	quitAction->plug(menu);
}

void KryptSystemTray::slotConfigChanged()
{
	loadConfig();

	emit sigConfigChanged();

	if (_confDlg)
	{
		_confDlg->deleteLater();
		_confDlg = 0;
	}
}

void KryptSystemTray::loadConfig()
{
	_cfg->setGroup(KRYPT_CONF_TRAY_GROUP);

	_showUmount = _cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_UMOUNT, true);
	_showMount = _cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_MOUNT, true);
	_showEncrypt = _cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_ENCRYPT, true);
	_showDecrypt = _cfg->readBoolEntry(KRYPT_CONF_TRAY_SHOW_DECRYPT, true);
	_autoEncrypt = _cfg->readBoolEntry(KRYPT_CONF_TRAY_AUTO_ENCRYPT, true);

	_cfg->setGroup(KRYPT_CONF_DEVICES_GROUP);

	QStringList ignored = _cfg->readListEntry(KRYPT_CONF_DEVICES_IGNORED);

	_devsIgnored.clear();

	_cfg->setGroup(KRYPT_CONF_DEVICE_DESC_GROUP);

	for ( QStringList::Iterator it = ignored.begin(); it != ignored.end(); ++it )
	{
		QString udi = *it;
		int id = getID(udi, _cfg->readEntry(udi, "unknown"));

		_devsIgnored.append(id);
	}
}

void KryptSystemTray::slotDeviceIsEncrypted(const QString &udi, const QString &desc)
{
	int id = getID(udi, desc);
	removeID(id);
	_devsEncrypted.append(id);
}

void KryptSystemTray::slotDeviceIsMounted(const QString &udi, const QString &desc)
{
	int id = getID(udi, desc);
	removeID(id);
	_devsMounted.append(id);
}

void KryptSystemTray::slotDeviceIsUmounted(const QString &udi, const QString &desc)
{
	bool doEnc = false;

	int id = getID(udi, desc);

	if (_toEncrypt.contains(id) || (_devsMounted.contains(id) && _autoEncrypt))
	{
		doEnc = true;
	}

	removeID(id);
	_devsUmounted.append(id);

	if (doEnc)
	{
		_toEncrypt.remove(id);
		emit sigEncryptDevice(udi);
	}
}

void KryptSystemTray::slotDeviceRemoved(const QString &udi)
{
	if (_udi2ID.contains(udi))
	{
		removeID(_udi2ID[udi]);
	}
}

void KryptSystemTray::slotUmountClicked(int id)
{
	if (_id2Udi.contains(id)) emit sigUmountDevice(_id2Udi[id]);
}

void KryptSystemTray::slotMountClicked(int id)
{
	if (_id2Udi.contains(id)) emit sigMountDevice(_id2Udi[id]);
}

void KryptSystemTray::slotDecryptClicked(int id)
{
	if (_id2Udi.contains(id)) emit sigDecryptDevice(_id2Udi[id]);
}

void KryptSystemTray::slotEncryptClicked(int id)
{
	if (_id2Udi.contains(id))
	{
		if (_devsMounted.contains(id))
		{
			emit sigUmountDevice(_id2Udi[id]);
			_toEncrypt.remove(id);
			_toEncrypt.append(id);
		}
		else
		{
			emit sigEncryptDevice(_id2Udi[id]);
		}
	}
}

void KryptSystemTray::removeID(int id)
{
	_devsEncrypted.remove(id);
	_devsMounted.remove(id);
	_devsUmounted.remove(id);
}

int KryptSystemTray::getID(const QString &udi, const QString &desc)
{
	if (!_udi2ID.contains(udi))
	{
		int id = _nextID++;

		_udi2ID.insert(udi, id);
		_id2Udi.insert(id, udi);
		_id2Desc.insert(id, desc);
	}

	return _udi2ID[udi];
}

void KryptSystemTray::slotPrefs()
{
	if (_confDlg)
	{
		delete _confDlg;
		_confDlg = 0;
	}

	_confDlg = new KryptConf(_cfg);

	connect( _confDlg, SIGNAL(sigConfChanged()),
		 this, SLOT(slotConfigChanged()));

	_confDlg->show();
}
