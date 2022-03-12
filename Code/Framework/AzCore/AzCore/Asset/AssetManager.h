/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetContainer.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h> // used as allocator for most components
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

namespace AZ::Data
{
    struct AssetContainerKey;
}

namespace AZStd
{
    template<>
    struct hash<AZ::Data::AssetContainerKey>
    {
        size_t operator()(const AZ::Data::AssetContainerKey& obj) const;
    };
}

namespace AZ
{
    namespace IO
    {
        class GenericStream;
        enum class OpenMode : AZ::u32;
    }

    namespace IO::IStreamerTypes
    {
        class RequestMemoryAllocator;
    }

    namespace Data
    {
        class AssetHandler;
        class AssetCatalog;
        class AssetDatabaseJob;
        class WaitForAsset;

        struct IDebugAssetEvent
        {
            AZ_RTTI(IDebugAssetEvent, "{1FEF8289-C730-426D-B3B9-4BBA66339D66}");

            IDebugAssetEvent() = default;
            virtual ~IDebugAssetEvent() = default;

            virtual void AssetStatusUpdate(AZ::Data::AssetId id, AZ::Data::AssetData::AssetStatus status) = 0;
            virtual void ReleaseAsset(AZ::Data::AssetId id) = 0;
        };

        struct AssetContainerKey
        {
            AssetId m_assetId;
            AssetLoadParameters m_loadParameters;

            bool operator==(const AssetContainerKey& rhs) const
            {
                return m_assetId == rhs.m_assetId && m_loadParameters == rhs.m_loadParameters;
            }
        };

        class AssetStreamInfo
        {
        public:
            AssetStreamInfo()
                : m_streamFlags(IO::OpenMode())
                , m_dataLen(0)
                , m_dataOffset(0)
            {}

            bool IsValid() const
            {
                return !m_streamName.empty();
            }

            AZStd::string   m_streamName;
            IO::OpenMode    m_streamFlags;
            u64             m_dataLen;
            u64             m_dataOffset;
        };

        struct AssetDependencyEntry
        {
            AssetId     m_assetId;
            AssetType   m_assetType;
        };
        typedef AZStd::vector<AssetDependencyEntry> AssetDependencyList;

        /*
         * This is the base class for Async AssetDatabase jobs
         */
        class AssetDatabaseJob
            : public AZStd::intrusive_list_node<AssetDatabaseJob>
        {
            friend class AssetManager;

        protected:
            AssetDatabaseJob(AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler);
            virtual ~AssetDatabaseJob();

            AssetManager*      m_owner;
            AssetInternal::WeakAsset<AssetData>    m_asset;
            AssetHandler*       m_assetHandler;
        };

