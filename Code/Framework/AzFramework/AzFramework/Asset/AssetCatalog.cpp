/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetCatalog.h"

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/wildcard.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

// uncomment to have the catalog be dumped to stdout:
//#define DEBUG_DUMP_CATALOG


namespace AzFramework
{
    //=========================================================================
    // AssetCatalog ctor
    //=========================================================================
    AssetCatalog::AssetCatalog()
        : m_shutdownThreadSignal(false)
        , m_registry(aznew AssetRegistry())
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        // We need to start listening immediately - Updates are not queued
        StartMonitoringAssets();
    }

    AssetCatalog::AssetCatalog(bool directConnections)
        : m_shutdownThreadSignal(false)
        , m_registry(aznew AssetRegistry())
        , m_directConnections(directConnections)
    {
        if (m_directConnections)
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
            // We need to start listening immediately - Updates are not queued
            StartMonitoringAssets();
        }
    }

    //=========================================================================
    // AssetCatalog dtor
    //=========================================================================
    AssetCatalog::~AssetCatalog()
    {
        AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        DisableCatalog();
        Reset();
    }

    //=========================================================================
    // Reset
    //=========================================================================
    void AssetCatalog::Reset()
    {
        StopMonitoringAssets();

        ClearCatalog();
    }

    //=========================================================================
    // EnableCatalog
    //=========================================================================
    void AssetCatalog::EnableCatalogForAsset(const AZ::Data::AssetType& assetType)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().RegisterCatalog(this, assetType);
    }

    //=========================================================================
    // GetHandledAssetTypes
    //=========================================================================
    void AssetCatalog::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready.");
        AZ::Data::AssetManager::Instance().GetHandledAssetTypes(this, assetTypes);
    }

    //=========================================================================
    // GetAssetTypeByDisplayName
    //=========================================================================
    AZ::Data::AssetType AssetCatalog::GetAssetTypeByDisplayName(const AZStd::string_view displayName)
    {
        AZStd::vector<AZ::Data::AssetType> assetTypes;
        GetHandledAssetTypes(assetTypes);

        for (auto assetType : assetTypes)
        {
            AZStd::string assetTypeName = "";
            AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

            if (assetTypeName == displayName)
            {
                return assetType;
            }
        }

        return AZ::Data::AssetType::CreateNull();
    }

    //=========================================================================
    // DisableCatalog
    //=========================================================================
    void AssetCatalog::DisableCatalog()
    {
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
        }
    }

    //=========================================================================
    // AddExtension
    //=========================================================================
    void AssetCatalog::AddExtension(const char* extension)
    {
        AZStd::string extensionFixed = extension;
        EBUS_EVENT(ApplicationRequests::Bus, NormalizePath, extensionFixed);

        if (extensionFixed.empty())
        {
            AZ_Error("AssetCatalog", false, "Invalid extension provided: %s", extension);
            return;
        }

        if (extensionFixed[0] == '.')
        {
            extensionFixed = extensionFixed.substr(1);
        }

        EBUS_EVENT(ApplicationRequests::Bus, NormalizePath, extensionFixed);

        if (m_extensions.end() == AZStd::find(m_extensions.begin(), m_extensions.end(), extensionFixed))
        {
            m_extensions.insert(AZStd::move(extensionFixed));
        }
    }

    //=========================================================================
    // GetAssetPathById
    //=========================================================================
    AZStd::string AssetCatalog::GetAssetPathById(const AZ::Data::AssetId& id)
    {
        return GetAssetPathByIdInternal(id);
    }


    //=========================================================================
    // GetAssetPathByIdInternal
    //=========================================================================
    AZStd::string AssetCatalog::GetAssetPathByIdInternal(const AZ::Data::AssetId& id) const
    {
        if (!id.IsValid())
        {
            return AZStd::string();
        }

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        auto foundIter = m_registry->m_assetIdToInfo.find(id);
        if (foundIter != m_registry->m_assetIdToInfo.end())
        {
            return foundIter->second.m_relativePath;
        }

        // we did not find it - try the backup mapping!
        AZ::Data::AssetId legacyMapping = m_registry->GetAssetIdByLegacyAssetId(id);
        if (legacyMapping.IsValid())
        {
            return GetAssetPathByIdInternal(legacyMapping);
        }

        return AZStd::string();
    }

    //=========================================================================
    // GetAssetInfoById
    //=========================================================================
    AZ::Data::AssetInfo AssetCatalog::GetAssetInfoById(const AZ::Data::AssetId& id)
    {
        return GetAssetInfoByIdInternal(id);
    }

    //=========================================================================
    // GetAssetInfoByIdInternal
    //=========================================================================
    AZ::Data::AssetInfo AssetCatalog::GetAssetInfoByIdInternal(const AZ::Data::AssetId& id) const
    {
        if (!id.IsValid())
        {
            return AZ::Data::AssetInfo();
        }

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        auto foundIter = m_registry->m_assetIdToInfo.find(id);
        if (foundIter != m_registry->m_assetIdToInfo.end())
        {
            return foundIter->second;
        }

        // we did not find it - try the backup mapping!
        AZ::Data::AssetId legacyMapping = m_registry->GetAssetIdByLegacyAssetId(id);
        if (legacyMapping.IsValid())
        {
            return GetAssetInfoByIdInternal(legacyMapping);
        }

        return AZ::Data::AssetInfo();
    }

    //=========================================================================
    // GetAssetIdByPath
    //=========================================================================
    AZ::Data::AssetId AssetCatalog::GetAssetIdByPath(const char* path, const AZ::Data::AssetType& typeToRegister, bool autoRegisterIfNotFound)
    {
        if (!path || !path[0])
        {
            return AZ::Data::AssetId();
        }
        m_pathBuffer = path;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, m_pathBuffer);
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            AZ::Data::AssetId foundId = m_registry->GetAssetIdByPath(m_pathBuffer.c_str());
            if (foundId.IsValid())
            {
                const AZ::Data::AssetInfo& assetInfo = m_registry->m_assetIdToInfo.find(foundId)->second;

                // If the type is already registered, but with no valid type, allow it to be re-registered.
                // Otherwise, return the Id.
                if (!autoRegisterIfNotFound || !assetInfo.m_assetType.IsNull())
                {
                    return foundId;
                }
            }
        }

        if (autoRegisterIfNotFound)
        {
            AZ_Error("AssetCatalog", typeToRegister != AZ::Data::s_invalidAssetType,
                "Invalid asset type provided for registration of asset \"%s\".", m_pathBuffer.c_str());

            // note, we are intentionally allowing missing files, since this is an explicit ask to add.
            AZ::u64 fileSize = 0;
            auto* fileIo = AZ::IO::FileIOBase::GetInstance();
            if (fileIo->Exists(m_pathBuffer.c_str()))
            {
                fileIo->Size(m_pathBuffer.c_str(), fileSize);
            }

            AZ::Data::AssetInfo newInfo;
            newInfo.m_relativePath = m_pathBuffer;
            newInfo.m_assetType = typeToRegister;
            newInfo.m_sizeBytes = fileSize;
            AZ::Data::AssetId generatedID = GenerateAssetIdTEMP(newInfo.m_relativePath.c_str());
            newInfo.m_assetId = generatedID;

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
                m_registry->RegisterAsset(generatedID, newInfo);
            }

            EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetAdded, generatedID);

            return generatedID;
        }

        return AZ::Data::AssetId();
    }


    AZStd::vector<AZStd::string> AssetCatalog::GetRegisteredAssetPaths()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        AZStd::vector<AZStd::string> registeredAssetPaths;
        for (auto assetIdToInfoPair : m_registry->m_assetIdToInfo)
        {
            registeredAssetPaths.emplace_back(assetIdToInfoPair.second.m_relativePath);
        }

        return registeredAssetPaths;
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetDirectProductDependencies(const AZ::Data::AssetId& id)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
        auto itr = m_registry->m_assetDependencies.find(id);

        if (itr == m_registry->m_assetDependencies.end())
        {
            return AZ::Failure<AZStd::string>("Failed to find asset in dependency map");
        }

        return AZ::Success(itr->second);
    }
    
    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetAllProductDependencies(const AZ::Data::AssetId& id)
    {
        return GetAllProductDependenciesFilter(id, {}, {});
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetAllProductDependenciesFilter(const AZ::Data::AssetId& id, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList)
    {
        AZStd::vector<AZ::Data::ProductDependency> dependencyList;
        AZStd::unordered_set<AZ::Data::AssetId> assetSet;
        AZ::Data::PreloadAssetListType preloadList;
        if (exclusionList.find(id) != exclusionList.end())
        {
            return AZ::Success(AZStd::move(dependencyList));
        }

        for (const AZStd::string& wildcardPattern : wildcardPatternExclusionList)
        {
            if (DoesAssetIdMatchWildcardPattern(id, wildcardPattern))
            {
                return AZ::Success(AZStd::move(dependencyList));
            }
        }

        AddAssetDependencies(id, assetSet, dependencyList, exclusionList, wildcardPatternExclusionList, preloadList);

        // dependencyList will be appended to while looping, so use a traditional loop
        for (size_t i = 0; i < dependencyList.size(); ++i)
        {
            // Copy the asset Id out of the list into a temp variable before passing it in.  AddAssetDependencies will modify
            // dependencyList, which can cause reallocations of the list, so the reference to a specific entry might not remain
            // valid throughout the entire call.
            AZ::Data::AssetId searchId = dependencyList[i].m_assetId;
            AddAssetDependencies(searchId, assetSet, dependencyList, exclusionList, wildcardPatternExclusionList, preloadList);
        }

        return AZ::Success(AZStd::move(dependencyList));
    }

    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> AssetCatalog::GetLoadBehaviorProductDependencies(const AZ::Data::AssetId& id, AZStd::unordered_set<AZ::Data::AssetId>& noloadSet, AZ::Data::PreloadAssetListType& preloadAssetList)
    {
        AZStd::vector<AZ::Data::ProductDependency> dependencyList;
        AZStd::vector<AZ::Data::ProductDependency> returnList;
        AZStd::unordered_set<AZ::Data::AssetId> assetSet;

        AddAssetDependencies(id, assetSet, dependencyList, {}, {}, preloadAssetList);

        // dependencyList will be appended to while looping, so use a traditional loop
        for (size_t i = 0; i < dependencyList.size(); ++i)
        {
            if (AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(dependencyList[i].m_flags) == AZ::Data::AssetLoadBehavior::NoLoad)
            {
                noloadSet.insert(dependencyList[i].m_assetId);
                assetSet.erase(dependencyList[i].m_assetId);
            }
            else
            {
                returnList.push_back(dependencyList[i]);
                // Copy the asset Id out of the list into a temp variable before passing it in.  AddAssetDependencies will modify
                // dependencyList, which can cause reallocations of the list, so the reference to a specific entry might not remain
                // valid throughout the entire call.
                AZ::Data::AssetId searchId = dependencyList[i].m_assetId;
                AddAssetDependencies(searchId, assetSet, dependencyList, {}, {}, preloadAssetList);
            }
        }

        return AZ::Success(AZStd::move(returnList));
    }

    bool AssetCatalog::DoesAssetIdMatchWildcardPatternInternal(const AZ::Data::AssetId& assetId, const AZStd::string& wildcardPattern) const
    {
        if (wildcardPattern.empty())
        {
            // pattern is empty, there is nothing to match
            return false;
        }

        AZStd::string relativePath = GetAssetPathByIdInternal(assetId);
        if (relativePath.empty())
        {
            // assetId did not resolve to a relative path, cannot be matched
            return false;
        }

        return AZStd::wildcard_match(wildcardPattern, relativePath);
    }

    bool AssetCatalog::DoesAssetIdMatchWildcardPattern(const AZ::Data::AssetId& assetId, const AZStd::string& wildcardPattern) 
    {
        return DoesAssetIdMatchWildcardPatternInternal(assetId, wildcardPattern);
    }

    void AssetCatalog::AddAssetDependencies(
        const AZ::Data::AssetId& searchAssetId,
        AZStd::unordered_set<AZ::Data::AssetId>& assetSet,
        AZStd::vector<AZ::Data::ProductDependency>& dependencyList,
        const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
        const AZStd::vector<AZStd::string>& wildcardPatternExclusionList,
        AZ::Data::PreloadAssetListType& preloadAssetList) const
    {
        using namespace AZ::Data;

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
        auto itr = m_registry->m_assetDependencies.find(searchAssetId);

        if (itr != m_registry->m_assetDependencies.end())
        {
            AZStd::vector<ProductDependency>& assetDependencyList = itr->second;

            for (const ProductDependency& dependency : assetDependencyList)
            {
                if (!dependency.m_assetId.IsValid())
                {
                    continue;
                }

                if(exclusionList.find(dependency.m_assetId) != exclusionList.end())
                {
                    continue;
                }

                bool isWildcardMatch = false;
                for (const AZStd::string& wildcardPattern : wildcardPatternExclusionList)
                {
                    isWildcardMatch = DoesAssetIdMatchWildcardPatternInternal(dependency.m_assetId, wildcardPattern);
                    if (isWildcardMatch)
                    {
                        break;
                    }
                }
                if (isWildcardMatch)
                {
                    continue;
                }

                auto loadBehavior = AZ::Data::ProductDependencyInfo::LoadBehaviorFromFlags(dependency.m_flags);
                if (loadBehavior == AZ::Data::AssetLoadBehavior::PreLoad)
                {
                    preloadAssetList[searchAssetId].insert(dependency.m_assetId);
                }

                // Only proceed if this ID is valid and we haven't encountered this assetId before.
                // Invalid IDs usually come from unmet path product dependencies.
                if (assetSet.find(dependency.m_assetId) == assetSet.end())
                {
                    assetSet.insert(dependency.m_assetId); // add to the set of already-encountered assets
                    dependencyList.push_back(dependency); // put it in the flat list of dependencies we've found
                }
            }
        }
    }

    //=========================================================================
    // GenerateAssetIdTEMP
    //=========================================================================
    AZ::Data::AssetId AssetCatalog::GenerateAssetIdTEMP(const char* path)
    {
        if ((!path) || (path[0] == 0))
        {
            return AZ::Data::AssetId();
        }
        AZStd::string relativePath = path;
        EBUS_EVENT(ApplicationRequests::Bus, MakePathAssetRootRelative, relativePath);
        return AZ::Data::AssetId(AZ::Uuid::CreateName(relativePath.c_str()));
    }

    //=========================================================================
    // EnumerateAssets
    //=========================================================================
    void AssetCatalog::EnumerateAssets(BeginAssetEnumerationCB beginCB, AssetEnumerationCB enumerateCB, EndAssetEnumerationCB endCB)
    {
        if (beginCB)
        {
            beginCB();
        }

        if (enumerateCB)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            for (auto& it : m_registry->m_assetIdToInfo)
            {
                enumerateCB(it.first, it.second);
            }
        }

        if (endCB)
        {
            endCB();
        }
    }

    //=========================================================================
    // InitializeCatalog
    //=========================================================================
    void AssetCatalog::InitializeCatalog(const char* catalogRegistryFile /*= nullptr*/)
    {
        bool shouldBroadcast = false;
        {
            // this scope controls the below lock guard, do not remove this scope.  
            // the lock must expire before we send out notifications to other systems.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            // Get asset root from application.
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Get(m_assetRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
            }

            // Reflect registry for serialization.
            AZ::SerializeContext* serializeContext = nullptr;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
            if (nullptr == serializeContext->FindClassData(AZ::AzTypeInfo<AssetRegistry>::Uuid()))
            {
                AssetRegistry::ReflectSerialize(serializeContext);
            }

            AZ_TracePrintf("AssetCatalog", "Initializing asset catalog with root \"%s\"", m_assetRoot.c_str());

            // even though this could be a chunk of memory to allocate and deallocate, this is many times faster and more efficient
            // in terms of memory AND fragmentation than allowing it to perform thousands of reads on physical media.
            AZStd::vector<char> bytes;
            if (catalogRegistryFile && AZ::IO::FileIOBase::GetInstance())
            {
                AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
                AZ::u64 size = 0;
                AZ::IO::FileIOBase::GetInstance()->Size(catalogRegistryFile, size);

                if (size)
                {
                    if (AZ::IO::FileIOBase::GetInstance()->Open(catalogRegistryFile, AZ::IO::OpenMode::ModeRead, handle))
                    {
                        bytes.resize_no_construct(size);
                        // this call will fail on purpose if bytes.size() != size successfully actually read from disk.
                        if (!AZ::IO::FileIOBase::GetInstance()->Read(handle, bytes.data(), bytes.size(), true))
                        {
                            AZ_Error("AssetCatalog", false, "File %s failed read - read was truncated!", catalogRegistryFile);
                            bytes.set_capacity(0);
                        }
                        AZ::IO::FileIOBase::GetInstance()->Close(handle);
                    }
                }
            }

            if (!bytes.empty())
            {
                AZStd::shared_ptr<AzFramework::AssetRegistry> prevRegistry;
                if (!m_initialized)
                {
                    // First time initialization may have updates already processed which we want to apply
                    prevRegistry = AZStd::move(m_registry);
                    m_registry.reset(aznew AssetRegistry());
                }
                AZ::IO::MemoryStream catalogStream(bytes.data(), bytes.size());
#if (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)
                ApplicationRequests::Bus::Broadcast(&ApplicationRequests::PumpSystemEventLoopWhileDoingWorkInNewThread,
                    AZStd::chrono::milliseconds(AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING_INTERVAL_MS),
                    [this, &catalogStream, &serializeContext]
                    {
                        AZ::Utils::LoadObjectFromStreamInPlace<AzFramework::AssetRegistry>(catalogStream, *m_registry.get(), serializeContext, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading));
                    },
                        "Asset Catalog Loading Thread"
                        );
#else
                AZ::Utils::LoadObjectFromStreamInPlace<AzFramework::AssetRegistry>(catalogStream, *m_registry.get(), serializeContext, AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading));
#endif // (AZ_TRAIT_PUMP_SYSTEM_EVENTS_WHILE_LOADING)

                AZ_TracePrintf("AssetCatalog", "Loaded registry containing %u assets.\n", m_registry->m_assetIdToInfo.size());

                // It's currently possible in tools for us to have received updates from AP which were applied before the catalog was ready to load
                if (!m_initialized)
                {
                    ApplyDeltaCatalog(prevRegistry);
                    m_initialized = true;
                }
                shouldBroadcast = true;
            }
            else
            {
                AZ_ErrorOnce("AssetCatalog", false, "Unable to load the asset catalog from %s!", catalogRegistryFile);
            }
        }

        // Send notification to other systems after expiring the lock guard above.
        if (shouldBroadcast)
        {
            // Queue this instead of calling it immediately.  The InitializeCatalog() call was likely called via the AssetCatalogRequestBus,
            // locking the mutex on that bus.  If OnCatalogLoaded is immediately triggered, any listeners responding to it will also be within
            // the mutex.  If the listener tries to perform a blocking asset load via GetAsset() / BlockUntilLoadComplete(), the spawned asset
            // thread will make a call to the AssetCatalogRequestBus and block on the held mutex.  This would cause a deadlock, since the listener
            // won't free the mutex until the load is complete.
            // So instead, queue the notification until after the AssetCatalogRequestBus mutex is unlocked for the current thread, and also
            // so that the entire AssetCatalog initialization is complete.
            auto OnCatalogLoaded = [catalogRegistryString = AZStd::string(catalogRegistryFile)]()
            {
                AssetCatalogEventBus::Broadcast(&AssetCatalogEventBus::Events::OnCatalogLoaded, catalogRegistryString.c_str());
            };
            AZ::Data::AssetCatalogRequestBus::QueueFunction(AZStd::move(OnCatalogLoaded));
        }
    }

    //=========================================================================
    // IsTrackedAssetType
    //=========================================================================
    bool AssetCatalog::IsTrackedAssetType(const char* assetFilename) const
    {
        AZStd::string assetExtension;
        AzFramework::StringFunc::Path::GetExtension(assetFilename, assetExtension, false);

        if (assetExtension.empty())
        {
            return false;
        }

        return (m_extensions.find(assetExtension) != m_extensions.end());
    }

    //=========================================================================
    // RegisterAsset
    //=========================================================================
    void AssetCatalog::RegisterAsset(const AZ::Data::AssetId& id, AZ::Data::AssetInfo& info)
    {
        AZ_Error("AssetCatalog", id != AZ::Data::s_invalidAssetType, "Registering asset \"%s\" with invalid type.", info.m_relativePath.c_str());

        info.m_assetId = id;
        if (info.m_sizeBytes == 0)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            fileIO->Size(info.m_relativePath.c_str(), info.m_sizeBytes);
        }
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
            m_registry->RegisterAsset(id, info);
        }
        EBUS_EVENT(AzFramework::AssetCatalogEventBus, OnCatalogAssetAdded, id);
    }

    //=========================================================================
    // UnregisterAsset
    //=========================================================================
    void AssetCatalog::UnregisterAsset(const AZ::Data::AssetId& assetId)
    {
        if (assetId.IsValid())
        {
            AZ::Data::AssetInfo assetInfo = GetAssetInfoById(assetId);

            AZ::TickBus::QueueFunction([assetId, assetInfo = AZStd::move(assetInfo)]()
            {
                AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetRemoved, assetId, assetInfo);
            });

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
            m_registry->UnregisterAsset(assetId);
        }
    }

    //=========================================================================
    // GetStreamInfoForLoad
    //=========================================================================
    AZ::Data::AssetStreamInfo AssetCatalog::GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        (void)assetType;

        AZ::Data::AssetInfo info;
        {
            // narrow-scoped lock.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
            info = GetAssetInfoById(assetId);
        }

        if (!info.m_relativePath.empty())
        {
            AZ::Data::AssetStreamInfo streamInfo;
            streamInfo.m_dataLen = info.m_sizeBytes;

            // All asset handlers are presently required to use the LoadAssetData override
            // that takes a stream.
            streamInfo.m_streamName = info.m_relativePath;
            streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary;

            return streamInfo;
        }

        return AZ::Data::AssetStreamInfo();
    }

    //=========================================================================
    // GetStreamInfoForSave
    //=========================================================================
    AZ::Data::AssetStreamInfo AssetCatalog::GetStreamInfoForSave(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        (void)assetType;

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            AZ::Data::AssetInfo info;
            {
                // narrow-scoped lock.
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
                info = GetAssetInfoById(assetId);
            }


            if (!info.m_relativePath.empty())
            {
                const char* devAssetRoot = fileIO->GetAlias("@projectroot@");
                if (devAssetRoot)
                {
                    AZ::Data::AssetStreamInfo streamInfo;
                    streamInfo.m_dataLen = info.m_sizeBytes;
                    streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
                    streamInfo.m_streamName = AZStd::string::format("%s/%s", devAssetRoot, info.m_relativePath.c_str());
                    return streamInfo;
                }
            }
        }

        return AZ::Data::AssetStreamInfo();
    }

    //=========================================================================
    // StartMonitoringAssetRoot
    //=========================================================================
    void AssetCatalog::StartMonitoringAssets()
    {
        AZStd::lock_guard<AZStd::mutex> m_monitorLock{ m_monitorMutex };
        if (m_monitoring)
        {
            return;
        }
        m_monitoring = true;
        if (m_directConnections)
        {
            AZ::Interface<AssetSystem::NetworkAssetUpdateInterface>::Register(this);
        }
    }

    //=========================================================================
    // StopMonitoringAssetRoot
    //=========================================================================
    void AssetCatalog::StopMonitoringAssets()
    {
        AZStd::lock_guard<AZStd::mutex> m_monitorLock{ m_monitorMutex };
        if (!m_monitoring)
        {
            return;
        }
        m_monitoring = false;

        if (m_directConnections)
        {
            AZ::Interface<AssetSystem::NetworkAssetUpdateInterface>::Unregister(this);
        }
    }

    //=========================================================================
    // NetworkAssetUpdateInterface
    //=========================================================================
    // note that this function is called from within a network job thread in order to update
    // the catalog as soon as new data is available, before other systems try to reach in and query
    // the results.  For this reason, it is a direct call and protects its structures with a lock.
    void AssetCatalog::AssetChanged(AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        AZStd::string relativePath = message.m_data;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, relativePath);

        AZ::Data::AssetId assetId = message.m_assetId;

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(relativePath.c_str(), extension, false);
        if (assetId.IsValid())
        {
            bool isNewAsset = false;
            {
                // this scope controls the below lock guard, do not remove this scope.  
                // the lock must expire before we send out notifications to other systems.
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

                // is it an add or a change?
                auto assetInfoPair = m_registry->m_assetIdToInfo.find(assetId);
                isNewAsset = (assetInfoPair == m_registry->m_assetIdToInfo.end());

    #if defined(AZ_ENABLE_TRACING)
                if (message.m_assetType == AZ::Data::s_invalidAssetType)
                {
                    AZ_TracePrintf("AssetCatalog", "Registering asset \"%s\" via AssetSystem message, but type is not set.\n", relativePath.c_str());
                }
    #endif

                const AZ::Data::AssetType& assetType = isNewAsset ? message.m_assetType : assetInfoPair->second.m_assetType;

                AZ::Data::AssetInfo newData;
                newData.m_assetId = assetId;
                newData.m_assetType = assetType;
                newData.m_relativePath = message.m_data;
                newData.m_sizeBytes = message.m_sizeBytes;

                m_registry->RegisterAsset(assetId, newData);
                m_registry->SetAssetDependencies(assetId, message.m_dependencies);

                for (const auto& mapping : message.m_legacyAssetIds)
                {
                    m_registry->RegisterLegacyAssetMapping(mapping, assetId);
                }
            }
            if (!isNewAsset)
            {
                // the following deliveries must happen on the main thread of the application:
                AZ::TickBus::QueueFunction([assetId]() 
                {
                    AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetChanged, assetId);
                });

                // in case someone has an ancient reference, notify on that too.
                for (const auto& mapping : message.m_legacyAssetIds)
                {
                    AZ::TickBus::QueueFunction([mapping]()
                    {
                        AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetChanged, mapping);
                    });
                    
                }
            }
            else
            {
                AZ::TickBus::QueueFunction([assetId]()
                {
                    AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetAdded, assetId);
                });
                for (const auto& mapping : message.m_legacyAssetIds)
                {
                    AZ::TickBus::QueueFunction([mapping]()
                    {
                        AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetAdded, mapping);
                    });
                }
            }

            AzFramework::LegacyAssetEventBus::QueueEvent(AZ::Crc32(extension.c_str()), &AzFramework::LegacyAssetEventBus::Events::OnFileChanged, relativePath);
            
            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::TickBus::QueueFunction([assetId]()
                {
                    AZ::Data::AssetManager::Instance().ReloadAsset(assetId, AZ::Data::AssetLoadBehavior::Default, true);
                });
            }
        }
        else
        {
            AZ_TracePrintf("AssetCatalog", "AssetChanged: invalid asset id: %s\n", assetId.ToString<AZStd::string>().c_str());
        }
    }

    //=========================================================================
    // NetworkAssetUpdateInterface
    //=========================================================================
    // note that this function is called from within a network job thread in order to update
    // the catalog as soon as new data is available, before other systems try to reach in and query
    // the results.  For this reason, it is a direct call and protects its structures with a lock.
    void AssetCatalog::AssetRemoved(AzFramework::AssetSystem::AssetNotificationMessage message)
    {
        AZStd::string relativePath = message.m_data;
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::MakePathAssetRootRelative, relativePath);

        const AZ::Data::AssetId assetId = message.m_assetId;
        if (assetId.IsValid())
        {
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(relativePath.c_str(), extension, false);
            UnregisterAsset(assetId);
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
                for (const auto& mapping : message.m_legacyAssetIds)
                {
                    m_registry->UnregisterLegacyAssetMapping(mapping);
                }
            }
            // queue this for later delivery, since we are not on the main thread:
            AzFramework::LegacyAssetEventBus::QueueEvent(AZ::Crc32(extension.c_str()), &AzFramework::LegacyAssetEventBus::Events::OnFileRemoved, relativePath);
        }
        else
        {
            AZ_TracePrintf("AssetCatalog", "AssetRemoved: invalid asset id: %s\n", assetId.ToString<AZStd::string>().c_str());
        }
    }

    //=========================================================================
    // LoadBaseCatalogInternal
    //=========================================================================
    bool AssetCatalog::LoadBaseCatalogInternal()
    {
        // it does not matter if this call fails - it will succeed if it can.  In release builds or builds where the user has turned off the
        // asset processing system because they have precompiled all assets, the call will fail but there will still be a valid catalog.
        bool catalogSavedOK = false;
        AzFramework::AssetSystemRequestBus::BroadcastResult(catalogSavedOK, &AzFramework::AssetSystem::AssetSystemRequests::SaveCatalog);

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZStd::string baseCatalogName;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_baseCatalogNameMutex);
            baseCatalogName = m_baseCatalogName;
        }
        if (fileIO && fileIO->Exists(baseCatalogName.c_str()))
        {
            InitializeCatalog(baseCatalogName.c_str());

#if defined(DEBUG_DUMP_CATALOG)
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

            for (auto& it : m_registry->m_assetIdToInfo)
            {
                AZ_TracePrintf("Asset Registry: AssetID->Info", "%s --> %s %llu bytes\n", it.first.ToString<AZStd::string>().c_str(), it.second.m_relativePath.c_str(), it.second.m_sizeBytes);
            }

#endif
            return true;
        }

        return false;
    }

    //=========================================================================
    // LoadCatalog
    //=========================================================================
    bool AssetCatalog::LoadCatalog(const char* catalogRegistryFile)
    {
        // right before we load the catalog, make sure you are listening for update events, so that you don't miss any in the gap 
        // that happens AFTER the catalog is saved but BEFORE you start monitoring them:
        StartMonitoringAssets();
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_baseCatalogNameMutex);
            m_baseCatalogName = catalogRegistryFile;
        }
        return LoadBaseCatalogInternal();
    }

    //=========================================================================
    // ClearCatalog
    //=========================================================================
    void AssetCatalog::ClearCatalog()
    {   
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_deltaCatalogMutex);
            m_deltaCatalogList.clear();
        }

        ResetRegistry();
    }

    //=========================================================================
    // ResetRegistry
    //=========================================================================
    void AssetCatalog::ResetRegistry()
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        m_registry->Clear();
        m_initialized = false;
    }


    //=========================================================================
    // AddCatalogEntry
    //=========================================================================
    void AssetCatalog::AddCatalogEntry(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_deltaCatalogMutex);
