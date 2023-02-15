/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
//#include <API/EditorAssetSystemAPI.h>

#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

#include <AzQtComponents/Components/Widgets/MessageBox.h>

#include <QDir>
#include <QPushButton>
#include <QWidget>
#include <QtWidgets/QMessageBox>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        void AssetBrowserViewUtils::RenameEntry(const AZStd::vector<AssetBrowserEntry*>& entries, QWidget* callingWidget)
        {
            if (entries.size() != 1)
            {
                return;
            }
            using namespace AzFramework::AssetSystem;
            bool connectedToAssetProcessor = false;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::AssetProcessorIsReady);

            if (connectedToAssetProcessor)
            {
                using namespace AZ::IO;
                AssetBrowserEntry* item = entries[0];
                bool isFolder = item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
                Path toPath;
                Path fromPath;
                if (isFolder)
                {
                    // There is currently a bug in AssetProcessorBatch that doesn't handle empty folders
                    // This code is needed until that bug is fixed. GHI 13340
                    if (IsFolderEmpty(item->GetFullPath()))
                    {
                        EditName(callingWidget);
                                                
                        return;
                    }
                    fromPath = item->GetFullPath() + "/*";
                    toPath = item->GetFullPath() + "TempFolderTestName/*";
                }
                else
                {
                    fromPath = item->GetFullPath();
                    toPath = fromPath;
                    toPath.ReplaceExtension("renameFileTestExtension");
                }
                AssetChangeReportRequest request(
                    AZ::OSString(fromPath.c_str()), AZ::OSString(toPath.c_str()), AssetChangeReportRequest::ChangeType::CheckMove);
                AssetChangeReportResponse response;

                if (SendRequest(request, response))
                {
                    if (!response.m_lines.empty())
                    {
                        AZStd::string message;
                        AZ::StringFunc::Join(message, response.m_lines.begin(), response.m_lines.end(), "\n");
                        AzQtComponents::FixedWidthMessageBox msgBox(
                            600,
                            QObject::tr(isFolder ? "Before Rename Folder Information" : "Before Rename Asset Information"),
                            QObject::tr("The asset you are renaming may be referenced in other assets."),
                            QObject::tr("More information can be found by pressing \"Show Details...\"."),
                            message.c_str(),
                            QMessageBox::Warning,
                            QMessageBox::Cancel,
                            QMessageBox::Yes,
                            callingWidget);
                        auto* renameButton = msgBox.addButton(QObject::tr("Rename"), QMessageBox::YesRole);
                        msgBox.exec();

                        if (msgBox.clickedButton() == static_cast<QAbstractButton*>(renameButton))
                        {
                            EditName(callingWidget);
                        }
                    }
                    else
                    {
                        EditName(callingWidget);
                    }
                }
            }
        }

        void AssetBrowserViewUtils::AfterRename(QString newVal, AZStd::vector<AssetBrowserEntry*>& entries, QWidget* callingWidget)
        {
            if (entries.size() != 1)
            {
                return;
            }
            using namespace AZ::IO;
            AssetBrowserEntry* item = entries[0];
            bool isFolder = item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
            bool isEmptyFolder = isFolder && IsFolderEmpty(item->GetFullPath());
            Path toPath;
            Path fromPath;
            if (isFolder)
            {
                Path tempPath = item->GetFullPath();
                tempPath.ReplaceFilename(newVal.toStdString().c_str());
                // There is currently a bug in AssetProcessorBatch that doesn't handle empty folders
                // This code is needed until that bug is fixed. GHI 13340
                if (isEmptyFolder)
                {
                    fromPath = item->GetFullPath();
                    toPath = tempPath.String();
                    AZ::IO::SystemFile::Rename(fromPath.c_str(), toPath.c_str());
                    return;
                }
                else
                {
                    fromPath = item->GetFullPath() + "/*";
                    toPath = tempPath.String() + "/*";
                }
            }
            else
            {
                fromPath = item->GetFullPath();
                PathView extension = fromPath.Extension();
                toPath = fromPath;
                toPath.ReplaceFilename(newVal.toStdString().c_str());
                toPath.ReplaceExtension(extension);
            }
            // if the source path is the same as the destintion path then we don't need to go any further
            if (fromPath == toPath)
            {
                return;
            }
            using namespace AzFramework::AssetSystem;
            AssetChangeReportRequest moveRequest(
                AZ::OSString(fromPath.c_str()), AZ::OSString(toPath.c_str()), AssetChangeReportRequest::ChangeType::Move);
            AssetChangeReportResponse moveResponse;
            if (SendRequest(moveRequest, moveResponse))
            {
                if (!moveResponse.m_lines.empty())
                {
                    AZStd::string message;
                    AZ::StringFunc::Join(message, moveResponse.m_lines.begin(), moveResponse.m_lines.end(), "\n");
                    AzQtComponents::FixedWidthMessageBox msgBox(
                        600,
                        QObject::tr(isFolder ? "After Rename Folder Information" : "After Rename Asset Information"),
                        QObject::tr("The asset has been renamed."),
                        QObject::tr("More information can be found by pressing \"Show Details...\"."),
                        message.c_str(),
                        QMessageBox::Information,
                        QMessageBox::Ok,
                        QMessageBox::Ok,
                        callingWidget);
                    msgBox.exec();
                }
                if (isFolder)
                {
                    AZ::IO::SystemFile::DeleteDir(item->GetFullPath().c_str());
                }
            }
        }

        bool AssetBrowserViewUtils::IsFolderEmpty(AZStd::string_view path)
        {
            return QDir(path.data()).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        }

        void AssetBrowserViewUtils::EditName(QWidget* callingWidget)
        {
            AssetBrowserTreeView* treeView = qobject_cast<AssetBrowserTreeView*>(callingWidget);
            AssetBrowserTableView* tableView = qobject_cast<AssetBrowserTableView*>(callingWidget);

            if (treeView)
            {
                treeView->edit(treeView->currentIndex());
            }
            else if (tableView)
            {
                tableView->edit(tableView->currentIndex());
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
