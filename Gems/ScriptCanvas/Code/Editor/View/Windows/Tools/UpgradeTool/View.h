/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <IConsole.h>
#include <ISystem.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/Core.h>
#endif

#include <Editor/View/Windows/Tools/UpgradeTool/ViewTraits.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>

class QPushButton;

namespace Ui
{
    class View;
}

namespace AzQtComponents
{
    class StyledBusyLabel;
}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
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
        //! Handles display change notifications, handles state change notifications, sends control requests
        class View
            : public AzQtComponents::StyledDialog
            , private AZ::SystemTickBus::Handler
            , private UpgradeNotifications::Bus::Handler
            , private ViewRequestsBus::Handler
            , private ModelNotificationsBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(View, AZ::SystemAllocator, 0);

            explicit View(QWidget* parent = nullptr);
            
        private:

            static constexpr int ColumnAsset = 0;
            static constexpr int ColumnAction = 1;
            static constexpr int ColumnBrowse = 2;
            static constexpr int ColumnStatus = 3;

            void OnCloseButtonPress();
            void OnScanButtonPress();
            void OnUpgradeAllButtonPress();

            enum class ProcessState
            {
                Inactive,
                Backup,
                Scan,
                Upgrade,
            };
            ProcessState m_state = ProcessState::Inactive;

            void DoScan();
            void ScanComplete(const AZ::Data::Asset<AZ::Data::AssetData>&);

            void InspectAsset(AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::Data::AssetInfo& assetInfo);

            // SystemTickBus::Handler
            void OnSystemTick() override;

            enum class OperationResult
            {
                Success,
                Failure,
            };

            void GraphUpgradeComplete(const AZ::Data::Asset<AZ::Data::AssetData>, OperationResult result, AZStd::string_view message);

            bool IsUpgrading() const { return false; }

            bool m_inProgress = false;
            // scan fields
            size_t m_currentAssetRowIndex = 0;
            size_t m_inspectedAssets = 0;
            size_t m_failedAssets = 0;
            size_t m_discoveredAssets = 0;

            IUpgradeRequests::AssetList m_assetsToInspect;
            IUpgradeRequests::AssetList::iterator m_inspectingAsset;
            using UpgradeAssets = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
            UpgradeAssets m_assetsToUpgrade;
            UpgradeAssets::iterator m_inProgressAsset;

            AZ::Data::Asset<AZ::Data::AssetData> m_currentAsset;

            AZStd::unique_ptr<Ui::View> m_ui;

            
            // upgrade fields
            AZStd::recursive_mutex m_mutex;
            bool m_upgradeComplete = false;
            AZ::Data::Asset<AZ::Data::AssetData> m_upgradeAsset;
            int m_upgradeAssetIndex = 0;
            OperationResult m_upgradeResult;
            AZStd::string m_upgradeMessage;
            AZStd::string m_tmpFileName;

            AZStd::unique_ptr<EditorKeepAlive> m_keepEditorAlive;

            AZStd::deque<AZStd::string> m_logs;

            AZ::Entity* m_scriptCanvasEntity = nullptr;

            bool m_isUpgradingSingleGraph = false;

            void UpgradeSingle(QPushButton* item, AzQtComponents::StyledBusyLabel* spinner, AZ::Data::AssetInfo assetInfo);

            void FlushLogs();

            void FinalizeUpgrade();
            void FinalizeScan();

            void BackupComplete();
            AZStd::string BackupGraph(const AZ::Data::Asset<AZ::Data::AssetData>&);
            void UpgradeGraph(const AZ::Data::Asset<AZ::Data::AssetData>&);

            void GraphUpgradeCompleteUIUpdate(const AZ::Data::Asset<AZ::Data::AssetData> asset, OperationResult result, AZStd::string_view message);
            void OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>&, bool skipped = false) override;

            void OnSourceFileReleased(AZ::Data::Asset<AZ::Data::AssetData> asset);

            void closeEvent(QCloseEvent* event) override;

            bool m_overwriteAll = false;
            void PerformMove(AZ::Data::Asset<AZ::Data::AssetData> asset, AZStd::string source, AZStd::string target, size_t remainingAttempts);

            void Log(const char* format, ...);
        };
    }
}
