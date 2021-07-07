/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <precompiled.h>

#include <QMessageBox>
#include <QDateTime>

#include "UpgradeTool.h"

#include <Asset/Functions/ScriptCanvasFunctionAsset.h>

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
#include <Editor/View/Windows/Tools/UpgradeTool/UpgradeTool.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ui_UpgradeTool.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

#include "UpgradeHelper.h"

namespace ScriptCanvasEditor
{
    UpgradeTool::UpgradeTool(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent)
        , m_ui(new Ui::UpgradeTool())
    {
        m_ui->setupUi(this);

        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_ui->doNotAskCheckbox);
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_ui->makeBackupCheckbox);

        m_ui->progressFrame->setVisible(false);

        connect(m_ui->upgradeButton, &QPushButton::pressed, this, &UpgradeTool::OnUpgrade);
        connect(m_ui->notNowButton, &QPushButton::pressed, this, &UpgradeTool::OnNoThanks);

        UpgradeNotifications::Bus::Handler::BusConnect();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        resize(700, 100);
    }

    void UpgradeTool::OnNoThanks()
    {
        DisconnectBuses();

        UpdateSettings();

        reject();
    }

    void UpgradeTool::UpdateSettings()
    {
        auto userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        if (userSettings)
        {
            userSettings->m_showUpgradeDialog = !m_ui->doNotAskCheckbox->isChecked();

            AZ::UserSettingsOwnerRequestBus::Event(AZ::UserSettings::CT_LOCAL, &AZ::UserSettingsOwnerRequests::SaveSettings);
        }
    }

    UpgradeTool::~UpgradeTool()
    {
        DisconnectBuses();
    }

    void UpgradeTool::DisconnectBuses()
    {
        UpgradeNotifications::Bus::Handler::BusDisconnect();

        AZ::SystemTickBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    void UpgradeTool::closeEvent(QCloseEvent* event)
    {
        m_keepEditorAlive.reset();

        DisconnectBuses();

        UpgradeNotifications::Bus::Broadcast(&UpgradeNotifications::OnUpgradeCancelled);

        AzQtComponents::StyledDialog::closeEvent(event);
    }

    bool UpgradeTool::HasBackup() const
    {
        return m_ui->makeBackupCheckbox->isChecked();
    }

    void UpgradeTool::OnUpgrade()
    {
        setWindowFlag(Qt::WindowCloseButtonHint, false);

        m_keepEditorAlive = AZStd::make_unique<EditorKeepAlive>();

        UpdateSettings();

        UpgradeNotifications::Bus::Broadcast(&UpgradeNotifications::OnUpgradeStart);

        m_assetsToUpgrade.clear();

        IUpgradeRequests* upgradeRequests = AZ::Interface<IUpgradeRequests>::Get();
        m_assetsToUpgrade = upgradeRequests->GetAssetsToUpgrade();

        AZ::SystemTickBus::Handler::BusConnect();

        if (m_ui->makeBackupCheckbox->isChecked())
        {
            if (!DoBackup())
            {
                // There was a problem, ask if the user wants to keep going or abort
                QMessageBox mb(QMessageBox::Warning,
                    QObject::tr("Backup Failed"),
                    QObject::tr("Failed to backup your Script Canvas graphs, do you want to proceed with upgrade?"),
                    QMessageBox::Yes | QMessageBox::No, this);

                if (mb.exec() == QMessageBox::Yes)
                {
                    DoUpgrade();
                }
            }
        }
        else
        {
            DoUpgrade();
        }
    }

    bool UpgradeTool::DoBackup()
    {
        if (!m_assetsToUpgrade.empty())
        {
            m_state = UpgradeState::Backup;

            m_inProgressAsset = m_assetsToUpgrade.begin();

            m_ui->progressFrame->setVisible(true);
            m_ui->progressBar->setRange(0, aznumeric_cast<int>(m_assetsToUpgrade.size()));

            m_ui->spinner->SetIsBusy(true);
            m_ui->spinner->SetBusyIconSize(32);

            m_ui->upgradeButton->setEnabled(false);
            m_ui->notNowButton->setEnabled(false);
            m_ui->doNotAskCheckbox->setEnabled(false);
            m_ui->makeBackupCheckbox->setEnabled(false);

            // Make the folder for the backup

            QDateTime theTime = QDateTime::currentDateTime();
            QString subFolder = theTime.toString("yyyy-MM-dd [HH.mm.ss]");

            m_backupPath = AZStd::string::format("@devroot@/ScriptCanvas_BACKUP/%s", subFolder.toUtf8().data());
            char backupPathCStr[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(m_backupPath.c_str(), backupPathCStr, AZ_MAX_PATH_LEN);
            m_backupPath = backupPathCStr;

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(m_backupPath.c_str()))
            {
                if (AZ::IO::FileIOBase::GetInstance()->CreatePath(m_backupPath.c_str()) != AZ::IO::ResultCode::Success)
                {
                    AZ_Error("Script Canvas", false, "Failed to create backup folder %s", m_backupPath.c_str());

                    return false;
                }
            }
        }

        return true;
    }

    void UpgradeTool::BackupComplete()
    {
        m_currentAssetIndex = 0;
        m_ui->progressBar->setValue(0);

        DoUpgrade();
    }

    void UpgradeTool::DoUpgrade()
    {
        m_state = UpgradeState::Upgrade;

        if (!m_assetsToUpgrade.empty())
        {
            m_ui->progressFrame->setVisible(true);
            m_ui->progressBar->setRange(0, aznumeric_cast<int>(m_assetsToUpgrade.size()));

            m_ui->spinner->SetIsBusy(true);
            m_ui->spinner->SetBusyIconSize(32);

            m_ui->upgradeButton->setEnabled(false);
            m_ui->notNowButton->setEnabled(false);
            m_ui->doNotAskCheckbox->setEnabled(false);
            m_ui->makeBackupCheckbox->setEnabled(false);

            m_inProgressAsset = m_assetsToUpgrade.begin();
        }
    }

    void UpgradeTool::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Start asset upgrade job when current asset is unassigned only
        // If current asset is present, there is ongoing progress handling this asset already
        if (IsOnReadyAssetForCurrentProcess(asset.GetId()))
        {
            m_inProgress = true;
            m_currentAsset = asset;
            m_scriptCanvasEntity = AssetUpgradeJob(asset);
            if (!m_scriptCanvasEntity)
            {
                ResetUpgradeCurrentAsset();
            }
        }
    }

    void UpgradeTool::OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Reset upgrade target when script canvas entity is unassigned only.
        // If script canvas entity is present, we should let the conversion progress finish itself.
        if (IsCurrentProcessFreeToAbort(asset.GetId()))
        {
            AZ_TracePrintf("Script Canvas", "Asset fails to get load: %s\n", asset.GetHint().c_str());
            ResetUpgradeCurrentAsset();
        }
    }

    void UpgradeTool::OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType)
    {
        // Reset upgrade target when script canvas entity is unassigned only.
        // If script canvas entity is present, we should let the conversion progress finish itself.
        if (IsCurrentProcessFreeToAbort(assetId))
        {
            AZ_TracePrintf("Script Canvas", "Asset gets unloaded: %s\n", m_inProgressAsset->m_relativePath.c_str());
            ResetUpgradeCurrentAsset();
        }
    }


    void UpgradeTool::OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>& asset, bool skipped /*=false*/)
    {
        if (!skipped)
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
                    else if (asset.GetType() == azrtti_typeid<ScriptCanvasFunctionAsset>())
                    {
                        tmpFilesaved = AZ::Utils::SaveObjectToStream<ScriptCanvas::ScriptCanvasData>(fileStream, AZ::DataStream::ST_XML, &asset.GetAs<ScriptCanvasFunctionAsset>()->GetScriptCanvasData());
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
                                MakeWriteable(info);

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
                                    MakeWriteable(info);

                                    if (tmpFilesaved)
                                    {
                                        PerformMove(asset, tmpFileName, fullPath);
                                    }
                                }

                            }
                        }
                    });
            }
        }
        else
        {
            // We skipped the upgrade (it's up to date), just mark it complete
            AZ::SystemTickBus::QueueFunction([this, asset]() { UpgradeComplete(asset, true); });

        }

    }

    void UpgradeTool::MakeWriteable(const AzToolsFramework::SourceControlFileInfo& info)
    {
        AZ::IO::SystemFile::SetWritable(info.m_filePath.c_str(), true);
    }

    void UpgradeTool::PerformMove(AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target)
    {
        auto moveResult = AZ::IO::SmartMove(source.c_str(), target.c_str());
        if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
        {
            // Bump the slice asset up in the asset processor's queue.
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, target.c_str());

            AZ::SystemTickBus::QueueFunction([this, &asset]() { UpgradeComplete(asset); });
        }
        else
        {
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
            streamer->SetRequestCompleteCallback(flushRequest, [this, &asset, source, target]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    // Continue saving.
                    AZ::SystemTickBus::QueueFunction([this, &asset, source, target]() { RetryMove(asset, source, target); });
                });
            streamer->QueueRequest(flushRequest);
        }
    }

    bool UpgradeTool::IsOnReadyAssetForCurrentProcess(const AZ::Data::AssetId& assetId)
    {
        return !m_currentAsset && m_inProgressAsset != m_assetsToUpgrade.end() && m_inProgressAsset->m_assetId == assetId;
    }

    bool UpgradeTool::IsCurrentProcessFreeToAbort(const AZ::Data::AssetId& assetId)
    {
        return !m_scriptCanvasEntity && m_inProgressAsset != m_assetsToUpgrade.end() && m_inProgressAsset->m_assetId == assetId;
    }

    bool UpgradeTool::IsUpgradeCompleteForAllAssets()
    {
        return !m_inProgress && !m_currentAsset && !m_scriptCanvasEntity && m_inProgressAsset == m_assetsToUpgrade.end();
    }

    bool UpgradeTool::IsUpgradeCompleteForCurrentAsset()
    {
        return !m_inProgress && !m_currentAsset && !m_scriptCanvasEntity && m_inProgressAsset != m_assetsToUpgrade.end();
    }

    void UpgradeTool::ResetUpgradeCurrentAsset()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_currentAsset.GetId());

        if (m_scriptCanvasEntity)
        {
            m_scriptCanvasEntity->Deactivate();
            m_scriptCanvasEntity = nullptr;
        }

        if (m_inProgressAsset != m_assetsToUpgrade.end())
        {
            m_inProgressAsset = m_assetsToUpgrade.erase(m_inProgressAsset);
        }

        m_currentAsset.Release();
        m_currentAsset = {};
        m_inProgress = false;
    }

    void UpgradeTool::OnSystemTick()
    {
        switch (m_state)
        {
        case UpgradeTool::UpgradeState::Upgrade:

            if (IsUpgradeCompleteForCurrentAsset())
            {
                m_inProgress = true;
                AZ::Data::AssetInfo& assetToUpgrade = *m_inProgressAsset;

                if (!AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(assetToUpgrade.m_assetId))
                {
                    AZ::Data::AssetBus::MultiHandler::BusConnect(assetToUpgrade.m_assetId);
                }

                auto asset = AZ::Data::AssetManager::Instance().GetAsset(assetToUpgrade.m_assetId, assetToUpgrade.m_assetType, AZ::Data::AssetLoadBehavior::Default);
                asset.BlockUntilLoadComplete();

                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(assetToUpgrade.m_relativePath);
                streamer->SetRequestCompleteCallback(flushRequest, [this]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                    {
                    });
                streamer->QueueRequest(flushRequest);

                if (asset.IsReady() || asset.GetStatus() == AZ::Data::AssetData::AssetStatus::ReadyPreNotify)
                {
                    m_currentAsset = asset;
                    m_scriptCanvasEntity = AssetUpgradeJob(asset);
                    if (!m_scriptCanvasEntity)
                    {
                        ResetUpgradeCurrentAsset();
                    }
                }

                m_ui->spinner->SetText(QObject::tr("%1").arg(assetToUpgrade.m_relativePath.c_str()));
            }
            else if (IsUpgradeCompleteForAllAssets())
            {
                FinalizeUpgrade();
            }
            break;

        case UpgradeTool::UpgradeState::Backup:

            if (m_inProgressAsset != m_assetsToUpgrade.end())
            {
                AZ::Data::AssetInfo& assetToBackup = *m_inProgressAsset;

                m_ui->spinner->SetText(QObject::tr("%1").arg(assetToBackup.m_relativePath.c_str()));

                BackupAsset(assetToBackup);

                m_ui->progressBar->setValue(aznumeric_cast<int>(++m_currentAssetIndex));
            }
            else
            {
                BackupComplete();
            }
            break;

        default:
            break;
        }

        AZ::Data::AssetManager::Instance().DispatchEvents();
        AZ::SystemTickBus::ExecuteQueuedEvents();

    }

    void UpgradeTool::BackupAsset(const AZ::Data::AssetInfo& assetInfo)
    {

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

        // Using this to get the watch folder
        AZStd::string watchFolder;
        AZ::Data::AssetInfo assetInfo2;
        bool sourceInfoFound{};
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, assetInfo.m_relativePath.c_str(), assetInfo2, watchFolder);
        if (sourceInfoFound)
        {
            AZStd::string assetPath;
            AzFramework::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), assetPath);

            sourceFilePath = assetPath;
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
        AzFramework::StringFunc::Path::Join(m_backupPath.c_str(), relativePath.c_str(), targetFilePath);

        if (AZ::IO::FileIOBase::GetInstance()->Copy(sourceFilePath.c_str(), targetFilePath.c_str()) != AZ::IO::ResultCode::Error)
        {
            AZStd::string filename;
            AzFramework::StringFunc::Path::GetFileName(sourceFilePath.c_str(), filename);
            AZ_TracePrintf("Script Canvas", "Backup: %s -> %s\n", filename.c_str(), targetFilePath.c_str());
        }
        else
        {
            AZ_TracePrintf("Script Canvas", "(Error) Failed to create backup: %s  -> %s\n", sourceFilePath.c_str(), targetFilePath.c_str());
        }

        ++m_inProgressAsset;
    }

    void UpgradeTool::UpgradeComplete(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool skipped /*= false*/)
    {
        m_ui->progressBar->setValue(aznumeric_cast<int>(++m_currentAssetIndex));

        ResetUpgradeCurrentAsset();

        if (!skipped)
        {
            AZStd::string filename;
            AzFramework::StringFunc::Path::GetFileName(asset.GetHint().c_str(), filename);
            AZ_TracePrintf("Script Canvas", "%s -> Upgraded and Saved!\n", filename.c_str());
        }
    }

    void UpgradeTool::FinalizeUpgrade()
    {
        setWindowFlag(Qt::WindowCloseButtonHint, true);

        AZ::SystemTickBus::Handler::BusDisconnect();

        m_currentAsset = {};

        SaveLog();

        UpgradeNotifications::Bus::Broadcast(&UpgradeNotifications::OnUpgradeComplete);

        AZ_TracePrintf("Script Canvas", "\nUpgrade Complete!\n");

        DisconnectBuses();

        accept();
    }

    template <typename AssetType>
    AZ::Entity* UpgradeGraph(AZ::Data::Asset<AZ::Data::AssetData>& asset, UpgradeTool* upgradeTool)
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

        auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
        AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");

        bool isLatest = graphComponent->GetVersion().IsLatest();
        if (isLatest)
        {
            ++upgradeTool->SkippedGraphCount();

            // No need to upgrade
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

        if (graphComponent)
        {
            if (!graphComponent->UpgradeGraph(asset))
            {
                ++upgradeTool->SkippedGraphCount();
            }
            else
            {
                ++upgradeTool->UpgradedGraphCount();
            }
        }

        return scriptCanvasEntity;
    }

    void UpgradeTool::SaveLog()
    {
        AZStd::string outputFileName = AZStd::string::format("@devroot@/ScriptCanvasUpgradeReport.html");

        char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(outputFileName.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);

        AZStd::string endPath = resolvedBuffer;
        AZ::StringFunc::Path::Normalize(endPath);

        AZ::IO::SystemFile outputFile;
        if (!outputFile.Open(endPath.c_str(),
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error("Script Canvas", false, "Failed to open file for writing: %s", endPath.c_str());
            return;
        }

        QDateTime theTime = QDateTime::currentDateTime();
        AZStd::string timeStamp = theTime.toString("yyyy-MM-dd [HH.mm.ss]").toUtf8().data();

        AZStd::string header = "<html>\n<head>\n<style>\nbody {color:white; background:black;}\n .error { color:red;}\n .warning {color:darkorange;}\n</style>\n</head>\n<body>\n";
        header.append(AZStd::string::format("Log captured: %s<br>\n", timeStamp.c_str()).c_str());

        outputFile.Write(header.c_str(), header.size());
        for (auto& log : m_logs)
        {
            AZStd::string logText = AZStd::string::format("%s<br>", log.c_str());
            AzFramework::StringFunc::Replace(logText, "\n", "<br>\n");
            outputFile.Write(logText.data(), logText.size());
        }
        AZStd::string footer = "\n</body>\n</html>";
        outputFile.Write(footer.c_str(), footer.size());



        outputFile.Close();
    }

    AZ::Entity* UpgradeTool::AssetUpgradeJob(AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        using namespace ScriptCanvasEditor;

        AZ_Assert(asset.IsReady(), "The asset must be ready by now");

        AZStd::lock_guard<AZStd::recursive_mutex> myLocker(m_mutex);

        AZ::Entity* scriptCanvasEntity = nullptr;
        if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
        {
            scriptCanvasEntity = UpgradeGraph<ScriptCanvasAsset>(asset, this);
        }
        else if (asset.GetType() == azrtti_typeid<ScriptCanvasFunctionAsset>())
        {
            scriptCanvasEntity = UpgradeGraph<ScriptCanvasFunctionAsset>(asset, this);
        }

        if (!scriptCanvasEntity)
        {
            // This may happen if the graph failed or did not need to upgrade
            AZ_TracePrintf("Script Canvas", "%s .. up to date!\n", asset.GetHint().c_str());
            return nullptr;
        }

        // The rest will happen when we get notified that the graph is done.
        return scriptCanvasEntity;
    }

    void UpgradeTool::RetryMove(AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target)
    {
        auto moveResult = AZ::IO::SmartMove(source.c_str(), target.c_str());
        if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
        {
            // Bump the slice asset up in the asset processor's queue.
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, target.c_str());

            AZ::SystemTickBus::QueueFunction([this, &asset]() { UpgradeComplete(asset); });
        }
        else
        {
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
            streamer->SetRequestCompleteCallback(flushRequest, [this, &asset, source, target]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    // Continue saving.
                    AZ::SystemTickBus::QueueFunction([this, &asset, source, target]() { RetryMove(asset, source, target); });
                });
            streamer->QueueRequest(flushRequest);


            //AZ::SystemTickBus::QueueFunction([this, &asset, source, target]() { RetryMove(asset, source, target); });
        }
    }

    void UpgradeTool::CaptureLogFromTraceBus(const char* /*window*/, const char* message)
    {
        AZStd::string msg = message;
        if (msg.ends_with("\n"))
        {
            msg = msg.substr(0, msg.size() - 1);
        }

        m_logs.push_back(msg);
    }

    bool UpgradeTool::OnPreError(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        AZStd::string msg = AZStd::string::format("<span class='error'>(Error): %s</span><br>", message);
        CaptureLogFromTraceBus(window, msg.c_str());

        return false;
    }

    bool UpgradeTool::OnPreWarning(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        AZStd::string msg = AZStd::string::format("<span class='warning'>(Warning): %s</span><br>", message);
        CaptureLogFromTraceBus(window, msg.c_str());

        return false;
    }

    bool UpgradeTool::OnException(const char* message)
    {
        AZStd::string msg = AZStd::string::format("<span class='error'>(Exception): %s</span><br>", message);
        CaptureLogFromTraceBus("Script Canvas", msg.c_str());

        return false;
    }

    bool UpgradeTool::OnPrintf(const char* window, const char* message)
    {
        CaptureLogFromTraceBus(window, message);
        return false;
    }

    ScriptCanvasEditor::EditorKeepAlive::EditorKeepAlive()
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

    ScriptCanvasEditor::EditorKeepAlive::~EditorKeepAlive()
    {
        if (m_edKeepEditorActive)
        {
            m_edKeepEditorActive->Set(m_keepEditorActive);
        }
    }

#include <Editor/View/Windows/Tools/UpgradeTool/moc_UpgradeTool.cpp>

}
