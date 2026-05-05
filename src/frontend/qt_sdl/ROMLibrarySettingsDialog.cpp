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

#include <QFileDialog>
#include <QMessageBox>

#include "Config.h"
#include "main.h"

#include "ROMLibrarySettingsDialog.h"
#include "ui_ROMLibrarySettingsDialog.h"

ROMLibrarySettingsDialog* ROMLibrarySettingsDialog::currentDlg = nullptr;

ROMLibrarySettingsDialog::ROMLibrarySettingsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::ROMLibrarySettingsDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    auto& cfg = ((MainWindow*)parent)->getEmuInstance()->getGlobalConfig();
    QString paths = QString::fromStdString(cfg.GetString("ROMLibrary.Paths"));

    if (!paths.isEmpty())
    {
        QStringList dirs = paths.split(';', Qt::SkipEmptyParts);
        for (const QString& dir : dirs)
        {
            QString trimmed = dir.trimmed();
            if (!trimmed.isEmpty())
                ui->lstDirectories->addItem(trimmed);
        }
    }

    ui->btnRemove->setEnabled(false);

    connect(ui->lstDirectories, &QListWidget::itemSelectionChanged, this, [this]()
    {
        ui->btnRemove->setEnabled(ui->lstDirectories->currentRow() >= 0);
    });
}

ROMLibrarySettingsDialog::~ROMLibrarySettingsDialog()
{
    delete ui;
}

void ROMLibrarySettingsDialog::done(int r)
{
    if (!((MainWindow*)parent())->getEmuInstance())
    {
        QDialog::done(r);
        closeDlg();
        return;
    }

    if (r == QDialog::Accepted)
    {
        QStringList dirs;
        for (int i = 0; i < ui->lstDirectories->count(); i++)
            dirs.append(ui->lstDirectories->item(i)->text());

        auto& cfg = ((MainWindow*)parent())->getEmuInstance()->getGlobalConfig();
        cfg.SetString("ROMLibrary.Paths", dirs.join(';').toStdString());
        cfg.SetString("ROMLibrary.Path", "");
        Config::Save();

        emit romLibrarySettingsChanged();
    }

    QDialog::done(r);
    closeDlg();
}

void ROMLibrarySettingsDialog::on_btnAdd_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Add ROM Directory", QString());
    if (dir.isEmpty())
        return;

    for (int i = 0; i < ui->lstDirectories->count(); i++)
    {
        if (ui->lstDirectories->item(i)->text() == dir)
        {
            QMessageBox::information(this, "ROM Library Settings", "This directory is already in the list.");
            return;
        }
    }

    ui->lstDirectories->addItem(dir);
}

void ROMLibrarySettingsDialog::on_btnRemove_clicked()
{
    int row = ui->lstDirectories->currentRow();
    if (row >= 0)
        delete ui->lstDirectories->takeItem(row);
}