        /**
         * AssetDatabase handles the creation, refcounting and automatic
         * destruction of assets.
         *
         * In general for any events while loading/saving/etc. create an AssetEventHandler and pass
         * it to AssetDatabase::GetAsset().
         * You can also connect to AssetBus if you want to listen for
         * events without holding an asset.
         * If an asset is ready at the time you connect to AssetBus or GetAsset() is called,
         * your handler will be notified immediately, otherwise all events are dispatched asynchronously.
         */
        class AssetManager
            : private AssetManagerBus::Handler
        {
            friend class AssetData;
            friend class AssetDatabaseJob;
            friend class ReloadAssetJob;
            friend class LoadAssetJob;
            friend Asset<AssetData> AssetInternal::GetAssetData(const AssetId& id, AssetLoadBehavior assetReferenceLoadBehavior);
            friend class AssetContainer;
            friend class WaitForAsset;

        public:
            struct Descriptor
            {
                Descriptor() = default;
            };

            typedef AZStd::unordered_map<AssetType, AssetHandler*> AssetHandlerMap;
            typedef AZStd::unordered_map<AssetType, AssetCatalog*> AssetCatalogMap;
            typedef AZStd::unordered_map<AssetId, AssetData*> AssetMap;
            typedef AZStd::unordered_map<AssetContainerKey, AZStd::weak_ptr<AssetContainer>> WeakAssetContainerMap;
            typedef AZStd::unordered_map<AssetContainer*, AZStd::shared_ptr<AssetContainer>> OwnedAssetContainerMap;

            AZ_CLASS_ALLOCATOR(AssetManager, SystemAllocator, 0);

            static bool Create(const Descriptor& desc);
            static void Destroy();
            static bool IsReady();
            static AssetManager& Instance();
            // Takes ownership
            static bool SetInstance(AssetManager* assetManager);
            // @{ Asset handler management
            /// Register handler with the system for a particular asset type.
            /// A handler should be registered for each asset type it handles.
            /// Please note that all the handlers are registered just once during app startup from the main thread
            /// and therefore this is not a thread safe method and should not be invoked from different threads.
            void RegisterHandler(AssetHandler* handler, const AssetType& assetType);
            /// Unregister handler from the asset system.
            /// Please note that all the handlers are unregistered just once during app shutdown from the main thread
            /// and therefore this is not a thread safe method and should not be invoked from different threads.
            void UnregisterHandler(AssetHandler* handler);
            // @}

            // @{ Asset catalog management
            /// Register a catalog with the system for a particular asset type.
            /// A catalog should be registered for each asset type it is responsible for.
            void RegisterCatalog(AssetCatalog* catalog, const AssetType& assetType);
            /// Unregister catalog from the asset system.
            void UnregisterCatalog(AssetCatalog* catalog);
            // @}

            void GetHandledAssetTypes(AssetCatalog* catalog, AZStd::vector<AZ::Data::AssetType>& assetTypes);

            /// Prevents assets from being released when no longer referenced.
            void SuspendAssetRelease();
            /// Resumes releasing assets that are no longer referenced.  Any currently un-referenced assets will be released upon calling this.
            void ResumeAssetRelease();

            /**
             * Blocks the current thread until the specified asset has finished loading (whether successful or not)
             * \param asset a valid asset which has already been requested to load.  It is an error to block on an asset which has not been requested to load already
             * This will return as soon as the asset has finished loading (i.e. the appropriate internal AssetJobBus notification has triggered)
             * It does not wait for the AssetManager to notify external listeners via the AssetBus OnAsset* events.
             * If the asset is loaded successfully, the return state may be ReadyPreNotify or Ready depending on thread timing
             */
            AssetData::AssetStatus BlockUntilLoadComplete(const Asset<AssetData>& asset);

            /**
             * Gets an asset from the database, if not present it loads it from the catalog/stream. For events register a handler by calling RegisterEventHandler().
             * \param assetId a valid id of the asset
             * \param assetReferenceLoadBehavior the AssetLoadBehavior set on the returned Asset<T> object.  Important (only) when the
             *    Asset<T> is saved to disk as this behavior will be preserved and used when loading the asset containing this reference
             *    This specifically does not have a default parameter to ensure callers intentionally choose the correct behavior
             *    For asset references intended to be saved to disk
             * \param loadParams optional set of parameters to control loading
             * Keep in mind that this is an async operation, the asset will not be loaded after the call to this function completes.
             */
            template<class AssetClass>
            Asset<AssetClass> GetAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior, const AssetLoadParameters& loadParams = AssetLoadParameters{});

            /**
            * Gets an asset from the database, if not present it loads it from the catalog/stream. For events register a handler by calling RegisterEventHandler().
            * \param assetId a valid id of the asset
            * \param assetType type id of the asset
            * \param assetReferenceLoadBehavior the AssetLoadBehavior set on the returned Asset<T> object.  Important (only) when the
            *    Asset<T> is saved to disk as this behavior will be preserved and used when loading the asset containing this reference
            *    This specifically does not have a default parameter to ensure callers intentionally choose the correct behavior
            *    For asset references intended to be saved to disk
            * \param loadParams optional set of parameters to control loading
            * Keep in mind that this async operation, asset will not be loaded after the call to this function completes.
            **/
            Asset<AssetData> GetAsset(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior, const AssetLoadParameters& loadParams = AssetLoadParameters{});

