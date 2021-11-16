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
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        struct ModifyConfiguration
        {
            AZStd::function<void(SourceHandle&)> modification;
            AZStd::function<bool()> onReadOnlyFile;
            SourceHandle modifySingleAsset;
            bool backupGraphBeforeModification = false;
            bool successfulDependencyUpgradeRequired = true;
        };

        struct ModificationResult
        {
            SourceHandle asset;
            AZStd::string errorMessage;
        };

        struct ModificationResults
        {
            AZStd::vector<SourceHandle> m_successes;
            AZStd::vector<ModificationResult> m_failures;
        };

        struct ScanConfiguration
        {
            enum class Filter
            {
                Include,
                Exclude
            };

            AZStd::function<Filter(const SourceHandle&)> filter;
            bool reportFilteredGraphs = false;
        };

        struct ScanResult
        {
            AZStd::vector<SourceHandle> m_catalogAssets;
            AZStd::vector<SourceHandle> m_unfiltered;
            AZStd::vector<SourceHandle> m_filteredAssets;
            AZStd::vector<SourceHandle> m_loadErrors;
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
            virtual void OnScanFilteredGraph(const SourceHandle& info) = 0;
            virtual void OnScanLoadFailure(const SourceHandle& info) = 0;
            virtual void OnScanUnFilteredGraph(const SourceHandle& info) = 0;

            virtual void OnUpgradeBegin(const ModifyConfiguration& config, const AZStd::vector<SourceHandle>& assets) = 0;
            virtual void OnUpgradeComplete(const ModificationResults& results) = 0;
            virtual void OnUpgradeDependenciesGathered(const SourceHandle& info, Result result) = 0;
            virtual void OnUpgradeDependencySortBegin(const ModifyConfiguration& config, const AZStd::vector<SourceHandle>& assets) = 0;
            virtual void OnUpgradeDependencySortEnd
                ( const ModifyConfiguration& config
                , const AZStd::vector<SourceHandle>& assets
                , const AZStd::vector<size_t>& sortedOrder) = 0;
            virtual void OnUpgradeModificationBegin(const ModifyConfiguration& config, const SourceHandle& info) = 0;
            virtual void OnUpgradeModificationEnd(const ModifyConfiguration& config, const SourceHandle& info, ModificationResult result) = 0;
        };
        using ModelNotificationsBus = AZ::EBus<ModelNotificationsTraits>;
    }
}
