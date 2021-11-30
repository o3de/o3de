/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/EntityId.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Components/StyledDialog.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <ISystem.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/Core.h>
AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QProgressBar>
AZ_POP_DISABLE_WARNING
#endif

class QPushButton;
class QTableWidgetItem;

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
        //! A tool that collects and upgrades all Script Canvas graphs in the asset catalog
        //! Handles display change notifications, handles state change notifications, sends control requests
        class Controller
            : public AzQtComponents::StyledDialog
            , private UpgradeNotificationsBus::Handler
            , private ModelNotificationsBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(Controller, AZ::SystemAllocator, 0);

            explicit Controller(QWidget* parent = nullptr);
            ~Controller();

        private:
            static constexpr int ColumnAsset = 0;
            static constexpr int ColumnAction = 1;
            static constexpr int ColumnBrowse = 2;
            static constexpr int ColumnStatus = 3;

            Ui::View* m_view = nullptr;
            int m_handledAssetCount = 0;

            void AddLogEntries();
            void EnableAllUpgradeButtons();
            QList<QTableWidgetItem*> FindTableItems(const AZ::Data::AssetInfo& assetInfo);

            void OnButtonPressClose();
            void OnButtonPressScan();
            void OnButtonPressUpgrade();
            void OnButtonPressUpgradeImplementation(const AZ::Data::AssetInfo& assetInfo);
            void OnButtonPressUpgradeSingle(const AZ::Data::AssetInfo& assetInfo);

            void OnGraphUpgradeComplete(AZ::Data::Asset<AZ::Data::AssetData>&, bool skipped) override;
                        
            void OnScanBegin(size_t assetCount) override;
            void OnScanComplete(const ScanResult& result) override;
            void OnScanFilteredGraph(const AZ::Data::AssetInfo& info) override;
            void OnScanLoadFailure(const AZ::Data::AssetInfo& info) override;
            void OnScanUnFilteredGraph(const AZ::Data::AssetInfo& info) override;
            enum class Filtered { No, Yes };
            void OnScannedGraph(const AZ::Data::AssetInfo& info, Filtered filtered);
            void OnScannedGraphResult(const AZ::Data::AssetInfo& info);

            // for single operation UI updates, just check the assets size, or note it on the request
            void OnUpgradeBegin(const ModifyConfiguration& config, const WorkingAssets& assets) override;
            void OnUpgradeComplete(const ModificationResults& results) override;
            void OnUpgradeDependenciesGathered(const AZ::Data::AssetInfo& info, Result result) override;
            void OnUpgradeDependencySortBegin(const ModifyConfiguration& config, const WorkingAssets& assets) override;
            void OnUpgradeDependencySortEnd
                ( const ModifyConfiguration& config
                , const WorkingAssets& assets
                , const AZStd::vector<size_t>& sortedOrder) override;
            void OnUpgradeModificationBegin(const ModifyConfiguration& config, const AZ::Data::AssetInfo& info) override;
            void OnUpgradeModificationEnd(const ModifyConfiguration& config, const AZ::Data::AssetInfo& info, ModificationResult result) override;

            void SetLoggingPreferences();
            void SetSpinnerIsBusy(bool isBusy);
            void SetRowBusy(int index);
            void SetRowFailed(int index, AZStd::string_view message);
            void SetRowPending(int index);
            void SetRowsBusy();
            void SetRowSucceeded(int index);
        };
    }
}
