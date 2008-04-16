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

#ifndef _KRYPT_APP_H_
#define _KRYPT_APP_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kconfig.h>
#include <kuniqueapplication.h>

class KryptSystemTray;

class HALBackend;

class KryptApp : public KUniqueApplication
{
		Q_OBJECT

	public:
		KryptApp();

	protected slots:
		void slotDevNew(const QString &udi);
		void slotDevMapped(const QString &udi);
		void slotDevUnmapped(const QString &udi);
		void slotDevMounted(const QString &udi);
		void slotDevUmounted(const QString &udi);
		void slotNewInfo(const QString &info);
		void slotPopPassDialog(const QString &udi);
		void slotError(const QString &udi, const QString &errorName, const QString &errorMsg);
		void slotLoadConfig();

	protected:
		KConfig _cfg;
		KryptSystemTray *_tray;
                HALBackend* _halBackend;
		bool _showPopUp;
		QStringList _devsKnown;
		QStringList _devsIgnored;

		QString getUdiDesc(const QString& udi);

		void checkKnown(const QString &udi);
};

#endif // _KRYPT_APP_H_
