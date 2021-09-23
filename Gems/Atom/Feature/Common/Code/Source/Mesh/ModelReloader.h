/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <RenderCommon.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>
#include <Atom/Feature/Mesh/ModelReloaderSystemInterface.h>
#include <Source/Mesh/ModelReloaderSystem.h>

namespace AZ
{
    namespace RPI
    {
        class ModelAsset;
    }

    namespace Render
    {
        class ModelReloader
            : private Data::AssetBus::MultiHandler
        {
            using DependencyList = AZStd::vector<Data::Asset<Data::AssetData>>;
        public:
            AZ_RTTI(AZ::Render::ModelReloader, "{99B75A6A-62B6-490A-9953-029BE7D69452}");

            ModelReloader() = default;
            ModelReloader(Data::Asset<RPI::ModelAsset> modelAsset, RemoveModelFromReloaderSystemEvent::Handler& removeReloaderFromSystemHandler);
            void ConnectOnReloadedEventHandler(ModelReloadedEvent::Handler& onReloadedEventHandler);

        private:
            enum class State
            {
                WaitingForMeshDependencies,
                WaitingForModelDependencies,
                WaitingForModel
            };

            // Data::AssetBus::MultiHandler overrides...
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;            
            void OnAssetReloadError(Data::Asset<Data::AssetData> asset) override;

            void InsertMeshDependencyIfUnique(Data::Asset<Data::AssetData> asset);
            void ReloadDependenciesAndWait();
            void AdvanceToNextLevelOfHierarchy();
            DependencyList& GetPendingDependencies();

            ModelReloadedEvent m_onModelReloaded;
            RemoveModelFromReloaderSystemEvent m_onRemoveReloaderFromSystem;

            // Keep track of all the asset references for each level of the hierarchy
            DependencyList m_modelAsset;
            DependencyList m_meshDependencies;
            DependencyList m_modelDependencies;
            
            AZStd::bitset<1024> m_pendingDependencies;
            State m_state;
        };

    } // namespace Render
} // namespace AZ
