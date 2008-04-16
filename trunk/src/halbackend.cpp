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

#include "halbackend.h"
#include "halbackend.moc"

#include <qfile.h>
#include <kdebug.h>

/* Static instance of this class, for static HAL callbacks */
HALBackend* HALBackend::s_HALBackend = 0;

HALBackend::HALBackend()
		: QObject()
		, _halContext ( 0 )
		, _dBusQtConnection ( 0 )
		, _dbus_connection ( 0 )

{
	s_HALBackend = this;
	_dbusOK = false;
	_halOK = false;

	if (!initDBus()) return;

	_dbusOK = true;

	if (!initHAL()) return;

	_halOK = true;
}

HALBackend::~HALBackend()
{
	if (_halOK) closeHAL();
	if (_dbusOK) closeDBus();
}

bool HALBackend::isOK()
{
	return _dbusOK && _halOK;
}

void HALBackend::sendInfo(const QString& info)
{
	emit sigNewInfo(info);
}

void HALBackend::removeDevice ( const QString& udi )
{
	if (luksToClear.contains(udi))
	{
		sendInfo(QString("Device Removed: '%1'").arg(udi));
		emit sigDevRemoved ( udi );
	}
}

void HALBackend::modifyDevice ( const QString& udi, const QString& key, bool isAdded )
{
	if (luksToClear.contains(udi))
	{
		if (key == "info.callouts.remove")
		{
			if (volumeStatus(udi) == VOL_MAPPED)
			{
				// mapped
				sendInfo(QString("Device Mapped: '%1'").arg(udi));
				emit sigDevMapped(udi);
			}
			else
			{
				// unmapped
				sendInfo(QString("Device Unmapped: '%1'").arg(udi));
				emit sigDevUnmapped(udi);
			}
		}
	}
	else if (clearToLuks.contains(udi))
	{
		QString val = clearToLuks[udi];

		if (key == "volume.is_mounted")
		{
			if (getHalPropertyBool(udi, key))
			{
				// mounted
				sendInfo(QString("Device Mounted: '%1' as '%2'").arg(val).arg(udi));
				emit sigDevMounted(val);
			}
			else
			{
				// unmounted
				sendInfo(QString("Device Unmounted: '%1' from '%2'").arg(val).arg(udi));
				emit sigDevUmounted(val);
			}
		}
	}
	else
	{
		QString val = getHalPropertyString ( udi, "volume.crypto_luks.clear.backing_volume" );

		if (val.length() > 0)
		{
			luksToClear.insert(val, udi);
			clearToLuks.insert(udi, val);

			modifyDevice(udi, key, isAdded);
		}
	}
}

bool HALBackend::getDeviceInfo(const QString& udi, QString &vendor, QString &product, QString &blockDevice, QString &type, QString &mountPoint)
{
	if (!luksToClear.contains(udi)) return false;

	QString parentUDI = getHalPropertyString ( udi, "info.parent" );

	vendor = getHalPropertyString ( parentUDI, "info.vendor" );
	product = getHalPropertyString ( parentUDI, "info.product" );
	blockDevice = getHalPropertyString ( udi, "block.device" );
	type = getHalPropertyString ( parentUDI, "storage.drive_type" );

	QString clear = luksToClear[udi];

	if (clear.length() > 0)
	{
		mountPoint = getHalPropertyString(clear, "volume.mount_point");
	}

	return true;
}

void HALBackend::addDevice ( const QString& udi )
{
	/* We don't deal with devices that do not expose their capabilities.
	If we don't check this, we will get a lot of warning messages from libhal */

	if ( !libhal_device_property_exists ( _halContext, udi.ascii(), "info.capabilities", NULL ) )
		return;

	/* Add volume block devices */
	if ( !libhal_device_query_capability ( _halContext, udi.ascii(), "volume", NULL ) )
		return;

	if ( getHalPropertyString ( udi, "volume.fsusage" ) != "crypto" )
		return;

	if ( getHalPropertyString ( udi, "volume.fstype" ) != "crypto_LUKS" )
		return;

	luksToClear.insert(udi, "");

	VolMapStatus stat = volumeStatus ( udi );

	if ( stat == VOL_UNKNOWN ) return;

	if ( stat == VOL_MOUNTED )
	{
		sendInfo(QString("Added mapped and mounted device '%1'").arg(udi));
		emit sigDevMapped(udi);
		emit sigDevMounted(udi);

		return;
	}

	if (stat == VOL_MAPPED )
	{
		sendInfo(QString("Added mapped device '%1'").arg(udi));
		emit sigDevMapped(udi);

		return;
	}

	sendInfo(QString("Added NOT mapped device '%1'").arg(udi));
	emit sigDevNew(udi);
}