            /**
             * Locates an existing in-memory asset, if the asset is unknown, a new in-memory asset will be created.
             * The asset will not be queued for load.
             * \param assetReferenceLoadBehavior the AssetLoadBehavior set on the returned Asset<T> object.  Important (only) when the
             *    Asset<T> is saved to disk as this behavior will be preserved and used when loading the asset containing this reference
             *    This specifically does not have a default parameter to ensure callers intentionally choose the correct behavior
             *    For asset references intended to be saved to disk
             */
            template<class AssetClass>
            Asset<AssetClass> FindOrCreateAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior);
            Asset<AssetData> FindOrCreateAsset(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior);

            /** Locates an existing in-memory asset. If the asset is unknown, a null asset pointer is returned.
             * \param assetReferenceLoadBehavior the AssetLoadBehavior set on the returned Asset<T> object.  Important (only) when the
             *    Asset<T> is saved to disk as this behavior will be preserved and used when loading the asset containing this reference
             *    This specifically does not have a default parameter to ensure callers intentionally choose the correct behavior
             *    For asset references intended to be saved to disk
             */
            template<class AssetClass>
            Asset<AssetClass> FindAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior);
            Asset<AssetData> FindAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior);

            /** Creates an in-memory asset and returns the pointer. If the asset already exists it will return NULL (then you should use GetAsset/FindAsset to obtain it).
             * \param assetReferenceLoadBehavior the AssetLoadBehavior set on the returned Asset<T> object.  Important (only) when the
             *    Asset<T> is saved to disk as this behavior will be preserved and used when loading the asset containing this reference
             */
            template<class AssetClass>
            Asset<AssetClass> CreateAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior = AssetLoadBehavior::Default);
            Asset<AssetData> CreateAsset(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior = AssetLoadBehavior::Default);

            /**
             * Triggers an asset save an asset if possible. In general most assets will NOT support save as they are generated from external tool.
             * This is the interface for the rare cases we do save. If you want to know the state of the save (if completed and result)
             * listen on the AssetBus.
             */
            void SaveAsset(const Asset<AssetData>& asset);

            /**
             * Requests a reload of a given asset from storage.
             */
            void ReloadAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior, bool isAutoReload = false);

            /**
             * Reloads an asset from provided in-memory data.
             * Ownership of the provided asset data is transferred to the asset manager.
             */
            void ReloadAssetFromData(const Asset<AssetData>& asset);

            /**
            * Assign new data for the specified asset Id. This is effectively reloading the asset
            * with the provided data. Listeners will be notified to process the new data.
            */
            void AssignAssetData(const Asset<AssetData>& asset);

            /**
            * Gets a pointer to an asset handler for a type.
            * Returns nullptr if a handler for that type does not exist.
            */
            AssetHandler* GetHandler(const AssetType& assetType);

            AssetStreamInfo     GetLoadStreamInfoForAsset(const AssetId& assetId, const AssetType& assetType);
            AssetStreamInfo     GetSaveStreamInfoForAsset(const AssetId& assetId, const AssetType& assetType);

            void        DispatchEvents();

            /**
            * Old 'legacy' assetIds and asset hints can be automatically replaced  with new ones during deserialize / assignment.
            * This operation can be somewhat costly, and its only useful if the program subsequently re-saves the files its loading so that
            * the asset hints and assetIds actually persist.  Thus, it can be disabled in situations where you know you are not going to be
            * saving over or creating new source files (for example builders/background apps)
            * By default, it is enabled.
            */
            void        SetAssetInfoUpgradingEnabled(bool enable);
            bool        GetAssetInfoUpgradingEnabled() const;
            bool        ShouldCancelAllActiveJobs() const;

            /**
             * Parallel dependent loading is enabled by default, but needs to be disabled by Asset Builders or other tools connecting
             * directly with the Asset Processor because dependency information isn't guaranteed to be complete and usable for loading
             * dependencies when querying during asset building.  It only becomes usable after assets have finished building.
             *
             **/
            void        SetParallelDependentLoadingEnabled(bool enable);
            bool        GetParallelDependentLoadingEnabled() const;

            /**
            * This method must be invoked before you start unregistering handlers manually and shutting down the asset manager.
            * This method ensures that all jobs in flight are either canceled or completed.
            * This method is automatically called in the destructor but if you are unregistering handlers manually,
            * you must invoke it yourself.
            */
            void        PrepareShutDown();

            /**
            * Returns whether or not any threaded asset requests are currently active.
            */
            bool HasActiveJobsOrStreamerRequests();

        protected:
            AssetManager(const Descriptor& desc);
            virtual ~AssetManager();

            void WaitForActiveJobsAndStreamerRequestsToFinish();

            void NotifyAssetReady(Asset<AssetData> asset);
            void NotifyAssetPreReload(Asset<AssetData> asset);
            void NotifyAssetReloaded(Asset<AssetData> asset);
            void NotifyAssetReloadError(Asset<AssetData> asset);
            void NotifyAssetError(Asset<AssetData> asset);
            void NotifyAssetCanceled(AssetId assetId);
            void NotifyAssetContainerReady(Asset<AssetData> asset);
            void ReleaseAsset(AssetData* asset, AssetId assetId, AssetType assetType, bool removeAssetFromHash, int creationToken);
            void OnAssetUnused(AssetData* asset);

            void AddJob(AssetDatabaseJob* job);
            void RemoveJob(AssetDatabaseJob* job);
            void AddActiveStreamerRequest(AssetId assetId, AZStd::shared_ptr<AssetDataStream> readRequest);
            void RescheduleStreamerRequest(AssetId assetId, AZStd::chrono::milliseconds newDeadline, AZ::IO::IStreamerTypes::Priority newPriority);
            void RemoveActiveStreamerRequest(AssetId assetId);
            void AddBlockingRequest(AssetId assetId, WaitForAsset* blockingRequest);
            void RemoveBlockingRequest(AssetId assetId, WaitForAsset* blockingRequest);

            void ValidateAndPostLoad(AZ::Data::Asset < AZ::Data::AssetData>& asset, bool loadSucceeded, bool isReload, AZ::Data::AssetHandler* assetHandler = nullptr);
            void PostLoad(AZ::Data::Asset < AZ::Data::AssetData>& asset, bool loadSucceeded, bool isReload, AZ::Data::AssetHandler* assetHandler = nullptr);

            Asset<AssetData> GetAssetInternal(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior, const AssetLoadParameters& loadParams = AssetLoadParameters{}, AssetInfo assetInfo = AssetInfo(), bool signalLoaded = false);

            void UpdateDebugStatus(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

            /**
            * Gets a root asset and dependencies as individual async loads if necessary.
            * \param assetId a valid id of the asset
            * \param loadFilter optional filter predicate for dependent asset loads.
            * If the asset container is already loaded just hand back a new shared ptr
            **/
            AZStd::shared_ptr<AssetContainer> GetAssetContainer(Asset<AssetData> asset, const AssetLoadParameters& loadParams = AssetLoadParameters{});
            /**
            * Creates a new shared AssetContainer with an optional loadFilter
            * **/
            virtual AZStd::shared_ptr<AssetContainer> CreateAssetContainer(Asset<AssetData> asset, const AssetLoadParameters& loadParams = AssetLoadParameters{}) const;


            /**
            * Releases all references to asset containers that are currently attempting to load this asset.
            * If all "external" references to the asset are destroyed (i.e. nothing but loading code references the asset),
            * this makes sure that the containers are cleaned up and the loading is canceled as a part of destroying the AssetData.
            **/
            void ReleaseAssetContainersForAsset(AssetData* asset);

            /**
            * Clears all references to the owned asset container.
            **/
            void ReleaseOwnedAssetContainer(AssetContainer* assetContainer);

            //////////////////////////////////////////////////////////////////////////
            // AssetManagerBus
            void OnAssetReady(const Asset<AssetData>& asset) override;
            void OnAssetReloaded(const Asset<AssetData>& asset) override;
            void OnAssetReloadError(const Asset<AssetData>& asset) override;
            void OnAssetError(const Asset<AssetData>& asset) override;
            void OnAssetCanceled(AssetId asset) override;
            void OnAssetContainerReady(AssetContainer* container) override;
            void OnAssetContainerCanceled(AssetContainer* container) override;
            //////////////////////////////////////////////////////////////////////////


            //! Get the load stream info for an asset, including missing-asset substitution and custom AssetHandler overrides.
            AssetStreamInfo GetModifiedLoadStreamInfoForAsset(const Asset<AssetData>& asset, AssetHandler* handler);

            //! Queue an async file load with the AssetDataStream as the first step in an asset load
            void QueueAsyncStreamLoad(Asset<AssetData> asset, AZStd::shared_ptr<AssetDataStream> dataStream,
                const AZ::Data::AssetStreamInfo& streamInfo, bool isReload,
                AssetHandler* handler, const AssetLoadParameters& loadParameters, bool signalLoaded);

            AssetHandlerMap         m_handlers;
            AssetCatalogMap         m_catalogs;
            AZStd::recursive_mutex  m_catalogMutex;     // lock when accessing the catalog map
            AssetMap                m_assets;
            AZStd::recursive_mutex  m_assetMutex;       // lock when accessing the asset map

            WeakAssetContainerMap   m_assetContainers;
            OwnedAssetContainerMap  m_ownedAssetContainers;
            AZStd::unordered_multimap<AssetId, AssetContainer*> m_ownedAssetContainerLookup;
            AZStd::recursive_mutex  m_assetContainerMutex;       // lock when accessing the assetContainers map

            AZStd::thread::id m_mainThreadId;
            IDebugAssetEvent* m_debugAssetEvents{ nullptr };

            int m_creationTokenGenerator = 0; // this is used to generate unique identifiers for assets

            typedef AZStd::unordered_map<AssetId, Asset<AssetData> > ReloadMap;
            ReloadMap               m_reloads;          // book-keeping and reference-holding for asset reloads

            typedef AZStd::intrusive_list<AssetDatabaseJob, AZStd::list_base_hook<AssetDatabaseJob> > ActiveJobList;
            ActiveJobList           m_activeJobs;

            //! The AssetDataStream read requests that are pending or processing for a specific asset.
            using AssetRequestMap = AZStd::unordered_map<AssetId, AZStd::shared_ptr<AssetDataStream>>;
            AssetRequestMap m_activeAssetDataStreamRequests;

            // Lock when accessing the list of active jobs or streamer requests
            AZStd::recursive_mutex  m_activeJobOrRequestMutex;

            //! The set of all blocking requests that currently exist, grouped by AssetId.
            //! The information is used internally to route LoadAssetJob processing to any thread that currently is blocked waiting
            //! for that load to complete.
            using BlockingRequestMap = AZStd::unordered_multimap<AssetId, WaitForAsset*>;
            BlockingRequestMap m_activeBlockingRequests;
            // Mutex lock when accessing the list of active blocking requests
            AZStd::recursive_mutex  m_activeBlockingRequestMutex;

            //! Enable or disable parallel loading of dependent assets via the use of Asset Containers.
            //! default = true, but Asset Builders and other tools using real-time in-progress dependency information need
            //! to set it to false.
            bool m_enableParallelDependentLoading = true;

            bool m_assetInfoUpgradingEnabled = true;

            static EnvironmentVariable<AssetManager*>  s_assetDB;

            // used internally by the cycle checking on the job system.  Used for blocking loads.
            void RegisterAssetLoading(const Asset<AssetData>& asset);


            // Variant of RegisterAssetLoading used for jobs which have been queued and need to verify the status of the asset
            // before loading in order to prevent cases where a load is queued, then a blocking load goes through, then the queued
            // load is processed.  This validation step leaves the loaded (And potentially modified) data as is in that case.
            bool ValidateAndRegisterAssetLoading(const Asset<AssetData>& asset);

            void UnregisterAssetLoading(const Asset<AssetData>& asset);

            // Setting this to true will cause all loadAssets jobs that have not started yet to cancel as soon as they start.
            bool m_cancelAllActiveJobs = false;

            AZStd::atomic_int m_suspendAssetRelease{ 0 };
        };

        /**
         * AssetHandlers are responsible for loading and destroying assets
         * when the asset manager requests it.
         *
         * To create a handler for a specific asset type, derive from this class
         * and register an instance of the handler with the asset manager.
         *
         * Asset handling functions may be called from multiple threads, so the
         * handlers need to be thread-safe.
         * It is ok for the handler to block the calling thread during the actual
         * asset load.
         *
         * NOTE! Because it doesn't go without saying:
         * It is NOT OK for an AssetHandler to queue work for another thread and block
         * on that work being finished, in the case that that thread is the same one doing
         * the blocking. That will result in a single thread deadlock.
         *
         * If you need to queue work, the logic needs to be similar to this:
         *
         AssetHandler::LoadResult MyAssetHandler::LoadAssetData(const Asset<AssetData>& asset, AZStd::shared_ptr<AssetDataStream> stream,
                                                                const AZ::Data::AssetFilterCB& assetLoadFilterCB)
         {
            .
            .
            .

            if (AZStd::this_thread::get_id() == m_loadingThreadId)
            {
                // load asset immediately
            }
            else
            {
                // queue job to load asset in thread identified by m_loadingThreadId
                auto* queuedJob = QueueLoadingOnOtherThread(...);

                // block waiting for queued job to complete
                queuedJob->BlockUntilComplete();
            }

            .
            .
            .
         }

         */
        class AssetHandler
        {
            friend class AssetManager;
            friend class AssetData;
        public:
            AZ_RTTI(AssetHandler, "{58BD1FDF-E668-42E5-9091-16F46022F551}");

            AssetHandler();
            virtual ~AssetHandler();

            // Called by the asset manager to create a new asset. No loading should occur during this call
            virtual AssetPtr CreateAsset(const AssetId& id, const AssetType& type) = 0;

            //! Result from LoadAssetData - it either finished loading, didn't finish and is waiting for more data, or had an error.
            enum class LoadResult : u8
            {

                Error,              // The provided data failed to load correctly
                MoreDataRequired,   // The provided data loaded correctly, but more data is required to finish the asset load
                LoadComplete        // The provided data loaded correctly, and the asset has been created
            };

            // Called by the asset manager to load in the asset data.
            LoadResult LoadAssetDataFromStream(
                const Asset<AssetData>& asset,
                AZStd::shared_ptr<AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB);

            // Called by the asset manager to perform actual asset save. Returns true if successful otherwise false (default - as we don't require support save).
            virtual bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) { (void)asset; (void)stream; return false; }

            //! Called when an asset requested to load is actually missing from the catalog when we are trying to resolve it
            //! from an ID to a file name and other streaming info.
            //! Here, optionally, you can return a non-empty asset ID for it to try to use that as fallback data instead.
            //! Providing it with a non-empty assetId will cause it to attach the handler to the file data for that asset instead,
            //! but still retain the original assetId for the loaded asset.  This allows you to perform simple 'placeholder'
            //! substitution for assets that are missing, errored, or still being compiled.  If you need your
            //! system to do something more complicated than simple substitution, the place for that is in the component entity
            //! class that requested the load in the first place.  This API is just for basic substitution cases.
            virtual AZ::Data::AssetId AssetMissingInCatalog(const Asset<AssetData>& /*asset*/) {return AZ::Data::AssetId(); }

            // Called after the data loading stage and after all dependencies have been fulfilled.
            // Override this if the asset needs post-load init. If overriden, the handler is responsible
            // for notifying the asset manager when the asset is ready via AssetDatabaseBus::OnAssetReady.
            virtual void InitAsset(const Asset<AssetData>& asset, bool loadStageSucceeded, bool isReload);

            // Called by the asset manager when an asset should be deleted.
            virtual void DestroyAsset(AssetPtr ptr) = 0;

            // Called by asset manager on registration.
            virtual void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) = 0;

            // Verify that the provided asset is of a type handled by this handler
            virtual bool CanHandleAsset(const AssetId& /*id*/) const { return true; }

            //! Give asset handlers the ability to optionally modify the stream info (asset path, I/O flags, etc) prior to loading.
            //! (Very few asset handlers should need this functionality)
            virtual void GetCustomAssetStreamInfoForLoad([[maybe_unused]] AssetStreamInfo& streamInfo) {}

            //! Asset Handlers have the ability to provide custom asset buffer allocators for any non-standard allocation needs.
            virtual IO::IStreamerTypes::RequestMemoryAllocator* GetAssetBufferAllocator() { return nullptr; }

            virtual void GetDefaultAssetLoadPriority([[maybe_unused]] AssetType type, AZStd::chrono::milliseconds& defaultDeadline,
                AZ::IO::IStreamerTypes::Priority& defaultPriority) const
            {
                defaultDeadline = IO::IStreamerTypes::s_noDeadline;
                defaultPriority = IO::IStreamerTypes::s_priorityMedium;
            }

        protected:

            // Called by the asset manager to perform actual asset load.
            virtual LoadResult LoadAssetData(
                const Asset<AssetData>& asset,
                AZStd::shared_ptr<AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) = 0;


        private:
            AZStd::atomic_int   m_nHandledTypes;    // how many asset types are currently being handled by this handler.
        };

        /**
         * Base interface to find an asset in a catalog. By design this is not
         * performance critical code (as we use it on load only), but it is important to make sure this catalog operates
         * in a reasonably fast way. Cache the information (if needed) about assets location (if we will
         * do often load/unload)
         *
         * Asset catalogs functions may be called from multiple threads, so make sure your code is thread safe.
         */
        class AssetCatalog
        {
        public:
            virtual ~AssetCatalog() {}

            /**
             * Find the stream the asset can be loaded from. Empty string if asset can't be found.
             * \param id - asset id
             */
            virtual AssetStreamInfo GetStreamInfoForLoad(const AssetId& assetId, const AssetType& assetType) = 0;

            /**
             * Same as \ref GetStreamInfoForLoad but for saving. It's not typical that assets will have 'save' support,
             * as they are generated from external tools, etc. But when needed, the framework provides an interface.
             */
            virtual AssetStreamInfo GetStreamInfoForSave(const AssetId& assetId, const AssetType& assetType)
            {
                (void)assetId;
                (void)assetType;
                AZ_Assert(false, "GetStreamInfoForSave() has not been implemented for assets of type 0x%x.", assetType);
                return AssetStreamInfo();
            }
        };

        //=========================================================================
        // GetAsset
        //=========================================================================
        template <class AssetClass>
        Asset<AssetClass> AssetManager::GetAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior, const AssetLoadParameters& loadParams)
        {
            Asset<AssetData> asset = GetAsset(assetId, AzTypeInfo<AssetClass>::Uuid(), assetReferenceLoadBehavior, loadParams);
            return static_pointer_cast<AssetClass>(asset);
        }

        template <class AssetClass>
        Asset<AssetClass> AssetManager::FindOrCreateAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            Asset<AssetData> asset = FindOrCreateAsset(assetId, AzTypeInfo<AssetClass>::Uuid(), assetReferenceLoadBehavior);
            return static_pointer_cast<AssetClass>(asset);
        }

        //=========================================================================
        // FindAsset
        //=========================================================================
        template<class AssetClass>
        Asset<AssetClass> AssetManager::FindAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            Asset<AssetData> asset = FindAsset(assetId, assetReferenceLoadBehavior);
            if (asset.GetAs<AssetClass>())
            {
                return static_pointer_cast<AssetClass>(asset);
            }
            return Asset<AssetData>();
        }

        //=========================================================================
        // CreateAsset
        //=========================================================================
        template<class AssetClass>
        Asset<AssetClass> AssetManager::CreateAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior)
        {
            Asset<AssetData> asset = CreateAsset(assetId, AzTypeInfo<AssetClass>::Uuid(), assetReferenceLoadBehavior);
            return static_pointer_cast<AssetClass>(asset);
        }
    }   // namespace Data
}   // namespace AZ
