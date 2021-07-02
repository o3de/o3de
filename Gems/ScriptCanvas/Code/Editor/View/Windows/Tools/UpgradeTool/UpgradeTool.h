/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QProgressBar>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <AzQtComponents/Components/StyledDialog.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ISystem.h>
#include <IConsole.h>
#include <AzCore/Debug/TraceMessageBus.h>
#endif

class QPushButton;

namespace Ui
{
    class UpgradeTool;
}

namespace ScriptCanvasEditor
{
    //! Scoped utility to set and restore the "ed_KeepEditorActive" CVar in order to allow
    //! the upgrade tool to work even if the editor is not in the foreground
    class EditorKeepAlive
    {
    public:
        EditorKeepAlive();
        ~EditorKeepAlive();

    private:
        int m_keepEditorActive;
        ICVar* m_edKeepEditorActive;
    };

    //! A tool that collects and upgrades all Script Canvas graphs in the asset catalog
    class UpgradeTool
        : public AzQtComponents::StyledDialog
        , private AZ::SystemTickBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
        , private UpgradeNotifications::Bus::Handler
        , private AZ::Debug::TraceMessageBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UpgradeTool, AZ::SystemAllocator, 0);

        UpgradeTool(QWidget* parent = nullptr);
        ~UpgradeTool();

        size_t& UpgradedGraphCount() { return m_upgradedAssets; }
        size_t& SkippedGraphCount() { return m_skippedAssets; }

        bool HasBackup() const;

    private:

        bool IsOnReadyAssetForCurrentProcess(const AZ::Data::AssetId& assetId);
        bool IsCurrentProcessFreeToAbort(const AZ::Data::AssetId& assetId);
        bool IsUpgradeCompleteForAllAssets();
        bool IsUpgradeCompleteForCurrentAsset();
        void ResetUpgradeCurrentAsset();

        void OnUpgrade();
        void OnNoThanks();
        void UpdateSettings();

        enum class UpgradeState
        {
            Inactive,
            Backup,
            Upgrade
        };
        UpgradeState m_state = UpgradeState::Inactive;

        bool DoBackup();
        void BackupAsset(const AZ::Data::AssetInfo& assetInfo);
        void BackupComplete();

        void DoUpgrade();
        void UpgradeComplete(const AZ::Data::Asset<AZ::Data::AssetData>&, bool skipped = false);

        AZ::Entity* AssetUpgradeJob(AZ::Data::Asset<AZ::Data::AssetData>& asset);

        // SystemTickBus::Handler
        void OnSystemTick() override;
        //

        // AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;
        //

        // AZ::Debug::TranceMessageBus::Handler
        bool OnException(const char* /*message*/) override;
        bool OnPrintf(const char* /*window*/, const char* /*message*/) override;
        bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        //

        void CaptureLogFromTraceBus(const char* window, const char* message);

        void OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>&, bool skipped = false) override;

        void RetryMove(AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target);

        void SaveLog();

        bool m_inProgress = false;
        size_t m_currentAssetIndex = 0;

        size_t m_upgradedAssets = 0;
        size_t m_skippedAssets = 0;

        IUpgradeRequests::AssetList m_assetsToUpgrade;
        IUpgradeRequests::AssetList::iterator m_inProgressAsset;

        AZ::Data::Asset<AZ::Data::AssetData> m_currentAsset;

        AZStd::unique_ptr<Ui::UpgradeTool> m_ui;
        AZStd::recursive_mutex m_mutex;

        AZStd::unique_ptr<EditorKeepAlive> m_keepEditorAlive;

        AZStd::vector<AZStd::string> m_logs;

        AZ::Entity* m_scriptCanvasEntity = nullptr;

        AZStd::string m_backupPath;

        void FinalizeUpgrade();
        void DisconnectBuses();

        void closeEvent(QCloseEvent* event) override;

        bool m_overwriteAll = false;
        void MakeWriteable(const AzToolsFramework::SourceControlFileInfo& info);
        void PerformMove(AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target);
    };
}
