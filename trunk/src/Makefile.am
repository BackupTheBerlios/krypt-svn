SUBDIRS = pics

# set the include path for X, qt and KDE
INCLUDES = $(DBUS_CFLAGS) $(HAL_CFLAGS) $(all_includes)

# these are the headers for your project
noinst_HEADERS = halbackend.h kryptapp.h kryptconf.h kryptdebug.h \
	kryptdevconf.h kryptdevice.h kryptdevitem.h kryptdialog.h kryptglobal.h kryptsystray.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/krypt.pot


#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = krypt

# the application source, library search path, and link libraries
krypt_SOURCES = confdialog.ui decryptdialog.ui devconfdialog.ui halbackend.cpp \
	kryptapp.cpp kryptconf.cpp kryptdevconf.cpp kryptdevice.cpp kryptdevicetriv.cpp \
	kryptdevitem.cpp kryptdialog.cpp kryptsystray.cpp main.cpp
krypt_LDFLAGS = $(KDE_RPATH) $(all_libraries)
krypt_LDADD = -lkwalletclient $(DBUS_LIBS) $(HAL_LIBS) $(LIB_KDEUI) -ldbus-qt-1

# this is where the desktop file will go
shelldesktopdir   = $(kde_appsdir)/Utilities
shelldesktop_DATA = krypt.desktop

KDE_ICON = krypt
