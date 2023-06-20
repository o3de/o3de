/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/Shader/ShaderVariantAsyncLoader.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>

#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RPI
    {
        AZ_CVAR(uint32_t, r_ShaderVariantAsyncLoader_ServiceLoopDelayOverride_ms, 0, nullptr, ConsoleFunctorFlags::Null,
            "Override the delay between iterations of checking for shader variant assets. 0 means use the default value (1000ms).");

        static Data::AssetId GetShaderVariantAssetUuidFromShaderVariantTreeId(const Data::AssetId& shaderVariantTreeAssetId, const AZ::Name& supervariantName, ShaderVariantStableId stableId)
        {
            AZStd::string variantTreeRelativePath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(variantTreeRelativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, shaderVariantTreeAssetId);
            if (variantTreeRelativePath.empty())
            {
                return {};
            }

            AZStd::string shaderName; // Just the file name, no extension, no parent directory.
            if (!AZ::StringFunc::Path::GetFileName(variantTreeRelativePath.c_str(), shaderName))
            {
                return {};
            }

            AZStd::string folderPath;
            if (!AZ::StringFunc::Path::GetFolderPath(variantTreeRelativePath.c_str(), folderPath))
            {
                return {};
            }

            AZStd::string shaderVariantProductPath;

            if (supervariantName.IsEmpty())
            {
                shaderVariantProductPath = AZStd::string::format("%s%s%s_%s_%u.%s",
                    folderPath.c_str(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, shaderName.c_str(),
                    RHI::Factory::Get().GetName().GetCStr(), stableId.GetIndex(), ShaderVariantAsset::Extension);
            }
            else
            {
                shaderVariantProductPath = AZStd::string::format("%s%s%s-%s_%s_%u.%s",
                    folderPath.c_str(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, shaderName.c_str(), supervariantName.GetCStr(),
                    RHI::Factory::Get().GetName().GetCStr(), stableId.GetIndex(), ShaderVariantAsset::Extension);
            }

            Data::AssetId variantAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(variantAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, shaderVariantProductPath.c_str(), AZ::Uuid(), false);

            return variantAssetId;
        }

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

            // first: AssetId of a ShaderVariantAsset
            // second: AssetId of its corresponding ShaderVariantTreeAsset 
            AZStd::vector<AZStd::pair<Data::AssetId, Data::AssetId>> shaderVariantPendingRequests;

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

                    {
                        shaderVariantPendingRequests.reserve(shaderVariantPendingRequests.size() + m_shaderVariantPendingRequests.size());
                        AZStd::move(m_shaderVariantPendingRequests.begin(), m_shaderVariantPendingRequests.end(), AZStd::back_inserter(shaderVariantPendingRequests));
                        m_shaderVariantPendingRequests.clear();
                    }

                }

                // Time to work hard.
                auto tupleItor = newShaderVariantPendingRequests.begin();
                while (tupleItor != newShaderVariantPendingRequests.end())
                {
                    const auto& shaderAsset = tupleItor->m_shaderAsset;
                    auto shaderVariantTreeAsset = GetShaderVariantTreeAsset(shaderAsset.GetId());
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

                        const auto& superVariantName = shaderAsset->GetSupervariantName(tupleItor->m_supervariantIndex);
                        Data::AssetId shaderVariantAssetId = GetShaderVariantAssetUuidFromShaderVariantTreeId(shaderVariantTreeAsset.GetId(), superVariantName, searchResult.GetStableId());
                        if (shaderVariantAssetId.IsValid())
                        {
                            AZStd::pair<Data::AssetId, Data::AssetId> pair(shaderVariantAssetId, shaderVariantTreeAsset.GetId());
                            shaderVariantPendingRequests.emplace_back(AZStd::move(pair));
                            tupleItor = newShaderVariantPendingRequests.erase(tupleItor);
                        }
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

                auto assetIdTupleItor = shaderVariantPendingRequests.begin();
                while (assetIdTupleItor != shaderVariantPendingRequests.end())
                {
                    if (TryToLoadShaderVariantAsset(assetIdTupleItor->first, assetIdTupleItor->second))
                    {
                        assetIdTupleItor = shaderVariantPendingRequests.erase(assetIdTupleItor);
                    }
                    else
                    {
                        assetIdTupleItor++;
                    }
                }

                AZStd::chrono::milliseconds delay{1000};
                if (r_ShaderVariantAsyncLoader_ServiceLoopDelayOverride_ms)
                {
                    delay = AZStd::chrono::milliseconds{r_ShaderVariantAsyncLoader_ServiceLoopDelayOverride_ms};
                }
                AZStd::this_thread::sleep_for(delay);
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
            m_shaderVariantAssetIdToShaderVariantTreeAssetId.clear();
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
            const Data::AssetId& shaderVariantTreeAssetId, ShaderVariantStableId variantStableId, const AZ::Name& supervariantName)
        {
            if (m_isServiceShutdown.load())
            {
                return false;
            }

            AZ_Assert(variantStableId != RootShaderVariantStableId, "Root Variants Are Found inside ShaderAssets");

            Data::AssetId shaderVariantAssetId = GetShaderVariantAssetUuidFromShaderVariantTreeId(shaderVariantTreeAssetId, supervariantName, variantStableId);
            {
                AZStd::pair<Data::AssetId, Data::AssetId> pair(shaderVariantAssetId, shaderVariantTreeAssetId);
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                m_shaderVariantPendingRequests.emplace_back(AZStd::move(pair));
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
                return shaderAsset->GetRootVariantAsset();
            }

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

            uint32_t subId = ShaderVariantAsset::MakeAssetProductSubId(RHI::Factory::Get().GetAPIUniqueIndex(), supervariantIndex.GetIndex(), variantStableId);
            ShaderVariantProductSubId shaderVariantProductSubId(subId);

            AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
            auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
            if (findIt == m_shaderVariantData.end())
            {
                return {};
            }
            ShaderVariantCollection& shaderVariantCollection = findIt->second;
            auto& shaderVariantsMap = shaderVariantCollection.m_shaderVariantsMap;
            auto variantFindIt = shaderVariantsMap.find(shaderVariantProductSubId);
            if (variantFindIt == shaderVariantsMap.end())
            {
                return {};
            }
            Data::Asset<ShaderVariantAsset> shaderVariantAsset = variantFindIt->second;
            if (shaderVariantAsset.IsReady())
            {
                Data::Asset<ShaderVariantAsset> registeredShaderVariantAsset =
                    AZ::Data::AssetManager::Instance().FindAsset<ShaderVariantAsset>(shaderVariantAsset.GetId(), AZ::Data::AssetLoadBehavior::NoLoad);
                if (!registeredShaderVariantAsset.GetId().IsValid())
                {
                    // The shader variant was removed from the asset database, this would normally happen when the source .shadervariantlist file
                    // is changed to remove a particular variant. Since it should no longer be available for use, remove it from the local map.
                    // Note that if we don't handle this special case, the AssetManager will fail to report OnAssetReady if/when this asset appears
                    // again, which might be a bug in the asset system.
                    shaderVariantsMap.erase(variantFindIt);
                    return {};
                }

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

                Data::AssetId shaderVariantTreeAssetId;
                auto findTreeItor = m_shaderVariantAssetIdToShaderVariantTreeAssetId.find(shaderVariantAsset.GetId());
                if (findTreeItor != m_shaderVariantAssetIdToShaderVariantTreeAssetId.end())
                {
                    shaderVariantTreeAssetId = findTreeItor->second;
                }
                else
                {
                    AZ_Assert(false, "Was expecting to have a ShaderVariantTreeAsset Id for shader variant asset with id=%s and hint=%s",
                        shaderVariantAsset.GetId().ToString<AZStd::string>().c_str(), shaderVariantAsset.GetHint().c_str());
                    return;
                }

                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    ShaderVariantProductSubId subId(shaderVariantAsset.GetId().m_subId);
                    shaderVariantMap.emplace(subId, shaderVariantAsset);
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
                Data::AssetId shaderVariantTreeAssetId;
                auto findTreeItor = m_shaderVariantAssetIdToShaderVariantTreeAssetId.find(shaderVariantAsset.GetId());
                if (findTreeItor != m_shaderVariantAssetIdToShaderVariantTreeAssetId.end())
                {
                    shaderVariantTreeAssetId = findTreeItor->second;
                }
                else
                {
                    AZ_Assert(false, "Was expecting to have a ShaderVariantTreeAsset Id for shader variant asset with id=%s and hint=%s",
                        shaderVariantAsset.GetId().ToString<AZStd::string>().c_str(), shaderVariantAsset.GetHint().c_str());
                    return;
                }

                m_shaderVariantAssetIdToShaderVariantTreeAssetId.erase(findTreeItor);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    ShaderVariantProductSubId subId(shaderVariantAsset.GetId().m_subId);
                    auto shaderVariantFindIt = shaderVariantMap.find(subId);
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

        bool ShaderVariantAsyncLoader::TryToLoadShaderVariantAsset(const Data::AssetId& shaderVariantAssetId, const Data::AssetId& shaderVariantTreeAssetId)
        {
            // Will be used to address the notification bus.
            Data::AssetId shaderAssetId;

            //If we already loaded such asset let's simply dispatch the notification on the next tick.
            Data::Asset<ShaderVariantAsset> shaderVariantAsset;
            {
                AZStd::unique_lock<decltype(m_mutex)> lock(m_mutex);
                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    shaderAssetId = shaderVariantCollection.m_shaderAssetId;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    ShaderVariantProductSubId subId(shaderVariantAssetId.m_subId);
                    auto variantFindIt = shaderVariantMap.find(subId);
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

                // Keep a record. This will make it easy to find the ShaderVariantTreeAssset when OnAssetReady(), OnAssetReloaded(), etc
                // are called on behalf the ShaderVariantAsset,
                m_shaderVariantAssetIdToShaderVariantTreeAssetId.emplace(shaderVariantAssetId, shaderVariantTreeAssetId);

                auto findIt = m_shaderVariantData.find(shaderVariantTreeAssetId);
                if (findIt != m_shaderVariantData.end())
                {
                    ShaderVariantCollection& shaderVariantCollection = findIt->second;
                    auto& shaderVariantMap = shaderVariantCollection.m_shaderVariantsMap;
                    ShaderVariantProductSubId subId(shaderVariantAssetId.m_subId);
                    shaderVariantMap.emplace(subId, shaderVariantAsset);
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
