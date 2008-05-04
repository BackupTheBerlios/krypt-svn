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

#include <qmap.h>
#include <kconfig.h>
#include <kuniqueapplication.h>
#include <kwallet.h>

#include "kryptdevice.h"

class KryptSystemTray;

class HALBackend;

class KryptConf;

class KryptApp : public KUniqueApplication
{
  Q_OBJECT

public:
  KryptApp();
  ~KryptApp();

  QValueList<KryptDevice*> getDevices() const;
  KryptDevice * getDevice ( const QString &udi, bool create );
  KryptDevice * getDevice ( int id );
  KConfig * getConfig();
  QString getHalDevEventDesc ( int eventID ) const;
  KryptSystemTray *getKryptTray();
  void checkKWallet();
  KWallet::Wallet *getKWallet();

  bool showMount() const;
  bool showUMount() const;
  bool showEncrypt() const;
  bool showDecrypt() const;
  bool showOptions() const;
  bool showPopup() const;
  bool autoEncrypt() const;
  bool autoDecrypt() const;
  bool notifyAutoEncrypt() const;
  bool notifyAutoDecrypt() const;
  bool notifyManualEncrypt() const;
  bool notifyManualDecrypt() const;
  bool useKWallet() const;
  bool groupByCategory() const;
  bool flatMenu() const;

signals:
  void signalConfigChanged();
  void signalKWalletReady ( bool isReady );

protected slots:
  void slotHALEvent ( int eventID, const QString &udi );
  void slotError ( const QString &udi, const QString &errorName, const QString &errorMsg );
  void slotPassError ( const QString &udi, const QString &errorName, const QString &errorMsg );
  void slotNewInfo ( const QString &info );
  void slotShowConfig();
  void slotConfigChanged();
  void slotConfigClosed();
  void slotWalletOpened ( bool success );

protected:
  KConfig _cfg;
  KWallet::Wallet *_kwallet;
  KryptConf *_confDlg;
  KryptSystemTray *_tray;
  QMap<QString, KryptDevice*> _udi2Dev;
  QMap<int, KryptDevice*> _id2Dev;

  bool _showMount;
  bool _showUMount;
  bool _showEncrypt;
  bool _showDecrypt;
  bool _showOptions;
  bool _showPopup;
  bool _autoEncrypt;
  bool _autoDecrypt;
  bool _notifyAutoEncrypt;
  bool _notifyAutoDecrypt;
  bool _notifyManualEncrypt;
  bool _notifyManualDecrypt;
  bool _useKWallet;
  bool _groupByCategory;
  bool _flatMenu;

  void createAllKnownDevices();
  void checkConfig();
  void loadConfig();
};

#endif // _KRYPT_APP_H_