void HALBackend::slotUmountDevice( const QString& udi)
{
	QString vol = luksToClear[udi];

	if (vol.length() < 1) return;

	//kdDebug() << __func__ << "(" << udi << ")" << endl;

	DBusError error;
	DBusMessage *msg = NULL;
	DBusPendingCall *pcall = NULL;

	const char **poptions = NULL;

	dbus_error_init(&error);

	msg = dbus_message_new_method_call("org.freedesktop.Hal",
					vol.ascii(),
					"org.freedesktop.Hal.Device.Volume", "Unmount");

	if (!msg) {
		kdDebug() << "Failed to create new dbus message. UDI: " << vol << endl;
		goto error;
	}

	if (!dbus_message_append_args (msg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &poptions, 0, DBUS_TYPE_INVALID))
	{
		kdDebug() << "Failed to appendd to dbus message. UDI: " << vol << endl;
		goto error;
	}

	if (!dbus_connection_send_with_reply(_dbus_connection, msg, &pcall, -1)) {
		kdDebug() << __func__ << "(): No Memory for sending message with reply..." << endl;
		goto error;
	}

	if (!pcall) {
		kdDebug() << __func__ << "(): PendingCall is NULL! Connection is disconnected!" << endl;
		goto error;
	}

	//kdDebug() << __func__ << ": OK" << endl;

	if (!dbus_pending_call_set_notify(pcall, HALBackend::cmdCallback, NULL, NULL)) {
		kdDebug() << __func__ << "(): Got some trouble while setting callback function..." << endl;
		goto error;
	}

	dbus_message_unref(msg);

	return;

error:
	if (msg)
		dbus_message_unref(msg);

	kdDebug() << __func__ << ": error " << endl;

	return;
}

void HALBackend::slotMountDevice( const QString& udi)
{
	QString vol = luksToClear[udi];

	if (vol.length() < 1) return;

	//kdDebug() << __func__ << "(" << udi << ")" << endl;

	DBusError error;
	DBusMessage *msg = NULL;
	DBusPendingCall *pcall = NULL;
	const char *empty = "";
	const char **poptions = NULL;

	dbus_error_init(&error);

	msg = dbus_message_new_method_call("org.freedesktop.Hal",
					   vol.ascii(),
							   "org.freedesktop.Hal.Device.Volume", "Mount");

	if (!msg) {
		kdDebug() << "Failed to create new dbus message. UDI: " << vol << endl;
		goto error;
	}

	if (!dbus_message_append_args ( msg, DBUS_TYPE_STRING, &empty, DBUS_TYPE_INVALID ) )
	{
		kdDebug() << "Failed to appendd to dbus message. UDI: " << vol << endl;
		goto error;
	}

	if (!dbus_message_append_args ( msg, DBUS_TYPE_STRING, &empty, DBUS_TYPE_INVALID ) )
	{
		kdDebug() << "Failed to appendd to dbus message. UDI: " << vol << endl;
		goto error;
	}

	if (!dbus_message_append_args (msg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &poptions, 0, DBUS_TYPE_INVALID))
	{
		kdDebug() << "Failed to appendd to dbus message. UDI: " << vol << endl;
		goto error;
	}

	if (!dbus_connection_send_with_reply(_dbus_connection, msg, &pcall, -1)) {
		kdDebug() << __func__ << "(): No Memory for sending message with reply..." << endl;
		goto error;
	}

	if (!pcall) {
		kdDebug() << __func__ << "(): PendingCall is NULL! Connection is disconnected!" << endl;
		goto error;
	}

	//kdDebug() << __func__ << ": OK" << endl;

	if (!dbus_pending_call_set_notify(pcall, HALBackend::cmdCallback, NULL, NULL)) {
		kdDebug() << __func__ << "(): Got some trouble while setting callback function..." << endl;
		goto error;
	}

	dbus_message_unref(msg);

	return;

error:
	if (msg)
		dbus_message_unref(msg);

	kdDebug() << __func__ << ": error " << endl;

	return;
}

