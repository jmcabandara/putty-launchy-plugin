#include "Options.h"

Options::Options(QSettings* _settings)
	: settings(_settings), dlg(NULL)
{
	readOptions();
}

void Options::readOptions()
{
	int arraySize;

#define XSTRING(name, def) \
	name = settings->value("PuTTY/" #name).toString();
#define XBOOL(name, def) \
	name = settings->value("PuTTY/" #name).toBool();
#define XSTRINGLIST(name, def) \
	name.clear(); \
	arraySize = settings->beginReadArray("PuTTY/" #name); \
		for (int i = 0; i < arraySize; ++i) { \
			settings->setArrayIndex(i); \
			textTriggers.append(settings->value("trigger").toString()); \
		} \
	settings->endArray();

#include "options.def"

#undef XSTRING
#undef XBOOL
#undef XSTRINGLIST
}

void Options::writeOptions()
{
#define XSTRING(name, def) \
	settings->setValue("PuTTY/" #name, name);
#define XBOOL XSTRING
#define XSTRINGLIST(name, def) \
	settings->beginWriteArray("PuTTY/" #name); \
		for (int i = 0; i < name.size(); ++i) { \
			settings->setArrayIndex(i); \
			settings->setValue("trigger", name.at(i)); \
		} \
	settings->endArray();

#include "options.def"

#undef XSTRING
#undef XBOOL
#undef XSTRINGLIST
}

void Options::writeDefaults()
{
#define XSTRING(name, def) \
	name = QString(def);
#define XBOOL(name, def) \
	name = def;
#define XITEM(def) \
	<< def
#define XSTRINGLIST(name, def) \
	name.clear(); \
	name def;

#include "options.def"

#undef XSTRING
#undef XBOOL
#undef XITEM
#undef XSTRINGLIST

	//write back
	writeOptions();
}

void Options::upgradeDefaults()
{
	/* write missing elements */
#define EXISTS(name) settings->contains("PuTTY/" #name)
#define XSTRING(name, def) \
	if (!EXISTS(name)) name = QString(def);
#define XBOOL(name, def) \
	if (!EXISTS(name)) name = def;
#define XITEM(def) \
	<< def
#define XSTRINGLIST(name, def) \
	if (!EXISTS(name/size)) name def;

#include "options.def"

#undef XSTRING
#undef XBOOL
#undef XITEM
#undef XSTRINGLIST
#undef EXISTS

	/* add any special version to version changes here */

	//to write back or not to write back, that is the question
	//lets not since that may allow us to change the default later
}

QWidget* Options::getDlg(QWidget* parent)
{
	if (dlg == NULL)
		dlg = new OptionsDlg(parent, this);

	return dlg;
}

void Options::killDlg(bool save)
{
	if (save) {
		dlg->writeBack();

		writeOptions();
	}

	delete dlg;

	dlg = NULL;
}

QString Options::getVersion()
{
	return settings->value("PuTTY/version").toString();
}

void Options::setVersion(const QString& version)
{
	settings->setValue("PuTTY/version", version);
}
