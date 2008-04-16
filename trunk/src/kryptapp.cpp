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

#include <kdebug.h>
#include <klocale.h>
#include <qmessagebox.h>

#include "kryptglobal.h"
#include "kryptapp.h"
#include "kryptapp.moc"

#include "halbackend.h"
#include "kryptsystray.h"
#include "kryptdialog.h"

KryptApp::KryptApp(): _cfg("kryptrc")
{
	_tray = new KryptSystemTray(&_cfg, 0L, "KryptSysTray");

	slotLoadConfig();

	_tray->show();

	_halBackend = new HALBackend();

	if (!_halBackend->isOK()) return;

	connect (_halBackend, SIGNAL(sigNewInfo(const QString&)),
		 this, SLOT(slotNewInfo(const QString&)));

	connect (_halBackend, SIGNAL(sigDevNew(const QString&)),
		 this, SLOT(slotDevNew(const QString&)));

	connect (_halBackend, SIGNAL(sigDevMapped(const QString&)),
		 this, SLOT(slotDevMapped(const QString&)));

	connect (_halBackend, SIGNAL(sigDevUnmapped(const QString&)),
		 this, SLOT(slotDevUnmapped(const QString&)));

	connect (_halBackend, SIGNAL(sigDevMounted(const QString&)),
		 this, SLOT(slotDevMounted(const QString&)));

	connect (_halBackend, SIGNAL(sigDevUmounted(const QString&)),
		 this, SLOT(slotDevUmounted(const QString&)));

	connect (_halBackend, SIGNAL(sigError(const QString&, const QString&, const QString&)),
		 this, SLOT(slotError(const QString&, const QString&, const QString&)));

	connect (_halBackend, SIGNAL(sigDevRemoved(const QString&)),
		 _tray, SLOT(slotDeviceRemoved(const QString&)));

	connect (_tray, SIGNAL(sigUmountDevice(const QString&)),
		 _halBackend, SLOT(slotUmountDevice(const QString&)));

	connect (_tray, SIGNAL(sigMountDevice(const QString&)),
		 _halBackend, SLOT(slotMountDevice(const QString&)));

	connect (_tray, SIGNAL(sigEncryptDevice(const QString&)),
		 _halBackend, SLOT(slotRemoveDevice(const QString&)));

	connect (_tray, SIGNAL(sigDecryptDevice(const QString&)),
		 this, SLOT(slotPopPassDialog(const QString&)));

	connect (_tray, SIGNAL(sigConfigChanged()),
		 this, SLOT(slotLoadConfig()));

	_halBackend->initScan();
}

QString KryptApp::getUdiDesc(const QString& udi)
{
	QString vendor;
	QString product;
	QString blockDev;
	QString type;
	QString mountPoint;

	if (!_halBackend->getDeviceInfo(udi, vendor, product, blockDev, type, mountPoint)) return QString();

	return QString("%1 %2 (%3)").arg(vendor).arg(product).arg(blockDev);
}

void KryptApp::slotDevNew(const QString& udi)
{
	checkKnown(udi);

	_tray->slotDeviceIsEncrypted(udi, getUdiDesc(udi));

	if (_showPopUp && !_devsIgnored.contains(udi))
	{
		slotPopPassDialog(udi);
	}
}

void KryptApp::slotDevMapped(const QString &udi)
{
	checkKnown(udi);

	_tray->slotDeviceIsUmounted(udi, getUdiDesc(udi));
}

void KryptApp::slotDevUnmapped(const QString &udi)
{
	checkKnown(udi);

	_tray->slotDeviceIsEncrypted(udi, getUdiDesc(udi));
}

void KryptApp::slotDevMounted(const QString &udi)
{
	checkKnown(udi);

	_tray->slotDeviceIsMounted(udi, getUdiDesc(udi));
}

void KryptApp::slotDevUmounted(const QString &udi)
{
	checkKnown(udi);

	_tray->slotDeviceIsUmounted(udi, getUdiDesc(udi));
}

void KryptApp::slotNewInfo(const QString &info)
{
	//kdDebug() << info << endl;
}

void KryptApp::slotError(const QString &udi, const QString &errorName, const QString &errorMsg)
{
	// If we use KMessageBox it is modal dialog - we don't want it,
	// as it breaks dbus/hal communication...

	QMessageBox* mb = new QMessageBox( "Krypt: HAL Error", QString("%1").arg(errorMsg),
		QMessageBox::Warning,
		QMessageBox::Ok | QMessageBox::Default | QMessageBox::Escape,
		0,0, 0, 0, false);

	mb->show();
}

void KryptApp::slotPopPassDialog(const QString &udi)
{
	QString vendor;
	QString product;
	QString blockDev;
	QString type;
	QString mountPoint;

	if (!_halBackend->getDeviceInfo(udi, vendor, product, blockDev, type, mountPoint)) return;

	KryptDialog* dialog = new KryptDialog ( udi, vendor, product, blockDev, type );

	connect ( dialog, SIGNAL ( sigPassword ( char *, const char * ) ),
		  _halBackend, SLOT ( slotSendPassword ( char *, const char * ) ) );

	connect ( _halBackend, SIGNAL ( sigPassError ( const QString&, const QString&, const QString& ) ),
		  dialog, SLOT ( slotPassError ( const QString&, const QString&, const QString& ) ) );

	connect ( _halBackend, SIGNAL ( sigDevRemoved ( const QString& ) ),
		  dialog, SLOT ( slotDevRemoved ( const QString& ) ) );

	connect ( _halBackend, SIGNAL ( sigDevMapped ( const QString& ) ),
		  dialog, SLOT ( slotDevMapped ( const QString& ) ) );

	dialog->show();
}

void KryptApp::slotLoadConfig()
{
	_cfg.setGroup(KRYPT_CONF_APP_GROUP);
	_showPopUp = _cfg.readBoolEntry(KRYPT_CONF_APP_SHOW_POPUP, true);

	_cfg.setGroup(KRYPT_CONF_DEVICES_GROUP);
	_devsKnown = _cfg.readListEntry(KRYPT_CONF_DEVICES_KNOWN);
	_devsIgnored = _cfg.readListEntry(KRYPT_CONF_DEVICES_IGNORED);
}

void KryptApp::checkKnown(const QString &udi)
{
	if (!_devsKnown.contains(udi))
	{
		_devsKnown.append(udi);

		_cfg.setGroup(KRYPT_CONF_DEVICES_GROUP);
		_cfg.writeEntry(KRYPT_CONF_DEVICES_KNOWN, _devsKnown);

		_cfg.setGroup(KRYPT_CONF_DEVICE_DESC_GROUP);
		_cfg.writeEntry(udi, getUdiDesc(udi));
	}
}
