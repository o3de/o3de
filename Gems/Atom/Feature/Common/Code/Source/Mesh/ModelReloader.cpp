/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/ModelReloader.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Public/Model/Model.h>

namespace AZ
{
    namespace Render
    {
        ModelReloader::ModelReloader(
            Data::Asset<RPI::ModelAsset> modelAsset)
        {
            m_modelAsset.push_back(modelAsset);
            m_pendingDependencyListStatus.reset();

            // Iterate over the model and track the assets that need to be reloaded
            for (auto& modelLodAsset : modelAsset->GetLodAssets())
            {
                for (auto& mesh : modelLodAsset->GetMeshes())
                {
                    for (auto& streamBufferInfo : mesh.GetStreamBufferInfoList())
                    {
                        InsertMeshDependencyIfUnique(streamBufferInfo.m_bufferAssetView.GetBufferAsset());
                        
                    }
                    InsertMeshDependencyIfUnique(mesh.GetIndexBufferAssetView().GetBufferAsset());
                }
                m_modelDependencies.push_back(modelLodAsset);
            }

            AZ_Assert(
                m_meshDependencies.size() <= m_pendingDependencyListStatus.size(),
                "There are more buffers used by the model %s than are supported by the ModelReloader.", modelAsset.GetHint().c_str());

            m_state = State::WaitingForMeshDependencies;
            ReloadDependenciesAndWait();
        }

        void ModelReloader::ConnectOnReloadedEventHandler(ModelReloadedEvent::Handler& onReloadedEventHandler)
        {
            onReloadedEventHandler.Connect(m_onModelReloaded);
        }

        void ModelReloader::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            DependencyList& pendingDependencies = GetPendingDependencyList();

            const Data::AssetId& reloadedAssetId = asset.GetId();

            // Find the index of the asset that was reloaded
            const auto matchesId = [reloadedAssetId](const Data::Asset<Data::AssetData>& asset){ return asset.GetId() == reloadedAssetId;};
            const auto& iter = AZStd::find_if(AZStd::begin(pendingDependencies), AZStd::end(pendingDependencies), matchesId);
            AZ_Assert(
                iter != AZStd::end(pendingDependencies),
                "ModelReloader - handling an AssetReloaded event for an asset that is not part of the dependency list.");
            size_t currentIndex = AZStd::distance(AZStd::begin(pendingDependencies), iter);

            // Keep a reference to the newly reloaded asset to prevent it from being immediately released
            pendingDependencies[currentIndex] = asset;
            Data::AssetBus::MultiHandler::BusDisconnect(reloadedAssetId);

            // Clear the bit, now that it has been reloaded
            m_pendingDependencyListStatus.reset(currentIndex);

            if (m_pendingDependencyListStatus.none())
            {
                AdvanceToNextLevelOfHierarchy();
            }
        }

        void ModelReloader::OnAssetReloadError(Data::Asset<Data::AssetData> asset)
        {
            // An error is actually okay/expected in some situations.
            // For example, if the 2nd UV set was removed, and we tried to reload the second uv set, the reload would fail.
            // We want to treat it as a success, and mark that dependency as 'up to date'
            OnAssetReloaded(asset);
        }

        void ModelReloader::InsertMeshDependencyIfUnique(Data::Asset<Data::AssetData> asset)
        {
            if (AZStd::find(AZStd::begin(m_meshDependencies), AZStd::end(m_meshDependencies), asset) == AZStd::end(m_meshDependencies))
            {
                // Multiple meshes may reference the same buffer, so only add the dependency if it is unique
                m_meshDependencies.push_back(asset);
            }
        }

        void ModelReloader::ReloadDependenciesAndWait()
        {
            // Get the current list of dependencies depending on the current state
            DependencyList& dependencies = GetPendingDependencyList();

            if (!m_pendingDependencyListStatus.none())
            {
                AZ_Assert(
                    m_pendingDependencyListStatus.none(),
                    "ModelReloader attempting to add new dependencies while still waiting for other dependencies in the hierarchy to "
                    "load.");
            }
            if (dependencies.empty())
            {
                // If the original model asset failed to load, it won't have any dependencies to reload
                AdvanceToNextLevelOfHierarchy();
            }
            AZ_Assert(
                dependencies.size() <= m_pendingDependencyListStatus.size(),
                "ModelReloader has more dependencies than can fit in the bitset. The size of m_pendingDependencyListStatus needs to be increased.");

            // Set all bits to 1
            m_pendingDependencyListStatus.set();
            // Clear the least significant n-bits
            m_pendingDependencyListStatus <<= dependencies.size();
            // Set the least significant n-bits to 1, and the rest to 0
            m_pendingDependencyListStatus.flip();

            // Reload all the assets
            for (Data::Asset<Data::AssetData>& dependencyAsset : dependencies)
            {
                Data::AssetBus::MultiHandler::BusConnect(dependencyAsset.GetId());
                dependencyAsset.Reload();
            }
        }

        void ModelReloader::AdvanceToNextLevelOfHierarchy()
        {
            switch (m_state)
            {
                case State::WaitingForMeshDependencies:
                    m_state = State::WaitingForModelDependencies;
                    ReloadDependenciesAndWait();
                    break;
                case State::WaitingForModelDependencies:
                    m_state = State::WaitingForModel;
                    ReloadDependenciesAndWait();
                    break;
                case State::WaitingForModel:
                    Data::AssetBus::MultiHandler::BusDisconnect();
                    // Since the model asset is finished reloading, orphan model from the instance database
                    // so that all of the buffer instances are re-created with the latest data
                    RPI::Model::TEMPOrphanFromDatabase(m_modelAsset.front());
                    // Signal that the model is ready
                    m_onModelReloaded.Signal(m_modelAsset.front());
                    // Remove this reloader from the ModelReloaderSystem
                    ModelReloaderSystemInterface::Get()->RemoveReloader(m_modelAsset.front().GetId());
                    delete this;
                    break;
            }
        }

        ModelReloader::DependencyList& ModelReloader::GetPendingDependencyList()
        {
            switch (m_state)
            {
                case State::WaitingForMeshDependencies:
                    return m_meshDependencies;
                    break;
                case State::WaitingForModelDependencies:
                    return m_modelDependencies;
                    break;
                case State::WaitingForModel:
                default:
                    return m_modelAsset;
                    break;
            }
        }
    } // namespace Render
} // namespace AZ
