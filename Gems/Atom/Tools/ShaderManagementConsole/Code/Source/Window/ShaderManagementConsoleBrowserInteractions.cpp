/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <Window/ShaderManagementConsoleBrowserInteractions.h>

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleBrowserInteractions::ShaderManagementConsoleBrowserInteractions()
    {
        using namespace AzToolsFramework::AssetBrowser;

        AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    ShaderManagementConsoleBrowserInteractions::~ShaderManagementConsoleBrowserInteractions()
    {
        AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails ShaderManagementConsoleBrowserInteractions::GetSourceFileDetails([[maybe_unused]] const char* fullSourceFileName)
    {
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    void ShaderManagementConsoleBrowserInteractions::AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
    {
        AssetBrowserEntry* entry = entries.empty() ? nullptr : entries.front();
        if (!entry)
        {
            return;
        }

        m_caller = caller;
        QObject::connect(m_caller, &QObject::destroyed, [this]()
        {
            m_caller = nullptr;
        });

        if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
        {
            const auto source = azalias_cast<const SourceAssetBrowserEntry*>(entry);
            AddContextMenuActionsForOtherSource(caller, menu, source);
        }
        else if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
        {
            const auto folder = azalias_cast<const FolderAssetBrowserEntry*>(entry);
            AddContextMenuActionsForFolder(caller, menu, folder);
        }
    }

    void ShaderManagementConsoleBrowserInteractions::AddContextMenuActionsForOtherSource(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry)
    {
        menu->addAction("Open", [entry]()
        {
            if (AzFramework::StringFunc::Path::IsExtension(entry->GetFullPath().c_str(), AZ::RPI::ShaderVariantListSourceData::Extension))
            {
                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, entry->GetFullPath().c_str());
            }
            else
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(entry->GetFullPath().c_str()));
            }
        });

        menu->addAction("Duplicate...", [entry]()
        {
            const QFileInfo duplicateFileInfo(AtomToolsFramework::GetDuplicationFileInfo(entry->GetFullPath().c_str()));
            if (!duplicateFileInfo.absoluteFilePath().isEmpty())
            {
                if (QFile::copy(entry->GetFullPath().c_str(), duplicateFileInfo.absoluteFilePath()))
                {
                    QFile::setPermissions(duplicateFileInfo.absoluteFilePath(), QFile::ReadOther | QFile::WriteOther);

                    // Auto add file to source control
                    AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                        duplicateFileInfo.absoluteFilePath().toUtf8().constData(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});
                }
            }
        });

        menu->addAction(AzQtComponents::fileBrowserActionName(), [entry]()
        {
            AzQtComponents::ShowFileOnDesktop(entry->GetFullPath().c_str());
        });

        menu->addSeparator();
        menu->addAction("Generate Shader Variant List", [entry]() {
            const QString script = "@engroot@/Gems/Atom/Tools/ShaderManagementConsole/Scripts/GenerateShaderVariantListForMaterials.py";
            AZStd::vector<AZStd::string_view> pythonArgs{ entry->GetFullPath() };
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, script.toUtf8().constData(), pythonArgs);
        });

        menu->addAction("Run Python on Asset...", [entry]()
        {
            const QString script = QFileDialog::getOpenFileName(nullptr, "Run Script", QString(), QString("*.py"));
            if (!script.isEmpty())
            {
                AZStd::vector<AZStd::string_view> pythonArgs { entry->GetFullPath() };
                AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, script.toUtf8().constData(), pythonArgs);
            }
        });

        AddPerforceMenuActions(caller, menu, entry);
    }

    void ShaderManagementConsoleBrowserInteractions::AddContextMenuActionsForFolder(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::FolderAssetBrowserEntry* entry)
    {
        menu->addAction(AzQtComponents::fileBrowserActionName(), [entry]()
        {
            AzQtComponents::ShowFileOnDesktop(entry->GetFullPath().c_str());
        });

        QAction* createFolderAction = menu->addAction(QObject::tr("Create new sub folder..."));
        QObject::connect(createFolderAction, &QAction::triggered, caller, [caller, entry]()
        {
            bool ok;
            QString newFolderName = QInputDialog::getText(caller, "Enter new folder name", "name:", QLineEdit::Normal, "NewFolder", &ok);
            if (ok)
            {
                if (newFolderName.isEmpty())
                {
                    QMessageBox msgBox(QMessageBox::Icon::Critical, "Error", "Folder name can't be empty", QMessageBox::Ok, caller);
                    msgBox.exec();
                }
                else
                {
                    AZStd::string newFolderPath;
                    AzFramework::StringFunc::Path::Join(entry->GetFullPath().c_str(), newFolderName.toUtf8().constData(), newFolderPath);
                    QDir dir(newFolderPath.c_str());
                    if (dir.exists())
                    {
                        QMessageBox::critical(caller, "Error", "Folder with this name already exists");
                        return;
                    }
                    auto result = dir.mkdir(newFolderPath.c_str());
                    if (!result)
                    {
                        AZ_Error("ShaderManagementConsoleBrowser", false, "Failed to make new folder");
                        return;
                    }
                }
            }
        });
    }

    void ShaderManagementConsoleBrowserInteractions::AddPerforceMenuActions([[maybe_unused]] QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        using namespace AzToolsFramework;

        bool isActive = false;
        SourceControlConnectionRequestBus::BroadcastResult(isActive, &SourceControlConnectionRequests::IsActive);

        if (isActive)
        {
            menu->addSeparator();

            AZStd::string path = entry->GetFullPath();
            AzFramework::StringFunc::Path::Normalize(path);

            QMenu* sourceControlMenu = menu->addMenu("Source Control");

            // Update the enabled state of source control menu actions only if menu is shown
            QMenu::connect(sourceControlMenu, &QMenu::aboutToShow, [this, path]()
            {
                SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::GetFileInfo, path.c_str(),
                    [this](bool success, const SourceControlFileInfo& info) { UpdateSourceControlActions(success, info); });
            });

            // add get latest action
            m_getLatestAction = sourceControlMenu->addAction("Get Latest", [path]()
            {
                SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestLatest, path.c_str(),
                    [](bool, const SourceControlFileInfo&) {});
            });
            QObject::connect(m_getLatestAction, &QObject::destroyed, [this]()
            {
                m_getLatestAction = nullptr;
            });
            m_getLatestAction->setEnabled(false);

            // add add action
            m_addAction = sourceControlMenu->addAction("Add", [path]()
            {
                SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, path.c_str(), true,
                    [path](bool, const SourceControlFileInfo&)
                {
                    SourceControlThumbnailRequestBus::Broadcast(&SourceControlThumbnailRequests::FileStatusChanged, path.c_str());
                });
            });
            QObject::connect(m_addAction, &QObject::destroyed, [this]()
            {
                m_addAction = nullptr;
            });
            m_addAction->setEnabled(false);

            // add checkout action
            m_checkOutAction = sourceControlMenu->addAction("Check Out", [path]()
            {
                SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, path.c_str(), true,
                    [path](bool, const SourceControlFileInfo&)
                {
                    SourceControlThumbnailRequestBus::Broadcast(&SourceControlThumbnailRequests::FileStatusChanged, path.c_str());
                });
            });
            QObject::connect(m_checkOutAction, &QObject::destroyed, [this]()
            {
                m_checkOutAction = nullptr;
            });
            m_checkOutAction->setEnabled(false);

            // add undo checkout action
            m_undoCheckOutAction = sourceControlMenu->addAction("Undo Check Out", [path]()
            {
                SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestRevert, path.c_str(),
                    [path](bool, const SourceControlFileInfo&)
                {
                    SourceControlThumbnailRequestBus::Broadcast(&SourceControlThumbnailRequests::FileStatusChanged, path.c_str());
                });
            });
            QObject::connect(m_undoCheckOutAction, &QObject::destroyed, [this]()
            {
                m_undoCheckOutAction = nullptr;
            });
            m_undoCheckOutAction->setEnabled(false);
        }
    }

    void ShaderManagementConsoleBrowserInteractions::UpdateSourceControlActions(bool success, AzToolsFramework::SourceControlFileInfo info)
    {
        if (!success && m_caller)
        {
            QMessageBox::critical(m_caller, "Error", "Source control operation failed.");
        }
        if (m_getLatestAction)
        {
            m_getLatestAction->setEnabled(info.IsManaged() && info.HasFlag(AzToolsFramework::SCF_OutOfDate));
        }
        if (m_addAction)
        {
            m_addAction->setEnabled(!info.IsManaged());
        }
        if (m_checkOutAction)
        {
            m_checkOutAction->setEnabled(info.IsManaged() && info.IsReadOnly() && !info.IsLockedByOther());
        }
        if (m_undoCheckOutAction)
        {
            m_undoCheckOutAction->setEnabled(info.IsManaged() && !info.IsReadOnly());
        }
    }
} // namespace ShaderManagementConsole
