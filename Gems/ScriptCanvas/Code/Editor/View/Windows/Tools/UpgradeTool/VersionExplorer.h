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
#include <AzQtComponents/Components/StyledDialog.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ISystem.h>
#include <IConsole.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include "UpgradeTool.h"
#endif

class QPushButton;

namespace Ui
{
    class VersionExplorer;
}

namespace AzQtComponents
{
    class StyledBusyLabel;
}

namespace ScriptCanvasEditor
{
    //! A tool that collects and upgrades all Script Canvas graphs in the asset catalog
    class VersionExplorer
        : public AzQtComponents::StyledDialog
        , private AZ::SystemTickBus::Handler
        , private UpgradeNotifications::Bus::Handler
        , private AZ::Debug::TraceMessageBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(VersionExplorer, AZ::SystemAllocator, 0);

        explicit VersionExplorer(QWidget* parent = nullptr);
        ~VersionExplorer();

    private:

        static constexpr int ColumnAsset = 0;
        static constexpr int ColumnAction = 1;
        static constexpr int ColumnBrowse = 2;
        static constexpr int ColumnStatus = 3;

        void OnScan();
        void OnClose();

        enum class ProcessState
        {
            Inactive,
            Backup,
            Scan,
            Upgrade
        };
        ProcessState m_state = ProcessState::Inactive;

        bool DoBackup();
        void BackupAsset(const AZ::Data::AssetInfo& assetInfo);
        void BackupComplete();

        void DoScan();
        void ScanComplete(const AZ::Data::Asset<AZ::Data::AssetData>&);

        void InspectAsset(AZ::Data::Asset<AZ::Data::AssetData>& asset);

        void OnUpgradeAll();

        // SystemTickBus::Handler
        void OnSystemTick() override;
        //

        // AZ::Debug::TranceMessageBus::Handler
        bool OnException(const char* /*message*/) override;
        bool OnPrintf(const char* /*window*/, const char* /*message*/) override;
        bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
        //

        void CaptureLogFromTraceBus(const char* window, const char* message);

        enum class OperationResult
        {
            Success,
            SkipBackup,
            BackupSuccess,
            BackupFail,
            BackupFail_CreateFolder,
            BackupFail_FileNotFound
        };

        void GraphUpgradeComplete(const AZ::Data::Asset<AZ::Data::AssetData>&, OperationResult result = OperationResult::Success);

        bool IsUpgrading() const;

        bool m_inProgress = false;
        size_t m_currentAssetIndex = 0;
        size_t m_inspectedAssets = 0;

        IUpgradeRequests::AssetList m_assetsToInspect;
        IUpgradeRequests::AssetList::iterator m_inspectingAsset;

        using UpgradeAssets = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
        UpgradeAssets m_assetsToUpgrade;
        UpgradeAssets::iterator m_inProgressAsset;

        AZ::Data::Asset<AZ::Data::AssetData> m_currentAsset;

        AZStd::unique_ptr<Ui::VersionExplorer> m_ui;

        AZStd::recursive_mutex m_mutex;

        AZStd::unique_ptr<EditorKeepAlive> m_keepEditorAlive;

        AZStd::deque<AZStd::string> m_logs;

        AZ::Entity* m_scriptCanvasEntity = nullptr;

        bool m_isUpgradingSingleGraph = false;

        void UpgradeSingle(QPushButton* item, AzQtComponents::StyledBusyLabel* spinner, const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        void FlushLogs();

        void FinalizeUpgrade();
        void FinalizeScan();

        OperationResult BackupGraph(const AZ::Data::Asset<AZ::Data::AssetData>&);
        void UpgradeGraph(const AZ::Data::Asset<AZ::Data::AssetData>&);

        void RetryMove(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target);

        void GraphUpgradeCompleteUIUpdate(const AZ::Data::Asset<AZ::Data::AssetData>& asset, OperationResult result = OperationResult::Success);
        void OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>&, bool skipped = false) override;

        void closeEvent(QCloseEvent* event) override;

        bool m_overwriteAll = false;
        void PerformMove(AZ::Data::Asset<AZ::Data::AssetData>& asset, const AZStd::string& source, const AZStd::string& target);
    };
}
