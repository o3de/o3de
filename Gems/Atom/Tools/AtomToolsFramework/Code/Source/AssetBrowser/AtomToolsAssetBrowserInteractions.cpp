/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/string/wildcard.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

namespace AtomToolsFramework
{
    AtomToolsAssetBrowserInteractions::AtomToolsAssetBrowserInteractions()
    {
        AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    AtomToolsAssetBrowserInteractions::~AtomToolsAssetBrowserInteractions()
    {
        AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    }

    void AtomToolsAssetBrowserInteractions::RegisterContextMenuActions(
        const FilterCallback& filterCallback, const ActionCallback& actionCallback)
    {
        m_contextMenuCallbacks.emplace_back(filterCallback, actionCallback);
    }

    void AtomToolsAssetBrowserInteractions::AddContextMenuActions(
        QWidget* caller, QMenu* menu, const AssetBrowserEntryVector& entries)
    {
        const AssetBrowserEntry* entry = entries.empty() ? nullptr : entries.front();
        if (!entry)
        {
            return;
        }

        m_caller = caller;
        QObject::connect(m_caller, &QObject::destroyed, [this]() { m_caller = nullptr; });

        // Add all of the custom context menu entries first
        for (const auto& contextMenuCallbackPair : m_contextMenuCallbacks)
        {
            if (contextMenuCallbackPair.first(entries))
            {
                contextMenuCallbackPair.second(caller, menu, entries);
            }
        }

        if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
        {
            AddContextMenuActionsForSourceEntries(caller, menu, entry);
        }
        else if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
        {
            AddContextMenuActionsForFolderEntries(caller, menu, entry);
        }

        AddContextMenuActionsForAllEntries(caller, menu, entry);
        AddContextMenuActionsForSourceControl(caller, menu, entry);
    }

    void AtomToolsAssetBrowserInteractions::AddContextMenuActionsForSourceEntries(
        [[maybe_unused]] QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        menu->addAction("Duplicate", [entry]()
            {
                const auto& duplicateFilePath = GetUniqueFilePath(entry->GetFullPath());
                if (QFile::copy(entry->GetFullPath().c_str(), duplicateFilePath.c_str()))
                {
                    QFile::setPermissions(duplicateFilePath.c_str(), QFile::ReadOther | QFile::WriteOther);

                    // Auto add file to source control
                    AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                        duplicateFilePath.c_str(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});
                }
            });

        QMenu* scriptsMenu = menu->addMenu(QObject::tr("Python Scripts"));
        const AZStd::vector<AZStd::string> arguments{ entry->GetFullPath() };
        AddRegisteredScriptToMenu(scriptsMenu, "/O3DE/AtomToolsFramework/AssetBrowser/ContextMenuScripts", arguments);
    }

    void AtomToolsAssetBrowserInteractions::AddContextMenuActionsForFolderEntries(
        QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        menu->addAction(QObject::tr("Create new sub folder..."), [caller, entry]()
            {
                bool ok = false;
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
                            AZ_Error("MaterialBrowser", false, "Failed to make new folder");
                            return;
                        }
                    }
                }
            });
    }

    void AtomToolsAssetBrowserInteractions::AddContextMenuActionsForAllEntries(
        [[maybe_unused]] QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        menu->addAction(AzQtComponents::fileBrowserActionName(), [entry]()
            {
                AzQtComponents::ShowFileOnDesktop(entry->GetFullPath().c_str());
            });

        menu->addSeparator();
        menu->addAction(QObject::tr("Copy Name To Clipboard"), [=]()
            {
                QApplication::clipboard()->setText(entry->GetName().c_str());
            });
        menu->addAction(QObject::tr("Copy Path To Clipboard"), [=]()
            {
                QApplication::clipboard()->setText(entry->GetFullPath().c_str());
            });
    }

    void AtomToolsAssetBrowserInteractions::AddContextMenuActionsForSourceControl(
        [[maybe_unused]] QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        using namespace AzToolsFramework;

        bool isActive = false;
        SourceControlConnectionRequestBus::BroadcastResult(isActive, &SourceControlConnectionRequests::IsActive);

        AZStd::string path = entry->GetFullPath();
        if (isActive && ValidateDocumentPath(path))
        {
            menu->addSeparator();

            QMenu* sourceControlMenu = menu->addMenu("Source Control");

            // Update the enabled state of source control menu actions only if menu is shown
            QMenu::connect(sourceControlMenu, &QMenu::aboutToShow, [this, path]()
                {
                    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::GetFileInfo, path.c_str(),
                        [this](bool success, const SourceControlFileInfo& info) { UpdateContextMenuActionsForSourceControl(success, info); });
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

    void AtomToolsAssetBrowserInteractions::UpdateContextMenuActionsForSourceControl(bool success, AzToolsFramework::SourceControlFileInfo info)
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
} // namespace AtomToolsFramework
