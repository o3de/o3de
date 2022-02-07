/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/Shader/ShaderVariantAsyncLoader.h>
#include <Atom/RPI.Public/Shader/Metrics/ShaderMetricsSystem.h>

#include <AzCore/Component/TickBus.h>

#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantAsyncLoader::Init()
        {
            m_isServiceShutdown.store(false);

            AZStd::thread_desc threadDesc;
            threadDesc.m_name = "ShaderVariantAsyncLoader";

            m_serviceThread = AZStd::thread(
                threadDesc,
                [this]()
                {
                    this->ThreadServiceLoop();
                });
        }

        void ShaderVariantAsyncLoader::ThreadServiceLoop()
        {
            AZStd::unordered_set<ShaderVariantAsyncLoader::TupleShaderAssetAndShaderVariantId> newShaderVariantPendingRequests;
            AZStd::unordered_set<Data::AssetId> shaderVariantTreePendingRequests;
            AZStd::unordered_set<Data::AssetId> shaderVariantPendingRequests;
            while (true)
            {
                //We'll wait here until there's work to do or this service has been shutdown.
                {
                    AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                    m_workCondition.wait(lock, [&]
                        {
                            return m_isServiceShutdown.load() ||
                                !m_newShaderVariantPendingRequests.empty() ||
                                !m_shaderVariantTreePendingRequests.empty() ||
                                !m_shaderVariantPendingRequests.empty() ||
                                !newShaderVariantPendingRequests.empty() ||
                                !shaderVariantTreePendingRequests.empty() ||
                                !shaderVariantPendingRequests.empty();
                        }
                    );
                }

                if (m_isServiceShutdown.load())
                {
                    break;
                }

                {
                    //Move pending requests to the local lists.
                    AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);

                    AZStd::for_each(
                        m_newShaderVariantPendingRequests.begin(), m_newShaderVariantPendingRequests.end(),
                        [&](const ShaderVariantAsyncLoader::TupleShaderAssetAndShaderVariantId& tuple) {
                            newShaderVariantPendingRequests.insert(tuple);
                        });
                    m_newShaderVariantPendingRequests.clear();

                    AZStd::for_each(m_shaderVariantTreePendingRequests.begin(), m_shaderVariantTreePendingRequests.end(),
                        [&](const Data::AssetId& assetId)
                        {
                            shaderVariantTreePendingRequests.insert(assetId);
                        });
                    m_shaderVariantTreePendingRequests.clear();

                    AZStd::for_each(m_shaderVariantPendingRequests.begin(), m_shaderVariantPendingRequests.end(),
                        [&](const Data::AssetId& assetId)
                        {
                            shaderVariantPendingRequests.insert(assetId);
                        });
                    m_shaderVariantPendingRequests.clear();
                }

                // Time to work hard.
                auto tupleItor = newShaderVariantPendingRequests.begin();
                while (tupleItor != newShaderVariantPendingRequests.end())
                {
                    auto shaderVariantTreeAsset = GetShaderVariantTreeAsset(tupleItor->m_shaderAsset.GetId());
                    if (shaderVariantTreeAsset)
                    {
                        AZ_Assert(shaderVariantTreeAsset.IsReady(), "shaderVariantTreeAsset is not ready!");
                        // Get the stableId from the variant tree.
                        auto searchResult = shaderVariantTreeAsset->FindVariantStableId(
                            tupleItor->m_shaderAsset->GetShaderOptionGroupLayout(), tupleItor->m_shaderVariantId);
                        if (searchResult.IsRoot())
                        {
                            tupleItor = newShaderVariantPendingRequests.erase(tupleItor);
                            continue;
                        }

                        // Record the request for metrics.
                        ShaderMetricsSystem::Get()->RequestShaderVariant(tupleItor->m_shaderAsset.Get(),  tupleItor->m_shaderVariantId, searchResult);

                        uint32_t shaderVariantProductSubId = ShaderVariantAsset::MakeAssetProductSubId(
                            RHI::Factory::Get().GetAPIUniqueIndex(), tupleItor->m_supervariantIndex.GetIndex(), searchResult.GetStableId());
                        Data::AssetId shaderVariantAssetId(shaderVariantTreeAsset.GetId().m_guid, shaderVariantProductSubId);
                        shaderVariantPendingRequests.insert(shaderVariantAssetId);
                        tupleItor = newShaderVariantPendingRequests.erase(tupleItor);
                        continue;
                    }
                    // If we are here the shaderVariantTreeAsset is not ready, but maybe it is already queued for loading,
                    // but we try to queue it anyways.
                    QueueShaderVariantTreeForLoading(*tupleItor, shaderVariantTreePendingRequests);
                    tupleItor++;
                }


                auto variantTreeItor = shaderVariantTreePendingRequests.begin();
                while (variantTreeItor != shaderVariantTreePendingRequests.end())
                {
                    if (TryToLoadShaderVariantTreeAsset(*variantTreeItor))
                    {
                        variantTreeItor = shaderVariantTreePendingRequests.erase(variantTreeItor);
                    }
                    else
                    {
                        variantTreeItor++;
                    }
                }

                auto variantItor = shaderVariantPendingRequests.begin();
                while (variantItor != shaderVariantPendingRequests.end())
                {
                    if (TryToLoadShaderVariantAsset(*variantItor))
                    {
                        variantItor = shaderVariantPendingRequests.erase(variantItor);
                    }
                    else
                    {
                        variantItor++;
                    }
                }

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1000));
            }
        }

        void ShaderVariantAsyncLoader::Shutdown()
        {
            if (m_isServiceShutdown.load())
            {
                return;
            }

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                m_isServiceShutdown.store(true);
            }

            m_workCondition.notify_one();
            m_serviceThread.join();
            Data::AssetBus::MultiHandler::BusDisconnect();

            m_newShaderVariantPendingRequests.clear();
            m_shaderVariantTreePendingRequests.clear();
            m_shaderVariantPendingRequests.clear();
            m_shaderVariantData.clear();
            m_shaderAssetIdToShaderVariantTreeAssetId.clear();
        }


        ///////////////////////////////////////////////////////////////////
        // IShaderVariantFinder overrides
        bool ShaderVariantAsyncLoader::QueueLoadShaderVariantAssetByVariantId(
            Data::Asset<ShaderAsset> shaderAsset, const ShaderVariantId& shaderVariantId, SupervariantIndex supervariantIndex)
        {
            if (m_isServiceShutdown.load())
            {
                return false;
            }

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                TupleShaderAssetAndShaderVariantId tuple = {shaderAsset, shaderVariantId, supervariantIndex};
                m_newShaderVariantPendingRequests.push_back(tuple);
            }
            m_workCondition.notify_one();
            return true;
        }

        bool ShaderVariantAsyncLoader::QueueLoadShaderVariantAsset(
            const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId, SupervariantIndex supervariantIndex)
        {
            if (m_isServiceShutdown.load())
            {
                return false;
            }

            AZ_Assert(variantStableId != RootShaderVariantStableId, "Root Variants Are Found inside ShaderAssets");

            uint32_t shaderVariantProductSubId =
                ShaderVariantAsset::MakeAssetProductSubId(RHI::Factory::Get().GetAPIUniqueIndex(), supervariantIndex.GetIndex(), variantStableId);
            Data::AssetId shaderVariantAssetId(shaderVariantTreeAssetId.m_guid, shaderVariantProductSubId);
            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                m_shaderVariantPendingRequests.push_back(shaderVariantAssetId);
            }
            m_workCondition.notify_one();
            return true;
        }

        bool ShaderVariantAsyncLoader::QueueLoadShaderVariantTreeAsset(const Data::AssetId& shaderAssetId)
        {
            if (m_isServiceShutdown.load())
            {
                return false;
            }

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                m_shaderVariantTreePendingRequests.push_back(shaderAssetId);
            }
            m_workCondition.notify_one();
            return true;
        }

        Data::Asset<ShaderVariantAsset> ShaderVariantAsyncLoader::GetShaderVariantAssetByVariantId(
            Data::Asset<ShaderAsset> shaderAsset, const ShaderVariantId& shaderVariantId, SupervariantIndex supervariantIndex)
        {
            Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset = GetShaderVariantTreeAsset(shaderAsset.GetId());
            if (!shaderVariantTreeAsset)
            {
                return {};
            }

            // Find the stable id.
            ShaderVariantSearchResult searchResult =
                shaderVariantTreeAsset->FindVariantStableId(shaderAsset->GetShaderOptionGroupLayout(), shaderVariantId);
            if (searchResult.IsRoot())
            {
                return shaderAsset->GetRootVariant();
            }

            // Record the request for metrics.
            ShaderMetricsSystem::Get()->RequestShaderVariant(shaderAsset.Get(), shaderVariantId, searchResult);

            return GetShaderVariantAsset(shaderVariantTreeAsset.GetId(), searchResult.GetStableId(), supervariantIndex);
        }

        Data::Asset<ShaderVariantAsset> ShaderVariantAsyncLoader::GetShaderVariantAssetByStableId(
            Data::Asset<ShaderAsset> shaderAsset, ShaderVariantStableId shaderVariantStableId, SupervariantIndex supervariantIndex)
        {
            AZ_Assert(shaderVariantStableId != RootShaderVariantStableId, "Root Variants Are Found inside ShaderAssets");

            Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset = GetShaderVariantTreeAsset(shaderAsset.GetId());
            if (!shaderVariantTreeAsset)
            {
                return {};
            }

            return GetShaderVariantAsset(shaderVariantTreeAsset.GetId(), shaderVariantStableId, supervariantIndex);
        }

        Data::Asset<ShaderVariantTreeAsset> ShaderVariantAsyncLoader::GetShaderVariantTreeAsset(const Data::AssetId& shaderAssetId)
        {
            AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
            auto assetIdFindIt = m_shaderAssetIdToShaderVariantTreeAssetId.find(shaderAssetId);
            if (assetIdFindIt == m_shaderAssetIdToShaderVariantTreeAssetId.end())
            {
                return {};
            }
            const Data::AssetId& shaderVariantTreeAssetId = assetIdFindIt->second;
            auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
            if (findIt == m_shaderVariantData.end())
            {
                return {};
            }
            ShaderVariantCollection& shaderVariantCollection = findIt->second;
            if (shaderVariantCollection.m_shaderVariantTree.IsReady())
            {
                return shaderVariantCollection.m_shaderVariantTree;
            }
            return {};
        }

        Data::Asset<ShaderVariantAsset> ShaderVariantAsyncLoader::GetShaderVariantAsset(
            const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId, SupervariantIndex supervariantIndex)
        {
            AZ_Assert(variantStableId != RootShaderVariantStableId, "Root Variants Are Found inside ShaderAssets");

            uint32_t shaderVariantProductSubId =
                ShaderVariantAsset::MakeAssetProductSubId(RHI::Factory::Get().GetAPIUniqueIndex(), supervariantIndex.GetIndex(), variantStableId);
            Data::AssetId shaderVariantAssetId(shaderVariantTreeAssetId.m_guid, shaderVariantProductSubId);

            AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
            auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
            if (findIt == m_shaderVariantData.end())
            {
                return {};
            }
            ShaderVariantCollection& shaderVariantCollection = findIt->second;
            auto& shaderVariantsMap = shaderVariantCollection.m_shaderVariantsMap;
            auto variantFindIt = shaderVariantsMap.find(shaderVariantAssetId);
            if (variantFindIt == shaderVariantsMap.end())
            {
                return {};
            }
            Data::Asset<ShaderVariantAsset> shaderVariantAsset = variantFindIt->second;
            if (shaderVariantAsset.IsReady())
            {
                return shaderVariantAsset;
            }
            return {};
        }


        void ShaderVariantAsyncLoader::Reset()
        {
            Shutdown();
            Init();
        }

        ///////////////////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler overrides
        void ShaderVariantAsyncLoader::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            //Cast it to our known asset types.
            if (asset.GetType() == AZ::AzTypeInfo<ShaderVariantTreeAsset>::Uuid())
            {
                OnShaderVariantTreeAssetReady(Data::static_pointer_cast<ShaderVariantTreeAsset>(asset));
                return;
            }

            if (asset.GetType() == AZ::AzTypeInfo<ShaderVariantAsset>::Uuid())
            {
                OnShaderVariantAssetReady(Data::static_pointer_cast<ShaderVariantAsset>(asset));
                return;
            }

            AZ_Error(LogName, false, "Got OnAssetReady for unknown asset type with id=%s and hint=%s\n", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetHint().c_str());
        }

        void ShaderVariantAsyncLoader::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            return OnAssetReady(asset);
        }

        void ShaderVariantAsyncLoader::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            //Cast it to our known asset types.
            if (asset.GetType() == AZ::AzTypeInfo<ShaderVariantTreeAsset>::Uuid())
            {
                OnShaderVariantTreeAssetError(Data::static_pointer_cast<ShaderVariantTreeAsset>(asset));
                return;
            }

            if (asset.GetType() == AZ::AzTypeInfo<ShaderVariantAsset>::Uuid())
            {
                OnShaderVariantAssetError(Data::static_pointer_cast<ShaderVariantAsset>(asset));
                return;
            }

            AZ_Error(LogName, false, "Got OnAssetError for unknown asset type with id=%s and hint=%s\n", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetHint().c_str());
        }
        ///////////////////////////////////////////////////////////////////////

        void ShaderVariantAsyncLoader::OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset)
        {
            // Will be used to address the notification bus.
            Data::AssetId shaderAssetId;

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAsset.GetId());
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    shaderVariantCollection.m_shaderVariantTree = shaderVariantTreeAsset;
                }
                else
                {
                    AZ_Assert(false, "Was expecting a ShaderVariantCollection for shader variant tree asset with id=%s and hint=%s",
                        shaderVariantTreeAsset.GetId().ToString<AZStd::string>().c_str(), shaderVariantTreeAsset.GetHint().c_str());
                    return;
                }
            }

            AZ::TickBus::QueueFunction([shaderAssetId, shaderVariantTreeAsset]()
                {
                    ShaderVariantFinderNotificationBus::Event(
                        shaderAssetId,
                        &ShaderVariantFinderNotification::OnShaderVariantTreeAssetReady, shaderVariantTreeAsset, false /*isError*/);
                });

        }

        void ShaderVariantAsyncLoader::OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> shaderVariantAsset)
        {
            // Will be used to address the notification bus.
            Data::AssetId shaderAssetId;

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                Data::AssetId shaderVariantTreeAssetId(shaderVariantAsset.GetId().m_guid, 0);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    shaderVariantMap.emplace(shaderVariantAsset.GetId(), shaderVariantAsset);
                }
                else
                {
                    AZ_Assert(false, "Was expecting a ShaderVariantCollection for shader variant asset with id=%s and hint=%s",
                        shaderVariantAsset.GetId().ToString<AZStd::string>().c_str(), shaderVariantAsset.GetHint().c_str());
                    return;
                }
            }

            AZ::TickBus::QueueFunction([shaderAssetId, shaderVariantAsset]()
                {
                    ShaderVariantFinderNotificationBus::Event(
                        shaderAssetId,
                        &ShaderVariantFinderNotification::OnShaderVariantAssetReady, shaderVariantAsset, false /*isError*/);
                });
        }

        void ShaderVariantAsyncLoader::OnShaderVariantTreeAssetError(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset)
        {
            // Will be used to address the notification bus.
            Data::AssetId shaderAssetId;

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAsset.GetId());
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    m_shaderVariantData.erase(findIt);
                }
                else
                {
                    AZ_Warning(LogName, false, "Was expecting a ShaderVariantCollection for shader variant tree asset with id=%s and hint=%s",
                        shaderVariantTreeAsset.GetId().ToString<AZStd::string>().c_str(), shaderVariantTreeAsset.GetHint().c_str());
                    return;
                }
            }

            AZ::TickBus::QueueFunction([shaderAssetId, shaderVariantTreeAsset]()
                {
                    ShaderVariantFinderNotificationBus::Event(
                        shaderAssetId,
                        &ShaderVariantFinderNotification::OnShaderVariantTreeAssetReady, shaderVariantTreeAsset, true /*isError*/);
                });
        }

        void ShaderVariantAsyncLoader::OnShaderVariantAssetError(Data::Asset<ShaderVariantAsset> shaderVariantAsset)
        {
            // Will be used to address the notification bus.
            Data::AssetId shaderAssetId;

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                Data::AssetId shaderVariantTreeAssetId(shaderVariantAsset.GetId().m_guid, 0);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    auto shaderVariantFindIt = shaderVariantMap.find(shaderVariantAsset.GetId());
                    if (shaderVariantFindIt != shaderVariantMap.end())
                    {
                        shaderVariantMap.erase(shaderVariantFindIt);
                    }
                }
            }

            AZ::TickBus::QueueFunction([shaderAssetId, shaderVariantAsset]()
                {
                    ShaderVariantFinderNotificationBus::Event(
                        shaderAssetId,
                        &ShaderVariantFinderNotification::OnShaderVariantAssetReady, shaderVariantAsset, true /*isError*/);
                });
        }


        void ShaderVariantAsyncLoader::QueueShaderVariantTreeForLoading(
            const TupleShaderAssetAndShaderVariantId& shaderAndVariantTuple,
            AZStd::unordered_set<Data::AssetId>& shaderVariantTreePendingRequests)
        {
            auto shaderAssetId = shaderAndVariantTuple.m_shaderAsset.GetId();
            if (shaderVariantTreePendingRequests.count(shaderAndVariantTuple.m_shaderAsset.GetId()))
            {
                // Already queued.
                return;
            }

            Data::AssetId shaderVariantTreeAssetId = ShaderVariantTreeAsset::GetShaderVariantTreeAssetIdFromShaderAssetId(shaderAssetId);
            if (!shaderVariantTreeAssetId.IsValid())
            {
                shaderVariantTreePendingRequests.insert(shaderAssetId);
                return;
            }

            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    // Already queued.
                    return;
                }
            }
            shaderVariantTreePendingRequests.insert(shaderAssetId);
        }

        bool ShaderVariantAsyncLoader::TryToLoadShaderVariantTreeAsset(const Data::AssetId& shaderAssetId)
        {
            Data::AssetId shaderVariantTreeAssetId = ShaderVariantTreeAsset::GetShaderVariantTreeAssetIdFromShaderAssetId(shaderAssetId);
            if (!shaderVariantTreeAssetId.IsValid())
            {
                // Return false in hope to retry later.
                return false;
            }

            //If we already loaded such asset let's simply dispatch the notification on the next tick.
            Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset;
            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                m_shaderAssetIdToShaderVariantTreeAssetId.emplace(shaderAssetId, shaderVariantTreeAssetId);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    shaderVariantTreeAsset = findIt->second.m_shaderVariantTree;
                }
            }
            if (shaderVariantTreeAsset.IsReady())
            {
                AZ::TickBus::QueueFunction([shaderAssetId, shaderVariantTreeAsset]()
                    {
                        ShaderVariantFinderNotificationBus::Event(
                            shaderAssetId,
                            &ShaderVariantFinderNotification::OnShaderVariantTreeAssetReady, shaderVariantTreeAsset, false /*isError*/);
                    });
                return true;
            }

            Data::AssetBus::MultiHandler::BusDisconnect(shaderVariantTreeAssetId);

            //Let's queue the asset for loading.
            shaderVariantTreeAsset = Data::AssetManager::Instance().GetAsset<AZ::RPI::ShaderVariantTreeAsset>(shaderVariantTreeAssetId, AZ::Data::AssetLoadBehavior::QueueLoad);
            if (shaderVariantTreeAsset.IsError())
            {
                // The asset doesn't exist in the database yet. Return false in hope to retry later.
                return false;
            }

            // We need to preserve a reference to shaderVariantTreeAsset, otherwise the asset load will be cancelled.
            // This means we have to create the ShaderVariantCollection struct before OnAssetReady is called.
            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderVariantCollection.m_shaderVariantTree = shaderVariantTreeAsset;
                    shaderVariantCollection.m_shaderAssetId = shaderAssetId;
                }
                else
                {
                    m_shaderVariantData.emplace(shaderVariantTreeAssetId, ShaderVariantCollection());
                    ShaderVariantCollection& collection = m_shaderVariantData[shaderVariantTreeAssetId];
                    collection.m_shaderAssetId = shaderAssetId;
                    collection.m_shaderVariantTree = shaderVariantTreeAsset;
                }
            }

            Data::AssetBus::MultiHandler::BusConnect(shaderVariantTreeAssetId);
            return true;
        }

        bool ShaderVariantAsyncLoader::TryToLoadShaderVariantAsset(const Data::AssetId& shaderVariantAssetId)
        {
            // Will be used to address the notification bus.
            Data::AssetId shaderAssetId;

            //If we already loaded such asset let's simply dispatch the notification on the next tick.
            Data::Asset<ShaderVariantAsset> shaderVariantAsset;
            Data::AssetId shaderVariantTreeAssetId(shaderVariantAssetId.m_guid, 0);
            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    auto variantFindIt = shaderVariantMap.find(shaderVariantAssetId);
                    if (variantFindIt != shaderVariantMap.end())
                    {
                        shaderVariantAsset = variantFindIt->second;
                    }
                }
                else
                {
                    AZ_Assert(false, "Looking for a variant without a tree.");
                    return true; //Returning true means the requested asset should be removed from the queue.
                }
            }
            if (shaderVariantAsset.IsReady())
            {
                AZ::TickBus::QueueFunction([shaderAssetId, shaderVariantAsset]()
                    {
                        ShaderVariantFinderNotificationBus::Event(
                            shaderAssetId,
                            &ShaderVariantFinderNotification::OnShaderVariantAssetReady, shaderVariantAsset, false /*isError*/);
                    });
                return true;
            }

            Data::AssetBus::MultiHandler::BusDisconnect(shaderVariantAssetId);

            // Make sure the asset actually exists
            Data::AssetInfo assetInfo;
            Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &Data::AssetCatalogRequestBus::Events::GetAssetInfoById, shaderVariantAssetId);
            if (!assetInfo.m_assetId.IsValid())
            {
                // The asset doesn't exist in the database yet. Return false to retry later.
                return false;
            }

            // Let's queue the asset for loading.
            shaderVariantAsset = Data::AssetManager::Instance().GetAsset<AZ::RPI::ShaderVariantAsset>(shaderVariantAssetId, AZ::Data::AssetLoadBehavior::QueueLoad);
            if (shaderVariantAsset.IsError())
            {
                // The asset exists (we just checked GetAssetInfoById above) but some error occurred.
                return false;
            }

            // We need to preserve a reference to shaderVariantAsset, otherwise the asset load will be cancelled.
            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    shaderVariantMap.emplace(shaderVariantAssetId, shaderVariantAsset);
                }
                else
                {
                    AZ_Assert(false, "Looking for a variant without a tree.");
                    return true; //Returning true means the requested asset should be removed from the queue.
                }
            }

            Data::AssetBus::MultiHandler::BusConnect(shaderVariantAssetId);
            return true;
        }

    } // namespace RPI
} // namespace AZ
