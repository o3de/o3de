/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QScrollBar>
#include <QToolButton>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Editor/Settings.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Controller.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

#include <Editor/View/Windows/Tools/UpgradeTool/ui_Controller.h>
#include <Editor/View/Windows/Tools/UpgradeTool/moc_Controller.cpp>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        EditorKeepAlive::EditorKeepAlive()
        {
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);

            m_edKeepEditorActive = system->GetIConsole()->GetCVar("ed_KeepEditorActive");

            if (m_edKeepEditorActive)
            {
                m_keepEditorActive = m_edKeepEditorActive->GetIVal();
                m_edKeepEditorActive->Set(1);
            }
        }

        EditorKeepAlive::~EditorKeepAlive()
        {
            if (m_edKeepEditorActive)
            {
                m_edKeepEditorActive->Set(m_keepEditorActive);
            }
        }

        Controller::Controller(QWidget* parent)
            : AzQtComponents::StyledDialog(parent)
            , m_view(new Ui::Controller())
        {
            m_view->setupUi(this);
            m_view->tableWidget->horizontalHeader()->setVisible(false);
            m_view->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            m_view->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
            m_view->tableWidget->setColumnWidth(3, 22);
            m_view->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
            m_view->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
            connect(m_view->scanButton, &QPushButton::pressed, this, &Controller::OnScanButtonPress);
            connect(m_view->closeButton, &QPushButton::pressed, this, &Controller::OnCloseButtonPress);
            connect(m_view->upgradeAllButton, &QPushButton::pressed, this, &Controller::OnUpgradeAllButtonPress);
            m_view->progressBar->setValue(0);
            m_view->progressBar->setVisible(false);

            ModelNotificationsBus::Handler::BusConnect();

            // move to model, maybe
            m_keepEditorAlive = AZStd::make_unique<EditorKeepAlive>();
        }

        void Controller::Log(const char* /*format*/, ...)
        {
//             if (m_view->verbose->isChecked())
//             {
//                 char sBuffer[2048];
//                 va_list ArgList;
//                 va_start(ArgList, format);
//                 azvsnprintf(sBuffer, sizeof(sBuffer), format, ArgList);
//                 sBuffer[sizeof(sBuffer) - 1] = '\0';
//                 va_end(ArgList);
// 
//                 AZ_TracePrintf(ScriptCanvas::k_VersionExplorerWindow.data(), "%s\n", sBuffer);
//             }
        }

        void Controller::OnCloseButtonPress()
        {
            reject();
        }

        void Controller::OnSystemTick()
        {
//             switch (m_state)
//             {
//             case ProcessState::Scan:
// 
//                 if (!m_inProgress && m_inspectingAsset != m_assetsToInspect.end())
//                 {
//                     m_inProgress = true;
//                     AZ::Data::AssetInfo& assetToUpgrade = *m_inspectingAsset;
//                     m_currentAsset = AZ::Data::AssetManager::Instance().GetAsset(assetToUpgrade.m_assetId, assetToUpgrade.m_assetType, AZ::Data::AssetLoadBehavior::PreLoad);
//                     Log("SystemTick::ProcessState::Scan: %s pre-blocking load hint", m_currentAsset.GetHint().c_str());
//                     m_currentAsset.BlockUntilLoadComplete();
//                     if (m_currentAsset.IsReady())
//                     {
//                         // The asset is ready, grab its info
//                         m_inProgress = true;
//                         InspectAsset(m_currentAsset, assetToUpgrade);
//                     }
//                     else
//                     {
//                         m_view->tableWidget->insertRow(static_cast<int>(m_currentAssetRowIndex));
//                         QTableWidgetItem* rowName = new QTableWidgetItem
//                         (tr(AZStd::string::format("Error: %s", assetToUpgrade.m_relativePath.c_str()).c_str()));
//                         m_view->tableWidget->setItem(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAsset), rowName);
//                         ++m_currentAssetRowIndex;
// 
//                         Log("SystemTick::ProcessState::Scan: %s post-blocking load, problem loading asset", assetToUpgrade.m_relativePath.c_str());
//                         ++m_failedAssets;
//                         ScanComplete(m_currentAsset);
//                     }
//                 }
//                 break;
// 
//             case ProcessState::Upgrade:
//             {
//                 AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
//                 if (m_upgradeComplete)
//                 {
//                     ++m_upgradeAssetIndex;
//                     m_inProgress = false;
//                     m_view->progressBar->setVisible(true);
//                     m_view->progressBar->setValue(m_upgradeAssetIndex);
// 
//                     if (m_scriptCanvasEntity)
//                     {
//                         m_scriptCanvasEntity->Deactivate();
//                         m_scriptCanvasEntity = nullptr;
//                     }
// 
//                     GraphUpgradeCompleteUIUpdate(m_upgradeAsset, m_upgradeResult, m_upgradeMessage);
// 
//                     if (!m_isUpgradingSingleGraph)
//                     {
//                         if (m_inProgressAsset != m_assetsToUpgrade.end())
//                         {
//                             m_inProgressAsset = m_assetsToUpgrade.erase(m_inProgressAsset);
//                         }
// 
//                         if (m_inProgressAsset == m_assetsToUpgrade.end())
//                         {
//                             FinalizeUpgrade();
//                         }
//                     }
//                     else
//                     {
//                         m_inProgressAsset = m_assetsToUpgrade.erase(m_inProgressAsset);
//                         m_inProgress = false;
//                         m_state = ProcessState::Inactive;
//                         // m_settingsCache.reset();
//                         AZ::SystemTickBus::Handler::BusDisconnect();
// 
//                     }
// 
//                     m_isUpgradingSingleGraph = false;
// 
//                     if (m_assetsToUpgrade.empty())
//                     {
//                         m_view->upgradeAllButton->setEnabled(false);
//                     }
// 
//                     m_upgradeComplete = false;
//                 }
// 
//                 if (!IsUpgrading() && m_state == ProcessState::Upgrade)
//                 {
//                     AZStd::string errorMessage = BackupGraph(*m_inProgressAsset);
//                     // Make the backup
//                     if (errorMessage.empty())
//                     {
//                         Log("SystemTick::ProcessState::Upgrade: Backup Success %s ", m_inProgressAsset->GetHint().c_str());
//                         QList<QTableWidgetItem*> items = m_view->tableWidget->findItems(m_inProgressAsset->GetHint().c_str(), Qt::MatchFlag::MatchExactly);
//                         if (!items.isEmpty())
//                         {
//                             for (auto* item : items)
//                             {
//                                 int row = item->row();
//                                 AzQtComponents::StyledBusyLabel* spinner = qobject_cast<AzQtComponents::StyledBusyLabel*>(m_view->tableWidget->cellWidget(row, ColumnStatus));
//                                 spinner->SetIsBusy(true);
//                             }
//                         }
// 
//                         // Upgrade the graph
//                         UpgradeGraph(*m_inProgressAsset);
//                     }
//                     else
//                     {
//                         Log("SystemTick::ProcessState::Upgrade: Backup Failed %s ", m_inProgressAsset->GetHint().c_str());
//                         GraphUpgradeComplete(*m_inProgressAsset, OperationResult::Failure, errorMessage);
//                     }
// 
//                 }
//                 break;
//             }
//             default:
//                 break;
//             }
// 
//             FlushLogs();
// 
//             AZ::Data::AssetManager::Instance().DispatchEvents();
//             AZ::SystemTickBus::ExecuteQueuedEvents();
        }

        // Backup

        void Controller::OnUpgradeAllButtonPress()
        {
            m_state = ProcessState::Upgrade;
            //m_settingsCache = AZStd::make_unique<ScriptCanvas::Grammar::SettingsCache>();
            ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
            ScriptCanvas::Grammar::g_printAbstractCodeModel = false;
            ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
            AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(true);
            AZ::Interface<IUpgradeRequests>::Get()->ClearGraphsThatNeedUpgrade();
            m_inProgressAsset = m_assetsToUpgrade.begin();
            AZ::SystemTickBus::Handler::BusConnect();
            m_view->progressBar->setVisible(true);
            m_view->progressBar->setRange(0, aznumeric_cast<int>(m_assetsToUpgrade.size()));
            m_view->progressBar->setValue(m_upgradeAssetIndex);
            m_keepEditorAlive = AZStd::make_unique<EditorKeepAlive>();
        }

        AZStd::string Controller::BackupGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            if (!m_view->makeBackupCheckbox->isChecked())
            {
                // considered a success
                return "";
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
                    AZ_Error(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Failed to create backup folder %s", backupPath.c_str());
                    return "Failed to create backup folder";
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
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Controller::BackupGraph: Failed to find file: %s", asset.GetHint().c_str());
                return "Failed to find source file";
            }

            devRoot = devRootCStr;
            AzFramework::StringFunc::Path::Normalize(devRoot);

            relativePath = sourceFilePath;
            AzFramework::StringFunc::Replace(relativePath, devRoot.c_str(), "");
            if (relativePath.starts_with("/"))
            {
                relativePath = relativePath.substr(1, relativePath.size() - 1);
            }

            AzFramework::StringFunc::Path::Normalize(relativePath);
            AzFramework::StringFunc::Path::Normalize(backupPath);

            AZStd::string targetFilePath = backupPath;
            targetFilePath += relativePath;

            if (AZ::IO::FileIOBase::GetInstance()->Copy(sourceFilePath.c_str(), targetFilePath.c_str()) != AZ::IO::ResultCode::Success)
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Controller::BackupGraph: Error creating backup: %s  ---> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
                return "Failed to copy source file to backup location";
            }

            Log("Controller::BackupGraph: Backed up: %s  ---> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
            return "";
        }

        void Controller::UpgradeGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            m_inProgress = true;
            m_upgradeComplete = false;
            Log("UpgradeGraph %s ", m_inProgressAsset->GetHint().c_str());
            m_view->spinner->SetText(QObject::tr("Upgrading: %1").arg(asset.GetHint().c_str()));
            m_scriptCanvasEntity = nullptr;

            UpgradeNotifications::Bus::Handler::BusConnect();

            if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
            {
                ScriptCanvasAsset* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
                AZ_Assert(scriptCanvasAsset, "Unable to get the asset of ScriptCanvasAsset, but received type: %s"
                    , azrtti_typeid<ScriptCanvasAsset>().template ToString<AZStd::string>().c_str());

                if (!scriptCanvasAsset)
                {
                    return;
                }

                AZ::Entity* scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
                AZ_Assert(scriptCanvasEntity, "Controller::UpgradeGraph The Script Canvas asset must have a valid entity");
                if (!scriptCanvasEntity)
                {
                    return;
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

                AZ_Assert(scriptCanvasEntity->GetState() == AZ::Entity::State::Active, "Graph entity is not active");
                auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
                AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");

                if (graphComponent)
                {
                    m_scriptCanvasEntity = scriptCanvasEntity;

                    graphComponent->UpgradeGraph
                    (asset
                        , m_view->forceUpgrade->isChecked() ? Graph::UpgradeRequest::Forced : Graph::UpgradeRequest::IfOutOfDate
                        , m_view->verbose->isChecked());
                }
            }

            AZ_Assert(m_scriptCanvasEntity, "The ScriptCanvas asset should have an entity");
        }

        void Controller::OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>& asset, bool /*skipped*/ /*= false*/)
        {
            AZStd::string relativePath, fullPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, asset.GetId());
            bool fullPathFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);
            if (!fullPathFound)
            {
                AZ_Error(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Full source path not found for %s", relativePath.c_str());
            }

            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(fullPath);
            streamer->SetRequestCompleteCallback(flushRequest, [this, asset]([[maybe_unused]] AZ::IO::FileRequestHandle request)
            {
                this->OnSourceFileReleased(asset);
            });
            streamer->QueueRequest(flushRequest);
        }

        void Controller::OnSourceFileReleased(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            AZStd::string relativePath, fullPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, asset.GetId());
            bool fullPathFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);
            m_tmpFileName.clear();
            AZStd::string tmpFileName;
            // here we are saving the graph to a temp file instead of the original file and then copying the temp file to the original file.
            // This ensures that AP will not a get a file change notification on an incomplete graph file causing it to fail processing. Temp files are ignored by AP.
            if (!AZ::IO::CreateTempFileName(fullPath.c_str(), tmpFileName))
            {
                GraphUpgradeComplete(asset, OperationResult::Failure, "Failure to create temporary file name");
                return;
            }

            bool tempSavedSucceeded = false;
            AZ::IO::FileIOStream fileStream(tmpFileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            if (fileStream.IsOpen())
            {
                if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
                {
                    ScriptCanvasEditor::ScriptCanvasAssetHandler handler;
                    tempSavedSucceeded = handler.SaveAssetData(asset, &fileStream);
                }

                fileStream.Close();
            }

            // attempt to remove temporary file no matter what
            m_tmpFileName = tmpFileName;
            if (!tempSavedSucceeded)
            {
                GraphUpgradeComplete(asset, OperationResult::Failure, "Save asset data to temporary file failed");
                return;
            }

            using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
            SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, fullPath.c_str(), true,
                [this, asset, fullPath, tmpFileName]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
            {
                constexpr const size_t k_maxAttemps = 10;

                if (!info.IsReadOnly())
                {
                    PerformMove(asset, tmpFileName, fullPath, k_maxAttemps);
                }
                else
                {
                    if (m_overwriteAll)
                    {
                        AZ::IO::SystemFile::SetWritable(info.m_filePath.c_str(), true);
                        PerformMove(asset, tmpFileName, fullPath, k_maxAttemps);
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
                            PerformMove(asset, tmpFileName, fullPath, k_maxAttemps);
                        }
                    }
                }
            });
        }

        void Controller::PerformMove(AZ::Data::Asset<AZ::Data::AssetData> asset, AZStd::string source, AZStd::string target
            , size_t remainingAttempts)
        {
            // ViewCpp::FileEventHandler fileEventHandler;

            if (remainingAttempts == 0)
            {
                // all attempts failed, give up
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "moving converted file to source destination failed: %s. giving up", target.c_str());
                GraphUpgradeComplete(asset, OperationResult::Failure, "Failed to move updated file from backup to source destination");
            }
            else if (remainingAttempts == 2)
            {
                // before the final attempt, flush all caches
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "moving converted file to source destination failed: %s, trying again", target.c_str());
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                AZ::IO::FileRequestPtr flushRequest = streamer->FlushCaches();
                streamer->SetRequestCompleteCallback(flushRequest
                    , [this, asset, remainingAttempts, source, target]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    // Continue saving.
                    AZ::SystemTickBus::QueueFunction(
                        [this, asset, remainingAttempts, source, target]() { PerformMove(asset, source, target, remainingAttempts - 1); });
                });
                streamer->QueueRequest(flushRequest);
            }
            else
            {
                // the actual move attempt
                auto moveResult = AZ::IO::SmartMove(source.c_str(), target.c_str());
                if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
                {
                    m_tmpFileName.clear();
                    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                    AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
                    // Bump the slice asset up in the asset processor's queue.
                    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, target.c_str());
                    AZ::SystemTickBus::QueueFunction([this, asset]()
                    {
                        GraphUpgradeComplete(asset, OperationResult::Success, "");
                    });
                }
                else
                {
                    AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "moving converted file to source destination failed: %s, trying again", target.c_str());
                    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                    AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
                    streamer->SetRequestCompleteCallback(flushRequest, [this, asset, source, target, remainingAttempts]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                    {
                        // Continue saving.
                        AZ::SystemTickBus::QueueFunction([this, asset, source, target, remainingAttempts]() { PerformMove(asset, source, target, remainingAttempts - 1); });
                    });
                    streamer->QueueRequest(flushRequest);
                }
            }
        }

        void Controller::GraphUpgradeComplete
        (const AZ::Data::Asset<AZ::Data::AssetData> asset, OperationResult result, AZStd::string_view message)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            m_upgradeComplete = true;
            m_upgradeResult = result;
            m_upgradeMessage = message;
            m_upgradeAsset = asset;

            if (!m_tmpFileName.empty())
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                AZ_Assert(fileIO, "GraphUpgradeComplete: No FileIO instance");

                if (fileIO->Exists(m_tmpFileName.c_str()) && !fileIO->Remove(m_tmpFileName.c_str()))
                {
                    AZ_TracePrintf(ScriptCanvas::k_VersionExplorerWindow.data(), "Failed to remove temporary file: %s", m_tmpFileName.c_str());
                }
            }

            if (m_upgradeResult == OperationResult::Failure)
            {
                AZ::Interface<IUpgradeRequests>::Get()->GraphNeedsManualUpgrade(asset.GetId());
            }

            m_tmpFileName.clear();
        }

        void Controller::GraphUpgradeCompleteUIUpdate
        (const AZ::Data::Asset<AZ::Data::AssetData> asset, OperationResult result, AZStd::string_view message)
        {
            QString text = asset.GetHint().c_str();
            QList<QTableWidgetItem*> items = m_view->tableWidget->findItems(text, Qt::MatchFlag::MatchExactly);

            if (!items.isEmpty())
            {
                for (auto* item : items)
                {
                    int row = item->row();
                    QTableWidgetItem* label = m_view->tableWidget->item(row, ColumnAsset);
                    QString assetName = asset.GetHint().c_str();

                    if (label->text().compare(assetName) == 0)
                    {
                        m_view->tableWidget->removeCellWidget(row, ColumnAction);
                        m_view->tableWidget->removeCellWidget(row, ColumnStatus);

                        QToolButton* doneButton = new QToolButton(this);
                        doneButton->setToolTip("Upgrade complete");
                        if (result == OperationResult::Success)
                        {
                            doneButton->setIcon(QIcon(":/stylesheet/img/UI20/checkmark-menu.svg"));
                        }
                        else
                        {
                            doneButton->setIcon(QIcon(":/stylesheet/img/UI20/titlebar-close.svg"));
                            doneButton->setToolTip(message.data());
                        }

                        m_view->tableWidget->setCellWidget(row, ColumnStatus, doneButton);
                    }
                }
            }
        }

        void Controller::FinalizeUpgrade()
        {
            Log("FinalizeUpgrade!");
            m_inProgress = false;
            m_assetsToUpgrade.clear();
            m_view->upgradeAllButton->setEnabled(false);
            m_view->onlyShowOutdated->setEnabled(true);
            m_keepEditorAlive.reset();
            m_view->progressBar->setVisible(false);

            // Manual correction
            size_t assetsThatNeedManualInspection = AZ::Interface<IUpgradeRequests>::Get()->GetGraphsThatNeedManualUpgrade().size();
            if (assetsThatNeedManualInspection > 0)
            {
                m_view->spinner->SetText("<html><head/><body><img src=':/stylesheet/img/UI20/Info.svg' width='16' height='16'/>Some graphs will require manual corrections, you will be prompted to review them upon closing this dialog</body></html>");
            }
            else
            {
                m_view->spinner->SetText("Upgrade complete.");
            }

            AZ::SystemTickBus::Handler::BusDisconnect();
            AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(false);
            //m_settingsCache.reset();
        }

        void Controller::OnScanButtonPress()
        {
            auto isUpToDate = [this](AZ::Data::Asset<AZ::Data::AssetData> asset)
            {
                Log("InspectAsset: %s", asset.GetHint().c_str());
                AZ::Entity* scriptCanvasEntity = nullptr;

                if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
                {
                    ScriptCanvasAsset* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
                    if (!scriptCanvasAsset)
                    {
                        Log("InspectAsset: %s, AsestData failed to return ScriptCanvasAsset", asset.GetHint().c_str());
                        return true;
                    }

                    scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
                    AZ_Assert(scriptCanvasEntity, "The Script Canvas asset must have a valid entity");
                }

                auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
                AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");
                return !m_view->forceUpgrade->isChecked() && graphComponent->GetVersion().IsLatest();
            };

            ScanConfiguration config;
            config.reportFilteredGraphs = !m_view->onlyShowOutdated->isChecked();
            config.filter = isUpToDate;

            ModelRequestsBus::Broadcast(&ModelRequestsTraits::Scan, config);
        }

        void Controller::OnScanBegin(size_t assetCount)
        {
            m_currentAssetRowIndex = 0;
            m_view->tableWidget->setRowCount(0);
            m_view->progressBar->setVisible(true);
            m_view->progressBar->setRange(0, aznumeric_cast<int>(assetCount));
            m_view->progressBar->setValue(0);
            m_view->scanButton->setEnabled(false);
            m_view->upgradeAllButton->setEnabled(false);
            m_view->onlyShowOutdated->setEnabled(false);
        }

        void Controller::OnScanComplete(const ScanResult& result)
        {
            Log("Full Scan Complete.");

            m_view->onlyShowOutdated->setEnabled(true);

            // Enable all the Upgrade buttons
            for (int row = 0; row < m_view->tableWidget->rowCount(); ++row)
            {
                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(true);
                }
            }

            QString spinnerText = QStringLiteral("Scan Complete");
            spinnerText.append(QString::asprintf(" - Discovered: %zu, Failed: %zu, Upgradeable: %zu, Up-to-date: %zu"
                , result.m_catalogAssets.size()
                , result.m_loadErrors.size()
                , result.m_unfiltered.size()
                , result.m_filteredAssets.size()));

            m_view->spinner->SetText(spinnerText);
            m_view->progressBar->setVisible(false);

            if (!result.m_unfiltered.empty())
            {
                m_view->upgradeAllButton->setEnabled(true);
            }

            m_keepEditorAlive.reset();
        }

        void Controller::OnScanFilteredGraph(const AZ::Data::AssetInfo& info)
        {
            OnScannedGraph(info, Filtered::Yes);
        }

        void Controller::OnScannedGraph(const AZ::Data::AssetInfo& assetInfo, Filtered filtered)
        {
            m_view->tableWidget->insertRow(static_cast<int>(m_currentAssetRowIndex));
            QTableWidgetItem* rowName = new QTableWidgetItem(tr(assetInfo.m_relativePath.c_str()));
            m_view->tableWidget->setItem(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAsset), rowName);

            if (filtered == Filtered::No)
            {
                QPushButton* rowGoToButton = new QPushButton(this);
                rowGoToButton->setText("Upgrade");
                rowGoToButton->setEnabled(false);
                AzQtComponents::StyledBusyLabel* spinner = new AzQtComponents::StyledBusyLabel(this);
                spinner->SetBusyIconSize(16);
// 
//                 connect(rowGoToButton, &QPushButton::clicked, [this, rowGoToButton, assetInfo] {
// 
// //                     request upgrade of a single graph
// //                     AZ::SystemTickBus::QueueFunction([this, rowGoToButton, assetInfo]() {
// //                         // Queue the process state change because we can't connect to the SystemTick bus in a Qt lambda
// //                         UpgradeSingle(rowGoToButton, spinner, assetInfo);
// //                     });
// // 
// //                     AZ::SystemTickBus::ExecuteQueuedEvents();
// 
//                 });

                m_view->tableWidget->setCellWidget(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAction), rowGoToButton);
                m_view->tableWidget->setCellWidget(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnStatus), spinner);
            }

            char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
            AZStd::string path = AZStd::string::format("@devroot@/%s", assetInfo.m_relativePath.c_str());
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(path.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);
            AZ::StringFunc::Path::GetFullPath(resolvedBuffer, path);
            AZ::StringFunc::Path::Normalize(path);

            bool result = false;
            AZ::Data::AssetInfo info;
            AZStd::string watchFolder;
            QByteArray assetNameUtf8 = assetInfo.m_relativePath.c_str();
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult
                ( result
                , &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath
                , assetNameUtf8
                , info
                , watchFolder);
            AZ_Error
                ( ScriptCanvas::k_VersionExplorerWindow.data()
                , result
                , "Failed to locate asset info for '%s'.", assetNameUtf8.constData());

            QToolButton* browseButton = new QToolButton(this);
            browseButton->setToolTip(AzQtComponents::fileBrowserActionName());
            browseButton->setIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));

            QString absolutePath = QDir(watchFolder.c_str()).absoluteFilePath(info.m_relativePath.c_str());
            connect(browseButton, &QPushButton::clicked, [absolutePath] {
                AzQtComponents::ShowFileOnDesktop(absolutePath);
            });

            m_view->tableWidget->setCellWidget(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnBrowse), browseButton);
            OnScannedGraphResult(assetInfo);
        }

        void Controller::OnScannedGraphResult(const AZ::Data::AssetInfo& info)
        {
            m_view->progressBar->setValue(aznumeric_cast<int>(m_currentAssetRowIndex));
            ++m_currentAssetRowIndex;
            Log("ScanComplete: %s", info.m_relativePath.c_str());
            FlushLogs();
        }

        void Controller::OnScanLoadFailure(const AZ::Data::AssetInfo& info)
        {
            m_view->tableWidget->insertRow(static_cast<int>(m_currentAssetRowIndex));
            QTableWidgetItem* rowName = new QTableWidgetItem
                (tr(AZStd::string::format("Load Error: %s", info.m_relativePath.c_str()).c_str()));
            m_view->tableWidget->setItem(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAsset), rowName);
            OnScannedGraphResult(info);
        }

        void Controller::OnScanUnFilteredGraph(const AZ::Data::AssetInfo& info)
        {
            OnScannedGraph(info, Filtered::No);
        }

        void Controller::BackupComplete()
        {
            m_currentAssetRowIndex = 0;
            m_view->progressBar->setValue(0);
        }

        void Controller::UpgradeSingle
            ( QPushButton* rowGoToButton
            , AzQtComponents::StyledBusyLabel* spinner
            , AZ::Data::AssetInfo assetInfo)
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset
            (assetInfo.m_assetId, assetInfo.m_assetType, AZ::Data::AssetLoadBehavior::PreLoad);

            if (asset)
            {
                asset.BlockUntilLoadComplete();

                if (asset.IsReady())
                {
                    AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(true);
                    m_isUpgradingSingleGraph = true;
                    m_logs.clear();
                    m_view->textEdit->clear();
                    spinner->SetIsBusy(true);
                    rowGoToButton->setEnabled(false);

                    m_inProgressAsset = AZStd::find_if(m_assetsToUpgrade.begin(), m_assetsToUpgrade.end()
                        , [asset](const UpgradeAssets::value_type& assetToUpgrade)
                    {
                        return assetToUpgrade.GetId() == asset.GetId();
                    });

                    m_state = ProcessState::Upgrade;
                    AZ::SystemTickBus::Handler::BusConnect();
                }
            }
        }

        void Controller::ScanComplete(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            Log("ScanComplete: %s", asset.GetHint().c_str());
            m_inProgress = false;
            m_view->progressBar->setValue(aznumeric_cast<int>(m_currentAssetRowIndex));
            m_view->scanButton->setEnabled(true);

            m_inspectingAsset = m_assetsToInspect.erase(m_inspectingAsset);
            
            if (m_inspectingAsset == m_assetsToInspect.end())
            {
                AZ::SystemTickBus::QueueFunction([this]() { FinalizeScan(); });

                if (!m_assetsToUpgrade.empty())
                {
                    m_view->upgradeAllButton->setEnabled(true);
                }
            }
        }

        void Controller::FinalizeScan()
        {
            Log("FinalizeScan()");

            m_view->spinner->SetIsBusy(false);
            m_view->onlyShowOutdated->setEnabled(true);

            // Enable all the Upgrade buttons
            for (int row = 0; row < m_view->tableWidget->rowCount(); ++row)
            {
                QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction));
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
            else
            {
                spinnerText.append(QString::asprintf(" - Discovered: %zu, Inspected: %zu, Failed: %zu, Upgradeable: %zu"
                    , m_discoveredAssets, m_inspectedAssets, m_failedAssets, m_assetsToUpgrade.size()));
            }


            m_view->spinner->SetText(spinnerText);
            m_view->progressBar->setVisible(false);

            if (!m_assetsToUpgrade.empty())
            {
                m_view->upgradeAllButton->setEnabled(true);
            }

            AZ::SystemTickBus::Handler::BusDisconnect();

            m_keepEditorAlive.reset();
            //m_settingsCache.reset();
            m_state = ProcessState::Inactive;
        }

        void Controller::FlushLogs()
        {
            if (m_logs.empty())
            {
                return;
            }

            const QTextCursor oldCursor = m_view->textEdit->textCursor();
            QScrollBar* scrollBar = m_view->textEdit->verticalScrollBar();

            m_view->textEdit->moveCursor(QTextCursor::End);
            QTextCursor textCursor = m_view->textEdit->textCursor();

            while (!m_logs.empty())
            {
                auto line = "\n" + m_logs.front();

                m_logs.pop_front();

                textCursor.insertText(line.c_str());
            }

            scrollBar->setValue(scrollBar->maximum());
            m_view->textEdit->moveCursor(QTextCursor::StartOfLine);

        }

        void Controller::closeEvent(QCloseEvent* event)
        {
            m_keepEditorAlive.reset();

            AzQtComponents::StyledDialog::closeEvent(event);
        }
    }
}
