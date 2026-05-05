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

#include "ROMLibraryDialog.h"
#include "ui_ROMLibraryDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStyle>

#include <QListWidgetItem>
#include <QPixmap>

#include "NDSMetadata.h"
#include "Config.h"
#include "main.h"
#include "RainbowPushButton.h"

static const int RomPathRole = Qt::UserRole + 1;

ROMLibraryDialog::ROMLibraryDialog(QWidget* parent) : QWidget(parent), ui(new Ui::ROMLibraryDialog), mainWindow((MainWindow*)parent)
{
    ui->setupUi(this);

    ui->contentStack->setCurrentIndex(0);

    libraryDir = emuDirectory + QDir::separator() + "romlibrary";
    metadataPath = libraryDir + QDir::separator() + "metadata.toml";
    icoDir = libraryDir + QDir::separator() + "ico";

    loadRomDirectories();

    QPixmap iconPixmap(":/melon-icon");
    ui->headerIcon->setPixmap(iconPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->headerTitle->setText(RainbowPushButton::rainbowHtml("melonDS"));
    ui->headerTitle->setTextFormat(Qt::RichText);

    QPixmap logoPixmap(":/melon-logo");
    ui->lblEmptyLogo->setPixmap(logoPixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->btnRescan->setIcon(QIcon::fromTheme("view-refresh", style()->standardIcon(QStyle::SP_BrowserReload)));
    ui->btnRescan->setText("");

    connect(ui->romListWidget, &QListWidget::itemDoubleClicked, this, &ROMLibraryDialog::onRomItemActivated);

    ui->romListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->romListWidget->setTextElideMode(Qt::ElideRight);
}

ROMLibraryDialog::~ROMLibraryDialog()
{
    delete ui;
}

void ROMLibraryDialog::loadRomDirectories()
{
    romDirectories.clear();

    auto& cfg = mainWindow->getEmuInstance()->getGlobalConfig();
    QString paths = QString::fromStdString(cfg.GetString("ROMLibrary.Paths"));

    if (!paths.isEmpty())
    {
        QStringList dirs = paths.split(';', Qt::SkipEmptyParts);
        for (QString& dir : dirs)
        {
            dir = dir.trimmed();
            if (!dir.isEmpty() && QDir(dir).exists())
                romDirectories.append(dir);
        }
    }

    QString legacyPath = QString::fromStdString(cfg.GetString("ROMLibrary.Path"));
    if (!legacyPath.isEmpty())
    {
        if (QDir(legacyPath).exists() && !romDirectories.contains(legacyPath))
            romDirectories.append(legacyPath);

        saveRomDirectories();
        cfg.SetString("ROMLibrary.Path", "");
        Config::Save();
    }
}

void ROMLibraryDialog::saveRomDirectories()
{
    auto& cfg = mainWindow->getEmuInstance()->getGlobalConfig();
    cfg.SetString("ROMLibrary.Paths", romDirectories.join(';').toStdString());
    Config::Save();
}

void ROMLibraryDialog::refreshOnStart()
{
    refresh();
}

void ROMLibraryDialog::refresh()
{
    loadRomDirectories();

    romEntries.clear();
    cachedEntries.clear();

    if (!romDirectories.isEmpty())
    {
        ensureLibraryDirs();
        loadCachedMetadata();
        scanDirectories();
    }
    populateViews();
    updateContentPage();
}

void ROMLibraryDialog::updateContentPage()
{
    if (romDirectories.isEmpty() || romEntries.isEmpty())
        ui->contentStack->setCurrentIndex(0);
    else
        ui->contentStack->setCurrentIndex(1);

    if (romDirectories.isEmpty())
    {
        ui->lblStatus->setVisible(false);
    }
    else if (!romEntries.isEmpty())
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText(QString("%1 ROM(s)").arg(romEntries.size()));
    }
    else
    {
        ui->lblStatus->setVisible(true);
        ui->lblStatus->setText("No ROMs found in configured directories");
    }
}

void ROMLibraryDialog::ensureLibraryDirs()
{
    QDir().mkpath(libraryDir);
    QDir().mkpath(icoDir);
}

void ROMLibraryDialog::loadCachedMetadata()
{
    cachedEntries.clear();

    QFile file(metadataPath);
    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    NDSMetadata current;
    bool inEntry = false;

    auto extractQuotedValue = [](const QString& line) -> QString
    {
        int eq = line.indexOf('=');
        if (eq < 0)
            return {};

        QString val = line.mid(eq + 1).trimmed();
        int first = val.indexOf('"');
        if (first < 0)
            return {};

        QString result;
        result.reserve(val.size());
        bool escaped = false;
        for (int i = first + 1; i < val.size(); i++)
        {
            if (escaped)
            {
                result.append(val[i]);
                escaped = false;
            }
            else if (val[i] == '\\')
            {
                escaped = true;
            }
            else if (val[i] == '"')
            {
                break;
            }
            else
            {
                result.append(val[i]);
            }
        }
        return result;
    };

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        if (line == "[[rom]]")
        {
            if (inEntry && !current.romPath.isEmpty())
            {
                romEntries.append(current);
                cachedEntries.insert(current.romPath, romEntries.size() - 1);
            }

            current = NDSMetadata();
            inEntry = true;
            continue;
        }

        if (!inEntry)
            continue;

        if (line.startsWith("path ="))
            current.romPath = extractQuotedValue(line);
        else if (line.startsWith("title ="))
            current.title = extractQuotedValue(line);
        else if (line.startsWith("gamecode ="))
            current.gameCode = extractQuotedValue(line);
        else if (line.startsWith("icon ="))
            current.iconFile = extractQuotedValue(line);
    }

    if (inEntry && !current.romPath.isEmpty())
    {
        romEntries.append(current);
        cachedEntries.insert(current.romPath, romEntries.size() - 1);
    }

    file.close();
}

