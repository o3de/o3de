/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <precompiled.h>

#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QScrollBar>
#include <QDateTime>
#include <QToolButton>

#include "VersionExplorer.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <Editor/Settings.h>
#include <Editor/View/Windows/Tools/UpgradeTool/VersionExplorer.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ui_VersionExplorer.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>

namespace ScriptCanvasEditor
{
    VersionExplorer::VersionExplorer(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent)
        , m_ui(new Ui::VersionExplorer())
    {
        m_ui->setupUi(this);

        m_ui->tableWidget->horizontalHeader()->setVisible(false);
        m_ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        m_ui->tableWidget->setColumnWidth(3, 22);

        m_ui->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
        m_ui->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);

        connect(m_ui->scanButton, &QPushButton::pressed, this, &VersionExplorer::OnScan);
        connect(m_ui->closeButton, &QPushButton::pressed, this, &VersionExplorer::OnClose);
        connect(m_ui->upgradeAllButton, &QPushButton::pressed, this, &VersionExplorer::OnUpgradeAll);

        m_ui->progressBar->setValue(0);
        m_ui->progressBar->setVisible(false);

        m_keepEditorAlive = AZStd::make_unique<EditorKeepAlive>();

        m_inspectingAsset = m_assetsToInspect.end();

    }

    VersionExplorer::~VersionExplorer()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        UpgradeNotifications::Bus::Handler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

    }

    void VersionExplorer::OnClose()
    {
        reject();
    }

    bool VersionExplorer::IsUpgrading() const
    {
        return m_inProgressAsset != m_assetsToUpgrade.end() && m_inProgress;
    }

    void VersionExplorer::OnSystemTick()
    {
        switch (m_state)
        {
        case ProcessState::Scan:

            if (!m_inProgress && m_inspectingAsset != m_assetsToInspect.end())
            {
                m_inProgress = true;
                AZ::Data::AssetInfo& assetToUpgrade = *m_inspectingAsset;

                m_currentAsset = AZ::Data::AssetManager::Instance().GetAsset(assetToUpgrade.m_assetId, assetToUpgrade.m_assetType, AZ::Data::AssetLoadBehavior::PreLoad);
                m_currentAsset.BlockUntilLoadComplete();
                if (m_currentAsset.IsReady())
                {
                    // The asset is ready, grab its info
                    m_inProgress = true;
                    InspectAsset(m_currentAsset);
                }

                m_ui->spinner->SetText(QObject::tr("%1").arg(m_currentAsset.GetHint().c_str()));
            }
            break;

        case ProcessState::Upgrade:

            if (!IsUpgrading())
            {
                OperationResult result = BackupGraph(*m_inProgressAsset);
                // Make the backup
                if (result == OperationResult::BackupSuccess)
                {
                    QList<QTableWidgetItem*> items = m_ui->tableWidget->findItems(m_inProgressAsset->GetHint().c_str(), Qt::MatchFlag::MatchExactly);
                    if (!items.isEmpty())
                    {
                        for (auto* item : items)
                        {
                            int row = item->row();
                            AzQtComponents::StyledBusyLabel* spinner = qobject_cast<AzQtComponents::StyledBusyLabel*>(m_ui->tableWidget->cellWidget(row, ColumnStatus));
                            spinner->SetIsBusy(true);
                        }
                    }

                    // Upgrade the graph
                    UpgradeGraph(*m_inProgressAsset);
                }
                else
                {
                    GraphUpgradeComplete(*m_inProgressAsset, result);
                }

            }
            break;

        default:
            break;
        }

        FlushLogs();

        AZ::Data::AssetManager::Instance().DispatchEvents();
        AZ::SystemTickBus::ExecuteQueuedEvents();

    }

    // Backup

    void VersionExplorer::OnUpgradeAll()
    {
        AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(true);

        m_state = ProcessState::Upgrade;
        m_inProgressAsset = m_assetsToUpgrade.begin();

        AZ::SystemTickBus::Handler::BusConnect();
    }

    VersionExplorer::OperationResult VersionExplorer::BackupGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        bool makeBackup = m_ui->makeBackupCheckbox->isChecked();
        if (!makeBackup)
        {
            return OperationResult::SkipBackup;
        }

        QDateTime theTime = QDateTime::currentDateTime();
        QString subFolder = theTime.toString("yyyy-MM-dd [HH.mm.ss]");

        AZStd::string backupPath = AZStd::string::format("@devroot@/ScriptCanvas_BACKUP/%s", subFolder.toUtf8().data());
        char backupPathCStr[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(backupPath.c_str(), backupPathCStr, AZ_MAX_PATH_LEN);
        backupPath = backupPathCStr;

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(backupPath.c_str()))
        {
            if (AZ::IO::FileIOBase::GetInstance()->CreatePath(backupPath.c_str()) != AZ::IO::ResultCode::Success)
            {
                AZ_Error("Script Canvas", false, "Failed to create backup folder %s", backupPath.c_str());
                return OperationResult::BackupFail_CreateFolder;
            }
        }

        AZStd::string devRoot = "@devroot@";
        AZStd::string devAssets = "@devassets@";

        char devRootCStr[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(devRoot.c_str(), devRootCStr, AZ_MAX_PATH_LEN);

        char devAssetsCStr[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(devAssets.c_str(), devAssetsCStr, AZ_MAX_PATH_LEN);

        AZStd::string relativePath = devAssetsCStr;
        AzFramework::StringFunc::Replace(relativePath, devRootCStr, "");
        if (relativePath.starts_with("/"))
        {
            relativePath = relativePath.substr(1, relativePath.size() - 1);
        }

        AZStd::string sourceFilePath;

        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo;
        bool sourceInfoFound{};
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, asset.GetHint().c_str(), assetInfo, watchFolder);
        if (sourceInfoFound)
        {
            AZStd::string assetPath;
            AzFramework::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), assetPath);

            sourceFilePath = assetPath;
        }
        else
        {
            // The file no longer exists, we'll need to skip it.
            return OperationResult::BackupFail_FileNotFound;
        }

        devRoot = devRootCStr;
        AzFramework::StringFunc::Path::Normalize(devRoot);

        relativePath = sourceFilePath;
        AzFramework::StringFunc::Replace(relativePath, devRoot.c_str(), "");
        if (relativePath.starts_with("/"))
        {
            relativePath = relativePath.substr(1, relativePath.size() - 1);
        }

        AZStd::string targetFilePath;
        AzFramework::StringFunc::Path::Join(backupPath.c_str(), relativePath.c_str(), targetFilePath);

        if (AZ::IO::FileIOBase::GetInstance()->Copy(sourceFilePath.c_str(), targetFilePath.c_str()) != AZ::IO::ResultCode::Error)
        {
            AZ_TracePrintf("Script Canvas", "Backed up: %s  ---> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
        }
        else
        {
            AZ_TracePrintf("Script Canvas", "Error creating backup: %s  ---> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
            return OperationResult::BackupFail;
        }

        return OperationResult::BackupSuccess;
    }

    // Upgrade

    template <typename AssetType>
    AZ::Entity* UpgradeGraphProcess(const AZ::Data::Asset<AZ::Data::AssetData>& asset, VersionExplorer* /*versionExplorer*/)
    {
        AssetType* scriptCanvasAsset = asset.GetAs<AssetType>();
        AZ_Assert(scriptCanvasAsset, "Unable to get the asset of type: %s", azrtti_typeid<AssetType>().template ToString<AZStd::string>().c_str());

        if (!scriptCanvasAsset)
        {
            return nullptr;
        }

        AZ::Entity* scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
        AZ_Assert(scriptCanvasEntity, "The Script Canvas asset must have a valid entity");
        if (!scriptCanvasEntity)
        {
            return nullptr;
        }


        AZ::Entity* queryEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(queryEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasEntity->GetId());
        if (queryEntity)
        {
            if (queryEntity->GetState() == AZ::Entity::State::Active)
            {
                queryEntity->Deactivate();
            }

            scriptCanvasEntity = queryEntity;
        }

        if (scriptCanvasEntity->GetState() == AZ::Entity::State::Constructed)
        {
            scriptCanvasEntity->Init();
        }

        if (scriptCanvasEntity->GetState() == AZ::Entity::State::Init)
        {
            scriptCanvasEntity->Activate();
        }

        auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
        AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");

        if (graphComponent)
        {
            if (!graphComponent->UpgradeGraph(asset))
            {
                // The upgrade was skipped due to nothing to update (though if we're here, we identified that something was out of date)
            }
        }

        return scriptCanvasEntity;
    }

    void VersionExplorer::UpgradeGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        m_inProgress = true;

        m_ui->spinner->SetText(QObject::tr("Upgrading: %1").arg(asset.GetHint().c_str()));

        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        UpgradeNotifications::Bus::Handler::BusConnect();

        if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
        {
            m_scriptCanvasEntity = UpgradeGraphProcess<ScriptCanvasAsset>(asset, this);
        }

        if (!m_scriptCanvasEntity)
        {
            AZ_Assert(m_scriptCanvasEntity, "The ScriptCanvas asset should have an entity");
            return;
        }
    }

    void VersionExplorer::OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>& asset, bool /*skipped*/ /*= false*/)
    {
        AZStd::string relativePath, fullPath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, asset.GetId());

        bool fullPathFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);

        AZStd::string tmpFileName;
        bool tmpFilesaved = false;

        // here we are saving the graph to a temp file instead of the original file and then copying the temp file to the original file.
        // This ensures that AP will not a get a file change notification on an incomplete graph file causing it to fail processing. Temp files are ignored by AP.
        if (AZ::IO::CreateTempFileName(fullPath.c_str(), tmpFileName))
        {
            AZ::IO::FileIOStream fileStream(tmpFileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);

            if (fileStream.IsOpen())
            {
                if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
                {
                    tmpFilesaved = AZ::Utils::SaveObjectToStream<ScriptCanvas::ScriptCanvasData>(fileStream, AZ::DataStream::ST_XML, &asset.GetAs<ScriptCanvasAsset>()->GetScriptCanvasData());
                }

                fileStream.Close();
            }

            using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
            SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, fullPath.c_str(), true,
                [this, &asset, fullPath, tmpFileName, tmpFilesaved](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    if (!info.IsReadOnly())
                    {
                        if (tmpFilesaved)
                        {
                            PerformMove(asset, tmpFileName, fullPath);
                        }
                    }
                    else
                    {
                        if (m_overwriteAll)
                        {
                            AZ::IO::SystemFile::SetWritable(info.m_filePath.c_str(), true);

                            if (tmpFilesaved)
                            {
                                PerformMove(asset, tmpFileName, fullPath);
                            }
                        }
                        else
                        {
                            int result = QMessageBox::No;
                            if (!m_overwriteAll)
                            {
                                QMessageBox mb(QMessageBox::Warning,
                                    QObject::tr("Failed to Save Upgraded File"),
                                    QObject::tr("The upgraded file could not be saved because the file is read only.\nDo you want to make it writeable and overwrite it?"),
                                    QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No, this);

                                result = mb.exec();
                                if (result == QMessageBox::YesToAll)
                                {
                                    m_overwriteAll = true;
                                }
                            }

                            if (result == QMessageBox::Yes || m_overwriteAll)
                            {
                                AZ::IO::SystemFile::SetWritable(info.m_filePath.c_str(), true);

                                if (tmpFilesaved)
                                {
                                    PerformMove(asset, tmpFileName, fullPath);
                                }
                            }

                        }
                    }
                    //if (!info.IsReadOnly())
                    //{
                    //    if (tmpFilesaved)
                    //    {
                    //        auto normTarget = fullPath;
                    //        AzFramework::StringFunc::Path::Normalize(normTarget);

                    //        auto moveResult = AZ::IO::SmartMove(tmpFileName.c_str(), normTarget.c_str());
                    //        if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
                    //        {
                    //            // Bump the slice asset up in the asset processor's queue.
                    //            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, fullPath.c_str());

                    //            AZ::SystemTickBus::QueueFunction([this, asset]() { GraphUpgradeComplete(asset); });
                    //        }
                    //        else
                    //        {
                    //            AZ::SystemTickBus::QueueFunction([this, asset, tmpFileName, fullPath]() { RetryMove(asset, tmpFileName, fullPath); });
                    //        }

                    //    }
                    //}
                    //else
                    //{
                    //    QWidget* mainWindow = nullptr;
                    //    AzToolsFramework::EditorRequests::Bus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                    //    QMessageBox::warning(mainWindow, QObject::tr("Unable to Modify Script Canvas asset"),
                    //        QObject::tr("File is not writable."), QMessageBox::Ok, QMessageBox::Ok);
                    //}
                });
        }
    }

    void VersionExplorer::PerformMove(AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target)
    {
        auto moveResult = AZ::IO::SmartMove(source.c_str(), target.c_str());
        if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
        {
            // Bump the slice asset up in the asset processor's queue.
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, target.c_str());

            AZ::SystemTickBus::QueueFunction([this, asset]() { GraphUpgradeComplete(asset); });
        }
        else
        {
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
            streamer->SetRequestCompleteCallback(flushRequest, [this, &asset, &source, &target]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    // Continue saving.
                    AZ::SystemTickBus::QueueFunction([this, asset, source, target]() { RetryMove(asset, source, target); });
                });
            streamer->QueueRequest(flushRequest);

        }
    }

    void VersionExplorer::GraphUpgradeComplete(const AZ::Data::Asset<AZ::Data::AssetData>& asset, OperationResult result /*= OperationResult::Success*/)
    {
        m_inProgress = false;

        if (m_scriptCanvasEntity)
        {
            m_scriptCanvasEntity->Deactivate();
            m_scriptCanvasEntity = nullptr;
        }

        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

        GraphUpgradeCompleteUIUpdate(asset, result);

        if (!m_isUpgradingSingleGraph)
        {
            if (m_inProgressAsset != m_assetsToUpgrade.end())
            {
                m_inProgressAsset = m_assetsToUpgrade.erase(m_inProgressAsset);
            }

            if (m_inProgressAsset == m_assetsToUpgrade.end())
            {
                FinalizeUpgrade();
            }
        }
        else
        {
            m_inProgressAsset = m_assetsToUpgrade.erase(m_inProgressAsset);

            m_inProgress = false;
            m_state = ProcessState::Inactive;
            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        m_isUpgradingSingleGraph = false;

        if (m_assetsToUpgrade.empty())
        {
            m_ui->upgradeAllButton->setEnabled(false);
        }
    }

    void VersionExplorer::GraphUpgradeCompleteUIUpdate(const AZ::Data::Asset<AZ::Data::AssetData>& asset, OperationResult result /*= OperationResult::Success*/)
    {
        QString text = asset.GetHint().c_str();
        QList<QTableWidgetItem*> items = m_ui->tableWidget->findItems(text, Qt::MatchFlag::MatchExactly);
        if (!items.isEmpty())
        {
            for (auto* item : items)
            {
                int row = item->row();

                QTableWidgetItem* label = m_ui->tableWidget->item(row, ColumnAsset);

                QString assetName = asset.GetHint().c_str();
                if (label->text().compare(assetName) == 0)
                {
                    m_ui->tableWidget->removeCellWidget(row, ColumnAction);
                    m_ui->tableWidget->removeCellWidget(row, ColumnStatus);

                    QToolButton* doneButton = new QToolButton(this);
                    doneButton->setToolTip("Upgrade complete");
                    if (result == OperationResult::Success)
                    {
                        doneButton->setIcon(QIcon(":/stylesheet/img/UI20/checkmark-menu.svg"));
                    }
                    else
                    {
                        doneButton->setIcon(QIcon(":/stylesheet/img/UI20/titlebar-close.svg"));
                        if (result == OperationResult::BackupFail_FileNotFound)
                        {
                            doneButton->setToolTip("The file no longer exists");
                        }
                        else if (result == OperationResult::BackupFail_CreateFolder)
                        {
                            doneButton->setToolTip("Failed to create the backup folder");
                        }

                    }

                    m_ui->tableWidget->setCellWidget(row, ColumnStatus, doneButton);
                }


            }
        }
    }

    void VersionExplorer::FinalizeUpgrade()
    {
        m_inProgress = false;
        m_assetsToUpgrade.clear();

        m_ui->upgradeAllButton->setEnabled(false);
        m_ui->onlyShowOutdated->setEnabled(true);


        // Manual correction
        size_t assetsThatNeedManualInspection = AZ::Interface<IUpgradeRequests>::Get()->GetGraphsThatNeedManualUpgrade().size();
        if (assetsThatNeedManualInspection > 0)
        {
            m_ui->spinner->SetText("<html><head/><body><img src=':/stylesheet/img/UI20/Info.svg' width='16' height='16'/>Some graphs will require manual corrections, you will be prompted to review them upon closing this dialog</body></html>");
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        UpgradeNotifications::Bus::Handler::BusDisconnect();

        AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(false);
    }

    // Scanning

    void VersionExplorer::OnScan()
    {
        m_assetsToUpgrade.clear();
        m_assetsToInspect.clear();

        m_ui->tableWidget->setRowCount(0);
        m_inspectedAssets = 0;

        IUpgradeRequests* upgradeRequests = AZ::Interface<IUpgradeRequests>::Get();
        m_assetsToInspect = upgradeRequests->GetAssetsToUpgrade();

        DoScan();
    }

    void VersionExplorer::DoScan()
    {
        AZ::SystemTickBus::Handler::BusConnect();

        m_state = ProcessState::Scan;

        if (!m_assetsToInspect.empty())
        {
            m_ui->progressFrame->setVisible(true);
            m_ui->progressBar->setRange(0, aznumeric_cast<int>(m_assetsToInspect.size()));
            m_ui->progressBar->setValue(0);

            m_ui->spinner->SetIsBusy(true);
            m_ui->spinner->SetBusyIconSize(32);

            m_ui->scanButton->setEnabled(false);
            m_ui->upgradeAllButton->setEnabled(false);
            m_ui->onlyShowOutdated->setEnabled(false);

            m_inspectingAsset = m_assetsToInspect.begin();
        }
    }

    void VersionExplorer::BackupComplete()
    {
        m_currentAssetIndex = 0;
        m_ui->progressBar->setValue(0);

        DoScan();
    }

    void VersionExplorer::InspectAsset(AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ::Entity* scriptCanvasEntity = nullptr;
        if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
        {
            ScriptCanvasAsset* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
            if (!scriptCanvasAsset)
            {
                return;
            }

            scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
            AZ_Assert(scriptCanvasEntity, "The Script Canvas asset must have a valid entity");
        }

        auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
        AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");


        bool onlyShowOutdatedGraphs = m_ui->onlyShowOutdated->isChecked();

        if (onlyShowOutdatedGraphs && graphComponent->GetVersion().IsLatest())
        {
            ++m_currentAssetIndex;
            ScanComplete(asset);

            return;
        }

        m_ui->tableWidget->insertRow(m_inspectedAssets);
        QTableWidgetItem* rowName = new QTableWidgetItem(tr(asset.GetHint().c_str()));
        m_ui->tableWidget->setItem(m_inspectedAssets, ColumnAsset, rowName);

        if (!graphComponent->GetVersion().IsLatest())
        {
            m_assetsToUpgrade.push_back(asset);

            AzQtComponents::StyledBusyLabel* spinner = new AzQtComponents::StyledBusyLabel(this);
            spinner->SetBusyIconSize(16);

            QPushButton* rowGoToButton = new QPushButton(this);
            rowGoToButton->setText("Upgrade");
            rowGoToButton->setEnabled(false);
            connect(rowGoToButton, &QPushButton::clicked, [this, spinner, rowGoToButton, asset] {

                AZ::SystemTickBus::QueueFunction([this, rowGoToButton, spinner, asset]() {
                    // Queue the process state change because we can't connect to the SystemTick bus in a Qt lambda
                    UpgradeSingle(rowGoToButton, spinner, asset);
                    });

                AZ::SystemTickBus::ExecuteQueuedEvents();

                });
            m_ui->tableWidget->setCellWidget(m_inspectedAssets, ColumnAction, rowGoToButton);

            m_ui->tableWidget->setCellWidget(m_inspectedAssets, ColumnStatus, spinner);
        }

        QToolButton* browseButton = new QToolButton(this);
        browseButton->setToolTip(AzQtComponents::fileBrowserActionName());
        browseButton->setIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));

        char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
        AZStd::string path = AZStd::string::format("@devroot@/%s", asset.GetHint().c_str());
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(path.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);
        AZ::StringFunc::Path::GetFullPath(resolvedBuffer, path);
        AZ::StringFunc::Path::Normalize(path);

        bool result = false;
        AZ::Data::AssetInfo info;
        AZStd::string watchFolder;
        QByteArray assetNameUtf8 = asset.GetHint().c_str();
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, assetNameUtf8, info, watchFolder);
        if (!result)
        {
            AZ_Error("AssetProvider", false, "Failed to locate asset info for '%s'.", assetNameUtf8.constData());
        }

        QString absolutePath = QDir(watchFolder.c_str()).absoluteFilePath(info.m_relativePath.c_str());

        connect(browseButton, &QPushButton::clicked, [absolutePath] {
            AzQtComponents::ShowFileOnDesktop(absolutePath);
            });
        m_ui->tableWidget->setCellWidget(m_inspectedAssets, ColumnBrowse, browseButton);

        ++m_inspectedAssets;
        ++m_currentAssetIndex;

        ScanComplete(asset);
    }

    void VersionExplorer::UpgradeSingle(QPushButton* rowGoToButton, AzQtComponents::StyledBusyLabel* spinner, const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(true);

        m_isUpgradingSingleGraph = true;

        m_logs.clear();
        m_ui->textEdit->clear();

        spinner->SetIsBusy(true);
        rowGoToButton->setEnabled(false);

        m_inProgressAsset = AZStd::find_if(m_assetsToUpgrade.begin(), m_assetsToUpgrade.end(), [this, asset](const UpgradeAssets::value_type& assetToUpgrade)
            {
                return assetToUpgrade.GetId() == asset.GetId();
            });

        m_state = ProcessState::Upgrade;

        AZ::SystemTickBus::Handler::BusConnect();

    }

    void VersionExplorer::ScanComplete(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        m_inProgress = false;
        m_ui->progressBar->setValue(aznumeric_cast<int>(m_currentAssetIndex));
        m_ui->scanButton->setEnabled(true);

        m_inspectingAsset = m_assetsToInspect.erase(m_inspectingAsset);

        FlushLogs();

        if (m_inspectingAsset == m_assetsToInspect.end())
        {
            AZ::SystemTickBus::QueueFunction([this]() { FinalizeScan(); });

            if (!m_assetsToUpgrade.empty())
            {
                m_ui->upgradeAllButton->setEnabled(true);
            }
        }

        asset->Release();
    }

    void VersionExplorer::FinalizeScan()
    {
        m_ui->spinner->SetIsBusy(false);
        m_ui->onlyShowOutdated->setEnabled(true);

        // Enable all the Upgrade buttons
        for (int row = 0; row < m_ui->tableWidget->rowCount(); ++row)
        {
            QPushButton* button = qobject_cast<QPushButton*>(m_ui->tableWidget->cellWidget(row, ColumnAction));
            if (button)
            {
                button->setEnabled(true);
            }
        }


        QString spinnerText = QStringLiteral("Scan Complete");
        if (m_assetsToUpgrade.empty())
        {
            spinnerText.append(" - No graphs require upgrade!");
        }
        m_ui->spinner->SetText(spinnerText);
        m_ui->progressBar->setVisible(false);

        if (!m_assetsToUpgrade.empty())
        {
            m_ui->upgradeAllButton->setEnabled(true);
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        UpgradeNotifications::Bus::Handler::BusDisconnect();

        m_keepEditorAlive.reset();

    }

    // 

    void VersionExplorer::FlushLogs()
    {
        if (m_logs.empty())
        {
            return;
        }

        const QTextCursor oldCursor = m_ui->textEdit->textCursor();
        QScrollBar* scrollBar = m_ui->textEdit->verticalScrollBar();
        const int oldScrollValue = scrollBar->value();

        m_ui->textEdit->moveCursor(QTextCursor::End);
        QTextCursor textCursor = m_ui->textEdit->textCursor();

        while (!m_logs.empty())
        {
            auto line = "\n" + m_logs.front();

            m_logs.pop_front();

            textCursor.insertText(line.c_str());
        }

        scrollBar->setValue(scrollBar->maximum());
        m_ui->textEdit->moveCursor(QTextCursor::StartOfLine);

    }

    void VersionExplorer::CaptureLogFromTraceBus(const char* /*window*/, const char* message)
    {
        AZStd::string msg = message;
        if (msg.ends_with("\n"))
        {
            msg = msg.substr(0, msg.size() - 1);
        }

        m_logs.push_back(msg);
    }

    bool VersionExplorer::OnPreError(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        AZStd::string msg = AZStd::string::format("(Error): %s", message);
        CaptureLogFromTraceBus(window, msg.c_str());

        return false;
    }

    bool VersionExplorer::OnPreWarning(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        AZStd::string msg = AZStd::string::format("(Warning): %s", message);
        CaptureLogFromTraceBus(window, msg.c_str());

        return false;
    }

    bool VersionExplorer::OnException(const char* message)
    {
        AZStd::string msg = AZStd::string::format("(Exception): %s", message);
        CaptureLogFromTraceBus("Script Canvas", msg.c_str());

        return false;
    }

    bool VersionExplorer::OnPrintf(const char* window, const char* message)
    {
        CaptureLogFromTraceBus(window, message);
        return false;
    }

    void VersionExplorer::RetryMove(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target)
    {
        auto normTarget = target;
        AzFramework::StringFunc::Path::Normalize(normTarget);
        auto moveResult = AZ::IO::SmartMove(source.c_str(), normTarget.c_str());
        if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
        {
            // Bump the slice asset up in the asset processor's queue.
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, target.c_str());

            AZ::SystemTickBus::QueueFunction([this, asset]() { GraphUpgradeComplete(asset); });
        }
        else
        {
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
            streamer->SetRequestCompleteCallback(flushRequest, [this, &asset, &source, &target]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    // Continue saving.
                    AZ::SystemTickBus::QueueFunction([this, asset, source, target]() { RetryMove(asset, source, target); });
                });
            streamer->QueueRequest(flushRequest);


            //AZ::SystemTickBus::QueueFunction([this, asset, source, target]() { RetryMove(asset, source, target); });
        }
    }

    void VersionExplorer::closeEvent(QCloseEvent* event)
    {
        m_keepEditorAlive.reset();

        AzQtComponents::StyledDialog::closeEvent(event);
    }

#include <Editor/View/Windows/Tools/UpgradeTool/moc_VersionExplorer.cpp>

}
