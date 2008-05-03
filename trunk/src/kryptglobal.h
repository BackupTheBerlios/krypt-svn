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

#ifndef _KRYPT_GLOBAL_H_
#define _KRYPT_GLOBAL_H_

#define KRYPT_HAL_DEV_EVENT_NEW       0
#define KRYPT_HAL_DEV_EVENT_REMOVED   1
#define KRYPT_HAL_DEV_EVENT_MAPPED    2
#define KRYPT_HAL_DEV_EVENT_UNMAPPED  3
#define KRYPT_HAL_DEV_EVENT_MOUNTED   4
#define KRYPT_HAL_DEV_EVENT_UMOUNTED  5

#define KRYPT_CONF_UDI_PREFIX    "krypt_volume_"
#define KRYPT_CONF_GLOBAL_GROUP  "krypt_global"

#define KRYPT_CONF_GROUP_BY_CAT  "category_group"
#define KRYPT_CONF_FLAT_MENU     "flat_menu"
#define KRYPT_CONF_VERSION       "config_version"

#define KRYPT_CONF_SHOW_POPUP    "show_popup"

#define KRYPT_CONF_SHOW_UMOUNT   "show_umount"
#define KRYPT_CONF_SHOW_MOUNT    "show_mount"
#define KRYPT_CONF_SHOW_ENCRYPT  "show_encrypt"
#define KRYPT_CONF_SHOW_DECRYPT  "show_decrypt"
#define KRYPT_CONF_SHOW_OPTIONS  "show_options"

#define KRYPT_CONF_AUTO_ENCRYPT  "auto_encrypt"
#define KRYPT_CONF_AUTO_DECRYPT  "auto_decrypt"

#define KRYPT_CONF_PASS_IN_WALLET "use_kwallet"

// All the following HAVE TO BE lowercase!

#define KRYPT_CONF_OPT_ON        "on"
#define KRYPT_CONF_OPT_OFF       "off"
#define KRYPT_CONF_OPT_DEFAULT   "default"

#endif // _KRYPT_H_
