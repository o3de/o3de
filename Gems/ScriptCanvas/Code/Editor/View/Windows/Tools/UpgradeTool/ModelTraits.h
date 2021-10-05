/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        struct WorkingAsset
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset;
            AZ::Data::AssetInfo info;
        };

        using WorkingAssets = AZStd::vector<WorkingAsset>;

        struct ModifyConfiguration
        {
            AZStd::function<void(AZ::Data::Asset<AZ::Data::AssetData>)> modification;
            AZStd::function<bool()> onReadOnlyFile;
            AZ::Data::AssetInfo modifySingleAsset;
            bool backupGraphBeforeModification = false;
            bool successfulDependencyUpgradeRequired = true;
        };

        struct ModificationResult
        {
            AZ::Data::Asset<AZ::Data::AssetData> asset;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string errorMessage;
        };

        struct ModificationResults
        {
            AZStd::vector<AZ::Data::AssetInfo> m_successes;
            AZStd::vector<ModificationResult> m_failures;
        };

        struct ScanConfiguration
        {
            AZStd::function<bool(AZ::Data::Asset<AZ::Data::AssetData>)> filter;
            bool reportFilteredGraphs = false;
        };

        struct ScanResult
        {
            AZStd::vector<AZ::Data::AssetInfo> m_catalogAssets;
            WorkingAssets m_unfiltered;
            AZStd::vector<AZ::Data::AssetInfo> m_filteredAssets;
            AZStd::vector<AZ::Data::AssetInfo> m_loadErrors;
        };

        enum Result
        {
            Failure,
            Success
        };

        class ModificationNotificationsTraits
            : public AZ::EBusTraits
        {
        public:
            virtual void ModificationComplete(const ModificationResult& result) = 0;
        };
        using ModificationNotificationsBus = AZ::EBus<ModificationNotificationsTraits>;

        class ModelRequestsTraits
            : public AZ::EBusTraits
        {
        public:
            virtual const ModificationResults* GetResults() = 0;
            virtual void Modify(const ModifyConfiguration& modification) = 0;
            virtual void Scan(const ScanConfiguration& filter) = 0;
        };
        using ModelRequestsBus = AZ::EBus<ModelRequestsTraits>;

        class ModelNotificationsTraits
            : public AZ::EBusTraits
        {
        public:
            virtual void OnScanBegin(size_t assetCount) = 0;
            virtual void OnScanComplete(const ScanResult& result) = 0;
            virtual void OnScanFilteredGraph(const AZ::Data::AssetInfo& info) = 0;
            virtual void OnScanLoadFailure(const AZ::Data::AssetInfo& info) = 0;
            virtual void OnScanUnFilteredGraph(const AZ::Data::AssetInfo& info) = 0;

            virtual void OnUpgradeBegin(const ModifyConfiguration& config, const WorkingAssets& assets) = 0;
            virtual void OnUpgradeComplete(const ModificationResults& results) = 0;
            virtual void OnUpgradeDependenciesGathered(const AZ::Data::AssetInfo& info, Result result) = 0;
            virtual void OnUpgradeDependencySortBegin(const ModifyConfiguration& config, const WorkingAssets& assets) = 0;
            virtual void OnUpgradeDependencySortEnd
                ( const ModifyConfiguration& config
                , const WorkingAssets& assets
                , const AZStd::vector<size_t>& sortedOrder) = 0;
            virtual void OnUpgradeModificationBegin(const ModifyConfiguration& config, const AZ::Data::AssetInfo& info) = 0;
            virtual void OnUpgradeModificationEnd(const ModifyConfiguration& config, const AZ::Data::AssetInfo& info, ModificationResult result) = 0;
        };
        using ModelNotificationsBus = AZ::EBus<ModelNotificationsTraits>;
    }
}
