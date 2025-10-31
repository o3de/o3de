/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Render
    {
        // AsyncLoadTracker is for use by Feature Processors to track in-flight loading of assets that their sub-objects need.
        // For instance, the individual decals that are controlled by the decal FeatureProcessor will need materials to be loaded in asynchronously before use.
        template<typename FeatureProcessorHandle>
        class AsyncLoadTracker
        {
        public:

            using MaterialAssetPtr = AZ::Data::Asset<AZ::RPI::MaterialAsset>;

            void TrackAssetLoad(const FeatureProcessorHandle handle, const MaterialAssetPtr asset)
            {
                if (IsAssetLoading(handle))
                {
                    // We might have a case where a decal was told to load an asset, then while the load was being fulfilled, it was told to
                    // switch to a different asset. That is why we are removing the existing handle rather than just returning.
                    RemoveHandle(handle);
                }
                Add(handle, asset);
            }

            bool IsAssetLoading(const AZ::Data::AssetId asset) const
            {
                return m_inFlightHandlesByAsset.count(asset) > 0;
            }

            bool IsAssetLoading(const FeatureProcessorHandle handle) const
            {
                return m_inFlightHandles.count(handle) > 0;
            }

            AZStd::vector<FeatureProcessorHandle> GetHandlesByAsset(const AZ::Data::AssetId asset) const
            {
                const auto iter = m_inFlightHandlesByAsset.find(asset);
                if (iter != m_inFlightHandlesByAsset.end())
                {
                    return iter->second;
                }
                else
                {
                    return {};
                }
            }

            void RemoveAllHandlesWithAsset(const AZ::Data::AssetId asset)
            {
                const auto iter = m_inFlightHandlesByAsset.find(asset);
                if (iter == m_inFlightHandlesByAsset.end())
                {
                    return;
                }

                auto& handleList = iter->second;
                for (auto& handle : handleList)
                {
                    EraseFromInFlightHandles(handle);
                }
                m_inFlightHandlesByAsset.erase(iter);
            }

            void RemoveHandle(const FeatureProcessorHandle handle)
            {
                const auto asset = EraseFromInFlightHandles(handle);

                AZ_Assert(m_inFlightHandlesByAsset.count(asset.GetId()) > 0, "AsyncLoadTracker in a bad state");
                auto& handleList = m_inFlightHandlesByAsset[asset.GetId()];
                EraseFromVector(handleList, handle);
                if (handleList.empty())
                {
                    m_inFlightHandlesByAsset.erase(asset.GetId());
                }
            }

            bool AreAnyLoadsInFlight() const
            {
                return !m_inFlightHandles.empty();
            }

        private:

            // Helper function that erases from an AZStd::vector via swap() and pop_back()
            template<typename T, typename U>
            static void EraseFromVector(AZStd::vector<T>& vec, const U elementToErase)
            {
                const auto iter = AZStd::find(vec.begin(), vec.end(), elementToErase);
                AZ_Assert(iter != vec.end(), "EraseFromVector failed to find the given object");
                AZStd::swap(*iter, vec.back());
                vec.pop_back();
            }

            void Add(const FeatureProcessorHandle handle, const MaterialAssetPtr asset)
            {
                AZ_Assert(m_inFlightHandles.count(handle) == 0, "AsyncLoadTracker::Add() - told to add a handle that was already being tracked.");
                m_inFlightHandlesByAsset[asset.GetId()].push_back(handle);
                m_inFlightHandles[handle] = asset;
            }

            MaterialAssetPtr EraseFromInFlightHandles(const FeatureProcessorHandle handle)
            {
                const auto iter = m_inFlightHandles.find(handle);
                AZ_Assert(iter != m_inFlightHandles.end(), "Told to remove handle that was not present");
                const auto asset = iter->second;
                m_inFlightHandles.erase(iter);
                return asset;
            }

            // Tracks all objects that need a particular asset
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<FeatureProcessorHandle>> m_inFlightHandlesByAsset;

            // Hash table that tracks the reverse of the m_inFlightHandlesByAsset hash table.
            // i.e. for each object, it stores what asset that it needs.
            AZStd::unordered_map<FeatureProcessorHandle, MaterialAssetPtr> m_inFlightHandles;
        };
    }
}
