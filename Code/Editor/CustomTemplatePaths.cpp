/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorCommon.h"
#include "CustomTemplatePaths.h"
#include "ui_CustomTemplatePaths.h"

#include <QString>
#include <QListWidgetItem>

CustomTemplatePaths::CustomTemplatePaths(QWidget* parent) : QDialog(parent), ui(new Ui::CustomTemplatePaths)
{
    ui->setupUi(this);

    setStyleSheet("QListWidget::item {height: 16px; padding-left: 0px; padding-right: 0px; background-color: transparent;}");

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "O3DE");
    settings.beginGroup("CustomTemplatePaths");
    lastSelectedFolderPath = settings.value("lastSelectedFolderPath").toString();
    settings.endGroup();

    if(lastSelectedFolderPath.isEmpty())
    {
        lastSelectedFolderPath = (AZ::Utils::GetProjectPath().c_str());
    }

    QObject::connect(ui->pushButtonAdd, &QPushButton::clicked, this, [this]()
    {
        QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Folder"), lastSelectedFolderPath);
        if (!folderPath.isEmpty())
        {
            QFileInfo info(folderPath);
            auto* item = new QListWidgetItem(info.baseName());
            item->setIcon(QIcon(":/NewLevel/res/folder.png"));
            item->setData(Qt::UserRole, folderPath);
            item->setToolTip(folderPath);
            ui->listWidgetTemplatePaths->addItem(item);

            lastSelectedFolderPath = folderPath;

            SaveSettings();
        }
    });

    QObject::connect(ui->pushButtonRemove, &QPushButton::clicked, this, [this]()
    {
        if (const QListWidgetItem* selectedItem = ui->listWidgetTemplatePaths->currentItem())
        {
            int index = ui->listWidgetTemplatePaths->row(selectedItem);
            ui->listWidgetTemplatePaths->takeItem(index);

            SaveSettings();
        }
    });

    QObject::connect(ui->pushButtonOk, &QPushButton::clicked, this, &QDialog::accept);

    LoadSettings();
}

void CustomTemplatePaths::LoadSettings() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "O3DE");
    settings.beginGroup("CustomTemplatePaths");
    const QStringList folderPaths = settings.value("folderPaths").toStringList();
    settings.endGroup();

    for (auto& Item: folderPaths)
    {
        QFileInfo info(Item);
        auto* item = new QListWidgetItem(info.baseName());
        item->setIcon(QIcon(":/NewLevel/res/folder.png"));
        item->setData(Qt::UserRole, Item);
        item->setToolTip(Item);
        ui->listWidgetTemplatePaths->addItem(item);
    }
}

void CustomTemplatePaths::SaveSettings() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "O3DE");
    QStringList customTemplateFolderPaths;
    int itemCount = ui->listWidgetTemplatePaths->count();
    for (int i = 0; i < itemCount; ++i)
    {
        auto* item = ui->listWidgetTemplatePaths->item(i);
        customTemplateFolderPaths.append(item->data(Qt::UserRole).toString());
    }
    settings.beginGroup("CustomTemplatePaths");
    settings.setValue("folderPaths", customTemplateFolderPaths);
    settings.setValue("lastSelectedFolderPath", lastSelectedFolderPath);
    settings.endGroup();
}

CustomTemplatePaths::~CustomTemplatePaths()
{
    delete ui;
}
