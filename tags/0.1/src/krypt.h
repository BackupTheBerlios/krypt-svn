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

#ifndef _KRYPT_H_
#define _KRYPT_H_

#include <qobject.h>
#include <qmap.h>
#include <qstring.h>

/* DBus-Qt bindings */
#include <dbus/dbus.h>
#include <dbus/connection.h>
/* HAL libraries */
#include <libhal.h>

/**
 * @short Main Krypt Object
 * @author Jakub Schmidtke <sjakub@users.berlios.de>
 * @version 0.1
 */

class Krypt : public QObject
{
	Q_OBJECT

public:
	/**
	 * Default Constructor
	 */
	Krypt();

	/**
	 * Default Destructor
	 */
	virtual ~Krypt();

	bool isOK();
	void initScan();
	bool getDeviceInfo(const QString& udi,
					QString &vendor, QString &product,
					QString &blockDevice, QString &type,
					QString &mountPoint);

public slots:
	void slotSendPassword ( char* udi, const char *password );
	void slotUmountDevice( const QString& udi);
	void slotMountDevice( const QString& udi);
	void slotRemoveDevice(const QString& udi);

signals:
	void sigNewInfo(const QString& info);
	void sigPassError(const QString &udi, const QString &errorName, const QString &errorMsg);
	void sigError(const QString &udi, const QString &errorName, const QString &errorMsg);
	void sigDevNew(const QString& udi);
	void sigDevRemoved(const QString& udi);
	void sigDevMapped(const QString& udi);
	void sigDevUnmapped(const QString& udi);
	void sigDevMounted(const QString& udi);
	void sigDevUmounted(const QString& udi);

private:
	bool _halOK;
	bool _dbusOK;

	/**
	 * Maps mapping LUKS device UDI to Clear volume udi
	 * and the oposite way
	 */
	QMap<QString, QString> luksToClear;
	QMap<QString, QString> clearToLuks;

	static Krypt* s_Krypt;

	void sendInfo(const QString& info);

	bool initHAL();
	void closeHAL();
	bool initDBus();
	void closeDBus();

	enum VolMapStatus { VOL_UNKNOWN, VOL_NOT_MAPPED, VOL_MAPPED, VOL_MOUNTED };

	VolMapStatus volumeStatus ( const QString &udi );
	QString getHalPropertyString ( const QString &udi, const QString &prop );
	bool getHalPropertyBool ( const QString &udi, const QString &key );

	void addDevice ( const QString& udi );
	void removeDevice ( const QString& udi );
	void modifyDevice ( const QString& udi, const QString& key, bool isAdded );


	/* HAL and DBus structures */

	/**
	 * The HAL context connecting the whole application to the HAL
	 */
	LibHalContext*  _halContext;

	/**
	 * The DBus-Qt bindings connection for mainloop integration
	 */
	DBusQt::Connection* _dBusQtConnection;
	DBusConnection *_dbus_connection;

	/* Hal call-backs -- from gvm*/

	/** Invoked when a device is added to the Global Device List.
	 *
	 *  @param  ctx                 LibHal context
	 *  @param  udi                 Universal Device Id
	 */
	static void hal_device_added ( LibHalContext *ctx, const char *udi );

	/** Invoked when a device is removed from the Global Device List.
	  *
	  *  @param  ctx                 LibHal context
	  *  @param  udi                 Universal Device Id
	 */
	static void hal_device_removed ( LibHalContext *ctx, const char *udi );

	/** Invoked when a property of a device in the Global Device List is
	  *  changed, and we have we have subscribed to changes for that device.
	  *
	  *  @param  ctx                 LibHal context
	  *  @param  udi                 Univerisal Device Id
	  *  @param  key                 Key of property
	 */
	static void hal_device_property_modified ( LibHalContext *ctx, const char *udi, const char *key,
			dbus_bool_t is_removed, dbus_bool_t is_added );

	/** Type for callback when a non-continuos condition occurs on a device
	  *
	  *  @param  udi                 Univerisal Device Id
	  *  @param  condition_name      Name of the condition
	  *  @param  message             D-BUS message with variable parameters depending on condition
	 */
	static void hal_device_condition ( LibHalContext *ctx, const char *udi,
									   const char *condition_name,
									   const char* message
									 );

	static void passCallback ( DBusPendingCall* pcall, void* data );
	static void cmdCallback ( DBusPendingCall* pcall, void* data );
};

#endif // _KRYPT_H_