void ROMLibraryDialog::saveCachedMetadata()
{
    QFile file(metadataPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    for (const NDSMetadata& entry : romEntries)
    {
        auto escape = [](const QString& s) -> QString {
            return QString(s).replace('\\', "\\\\").replace('"', "\\\"");
        };

        out << "[[rom]]\n";
        out << "path = \"" << escape(entry.romPath) << "\"\n";
        out << "title = \"" << escape(entry.title) << "\"\n";
        out << "gamecode = \"" << escape(entry.gameCode) << "\"\n";
        out << "icon = \"" << escape(entry.iconFile) << "\"\n";
        out << "\n";
    }

    file.close();
}

void ROMLibraryDialog::scanSingleDirectory(const QString& romDir)
{
    QDir dir(romDir);
    if (!dir.exists())
        return;

    QDirIterator it(romDir, QStringList() << "*.nds", QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        QString filePath = it.next();

        if (cachedEntries.contains(filePath))
            continue;

        NDSMetadata metadata;
        QImage icon;
        if (!NDSMetadataExtractor::extractFromROM(filePath, metadata, &icon))
            continue;

        if (!metadata.iconFile.isEmpty() && !icon.isNull())
        {
            QString iconPath = icoDir + QDir::separator() + metadata.iconFile;
            NDSMetadataExtractor::saveIconPNG(icon, iconPath);
        }

        romEntries.append(metadata);
        cachedEntries.insert(metadata.romPath, romEntries.size() - 1);
    }
}

void ROMLibraryDialog::scanDirectories()
{
    int oldSize = romEntries.size();

    for (int i = romEntries.size() - 1; i >= 0; i--)
    {
        bool keep = QFile::exists(romEntries[i].romPath);
        if (keep)
        {
            bool inConfiguredDir = false;
            for (const QString& romDir : romDirectories)
            {
                if (QDir::cleanPath(romEntries[i].romPath).startsWith(QDir::cleanPath(romDir) + '/'))
                {
                    inConfiguredDir = true;
                    break;
                }
            }
            keep = inConfiguredDir;
        }
        if (!keep)
            romEntries.removeAt(i);
    }

    cachedEntries.clear();
    for (int i = 0; i < romEntries.size(); i++)
        cachedEntries.insert(romEntries[i].romPath, i);

    bool modified = (romEntries.size() != oldSize);

    for (const QString& romDir : romDirectories)
    {
        int beforeSize = romEntries.size();
        scanSingleDirectory(romDir);
        if (romEntries.size() != beforeSize)
            modified = true;
    }

    if (modified)
        saveCachedMetadata();
}

void ROMLibraryDialog::populateViews()
{
    ui->romListWidget->clear();

    for (const NDSMetadata& metadata : romEntries)
        addRomToList(metadata);
}

void ROMLibraryDialog::addRomToList(const NDSMetadata& metadata)
{
    QListWidgetItem* item = new QListWidgetItem(metadata.title, ui->romListWidget);
    item->setData(RomPathRole, metadata.romPath);

    if (!metadata.iconFile.isEmpty())
    {
        QString iconPath = icoDir + QDir::separator() + metadata.iconFile;
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull())
            item->setIcon(QIcon(pixmap));
    }
}

void ROMLibraryDialog::on_btnSetDirectory_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Add ROM Directory", QString());
    if (dir.isEmpty())
        return;

    if (romDirectories.contains(dir))
    {
        QMessageBox::information(this, "ROM Library", "This directory is already configured.");
        return;
    }

    romDirectories.append(dir);
    saveRomDirectories();

    ensureLibraryDirs();
    int oldSize = romEntries.size();
    scanSingleDirectory(dir);
    if (romEntries.size() != oldSize)
        saveCachedMetadata();
    populateViews();
    updateContentPage();
}

void ROMLibraryDialog::on_btnRescan_clicked()
{
    if (romDirectories.isEmpty())
    {
        QMessageBox::information(this, "ROM Library", "Please add a ROM directory first.");
        return;
    }

    romEntries.clear();
    cachedEntries.clear();

    ensureLibraryDirs();
    loadCachedMetadata();
    scanDirectories();
    populateViews();
    updateContentPage();
}

void ROMLibraryDialog::onRomItemActivated(QListWidgetItem* item)
{
    if (!item)
        return;

    QString romPath = item->data(RomPathRole).toString();
    if (romPath.isEmpty())
        return;

    emit loadROMRequested(romPath);
}
