/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_ASSET_DATABASE_BUS_H
#define AZCORE_ASSET_DATABASE_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzFramework
{
    class AssetRegistry;
}

namespace AZ
{
    namespace Data
    {
        /** Asset Information (returned by bus queries to the catalog)
        * Note that Multiple UUIDs may point at the same "asset information"
        * so that legacy UUIDs (such as those generated using a different scheme) can still resolve to a valid asset
        * however, only one such entry will have 'canonical' set to true, meaning its the latest scheme.
        * UIs which enumerate assets should only use canonical assets.
        */
        class AssetInfo
        {
        public:
            AZ_TYPE_INFO(AssetInfo, "{E6D8372B-8419-4287-B478-1353709A972F}");
            AZ::Data::AssetId m_assetId; // this is in case you look up by a legacy Id or other remapping and it resolves to a new ID.
            AZ::Data::AssetType m_assetType = s_invalidAssetType;
            AZ::u64 m_sizeBytes = 0;
            AZStd::string m_relativePath; // (legacy asset name)
        };

        struct ProductDependency
        {
            AZ_TYPE_INFO(ProductDependency, "{5B9A8F1C-407A-4D2B-88F4-A79584684CC4}");

            ProductDependency() = default;
            ProductDependency(const AZ::Data::AssetId& assetId, AZStd::bitset<64> flags) : m_assetId(assetId), m_flags(flags) {}

            AZ::Data::AssetId m_assetId;
            AZStd::bitset<64> m_flags;
        };

        using PreloadAssetListType = AZStd::unordered_map<AZ::Data::AssetId, AZStd::unordered_set<AZ::Data::AssetId>>;
        /**
         * Request bus for asset catalogs. Presently we expect only one asset catalog, so this
         * bus is limited to one handlers.
         */
        class AssetCatalogRequests
            : public AZ::EBusTraits
        {
        public:

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides - Application is a singleton
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            using MutexType = AZStd::recursive_mutex;

            static constexpr bool EnableEventQueue = true;
            using EventQueueMutexType = AZStd::mutex;
            struct PostThreadDispatchInvoker
            {
                ~PostThreadDispatchInvoker();
            };

            template <typename DispatchMutex>
            struct ThreadDispatchLockGuard
            {
                ThreadDispatchLockGuard(DispatchMutex& contextMutex)
                    : m_lock{ contextMutex }
                {}
                ThreadDispatchLockGuard(DispatchMutex& contextMutex, AZStd::adopt_lock_t adopt_lock)
                    : m_lock{ contextMutex, adopt_lock }
                {}
                ThreadDispatchLockGuard(const ThreadDispatchLockGuard&) = delete;
                ThreadDispatchLockGuard& operator=(const ThreadDispatchLockGuard&) = delete;
            private:
                PostThreadDispatchInvoker m_threadPolicyInvoker;
                using LockType = AZStd::conditional_t<LocklessDispatch, AZ::Internal::NullLockGuard<DispatchMutex>, AZStd::scoped_lock<DispatchMutex>>;
                LockType m_lock;
            };

            template <typename DispatchMutex, bool>
            using DispatchLockGuard = ThreadDispatchLockGuard<DispatchMutex>;
            //////////////////////////////////////////////////////////////////////////

            virtual ~AssetCatalogRequests() = default;

            /// Enables the catalog.
            virtual void EnableCatalogForAsset(const AZ::Data::AssetType& /*assetType*/) {}

            /// Disables the catalog.
            virtual void DisableCatalog() {}

            /// Enable monitoring of asset changes.
            virtual void StartMonitoringAssets() {};

            /// Stop monitoring of asset changes.
            virtual void StopMonitoringAssets() {};

            /// Populates catalog data from specified file.
            /// \param catalogRegistryFile cache-relative file path from which catalog should be pre-loaded.
            /// \return true if catalog was successfuly loaded.

            virtual bool LoadCatalog(const char* /*catalogRegistryFile*/) { return false; }
            virtual void ClearCatalog() {}

            /// Write out our existing catalog to the given file.
            virtual bool SaveCatalog(const char* /*outputFile*/) { return false; }
            
            /// Load a catalog file on top of our existing catalog data
            virtual bool AddDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> /*deltaCatalog*/) { return true; }
            /// Insert a new delta catalog at a particular index  
            virtual bool InsertDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> /*deltaCatalog*/, size_t /* slotNum */) { return true; }
            /// Insert a new delta catalog before the given next unique catalog name
            virtual bool InsertDeltaCatalogBefore(AZStd::shared_ptr<AzFramework::AssetRegistry> /*deltaCatalog*/, AZStd::shared_ptr<AzFramework::AssetRegistry> /*nextDeltaCatalog*/) { return true; }
            /// Remove a catalog from our delta list and rebuild the catalog from remaining items
            virtual bool RemoveDeltaCatalog(AZStd::shared_ptr<AzFramework::AssetRegistry> /*deltaCatalog*/) { return true; }
            /// Creates a manifest with the given DeltaCatalog name
            virtual bool CreateBundleManifest(const AZStd::string& /*deltaCatalogPath*/, const AZStd::vector<AZStd::string>& /*dependentBundleNames*/,
                const AZStd::string& /*fileDirectory*/, int /*bundleVersion*/, const AZStd::vector<AZ::IO::Path>& /*levelDirs*/) { return false; }
            /// Creates an instance of a registry containing info for just the specified files, and writes it out to a file at the specified path
            virtual bool CreateDeltaCatalog(const AZStd::vector<AZStd::string>& /*files*/, const AZStd::string& /*filePath*/) { return false; }