#if !defined(_RELEASE)
        for (const auto& thisElement : m_deltaCatalogList)
        {
            if (thisElement == deltaCatalog)
            {
                AZ_Warning("AssetCatalog", false, "Catalog %p already in catalog list!", deltaCatalog.get());
            }
        }
#endif
        m_deltaCatalogList.push_back(deltaCatalog);
    }

    //=========================================================================
    // InsertCatalogEntry
    //=========================================================================
    void AssetCatalog::InsertCatalogEntry(AZStd::shared_ptr<AssetRegistry> deltaCatalog, size_t catalogIndex)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_deltaCatalogMutex);
        if (catalogIndex > m_deltaCatalogList.size())
        {
            AZ_Warning("AssetCatalog", false, "Catalog name %p can't be inserted at slot %u", deltaCatalog.get(), catalogIndex);
            return;
        }
#if !defined(_RELEASE)
        for (const auto& thisElement : m_deltaCatalogList)
        {
            if (thisElement == deltaCatalog)
            {
                AZ_Warning("AssetCatalog", false, "Catalog %p already in catalog list!", deltaCatalog.get());
            }
        }
#endif
        m_deltaCatalogList.insert(m_deltaCatalogList.begin() + catalogIndex, deltaCatalog);
    }

    AZStd::shared_ptr<AzFramework::AssetRegistry> AssetCatalog::LoadCatalogFromFile(const char* catalogFile) 
    {
        AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog;
        deltaCatalog.reset(AZ::Utils::LoadObjectFromFile<AzFramework::AssetRegistry>(catalogFile));
        if (!deltaCatalog)
        {
            AZ_Error("AssetCatalog", false, "Failed to load catalog %s", catalogFile);
            return {};
        }
        return deltaCatalog;
    }


    //=========================================================================
    // AddDeltaCatalog
    //=========================================================================
    bool AssetCatalog::AddDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog)
    {
        if (!deltaCatalog)
        {
            return false;
        }
        AddCatalogEntry(deltaCatalog);
        ApplyDeltaCatalog(deltaCatalog);
        return true;
    }

    //=========================================================================
    // InsertDeltaCatalog
    //=========================================================================
    bool AssetCatalog::InsertDeltaCatalogBefore(AZStd::shared_ptr<AzFramework::AssetRegistry> deltaCatalog, AZStd::shared_ptr<AzFramework::AssetRegistry> nextCatalog)
    { 
        if (!nextCatalog)
        {
            AddDeltaCatalog(deltaCatalog);
        }
        else
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_deltaCatalogMutex);

            for (size_t slotNum = 0; slotNum < m_deltaCatalogList.size(); ++slotNum)
            {
                if (m_deltaCatalogList[slotNum] == nextCatalog)
                {
                    return InsertDeltaCatalog(deltaCatalog, slotNum);
                }
            }
            AZ_Warning("AssetCatalog", false, "Failed to find catalog %p to insert %p in front of", nextCatalog.get(), deltaCatalog.get());
            return false;

        }
        return true;
    }

    //=========================================================================
    // ApplyDeltaCatalog
    //=========================================================================
    bool AssetCatalog::ApplyDeltaCatalog(AZStd::shared_ptr<AssetRegistry> deltaCatalog)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);

        m_registry->AddRegistry(deltaCatalog);
        return true;
    }

    //=========================================================================
    // InsertDeltaCatalog
    //=========================================================================
    bool AssetCatalog::InsertDeltaCatalog(AZStd::shared_ptr<AssetRegistry> deltaCatalog, size_t slotIndex)
    {
        InsertCatalogEntry(deltaCatalog, slotIndex);
        ReloadCatalogs();
        return true;
    }

    //=========================================================================
    // ReloadCatalogs
    //=========================================================================
    bool AssetCatalog::ReloadCatalogs()
    {
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_baseCatalogNameMutex);
            // Currently only called by RemoveDeltaCatalog which holds ownership of the mutex
            if (!m_deltaCatalogList.size() && !m_baseCatalogName.length())
            {
                AZ_Warning("AssetCatalog", false, "CatalogList is empty - Failed to reload catalog list");
                return false;
            }
        }

        ResetRegistry();
        LoadBaseCatalogInternal();

        for (size_t catalogSlot = 0; catalogSlot < m_deltaCatalogList.size(); ++catalogSlot)
        {
            ApplyDeltaCatalog(m_deltaCatalogList[catalogSlot]);
        }
        return true;
    }

    //=========================================================================
    // RemoveDeltaCatalog
    //=========================================================================
    bool AssetCatalog::RemoveDeltaCatalog(AZStd::shared_ptr<AssetRegistry> deltaCatalog)
    {

        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_deltaCatalogMutex);
        for (auto thisElement = m_deltaCatalogList.begin(); thisElement != m_deltaCatalogList.end(); ++thisElement)
        {
            if (*thisElement == deltaCatalog)
            {
                m_deltaCatalogList.erase(thisElement);
                return ReloadCatalogs();
            }
        }
        AZ_Warning("AssetCatalog", false, "Failed to remove delta catalog %p (Not found)", deltaCatalog.get());
        return false;
    }

    //=========================================================================
    // SaveCatalog
    //=========================================================================
    bool AssetCatalog::SaveCatalog(const char* catalogRegistryFile)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_registryMutex);
        return SaveCatalog(catalogRegistryFile, m_registry.get());
    }

    //=========================================================================
    // SaveCatalog
    //=========================================================================
    bool AssetCatalog::SaveCatalog(const char* catalogRegistryFile, AzFramework::AssetRegistry* catalogRegistry)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");

        if(!AZ::Utils::SaveObjectToFile(catalogRegistryFile, AZ::DataStream::ST_BINARY, catalogRegistry, serializeContext))
        {
            AZ_Warning("AssetCatalog", false, "Failed to save catalog file %s", catalogRegistryFile);
            return false;
        }
        return true;
    }

    //=========================================================================
    // SaveAssetBundleManifest
    //=========================================================================
    bool AssetCatalog::SaveAssetBundleManifest(const char* assetBundleManifestFile, AzFramework::AssetBundleManifest* bundleManifest)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");

        if (!AZ::Utils::SaveObjectToFile(assetBundleManifestFile, AZ::DataStream::ST_XML, bundleManifest, serializeContext))
        {
            AZ_Warning("AssetCatalog", false, "Failed to save manifest file %s", assetBundleManifestFile);
            return false;
        }
        return true;
    }


    bool AssetCatalog::CreateBundleManifest(const AZStd::string& deltaCatalogPath, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& fileDirectory, int bundleVersion, const AZStd::vector<AZ::IO::Path>& levelDirs)
    {
        if (bundleVersion > AzFramework::AssetBundleManifest::CurrentBundleVersion || bundleVersion < 0)
        {
            AZ_Error("Asset Catalog", false, "Invalid bundle version (%d), Current bundle version is (%d).", bundleVersion, AzFramework::AssetBundleManifest::CurrentBundleVersion);
            return false;
        }
        AzFramework::AssetBundleManifest manifest;
        manifest.SetBundleVersion(bundleVersion);

        if (bundleVersion >= 1)
        {
            manifest.SetCatalogName(deltaCatalogPath);
            manifest.SetDependentBundleNames(dependentBundleNames);
        }

        if (bundleVersion >= 2)
        {
            manifest.SetLevelsDirectory(levelDirs);
        }

        AZStd::string manifestFilePath;
        AzFramework::StringFunc::Path::ConstructFull(fileDirectory.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, manifestFilePath, true);

        if (!AssetCatalog::SaveAssetBundleManifest(manifestFilePath.c_str(), &manifest))
        {
            AZ_Error("Asset Catalog", false, "Failed to serialize bundle manifest %s.", deltaCatalogPath.c_str());
            return false;
        }
        return true;
    }

    //=========================================================================
    // CreateDeltaCatalog
    //=========================================================================
    bool AssetCatalog::CreateDeltaCatalog(const AZStd::vector<AZStd::string>& files, const AZStd::string& filePath)
    {
        AzFramework::AssetRegistry deltaRegistry;
        AZStd::vector<AZ::Data::AssetId> deltaPakAssetIds;
        for (const AZStd::string& file : files)
        {
            AZ::Data::AssetId asset = m_registry->GetAssetIdByPath(file.c_str());
            if (!asset.IsValid())
            {
                // Asset is not listed in the registry, we can early out and fail as there should never be an asset that isn't in the registry.
                // Catalog and manifest files should have been trimmed from the list of files passed in prior to  this function
                AZ_Error("Asset Catalog", false, "Asset Catalog::CreateDeltaCatalog - Failed to add asset \"%s\" to the delta asset registry. Couldn't determine Asset ID for the given asset. This likely means that it is not in the source asset catalog. Rerun asset processor and regenerate pak files to remove old assets.", file.c_str());
                return false;
            }
            AZ::Data::AssetInfo assetInfo = GetAssetInfoById(asset);
            deltaRegistry.RegisterAsset(asset, assetInfo);
            deltaPakAssetIds.push_back(asset);

            AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> dependencyResult = GetDirectProductDependencies(asset);
            if (!dependencyResult.IsSuccess())
            {
                // This asset has no dependencies registered, so continue.
                continue;
            }
            for (const AZ::Data::ProductDependency& dependency : dependencyResult.GetValue())
            {
                deltaRegistry.RegisterAssetDependency(asset, dependency);
            }            
        }
        for (auto legacyToRealPair : m_registry->GetLegacyMappingSubsetFromRealIds(deltaPakAssetIds))
        {
            deltaRegistry.RegisterLegacyAssetMapping(legacyToRealPair.first, legacyToRealPair.second);
        }

        // serialize the registry
        if (!AssetCatalog::SaveCatalog(filePath.c_str(), &deltaRegistry))
        {
            AZ_Error("Asset Catalog", false, "Failed to serialize delta catalog %s.", filePath.c_str());
            return false;
        }

        return true;
    }

} // namespace AzFramework
