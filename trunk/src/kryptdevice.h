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

#ifndef _KRYPT_DEVICE_H_
#define _KRYPT_DEVICE_H_

#include <qobject.h>
#include <qstring.h>

class HALBackend;

class KryptApp;

class KConfig;

class KryptDialog;

class QPixmap;

class KryptDevice : public QObject
{
  Q_OBJECT

public:
  KryptDevice ( KryptApp *kryptApp, const QString & udi );
  ~KryptDevice();

  int getID() const;
  const QString & getUDI() const;
  QString getDesc ( ) const;
  const QString & getType() const;
  const QString & getProduct() const;
  const QString & getVendor() const;
  const QString & getBlockDev() const;
  QPixmap getIcon () const;

  bool isDecrypted() const;
  bool isMounted() const;
  bool isIgnored() const;
  bool isPresent() const;

  bool showMount() const;
  bool showUMount() const;
  bool showEncrypt() const;
  bool showDecrypt() const;
  bool showOptions() const;

  bool autoEncrypt() const;
  bool autoDecrypt() const;
  bool showPopup() const;

  void passDialogCanceled();

public slots:
  void slotHALEvent ( int eventID, const QString& udi );
  void slotPassError ( const QString &udi, const QString &errorName, const QString &errorMsg );
  void slotLoadConfig();
  void slotSaveConfig();

  void slotClickMount();
  void slotClickUMount();
  void slotClickEncrypt();
  void slotClickDecrypt();
  void slotClickOptions();

  void slotPassDecrypt ( const QString &password );

private:
  enum OptionType { OptionOff, OptionOn, OptionDefault };

  static int _lastDevID;
  KryptApp *_kryptApp;
  QString _udi;
  char *_cUdi;
  int _devID;
  HALBackend *_halBackend;
  KConfig *_cfg;
  KryptDialog *_passDialog;

  QString _vendor;
  QString _product;
  QString _blockDev;
  QString _type;
  QString _mountPoint;

  bool _isPresent;
  bool _isDecrypted;
  bool _isMounted;
  bool _isIgnored;

  bool _globShowMount;
  bool _globShowUMount;
  bool _globShowEncrypt;
  bool _globShowDecrypt;
  bool _globShowOptions;
  bool _globAutoEncrypt;
  bool _globAutoDecrypt;
  bool _globShowPopup;

  OptionType _showMount;
  OptionType _showUMount;
  OptionType _showEncrypt;
  OptionType _showDecrypt;
  OptionType _showOptions;
  OptionType _autoEncrypt;
  OptionType _autoDecrypt;
  OptionType _showPopup;

  void updateDeviceInfo();
  OptionType loadOption ( const char *opt );
  void saveOption ( const char *opt, OptionType optVal );
  void popupPassDialog();
  void recreateCUdi();
  void checkNewDevice();
};

#endif // _KRYPT_DEVICE_H_