            /// Adds an extension to the catalog's handled list.
            /// \param file extension to add to catalog's list of those handled. With and without prefix '.' are both accepted.
            virtual void AddExtension(const char* /*extension*/) {}

            /// Adds an asset type to the catalog's handled list.
            /// \param asset type to add to the catalog's list of those handled.
            virtual void AddAssetType(const AZ::Data::AssetType& /*assetType*/) {}

            /// Fills a vector with all registered AssetTypes.
            /// \param the list reference to fill with registered types.
            virtual void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& /*assetTypes*/) {}

            /// Get Asset Type Uuid from its Display Name
            virtual AZ::Data::AssetType GetAssetTypeByDisplayName(const AZStd::string_view /*displayName*/) { return AZ::Data::AssetType();  }

            /// Adds an asset to the catalog.
            /// \param id - the id to assign the asset. 
            /// \param info - the information to assign to that ID
            virtual void RegisterAsset(const AZ::Data::AssetId& /*id*/, AZ::Data::AssetInfo& /*info*/) {}

            /// Removes an asset from the catalog (by ID)
            virtual void UnregisterAsset(const AZ::Data::AssetId& /*id*/) {}

            /// Retrieves an asset-root-relative path by Id.
            /// \return asset relative path given an Id, if it's in the catalog, otherwise an empty string.
            virtual AZStd::string GetAssetPathById(const AZ::Data::AssetId& /*id*/) { return AZStd::string(); }

            /// Retrieves an asset Id given a full or asset-root-relative path.
            /// \param path - asset full or asset-root relative path.
            /// \param typeToRegister - if autoRegisterIfNotFound is set and the asset isn't already registered, it will be registered as this type.
            /// \param autoRegisterIfNotFound - registers the asset if not already in the catalog.
            /// \return valid AssetId if it's in the registry, otherwise an empty AssetId.
            virtual AZ::Data::AssetId GetAssetIdByPath(const char* /*path*/, const AZ::Data::AssetType& /*typeToRegister*/, bool /*autoRegisterIfNotFound*/) { return AZ::Data::AssetId(); }

            /// Retrieves file paths of all the registered assets
            virtual AZStd::vector<AZStd::string> GetRegisteredAssetPaths() { return AZStd::vector<AZStd::string>(); }

            /// Given an asset ID, retrieve general information about that asset.
            virtual AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& /*id*/) { return AssetInfo(); }

            /// Compute an asset Id from a path.
            /// This is TEMPORARY functionality. Side-by-side metadata and/or  will eventually contain Uuid information.
            /// For now it's computed based on path.
            /// \param path - asset full or asset-root relative path.
            /// \return AssetId computed from path. Returned Id will be invalid if input path is full, but not under the asset root.
            virtual AZ::Data::AssetId GenerateAssetIdTEMP(const char* /*path*/) { return AZ::Data::AssetId(); }

            /// Retrieves a list of all products the given (product) asset directly depends on.
            /// \param id - the id of the asset to look up the dependencies for
            /// \return AZ::Success containing a list of dependencies, AZ::Failure if id is not found
            virtual AZ::Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetDirectProductDependencies(const AssetId& /*id*/) { return AZ::Failure<AZStd::string>("Not implemented"); }

            /// Retrieves a list of all products the given (product) asset depends on (recursively).
            /// \param id - the id of the asset to look up the dependencies for
            /// \return AZ::Success containing a list of dependencies
            virtual AZ::Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetAllProductDependencies(const AssetId& /*id*/) { return AZ::Failure<AZStd::string>("Not implemented"); }

            /// Retrieves a list of products the given (product) asset depends on (recursively) which are not flagged as NoLoad.
            /// NoLoad dependencies will be returned in the noload set for the caller to load on demand if desired
            /// \param id - the id of the asset to look up the dependencies for
            /// \return AZ::Success containing a list of dependencies, noloadSet with the dependencies flagged as NoLoad, preloadLists contains the specific dependencies which are PreLoad for
            /// each asset.  These assets are all also found in the product dependency list which the noloadset are not.  This is because the intent of the return value product dependency list
            /// is the entire set of assets which need to load by default for the requested assetID, and the preload list is only to allow us to manage and communicate about subsets of those assets
            /// which have additional reporting requirements.  We don't want to report assets which have preload dependencies as "Ready" until all of their "PreLoad" dependencies are also ready
            /// NoLoad assets however simply wait for the user to request an additional load - they or their dependencies don't begin loading by default
            virtual AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetLoadBehaviorProductDependencies([[maybe_unused]] const AZ::Data::AssetId& id, [[maybe_unused]] AZStd::unordered_set<AZ::Data::AssetId>& noloadSet, [[maybe_unused]] PreloadAssetListType& preloadLists) { return AZ::Failure<AZStd::string>("Not implemented"); }

            /// Retrieves a list of all products the given (product) asset depends on (recursively).
            /// \param id - the id of the asset to look up the dependencies for
            /// \param exclusionList - list of AssetIds to ignore (recursively).  If a match is found, it and all its dependencies are skipped.
            /// \param wildcardPatternExclusionList - if a dependency matches any of these wildcard patterns, it should be ignored (recursively). If a match is found, it and all its dependencies are skipped.
            /// \return AZ::Success containing a list of dependencies
            virtual AZ::Outcome<AZStd::vector<ProductDependency>, AZStd::string> GetAllProductDependenciesFilter([[maybe_unused]] const AssetId& id, [[maybe_unused]] const AZStd::unordered_set<AssetId>& exclusionList, [[maybe_unused]] const AZStd::vector<AZStd::string>& wildcardPatternExclusionList) { return AZ::Failure<AZStd::string>("Not implemented"); }

            /// Checks the relative path of the asset associated with the assetId against the input wildcard pattern.
            /// Does not verify the validity of the input wildcard pattern.
            /// AssetIds that cannot be resolved to a relative path are treated as though they do not match the input pattern.
            /// \return true if the relative path associated with the input assetId matches the input wildcard pattern
            virtual bool DoesAssetIdMatchWildcardPattern(const AZ::Data::AssetId& /*assetId*/, const AZStd::string&/* wildcardPattern*/) { return false; }

            using BeginAssetEnumerationCB = AZStd::function< void() >;
            using AssetEnumerationCB = AZStd::function< void(const AZ::Data::AssetId /*id*/, const AZ::Data::AssetInfo& /*info*/) >;
            using EndAssetEnumerationCB = AZStd::function< void() >;

            /// Iterate through all assets and call the callback for each one.
            /// These callbacks will run on the same thread as the caller.
            /// \param beginCB - called before any assets are enumerated.
            /// \param enumerateCB - called for each asset.
            /// \param endCB - called after all assets are enumerated.
            virtual void EnumerateAssets(BeginAssetEnumerationCB /*beginCB*/, AssetEnumerationCB /*enumerateCB*/, EndAssetEnumerationCB /*endCB*/) {}
        };

        using AssetCatalogRequestBus = AZ::EBus<AssetCatalogRequests>;

        inline AssetCatalogRequests::PostThreadDispatchInvoker::~PostThreadDispatchInvoker()
        {
            if (!AssetCatalogRequestBus::IsInDispatchThisThread())
            {
                if (AssetCatalogRequestBus::QueuedEventCount())
                {
                    AssetCatalogRequestBus::ExecuteQueuedEvents();
                }
            }
        }

        /*
         * Events that AssetManager listens for
         */
        class AssetManagerEvents
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            typedef AZStd::recursive_mutex  MutexType;
            //////////////////////////////////////////////////////////////////////////

            virtual ~AssetManagerEvents() {}

            /// Signal that an asset is ready for use
            virtual void OnAssetReady(const Asset<AssetData>& asset) = 0;
            /// Signal that an asset has been reloaded
            virtual void OnAssetReloaded(const Asset<AssetData>& asset) = 0;
            /// Signal that an asset failed to reload.
            virtual void OnAssetReloadError(const Asset<AssetData>& asset) = 0;
            /// Signal that an asset error has occurred
            virtual void OnAssetError(const Asset<AssetData>& asset) = 0;
            /// Signal that an asset load has been canceled
            virtual void OnAssetCanceled(AssetId assetId) = 0;
            /// Signal that an asset container load has finished.
            virtual void OnAssetContainerReady(AssetContainer* container) = 0;

            /// When an asset is loaded as part of a container this signal is sent if the root asset is canceled / destroyed.
            /// The signal isn't sent until all the dependent assets in the container have finished loading, to help ensure that
            /// dependent assets don't get stuck in a perpetual loading state.
            virtual void OnAssetContainerCanceled(AssetContainer* container) = 0;
        };
        typedef EBus<AssetManagerEvents> AssetManagerBus;

        /*
         * Events that the AssetManager broadcasts.
         */
        class AssetManagerNotifications
            : public EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
            typedef AZStd::recursive_mutex  MutexType;
            //////////////////////////////////////////////////////////////////////////

            /// Notify listeners that asset events are starting to dispatch
            virtual void OnAssetEventsDispatchBegin() {}
            /// Notify listeners that all asset events have finished dispatching
            virtual void OnAssetEventsDispatchEnd() {}
        };
        typedef EBus<AssetManagerNotifications> AssetManagerNotificationBus;

    }   // namespace Data
}   // namespace AZ

DECLARE_EBUS_EXTERN_DLL_SINGLE_ADDRESS(Data::AssetManagerNotifications);
DECLARE_EBUS_EXTERN_DLL_SINGLE_ADDRESS(Data::AssetCatalogRequests);
DECLARE_EBUS_EXTERN_DLL_SINGLE_ADDRESS(Data::AssetManagerEvents);

#endif  // AZCORE_ASSET_DATABASE_BUS_H
#pragma once
