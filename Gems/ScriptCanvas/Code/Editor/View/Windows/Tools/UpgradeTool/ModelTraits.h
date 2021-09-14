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
        struct ModifyConfiguration
        {
            AZStd::function<void(AZ::Data::Asset<AZ::Data::AssetData>)> modification;
            bool backupGraphBeforeModification = false;
            // disabling this can be acceptable, but be careful
            bool successfulDependencyUpgradeRequired = true;
        };

        struct ScanConfiguration
        {
            AZStd::function<bool(AZ::Data::Asset<AZ::Data::AssetData>)> filter;
            bool reportFilteredGraphs = false;
        };

        class ModelRequestsTraits
            : public AZ::EBusTraits
        {
        public:
            virtual void Modify(const ModifyConfiguration& modification) = 0;
            virtual void Scan(const ScanConfiguration& filter) = 0;
        };
        using ModelRequestsBus = AZ::EBus<ModelRequestsTraits>;

        struct ModificationResult
        {

        };

        struct ScanResult
        {
            AZStd::vector<AZ::Data::AssetInfo> m_catalogAssets;
            AZStd::vector<AZ::Data::AssetInfo> m_unfiltered;
            AZStd::vector<AZ::Data::AssetInfo> m_filteredAssets;
            AZStd::vector<AZ::Data::AssetInfo> m_loadErrors;
        };

        class ModelNotificationsTraits
            : public AZ::EBusTraits
        {
        public:
            virtual void OnScanBegin(size_t assetCount) = 0;
            virtual void OnScanComplete(const ScanResult& result) = 0;
            virtual void OnScanFilteredGraph(const AZ::Data::AssetInfo& info) = 0;
            virtual void OnScanLoadFailure(const AZ::Data::AssetInfo& info) = 0;
            virtual void OnScanUnFilteredGraph(const AZ::Data::AssetInfo& info) = 0;

            virtual void OnUpgradeAllBegin() = 0;
            virtual void OnUpgradeAllComplete() = 0;
            virtual void OnUpgradeAllDependencySortBegin() = 0;
            virtual void OnUpgradeAllDependencySortEnd
                ( const AZStd::vector<AZ::Data::AssetInfo>& assets
                , const AZStd::vector<size_t>& sortedOrder) = 0;
        };
        using ModelNotificationsBus = AZ::EBus<ModelNotificationsTraits>;
    }
}