void HALBackend::slotRemoveDevice ( const QString& udi )
{
	if (udi.length() < 1) return;

	DBusError error;
	DBusMessage *msg = NULL;
	DBusPendingCall *pcall = NULL;

	dbus_error_init(&error);

	msg = dbus_message_new_method_call("org.freedesktop.Hal",
					udi.ascii(),
					"org.freedesktop.Hal.Device.Volume.Crypto", "Teardown");

	if (!msg) {
		kdDebug() << "Failed to create new dbus message. UDI: " << udi << endl;
		goto error;
	}


	if (!dbus_connection_send_with_reply(_dbus_connection, msg, &pcall, -1)) {
		kdDebug() << __func__ << "(): No Memory for sending message with reply..." << endl;
		goto error;
	}

	if (!pcall) {
		kdDebug() << __func__ << "(): PendingCall is NULL! Connection is disconnected!" << endl;
		goto error;
	}

	// kdDebug() << __func__ << ": OK" << endl;

	if (!dbus_pending_call_set_notify(pcall, HALBackend::cmdCallback, NULL, NULL)) {
		kdDebug() << __func__ << "(): Got some trouble while setting callback function..." << endl;
		goto error;
	}

	dbus_message_unref(msg);

	return;

error:
		if (msg)
		dbus_message_unref(msg);

	kdDebug() << __func__ << ": error " << endl;

	return;
}

void HALBackend::slotSendPassword ( char* udi, const char *password )
{
	DBusError error;
	DBusMessage *msg = 0;
	DBusPendingCall *pcall = 0;
	bool ok = true;

	dbus_error_init ( &error );

	msg = dbus_message_new_method_call (
			  "org.freedesktop.Hal", udi,
			  "org.freedesktop.Hal.Device.Volume.Crypto",
			  "Setup" );

	if ( !msg )
	{
		kdDebug() << __func__ << "(): Failed to create new dbus message. UDI: " << udi << endl;
		ok = false;
	}

	if ( ok && !dbus_message_append_args ( msg, DBUS_TYPE_STRING, &password, DBUS_TYPE_INVALID ) )
	{
		kdDebug() << __func__ << "(): Failed to setup a encrypt message.\n";
		ok = false;
	}

	if ( ok && !dbus_connection_send_with_reply ( _dbus_connection, msg, &pcall, -1 ) )
	{
		kdDebug() << __func__ << "(): No Memory for sending DBus message with reply...\n";
		ok = false;
	}

	if ( ok && !pcall )
	{
		kdDebug() << __func__ << "(): PendingCall is NULL! DBus connection is disconnected!\n";
		ok = false;
	}

	// Last two parameters only needed for dbus user data ( and freeing it ).
	if ( ok && !dbus_pending_call_set_notify ( pcall, HALBackend::passCallback, udi, NULL ) )
	{
		kdDebug() << __func__ << "(): Got some trouble while setting callback function...\n";
		ok = false;
	}

	dbus_message_unref ( msg );
}

void HALBackend::initScan()
{
	char **device_names;
	int num_devices;

	device_names = libhal_get_all_devices ( _halContext, &num_devices, NULL );

	if ( device_names )
	{
		QStringList devices;

		for ( int i = 0; i < num_devices; i++ )
		{
			devices.append(device_names[i]);
		}

		libhal_free_string_array ( device_names );

		for ( QStringList::Iterator it = devices.begin(); it != devices.end(); ++it )
		{
			addDevice((*it).ascii());
		}
	}
}

HALBackend::VolMapStatus HALBackend::volumeStatus ( const QString &udi )
{
	char **device_names;
	int num_devices;

	device_names = libhal_get_all_devices ( _halContext, &num_devices, NULL );

	if ( device_names == 0 )
	{
		// Something is wrong
		return VOL_UNKNOWN;
	}

	for ( int i = 0; i < num_devices; i++ )
	{
		QString val = getHalPropertyString ( device_names[i], "volume.crypto_luks.clear.backing_volume" );

		if ( udi == val )
		{
			luksToClear.insert(udi, device_names[i]);
			clearToLuks.insert(device_names[i], udi);

			QString blockDev = getHalPropertyString ( device_names[i], "block.device" );

			if (blockDev.startsWith("/dev/mapper/") && QFile::exists(blockDev))
			{
				VolMapStatus ret = VOL_MAPPED;

				if ( getHalPropertyBool(device_names[i], "volume.is_mounted") )
				{
					ret = VOL_MOUNTED;
				}

				libhal_free_string_array ( device_names );

				return ret;
			}
		}
	}

	if ( device_names ) libhal_free_string_array ( device_names );

	return VOL_NOT_MAPPED;
}

