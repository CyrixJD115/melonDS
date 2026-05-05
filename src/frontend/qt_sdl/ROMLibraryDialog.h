/*
    Copyright 2016-2026 melonDS team

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef ROMLIBRARYDIALOG_H
#define ROMLIBRARYDIALOG_H

#include <QWidget>
#include <QListWidget>
#include <QMap>

#include "NDSMetadata.h"

class MainWindow;

namespace Ui { class ROMLibraryDialog; }

class ROMLibraryDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ROMLibraryDialog(QWidget* parent);
    ~ROMLibraryDialog();

    void refreshOnStart();
    void refresh();

signals:
    void loadROMRequested(QString filepath);

private slots:
    void on_btnSetDirectory_clicked();
    void on_btnRescan_clicked();
    void onRomItemActivated(QListWidgetItem* item);

private:
    Ui::ROMLibraryDialog* ui;
    MainWindow* mainWindow;

    QStringList romDirectories;
    QString libraryDir;
    QString metadataPath;
    QString icoDir;

    QList<NDSMetadata> romEntries;
    QMap<QString, int> cachedEntries;

    void loadRomDirectories();
    void saveRomDirectories();
    void ensureLibraryDirs();
    void loadCachedMetadata();
    void saveCachedMetadata();
    void scanDirectories();
    void scanSingleDirectory(const QString& romDir);
    void populateViews();
    void addRomToList(const NDSMetadata& metadata);
    void updateContentPage();
};

#endif // ROMLIBRARYDIALOG_H
