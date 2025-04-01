/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
        m_importFileStr = importFileSelectionDialog.selectedFiles().first();

        ui->m_layoutListBox->clear();

        TToolNamesMap toolNames;
        XmlNodeRef dummyNode = nullptr;

        GetIEditor()->GetSettingsManager()->GetMatchingLayoutNames(toolNames, dummyNode, m_importFileStr);

        if (!toolNames.empty())
        {
            TToolNamesMap::const_iterator tIt = toolNames.begin();

            for (; tIt != toolNames.end(); ++tIt)
            {
                if (!tIt->second.isEmpty())
                {
                    ui->m_layoutListBox->addItem(tIt->second);
                }
                else
                if (!tIt->first.isEmpty())
                {
                    ui->m_layoutListBox->addItem(tIt->first);
                }
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
    QString importFileStr = m_importFileStr;
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
        ImportSettings(importFileStr);
    }

    QtViewPaneManager::instance()->CloseAllNonStandardPanes();

    // Import layout
    if (!layouts.isEmpty())
    {
        ImportLayouts(importFileStr, layouts);
    }
}

void CSettingsManagerDialog::ImportSettings(QString file)
{
    if (QFile::exists(file))
    {
        GetIEditor()->GetSettingsManager()->ImportSettings(file);
    }
}

void CSettingsManagerDialog::ImportLayouts(QString file, const QStringList& layouts)
{
    auto viewPaneManager = QtViewPaneManager::instance();

    if (!layouts.isEmpty())
    {
        TToolNamesMap& allToolNames = *GetIEditor()->GetSettingsManager()->GetToolNames();
        if (allToolNames.empty())
        {
            return;
        }

        TToolNamesMap toolNames;

        for (const QString& layoutStr : layouts)
        {
            auto it = std::find_if(allToolNames.begin(), allToolNames.end(),
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

        // Remove previous tool node
        XmlNodeRef layoutNode = XmlHelpers::CreateXmlNode(EDITOR_LAYOUT_NODE);

        // Find in file and add selected tool node
        GetIEditor()->GetSettingsManager()->GetMatchingLayoutNames(toolNames, layoutNode, file);

        // restore main window layout

        auto it = toolNames.find(QStringLiteral(MAINFRM_LAYOUT_NORMAL));
        if (it == toolNames.end())
        {
            it = toolNames.find(QStringLiteral(MAINFRM_LAYOUT_PREVIEW));
        }

        if (it != toolNames.end())
        {
            const QString& className = it->first;

            XmlNodeRef dockingLayoutNode = layoutNode->findChild(className.toUtf8().data());
            if (dockingLayoutNode)
            {
                viewPaneManager->DeserializeLayout(dockingLayoutNode);
            }
        }

        // restore tool panes layouts

        for (it = toolNames.begin(); it != toolNames.end(); ++it)
        {
            const QString& className = it->first;
            const QString& paneName = it->second;

            if (className == QStringLiteral(MAINFRM_LAYOUT_NORMAL) || className == QStringLiteral(MAINFRM_LAYOUT_PREVIEW))
            {
                continue;
            }

            XmlNodeRef dockingLayoutNode = layoutNode->findChild(className.toUtf8().data());
            if (!dockingLayoutNode)
            {
                continue;
            }

            auto toolPanel = FindViewPane<QMainWindow>(paneName);
            if (!toolPanel)
            {
                continue;
            }

            XmlNodeRef windowStateNode = dockingLayoutNode->findChild("WindowState");
            if (windowStateNode)
            {
                toolPanel->restoreState(QByteArray::fromHex(windowStateNode->getContent()));
            }
        }
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