void HALBackend::passCallback ( DBusPendingCall* pcall, void *data )
{
	char *udi = ( char* ) data;
	DBusMessage *reply = 0;
	DBusError error;

	QString errorName = QString::null;
	QString errorMsg = QString::null;

	reply  = dbus_pending_call_steal_reply ( pcall );

	if ( !reply )
	{
		kdDebug() << __func__ << "(): Stolen DBus reply is empty!\n";
		goto error;
	}

	if ( dbus_message_get_type ( reply ) == DBUS_MESSAGE_TYPE_ERROR )
	{

		dbus_error_init ( &error );
		dbus_set_error_from_message ( &error, reply );

		//kdDebug() << __func__ << "(): Received error: " << error.name << ": " << error.message << endl;
		errorName = QString ( error.name );
		errorMsg = QString ( error.message );

		dbus_error_free ( &error );
		goto error_and_unref;
	}

	dbus_message_unref ( reply );

	dbus_pending_call_unref ( pcall );

	return;

error_and_unref:
		dbus_message_unref ( reply );

error:
		dbus_pending_call_unref ( pcall );

	s_HALBackend->sigPassError( QString(udi), errorName, errorMsg );

	return;
}

void HALBackend::cmdCallback ( DBusPendingCall* pcall, void *data )
{
	char *udi = ( char* ) data;
	DBusMessage *reply = 0;
	DBusError error;

	QString errorName = QString::null;
	QString errorMsg = QString::null;

	reply  = dbus_pending_call_steal_reply ( pcall );

	if ( !reply )
	{
		kdDebug() << __func__ << "(): Stolen DBus reply is empty!\n";
		goto error;
	}

	if ( dbus_message_get_type ( reply ) == DBUS_MESSAGE_TYPE_ERROR )
	{

		dbus_error_init ( &error );
		dbus_set_error_from_message ( &error, reply );

		//kdDebug() << __func__ << "(): Received error: " << error.name << ": " << error.message << endl;
		errorName = QString ( error.name );
		errorMsg = QString ( error.message );

		dbus_error_free ( &error );
		goto error_and_unref;
	}

	dbus_message_unref ( reply );

	dbus_pending_call_unref ( pcall );

	return;

error_and_unref:
		dbus_message_unref ( reply );

error:
		dbus_pending_call_unref ( pcall );

	s_HALBackend->sigError( QString(udi), errorName, errorMsg );

	return;
}

QString HALBackend::getHalPropertyString ( const QString &udi, const QString &key )
{
	if (udi.length() < 1) return QString();

	char* val;
	DBusError error;

	dbus_error_init ( &error );

	val = libhal_device_get_property_string ( _halContext, udi.ascii(), key.ascii(), &error );

	QString qVal;

	if ( val )
	{
		qVal = val;
		libhal_free_string ( val );
	}

	return qVal;
}

bool HALBackend::getHalPropertyBool ( const QString &udi, const QString &key )
{
	bool val;
	DBusError error;

	dbus_error_init ( &error );

	val = libhal_device_get_property_bool ( _halContext, udi.ascii(), key.ascii(), &error );

	return val;
}

bool HALBackend::initDBus()
{
	DBusError error;

	dbus_error_init ( &error );

	_dbus_connection = dbus_bus_get_private ( DBUS_BUS_SYSTEM, &error );

	if ( !_dbus_connection || dbus_error_is_set ( &error ) )
	{
		dbus_error_free ( &error );
		libhal_ctx_free ( _halContext );
		_halContext = NULL;
		return false;
	}

	dbus_connection_set_exit_on_disconnect ( _dbus_connection, FALSE );

	_dBusQtConnection = new DBusQt::Connection ( this );
	_dBusQtConnection->dbus_connection_setup_with_qt_main ( _dbus_connection );

	return true;
}

