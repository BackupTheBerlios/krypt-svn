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

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kglobal.h>

#include "kryptapp.h"

static const char kryptVersion[] = "0.1";
static const KCmdLineOptions options[] =
{
	{ "login", I18N_NOOP("Application is being auto-started at KDE session start"), 0L },
	KCmdLineLastOption
};

int main(int argc, char **argv)
{
	KAboutData aboutData("krypt", I18N_NOOP("Krypt"), kryptVersion, I18N_NOOP("Krypt"), KAboutData::License_GPL, "(c) 2007, 2008 Jakub Schmidkte", 0L, "");
	aboutData.addAuthor( "Jakub Schmidtke", 0, "sjakub@users.berlios.de" );
	aboutData.setProductName("krypt");
	KGlobal::locale()->setMainCatalogue("krypt");

	KCmdLineArgs::init(argc,argv,&aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KApplication::addCmdLineOptions();

	KryptApp app;

	return app.exec();
}
