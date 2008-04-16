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

#ifndef _KRYPT_SYS_TRAY_H_
#define _KRYPT_SYS_TRAY_H_

#include <qstringlist.h>
#include <ksystemtray.h>
#include <kconfig.h>

class KryptConf;

class KryptSystemTray :  public KSystemTray
{
	Q_OBJECT

	public:
		KryptSystemTray(KConfig *cfg, QWidget *parent = 0, const char *name = 0);

		virtual void contextMenuAboutToShow(KPopupMenu *menu);

	signals:
		void sigUmountDevice(const QString &udi);
		void sigMountDevice(const QString &udi);
		void sigDecryptDevice(const QString &udi);
		void sigEncryptDevice(const QString &udi);
		void sigConfigChanged();

	public slots:
		void slotDeviceIsEncrypted(const QString &udi, const QString &desc);
		void slotDeviceIsMounted(const QString &udi, const QString &desc);
		void slotDeviceIsUmounted(const QString &udi, const QString &desc);
		void slotDeviceRemoved(const QString &udi);
 		void slotConfigChanged();

	protected slots:
		void slotUmountClicked(int id);
		void slotMountClicked(int id);
		void slotDecryptClicked(int id);
		void slotEncryptClicked(int id);
		void slotPrefs();

	protected:
		void mousePressEvent( QMouseEvent *e );

	private:
		QMap<QString, int> _udi2ID;
		QMap<int, QString> _id2Udi;
		QMap<int, QString> _id2Desc;

		QValueList<int> _devsIgnored;

		QValueList<int> _devsEncrypted;
		QValueList<int> _devsMounted;
		QValueList<int> _devsUmounted;
		QValueList<int> _toEncrypt;

		void removeID(int id);
		int getID(const QString &udi, const QString &desc);
		void loadConfig();

		KryptConf* _confDlg;
		KConfig* _cfg;
		int _nextID;

		bool _showUmount;
		bool _showMount;
		bool _showEncrypt;
		bool _showDecrypt;
		bool _autoEncrypt;
};

#endif // _KRYPT_SYS_TRAY_H_