void HALBackend::closeDBus()
{
	if ( _dBusQtConnection )
	{
		_dBusQtConnection->close();
		delete _dBusQtConnection;
	}

	if ( _dbus_connection )
	{
		dbus_connection_unref ( _dbus_connection );
	}
}

bool HALBackend::initHAL()
{
	DBusError error;

	dbus_error_init ( &error );

	_halContext = libhal_ctx_new();

	if ( !_halContext )
	{
		kdDebug() << __func__ << "(): Failed to initialize HAL!\n";

		return false;
	}

	libhal_ctx_set_dbus_connection ( _halContext, _dbus_connection );

	// HAL callback functions
	libhal_ctx_set_device_added ( _halContext, HALBackend::hal_device_added );
	libhal_ctx_set_device_removed ( _halContext, HALBackend::hal_device_removed );
	libhal_ctx_set_device_new_capability ( _halContext, NULL );
	libhal_ctx_set_device_lost_capability ( _halContext, NULL );
	libhal_ctx_set_device_property_modified ( _halContext, HALBackend::hal_device_property_modified );
	libhal_ctx_set_device_condition ( _halContext, HALBackend::hal_device_condition );

	if ( !libhal_ctx_init ( _halContext, &error ) )
	{
		if ( dbus_error_is_set ( &error ) )
			dbus_error_free ( &error );

		libhal_ctx_free ( _halContext );

		_halContext = NULL;

		kdDebug() << __func__ << "(): Failed to init HAL context!\n";

		return false;
	}

	if ( !libhal_device_property_watch_all ( _halContext, &error ) )
	{
		kdDebug() << __func__ << "(): Failed to watch HAL properties!\n";

		return false;
	}

	return true;
}

void HALBackend::closeHAL()
{
	DBusError error;

	if ( !_halContext ) return;

	dbus_error_init ( &error );

	if ( !libhal_ctx_shutdown ( _halContext, &error ) )
	{
		if ( dbus_error_is_set ( &error ) )
		{
			kdDebug() << __func__ << "(): DBus Error: "
					<< error.name << "; Message: " << error.message << endl;
			dbus_error_free ( &error );
		}

		// Go on and try at least to free hal context....
	}

	if ( !libhal_ctx_free ( _halContext ) )
	{
		kdDebug() << __func__ << "(): Freeing of hal context failed...\n";
	}

	_halContext = 0;
}

/******************************************
 ** HAL CALL-BACKS                        **
 ******************************************/

void HALBackend::hal_device_added ( LibHalContext *ctx, const char *udi )
{
	QString desc = QString("Device Added: %1").arg(udi);

	Q_UNUSED ( ctx );

	s_HALBackend->sendInfo(desc);
	s_HALBackend->addDevice ( QString(udi) );
}

void HALBackend::hal_device_removed ( LibHalContext *ctx, const char *udi )
{
	QString desc = QString("Device Removed: %1").arg(udi);

	Q_UNUSED ( ctx );

	s_HALBackend->sendInfo(desc);
	s_HALBackend->removeDevice ( QString(udi) );
}

void HALBackend::hal_device_property_modified ( LibHalContext *ctx, const char *udi,
					   const char *key, dbus_bool_t is_removed, dbus_bool_t is_added )
{
	QString desc = QString("Device Modified: %1; Key: %2; IsRemoved: %3; IsAdded: %4")
			.arg(udi).arg(key).arg(is_removed).arg(is_added);

	bool added = is_added;

	Q_UNUSED ( ctx );
	Q_UNUSED ( is_removed );
	Q_UNUSED ( is_added );

	s_HALBackend->sendInfo(desc);
	s_HALBackend->modifyDevice ( QString(udi), QString(key), added );
}

void HALBackend::hal_device_condition ( LibHalContext *ctx, const char *udi,
				   const char *condition_name, const char* message )
{
	QString desc = QString("Device Condition: %1; CondName: %2; Message: %3")
			.arg(udi).arg(condition_name).arg(message);

	Q_UNUSED ( ctx );
	Q_UNUSED ( message );

	s_HALBackend->sendInfo(desc);
	//s_HALBackend->conditionDevice ( QString(udi), QString(condition_name), QString(message) );
}
