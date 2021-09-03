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
#include <Editor/View/Windows/Tools/UpgradeTool/VersionExplorer.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ui_VersionExplorer.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

namespace VersionExplorerCpp
{
    class FileEventHandler
        : public AZ::IO::FileIOEventBus::Handler
    {
    public:
        int m_errorCode = 0;
        AZStd::string m_fileName;

        FileEventHandler()
        {
            BusConnect();
        }

        ~FileEventHandler()
        {
            BusDisconnect();
        }

        void OnError(const AZ::IO::SystemFile* /*file*/, const char* fileName, int errorCode) override
        {
            m_errorCode = errorCode;

            if (fileName)
            {
                m_fileName = fileName;
            }
        }
    };
}

namespace ScriptCanvasEditor
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

    void VersionExplorer::Log(const char* format, ...)
    {
        if (m_ui->verbose->isChecked())
        {
            char sBuffer[2048];
            va_list ArgList;
            va_start(ArgList, format);
            azvsnprintf(sBuffer, sizeof(sBuffer), format, ArgList);
            sBuffer[sizeof(sBuffer) - 1] = '\0';
            va_end(ArgList);

            AZ_TracePrintf(ScriptCanvas::k_VersionExplorerWindow.data(), "%s\n", sBuffer);
        }
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
                Log("SystemTick::ProcessState::Scan: %s pre-blocking load hint", m_currentAsset.GetHint().c_str());
                m_currentAsset.BlockUntilLoadComplete();
                if (m_currentAsset.IsReady())
                {
                    // The asset is ready, grab its info
                    m_inProgress = true;
                    InspectAsset(m_currentAsset, assetToUpgrade);
                }
                else
                {
                    m_ui->tableWidget->insertRow(static_cast<int>(m_currentAssetRowIndex));
                    QTableWidgetItem* rowName = new QTableWidgetItem
                        ( tr(AZStd::string::format("Error: %s", assetToUpgrade.m_relativePath.c_str()).c_str()));
                    m_ui->tableWidget->setItem(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAsset), rowName);
                    ++m_currentAssetRowIndex;

                    Log("SystemTick::ProcessState::Scan: %s post-blocking load, problem loading asset", assetToUpgrade.m_relativePath.c_str());
                    ++m_failedAssets;
                    ScanComplete(m_currentAsset);
                }
            }
            break;

        case ProcessState::Upgrade:
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
            if (m_upgradeComplete)
            {
                ++m_upgradeAssetIndex;
                m_inProgress = false;
                m_ui->progressBar->setVisible(true);
                m_ui->progressBar->setValue(m_upgradeAssetIndex);

                if (m_scriptCanvasEntity)
                {
                    m_scriptCanvasEntity->Deactivate();
                    m_scriptCanvasEntity = nullptr;
                }

                GraphUpgradeCompleteUIUpdate(m_upgradeAsset, m_upgradeResult, m_upgradeMessage);

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
                    AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
                }

                m_isUpgradingSingleGraph = false;

                if (m_assetsToUpgrade.empty())
                {
                    m_ui->upgradeAllButton->setEnabled(false);
                }

                m_upgradeComplete = false;
            }

            if (!IsUpgrading() && m_state == ProcessState::Upgrade)
            {
                AZStd::string errorMessage = BackupGraph(*m_inProgressAsset);
                // Make the backup
                if (errorMessage.empty())
                {
                    Log("SystemTick::ProcessState::Upgrade: Backup Success %s ", m_inProgressAsset->GetHint().c_str());
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
                    Log("SystemTick::ProcessState::Upgrade: Backup Failed %s ", m_inProgressAsset->GetHint().c_str());
                    GraphUpgradeComplete(*m_inProgressAsset, OperationResult::Failure, errorMessage);
                }

            }
            break;
        }
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
        m_state = ProcessState::Upgrade;
        // cache these...with a widget thing
        ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
        ScriptCanvas::Grammar::g_printAbstractCodeModel = false;
        ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
        AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(true);
        AZ::Interface<IUpgradeRequests>::Get()->ClearGraphsThatNeedUpgrade();
        m_inProgressAsset = m_assetsToUpgrade.begin();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
        m_ui->progressBar->setVisible(true);
        m_ui->progressBar->setRange(0, aznumeric_cast<int>(m_assetsToUpgrade.size()));
        m_ui->progressBar->setValue(m_upgradeAssetIndex);
    }

    AZStd::string VersionExplorer::BackupGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        bool makeBackup = m_ui->makeBackupCheckbox->isChecked();
        if (!makeBackup)
        {
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
            AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "VersionExplorer::BackupGraph: Failed to find file: %s", asset.GetHint().c_str());
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

        AZStd::string targetFilePath;
        AzFramework::StringFunc::Path::Join(backupPath.c_str(), relativePath.c_str(), targetFilePath);

        if (AZ::IO::FileIOBase::GetInstance()->Copy(sourceFilePath.c_str(), targetFilePath.c_str()) != AZ::IO::ResultCode::Error)
        {
            Log("VersionExplorer::BackupGraph: Backed up: %s  ---> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
            return "";
        }
        else
        {
            AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "VersionExplorer::BackupGraph: Error creating backup: %s  ---> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
            return "Failed to copy source file to backup location";
        }
    }

    void VersionExplorer::UpgradeGraph(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        m_inProgress = true;
        m_upgradeComplete = false;
        Log("UpgradeGraph %s ", m_inProgressAsset->GetHint().c_str());
        m_ui->spinner->SetText(QObject::tr("Upgrading: %1").arg(asset.GetHint().c_str()));
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
            AZ_Assert(scriptCanvasEntity, "VersionExplorer::UpgradeGraph The Script Canvas asset must have a valid entity");
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
                    ( asset
                    , m_ui->forceUpgrade->isChecked() ? Graph::UpgradeRequest::Forced : Graph::UpgradeRequest::IfOutOfDate
                    , m_ui->verbose->isChecked());
            }
        }

        AZ_Assert(m_scriptCanvasEntity, "The ScriptCanvas asset should have an entity");
    }

    void VersionExplorer::OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>& asset, bool /*skipped*/ /*= false*/)
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

    void VersionExplorer::OnSourceFileReleased(AZ::Data::Asset<AZ::Data::AssetData> asset)
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

    void VersionExplorer::PerformMove(AZ::Data::Asset<AZ::Data::AssetData> asset, AZStd::string source, AZStd::string target
        , size_t remainingAttempts)
    {
        VersionExplorerCpp::FileEventHandler fileEventHandler;

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
                    [this, asset, remainingAttempts, source, target](){ PerformMove(asset, source, target, remainingAttempts - 1); });
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

    void VersionExplorer::GraphUpgradeComplete
        ( const AZ::Data::Asset<AZ::Data::AssetData> asset, OperationResult result, AZStd::string_view message)
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

    void VersionExplorer::GraphUpgradeCompleteUIUpdate
        ( const AZ::Data::Asset<AZ::Data::AssetData> asset, OperationResult result, AZStd::string_view message)
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
                        doneButton->setToolTip(message.data());
                    }

                    m_ui->tableWidget->setCellWidget(row, ColumnStatus, doneButton);
                }
            }
        }
    }

    void VersionExplorer::FinalizeUpgrade()
    {
        Log("FinalizeUpgrade!");
        m_inProgress = false;
        m_assetsToUpgrade.clear();
        m_ui->upgradeAllButton->setEnabled(false);
        m_ui->onlyShowOutdated->setEnabled(true);

        m_ui->progressBar->setVisible(false);
        // Manual correction
        size_t assetsThatNeedManualInspection = AZ::Interface<IUpgradeRequests>::Get()->GetGraphsThatNeedManualUpgrade().size();
        if (assetsThatNeedManualInspection > 0)
        {
            m_ui->spinner->SetText("<html><head/><body><img src=':/stylesheet/img/UI20/Info.svg' width='16' height='16'/>Some graphs will require manual corrections, you will be prompted to review them upon closing this dialog</body></html>");
        }
        else
        {
            m_ui->spinner->SetText("Upgrade complete.");
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
        m_currentAssetRowIndex = 0;
        IUpgradeRequests* upgradeRequests = AZ::Interface<IUpgradeRequests>::Get();
        m_assetsToInspect = upgradeRequests->GetAssetsToUpgrade();
        DoScan();
    }

    void VersionExplorer::DoScan()
    {
        m_state = ProcessState::Scan;
        // cache pre-tool values (make a little widget that does that, actually
        // so one can destroy it and reset it
        ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;
        ScriptCanvas::Grammar::g_printAbstractCodeModel = false;
        ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;

        AZ::SystemTickBus::Handler::BusConnect();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        if (!m_assetsToInspect.empty())
        {
            m_discoveredAssets = m_assetsToInspect.size();
            m_failedAssets = 0;
            m_inspectedAssets = 0;
            m_currentAssetRowIndex = 0;
            m_ui->progressFrame->setVisible(true);
            m_ui->progressBar->setVisible(true);
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
        m_currentAssetRowIndex = 0;
        m_ui->progressBar->setValue(0);
        DoScan();
    }

    void VersionExplorer::InspectAsset(AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::Data::AssetInfo& assetInfo)
    {
        Log("InspectAsset: %s", asset.GetHint().c_str());
        AZ::Entity* scriptCanvasEntity = nullptr;
        if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
        {
            ScriptCanvasAsset* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
            if (!scriptCanvasAsset)
            {
                Log("InspectAsset: %s, AsestData failed to return ScriptCanvasAsset", asset.GetHint().c_str());
                return;
            }

            scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
            AZ_Assert(scriptCanvasEntity, "The Script Canvas asset must have a valid entity");
        }

        auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
        AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");

        bool onlyShowOutdatedGraphs = m_ui->onlyShowOutdated->isChecked();
        bool forceUpgrade = m_ui->forceUpgrade->isChecked();
        ScriptCanvas::VersionData graphVersion = graphComponent->GetVersion();


        if (!forceUpgrade && onlyShowOutdatedGraphs && graphVersion.IsLatest())
        {
            ScanComplete(asset);
            Log("InspectAsset: %s, is at latest", asset.GetHint().c_str());
            return;
        }

        m_ui->tableWidget->insertRow(static_cast<int>(m_currentAssetRowIndex));
        QTableWidgetItem* rowName = new QTableWidgetItem(tr(asset.GetHint().c_str()));
        m_ui->tableWidget->setItem(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAsset), rowName);

        if (forceUpgrade || !graphComponent->GetVersion().IsLatest())
        {
            m_assetsToUpgrade.push_back(asset);

            AzQtComponents::StyledBusyLabel* spinner = new AzQtComponents::StyledBusyLabel(this);
            spinner->SetBusyIconSize(16);

            QPushButton* rowGoToButton = new QPushButton(this);
            rowGoToButton->setText("Upgrade");
            rowGoToButton->setEnabled(false);

            connect(rowGoToButton, &QPushButton::clicked, [this, spinner, rowGoToButton, assetInfo] {

                AZ::SystemTickBus::QueueFunction([this, rowGoToButton, spinner, assetInfo]() {
                    // Queue the process state change because we can't connect to the SystemTick bus in a Qt lambda
                    UpgradeSingle(rowGoToButton, spinner, assetInfo);
                    });

                AZ::SystemTickBus::ExecuteQueuedEvents();

                });

            m_ui->tableWidget->setCellWidget(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnAction), rowGoToButton);
            m_ui->tableWidget->setCellWidget(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnStatus), spinner);
        }

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

        AZ_Error(ScriptCanvas::k_VersionExplorerWindow.data(), result, "Failed to locate asset info for '%s'.", assetNameUtf8.constData());

        QToolButton* browseButton = new QToolButton(this);
        browseButton->setToolTip(AzQtComponents::fileBrowserActionName());
        browseButton->setIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));

        QString absolutePath = QDir(watchFolder.c_str()).absoluteFilePath(info.m_relativePath.c_str());
        connect(browseButton, &QPushButton::clicked, [absolutePath] {
            AzQtComponents::ShowFileOnDesktop(absolutePath);
            });

        m_ui->tableWidget->setCellWidget(static_cast<int>(m_currentAssetRowIndex), static_cast<int>(ColumnBrowse), browseButton);
        ScanComplete(asset);
        ++m_inspectedAssets;
        ++m_currentAssetRowIndex;
    }

    void VersionExplorer::UpgradeSingle
        ( QPushButton* rowGoToButton
        , AzQtComponents::StyledBusyLabel* spinner
        , AZ::Data::AssetInfo assetInfo)
    {
        AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::AssetManager::Instance().GetAsset
            ( assetInfo.m_assetId, assetInfo.m_assetType, AZ::Data::AssetLoadBehavior::PreLoad);

        if (asset)
        {
            asset.BlockUntilLoadComplete();

            if (asset.IsReady())
            {
                AZ::Interface<IUpgradeRequests>::Get()->SetIsUpgrading(true);
                m_isUpgradingSingleGraph = true;
                m_logs.clear();
                m_ui->textEdit->clear();
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

    void VersionExplorer::ScanComplete(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        Log("ScanComplete: %s", asset.GetHint().c_str());
        m_inProgress = false;
        m_ui->progressBar->setValue(aznumeric_cast<int>(m_currentAssetRowIndex));
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
    }

    void VersionExplorer::FinalizeScan()
    {
        Log("FinalizeScan()");

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
        else
        {
            spinnerText.append(QString::asprintf(" - Discovered: %zu, Inspected: %zu, Failed: %zu, Upgradeable: %zu"
                , m_discoveredAssets, m_inspectedAssets, m_failedAssets, m_assetsToUpgrade.size()));
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
        m_state = ProcessState::Inactive;
    }

    void VersionExplorer::FlushLogs()
    {
        if (m_logs.empty())
        {
            return;
        }

        const QTextCursor oldCursor = m_ui->textEdit->textCursor();
        QScrollBar* scrollBar = m_ui->textEdit->verticalScrollBar();

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

    bool VersionExplorer::CaptureLogFromTraceBus(const char* window, const char* message)
    {
        if (m_ui->updateReportingOnly->isChecked() && window != ScriptCanvas::k_VersionExplorerWindow)
        {
            return true;
        }

        AZStd::string msg = message;
        if (msg.ends_with("\n"))
        {
            msg = msg.substr(0, msg.size() - 1);
        }

        m_logs.push_back(msg);
        return m_ui->updateReportingOnly->isChecked();
    }

    bool VersionExplorer::OnPreError(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        AZStd::string msg = AZStd::string::format("(Error): %s", message);
        return CaptureLogFromTraceBus(window, msg.c_str());
    }

    bool VersionExplorer::OnPreWarning(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        AZStd::string msg = AZStd::string::format("(Warning): %s", message);
        return CaptureLogFromTraceBus(window, msg.c_str());
    }

    bool VersionExplorer::OnException(const char* message)
    {
        AZStd::string msg = AZStd::string::format("(Exception): %s", message);
        return CaptureLogFromTraceBus("Script Canvas", msg.c_str());
    }

    bool VersionExplorer::OnPrintf(const char* window, const char* message)
    {
        return CaptureLogFromTraceBus(window, message);
    }

    void VersionExplorer::closeEvent(QCloseEvent* event)
    {
        m_keepEditorAlive.reset();

        AzQtComponents::StyledDialog::closeEvent(event);
    }

#include <Editor/View/Windows/Tools/UpgradeTool/moc_VersionExplorer.cpp>

}
