/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AzCore/base.h"
#include <AzCore/IO/FileIO.h>
#include "EditorDefs.h"

#include "SettingsManagerDialog.h"

// Qt
#include <QMessageBox>

// AzToolsFramework
#include <AzToolsFramework/API/ViewPaneOptions.h>

// Editor
#include "SettingsManager.h"
#include "MainWindow.h"
#include "QtViewPaneManager.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"    // for CAutoDirectoryRestoreFileDialog
#include "LyViewPaneNames.h"                        // for LyViewPane::

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_SettingsManagerDialog.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

//////////////////////////////////////////////////////////////////////////
CSettingsManagerDialog::CSettingsManagerDialog(QWidget* pParent)
    : QDialog(pParent)
    , ui(new Ui::SettingsManagerDialog)
{
    ui->setupUi(this);

    ui->m_skipToolsChk->setChecked(false);
    ui->m_importSettingsChk->setChecked(false);

    connect(ui->m_exportBtn, &QAbstractButton::clicked, this, &CSettingsManagerDialog::OnExportBtnClick);
    connect(ui->m_readBtn, &QAbstractButton::clicked, this, &CSettingsManagerDialog::OnReadBtnClick);
    connect(ui->m_importBtn, &QAbstractButton::clicked, this, &CSettingsManagerDialog::OnImportBtnClick);
    connect(ui->m_closeAllToolsBtn, &QAbstractButton::clicked, this, &CSettingsManagerDialog::OnCloseAllTools);

    connect(ui->m_layoutListBox, &QListWidget::itemSelectionChanged, this, &CSettingsManagerDialog::OnSelectionChanged);

    // Disable import button until a layout is selected.
    ui->m_importBtn->setEnabled(false);
}



CSettingsManagerDialog::~CSettingsManagerDialog()
{
}

const GUID& CSettingsManagerDialog::GetClassID()
{
    // {64E0B47F-FA9B-46a9-AEF4-BDAC021B5B2F}
    static const GUID guid =
    {
        0x64e0b47f, 0xfa9b, 0x46a9, { 0xae, 0xf4, 0xbd, 0xac, 0x2, 0x1b, 0x5b, 0x2f }
    };

    return guid;
}


void CSettingsManagerDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.paneRect = QRect(5, 100, 210, 505);
    options.showInMenu = false;

    AzToolsFramework::RegisterViewPane<CSettingsManagerDialog>(LyViewPane::EditorSettingsManager, LyViewPane::CategoryOther, options);
}


void CSettingsManagerDialog::OnReadBtnClick()
{
    char szFilters[] = "Editor Settings and Layout File (*.xml);;All files (*)";
    CAutoDirectoryRestoreFileDialog importFileSelectionDialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, "xml", {}, szFilters, {}, {}, this);

    if (importFileSelectionDialog.exec())
    {
        const QString importedFile = importFileSelectionDialog.selectedFiles().first();
        m_importFileStr = importedFile;

        ui->m_layoutListBox->clear();

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(importedFile.toUtf8().data(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            AZ_Warning("CSettingsManagerDialog", false, "Failed to open layout file ( %s ).\n", importedFile.toUtf8().data());
            return;
        }

        if (!fileStream.CanRead())
        {
            AZ_Warning("CSettingsManagerDialog", false, "Failed to read layout file ( %s ).\n", importedFile.toUtf8().data());
            return;
        }

        AZ::IO::SizeType length = fileStream.GetLength();

        if (length == 0)
        {
            return;
        }

        AZStd::vector<char> charBuffer;

        charBuffer.resize_no_construct(length + 1);
        fileStream.Read(length, charBuffer.data());
        charBuffer.back() = 0;

        if (!m_document.parse<0>(charBuffer.data()))
        {
            return;
        }

        auto layoutNames = GetIEditor()->GetSettingsManager()->GetMatchingLayoutNames(m_document);
        for (auto& en : layoutNames)
        {
            if (!en.humanReadable.isEmpty())
            {
                ui->m_layoutListBox->addItem(en.humanReadable);
            }
            else if (!en.key.isEmpty())
            {
                ui->m_layoutListBox->addItem(en.key);
            }
        }

        ui->m_layoutListBox->selectAll();
    }
}

