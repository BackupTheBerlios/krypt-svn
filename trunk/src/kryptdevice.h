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

#include <kicontheme.h>
#include <qobject.h>
#include <qstring.h>

class HALBackend;

class KryptApp;

class KConfig;

class KryptDialog;

class KryptDevConf;

class QPixmap;

class KryptDevice : public QObject
{
  Q_OBJECT

public:
  enum OptionType { OptionOff, OptionOn, OptionDefault };

  KryptDevice ( KryptApp *kryptApp, const QString & udi );
  ~KryptDevice();

  int getID() const;
  const QString & getUDI() const;
  QString getDesc () const;
  QString getName () const;
  const QString & getType() const;
  const QString & getProduct() const;
  const QString & getVendor() const;
  const QString & getBlockDev() const;
  QPixmap getIcon ( KIcon::StdSizes size = KIcon::SizeLarge ) const;
  QString getConfigGroup() const;

  static QString getConfigGroup ( const QString & forUdi );

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

  OptionType getOptShowMount() const;
  OptionType getOptShowUMount() const;
  OptionType getOptShowDecrypt() const;
  OptionType getOptShowEncrypt() const;
  OptionType getOptShowOptions() const;
  OptionType getOptShowPopup() const;
  OptionType getOptAutoDecrypt() const;
  OptionType getOptAutoEncrypt() const;

  void setOptShowMount ( OptionType nOpt );
  void setOptShowUMount ( OptionType nOpt );
  void setOptShowDecrypt ( OptionType nOpt );
  void setOptShowEncrypt ( OptionType nOpt );
  void setOptShowOptions ( OptionType nOpt );
  void setOptShowPopup ( OptionType nOpt );
  void setOptAutoDecrypt ( OptionType nOpt );
  void setOptAutoEncrypt ( OptionType nOpt );

  void setIgnored ( bool newIgnored );

  bool getStorePass() const;
  void setStorePass ( bool nVal );

  bool shouldAutoDecrypt() const;
  bool usesKWallet() const;

  QString getPassword();
  void setPassword ( const QString &pass );

  void checkKWallet();

signals:
  void signalConfigChanged();

public slots:
  void slotHALEvent ( int eventID, const QString& udi );
  void slotPassError ( const QString &udi, const QString &errorName, const QString &errorMsg );
  void slotLoadConfig();
  void slotSaveConfig();
  void slotKWalletReady ( bool isReady );

  void slotClickMount();
  void slotClickUMount();
  void slotClickEncrypt();
  void slotClickDecrypt();
  void slotClickOptions();

  void slotPassDecrypt ( );

protected slots:
  void slotClosedPassDialog();
  void slotClosedConfDialog();

private:
  static int _lastDevID;
  KryptApp *_kryptApp;
  QString _udi;
  char *_cUdi;
  int _devID;
  HALBackend *_halBackend;
  KConfig *_cfg;
  KryptDialog *_passDialog;
  KryptDevConf *_confDialog;

  QString _vendor;
  QString _product;
  QString _blockDev;
  QString _type;
  QString _mountPoint;

  QString _password;

  bool _isPresent;
  bool _isDecrypted;
  bool _isMounted;
  bool _isIgnored;

  bool _isManualEncrypt;
  bool _isAutoEncrypt;
  bool _isManualEncryptOfMounted;
  bool _isManualDecrypt;
  bool _isAutoDecrypt;

  bool _saveToKWallet;
  bool _waitingToDecrypt;
  bool _waitingToShowPassDialog;

  bool _isInit;

  OptionType _showMount;
  OptionType _showUMount;
  OptionType _showEncrypt;
  OptionType _showDecrypt;
  OptionType _showOptions;
  OptionType _autoEncrypt;
  OptionType _autoDecrypt;
  OptionType _showPopup;

  bool _storePass;

  void updateDeviceInfo();
  OptionType loadOption ( const char *opt );
  void saveOption ( const char *opt, OptionType optVal );
  void recreateCUdi();
  void checkNewDevice();
  void showPassDialog();

  void doEncrypt();
  void doUMount();

  QStringList obfuscate ( const QString & str );
  QString deobfuscate ( const QStringList & list );

  void showTrayMessage ( const QString & msg, const QPixmap & pixmap = QPixmap() );
};

#endif // _KRYPT_DEVICE_H_
