/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "preferencesdialog.h"
#include "application.h"
#include <QDir>
#include <QStringBuilder>
#include <glib.h>

using namespace LxImage;

PreferencesDialog::PreferencesDialog(QWidget* parent):
  QDialog(parent) {
  ui.setupUi(this);

  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  app->addWindow();

  initIconThemes(settings);
  ui.bgColor->setColor(settings.bgColor());
  ui.fullScreenBgColor->setColor(settings.fullScreenBgColor());
  ui.maxRecentFiles->setValue(settings.maxRecentFiles());
  ui.slideShowInterval->setValue(settings.slideShowInterval());
  ui.oulineBox->setChecked(settings.isOutlineShown());
  ui.annotationBox->setChecked(settings.isAnnotationsToolbarShown());
}

PreferencesDialog::~PreferencesDialog() {
  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
}

void PreferencesDialog::accept() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();

  // apply icon theme
  if(settings.useFallbackIconTheme()) {
    // only apply the value if icon theme combo box is in use
    // the combo box is hidden when auto-detection of icon theme by qt works.
    QString newIconTheme = ui.iconTheme->itemData(ui.iconTheme->currentIndex()).toString();
    if(newIconTheme != settings.fallbackIconTheme()) {
      settings.setFallbackIconTheme(newIconTheme);
      QIcon::setThemeName(newIconTheme);
      // update the UI by emitting a style change event
      const auto allWidgets = QApplication::allWidgets();
      for(QWidget *widget : allWidgets) {
        QEvent event(QEvent::StyleChange);
        QApplication::sendEvent(widget, &event);
      }
    }
  }

  settings.setBgColor(ui.bgColor->color());
  settings.setFullScreenBgColor(ui.fullScreenBgColor->color());
  settings.setMaxRecentFiles(ui.maxRecentFiles->value());
  settings.setSlideShowInterval(ui.slideShowInterval->value());
  settings.showOutline(ui.oulineBox->isChecked());
  settings.showAnnotationsToolbar(ui.annotationBox->isChecked());

  settings.save();
  QDialog::accept();
  app->applySettings();
}

static void findIconThemesInDir(QHash<QString, QString>& iconThemes, const QString& dirName) {
  QDir dir(dirName);
  const QStringList subDirs = dir.entryList(QDir::AllDirs);
  GKeyFile* kf = g_key_file_new();
  for(const QString& subDir : subDirs) {
    QString indexFile = dirName + QLatin1Char('/') + subDir + QStringLiteral("/index.theme");
    if(g_key_file_load_from_file(kf, indexFile.toLocal8Bit().constData(), GKeyFileFlags(0), nullptr)) {
      // FIXME: skip hidden ones
      // icon theme must have this key, so it has icons if it has this key
      // otherwise, it might be a cursor theme or any other kind of theme.
      if(g_key_file_has_key(kf, "Icon Theme", "Directories", nullptr)) {
        char* dispName = g_key_file_get_locale_string(kf, "Icon Theme", "Name", nullptr, nullptr);
        // char* comment = g_key_file_get_locale_string(kf, "Icon Theme", "Comment", NULL, NULL);
        iconThemes[subDir] = QString::fromUtf8(dispName);
        g_free(dispName);
      }
    }
  }
  g_key_file_free(kf);
}

void PreferencesDialog::initIconThemes(Settings& settings) {
  // check if auto-detection is done (for example, from xsettings)
  if(settings.useFallbackIconTheme()) { // auto-detection failed
    // load xdg icon themes and select the current one
    QHash<QString, QString> iconThemes;
    // user customed icon themes
    findIconThemesInDir(iconThemes, QString::fromUtf8(g_get_home_dir()) + QStringLiteral("/.icons"));

    // search for icons in system data dir
    const char* const* dataDirs = g_get_system_data_dirs();
    for(const char* const* dataDir = dataDirs; *dataDir; ++dataDir) {
      findIconThemesInDir(iconThemes, QString::fromUtf8((*dataDir)) + QStringLiteral("/icons"));
    }

    iconThemes.remove(QStringLiteral("hicolor")); // remove hicolor, which is only a fallback
    QHash<QString, QString>::const_iterator it;
    for(it = iconThemes.constBegin(); it != iconThemes.constEnd(); ++it) {
      ui.iconTheme->addItem(it.value(), it.key());
    }
    ui.iconTheme->model()->sort(0); // sort the list of icon theme names

    // select current theme name
    int n = ui.iconTheme->count();
    int i;
    for(i = 0; i < n; ++i) {
      QVariant itemData = ui.iconTheme->itemData(i);
      if(itemData == settings.fallbackIconTheme()) {
        break;
      }
    }
    if(i >= n)
      i = 0;
    ui.iconTheme->setCurrentIndex(i);
  }
  else { // auto-detection of icon theme works, hide the fallback icon theme combo box.
    ui.iconThemeLabel->hide();
    ui.iconTheme->hide();
  }
}

void PreferencesDialog::done(int r) {
  QDialog::done(r);
  deleteLater();
}