void CSettingsManagerDialog::OnExportBtnClick()
{
    char szFilters[] = "Editor Settings and Layout File (*.xml);;All files (*)";
    CAutoDirectoryRestoreFileDialog exportFileSelectionDialog(QFileDialog::AcceptSave, QFileDialog::AnyFile, "xml", "ExportedEditor.xml", szFilters, {}, {}, this);

    if (exportFileSelectionDialog.exec())
    {
        QString file = exportFileSelectionDialog.selectedFiles().first();
        GetIEditor()->GetSettingsManager()->SetExportFileName(file);
        QtViewPaneManager::instance()->ClosePane(LyViewPane::EditorSettingsManager);
        GetIEditor()->GetSettingsManager()->Export();
    }
}

void CSettingsManagerDialog::OnImportBtnClick()
{
    bool bImportSettings = ui->m_importSettingsChk->isChecked();

    QString ask = tr("This will close all opened Views. Make sure to save your projects and backup layout before continuing");

    QStringList layouts;

    for (auto& item : ui->m_layoutListBox->selectedItems())
    {
        layouts.append(item->text());
    }

    // Show warning window to the user in case he selects some layouts to import
    if (!layouts.isEmpty())
    {
        if (QMessageBox::question(this, tr("Editor"), ask) != QMessageBox::Yes)
        {
            return;
        }
    }

    // Import Settings
    if (bImportSettings)
    {
        ImportSettings(m_importFileStr);
    }

    QtViewPaneManager::instance()->CloseAllNonStandardPanes();

    // Import layout
    if (layouts.isEmpty())
    {
        return;
    }
    auto viewPaneManager = QtViewPaneManager::instance();
    auto settingsManager = GetIEditor()->GetSettingsManager();

    settingsManager->UpdateLayoutNode();
    TToolNamesMap& allToolNames = *settingsManager->GetToolNames();
    if (allToolNames.empty())
    {
        return;
    }


    TToolNamesMap toolNames;
    for (const QString& layoutStr : layouts)
    {
        auto it = AZStd::find_if(
            allToolNames.begin(),
            allToolNames.end(),
            [&](const std::pair<QString, QString>& v)
            {
                return layoutStr == v.second;
            });

        if (it == allToolNames.end())
        {
            continue;
        }

        toolNames.insert(*it);
    }
    auto it = toolNames.find(QStringLiteral(MAINFRM_LAYOUT_NORMAL));
    if (it == toolNames.end())
    {
        it = toolNames.find(QStringLiteral(MAINFRM_LAYOUT_PREVIEW));
    }

    AZStd::vector<struct CSettingsManager::FilterNameResult> filterResults = CSettingsManager::FilterMatchingLayoutNodes(m_document, toolNames);
    if (it != toolNames.end())
    {
        const QString& className = it->first;
        auto dockingLayoutNode = AZStd::find_if(
            filterResults.begin(),
            filterResults.end(),
            [&className](struct CSettingsManager::FilterNameResult& res)
            {
                return res.key == className;
            });
        if (dockingLayoutNode != filterResults.end())
        {
            viewPaneManager->DeserializeLayout(dockingLayoutNode->node);
        }
    }
    for(auto& filter: filterResults)
    {
        if (filter.key == QStringLiteral(MAINFRM_LAYOUT_NORMAL) || filter.key == QStringLiteral(MAINFRM_LAYOUT_PREVIEW)== 0)
        {
            return;
        }
        auto toolPanel = FindViewPane<QMainWindow>(filter.humanReadable);
        if (!toolPanel)
        {
            continue;
        }

        for (auto* it = filter.node->first_node(); it; it->next_sibling())
        {
            if (azstricmp(it->name(), "WindowState"))
            {
                toolPanel->restoreState(QByteArray::fromHex(it->value()));
                break;
            }
        }
    }
}

void CSettingsManagerDialog::ImportSettings(QString file)
{
    if (QFile::exists(file))
    {
        GetIEditor()->GetSettingsManager()->ImportSettings(file);
    }
}

void CSettingsManagerDialog::OnCloseAllTools()
{
    auto viewPaneManager = QtViewPaneManager::instance();

    viewPaneManager->CloseAllNonStandardPanes();
    viewPaneManager->SaveLayout();
}

void CSettingsManagerDialog::OnSelectionChanged()
{
    ui->m_importBtn->setEnabled(ui->m_layoutListBox->selectedItems().count());
}

#include <moc_SettingsManagerDialog.cpp>
