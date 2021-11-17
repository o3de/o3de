/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManager_private.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Console/IConsole.h>
#include <cinttypes>
#include <utility>
#include <AzCore/Serialization/ObjectStream.h>

namespace AZ::Data
{
    AZ_CVAR(bool, cl_assetLoadWarningEnable, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Enable warnings that show when AssetHandler::LoadAssetData has exceeded the time set in cl_assetLoadWarningMsThreshold.");
    AZ_CVAR(uint32_t, cl_assetLoadWarningMsThreshold, 100, nullptr, AZ::ConsoleFunctorFlags::Null,
            "Number of milliseconds that AssetHandler::LoadAssetData can execute for before printing a warning.");
    AZ_CVAR(int, cl_assetLoadDelay, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Number of milliseconds to artifically delay an asset load.");
    AZ_CVAR(bool, cl_assetLoadError, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Enable failure of all asset loads.");

    static constexpr char kAssetDBInstanceVarName[] = "AssetDatabaseInstance";

    /*
     * This is the base class for Async AssetDatabase jobs
     */
    class AssetDatabaseAsyncJob
        : public AssetDatabaseJob
        , public Job
    {
    public:
        AssetDatabaseAsyncJob(JobContext* jobContext, bool deleteWhenDone, AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
            : AssetDatabaseJob(owner, asset, assetHandler)
            , Job(deleteWhenDone, jobContext)
        {
        }

        ~AssetDatabaseAsyncJob() override
        {
        }
    };

    /**
     * Internally allows threads blocking on asset loads to be notified on load completion.
     */
    class BlockingAssetLoadEvents
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AssetId;
        using MutexType = AZStd::recursive_mutex;

        template <class Bus>
        struct AssetJobConnectionPolicy
            : public EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                typename Bus::BusIdType actualId = AssetInternal::ResolveAssetId(id);
                EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, actualId);

                // If the asset is loaded or failed already, deliver the status update immediately
                // Note that we check IsReady here, ReadyPreNotify must be tested because there is
                // a small gap between ReadyPreNotify and Ready where the callback could be missed.
                // Also note because the Asset<> reference isn't passed around anywhere, it doesn't matter
                // what the AssetLoadBehavior is set to, as it will never make it back to any callers.
                Asset<AssetData> assetData(AssetInternal::GetAssetData(actualId, AssetLoadBehavior::Default));
                if (assetData)
                {
                    if (assetData->IsReady() || assetData->IsError())
                    {
                        connectLock.unlock();
                        handler->OnLoadComplete();
                    }
                }
            }
        };

        template <typename Bus>
        using ConnectionPolicy = AssetJobConnectionPolicy<Bus>;

        virtual void OnLoadComplete() = 0;
        virtual void OnLoadCanceled(AssetId assetId) = 0;
    };

    using BlockingAssetLoadBus = EBus<BlockingAssetLoadEvents>;

    /*
     * This class processes async AssetDatabase load jobs
     */
    class LoadAssetJob
        : public AssetDatabaseAsyncJob
    {
    public:
        AZ_CLASS_ALLOCATOR(LoadAssetJob, ThreadPoolAllocator, 0);

        LoadAssetJob(AssetManager* owner, const Asset<AssetData>& asset,
            AZStd::shared_ptr<AssetDataStream> dataStream, bool isReload, AZ::IO::IStreamerTypes::RequestStatus requestState,
            AssetHandler* handler, const AssetLoadParameters& loadParams, bool signalLoaded)
            : AssetDatabaseAsyncJob(JobContext::GetGlobalContext(), true, owner, asset, handler)
            , m_dataStream(dataStream)
            , m_isReload(isReload)
            , m_requestState(requestState)
            , m_loadParams(loadParams)
            , m_signalLoaded(signalLoaded)
        {
            AZ_Assert(m_dataStream, "Data stream pointer received through the callback from AZ::IO::Streamer is invalid.");

            AZ_Assert((m_requestState == AZ::IO::IStreamerTypes::RequestStatus::Completed)
                || (m_requestState == AZ::IO::IStreamerTypes::RequestStatus::Canceled)
                || (m_requestState == AZ::IO::IStreamerTypes::RequestStatus::Failed),
                "AssetManager::LoadAssetJob was called with an unexpected streamer state: %i", m_requestState);
        }

        ~LoadAssetJob() override
        {
        }

        void Process() override
        {
            Asset<AssetData> asset = m_asset.GetStrongReference();

            // Verify that we didn't somehow get here after the Asset Manager has finished shutting down.
            AZ_Assert(AssetManager::IsReady(), "Asset Manager shutdown didn't clean up pending asset loads properly.");
            if (!AssetManager::IsReady())
            {
                return;
            }

            bool shouldCancel = m_owner->ShouldCancelAllActiveJobs()
                || !asset // No outstanding references, so cancel the load
                || m_requestState == AZ::IO::IStreamerTypes::RequestStatus::Canceled;

            if (shouldCancel)
            {
                BlockingAssetLoadBus::Event(m_asset.GetId(), &BlockingAssetLoadBus::Events::OnLoadCanceled, m_asset.GetId());
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetCanceled, m_asset.GetId());
            }
            else
            {

                AZ_PROFILE_SCOPE(AzCore, "AZ::Data::LoadAssetJob::Process: %s",
                    asset.GetHint().c_str());

                AZ_ASSET_ATTACH_TO_SCOPE(this);

                if (m_owner->ValidateAndRegisterAssetLoading(asset))
                {
                    LoadAndSignal(asset);
                }
            }
        }

        void LoadAndSignal(Asset<AssetData>& asset)
        {
            const bool loadSucceeded = LoadData();

            if (m_signalLoaded && loadSucceeded)
            {
                AZ_Assert(!m_isReload, "OnAssetDataLoaded signal isn't supported for asset reloads.");
                // This asset has preload dependencies, we need to evaluate whether they're all ready before calling PostLoad
                AssetLoadBus::Event(asset.GetId(), &AssetLoadBus::Events::OnAssetDataLoaded, asset);
            }
            else
            {
                // As long as we don't need to signal preload dependencies, just finish the load whether or not it was successful.
                m_owner->PostLoad(asset, loadSucceeded, m_isReload, m_assetHandler);
            }
        }

        bool LoadData()
        {
            Asset<AssetData> asset = m_asset.GetStrongReference();

            if(cl_assetLoadDelay > 0)
            {
                AZ_PROFILE_SCOPE(AzCore, "LoadData suspended");
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(cl_assetLoadDelay));
            }

            AZ_ASSET_NAMED_SCOPE(asset.GetHint().c_str());
            bool loadedSuccessfully = false;

            if (!cl_assetLoadError && m_requestState == AZ::IO::IStreamerTypes::RequestStatus::Completed)
            {
                if (m_dataStream->IsFullyLoaded())
                {
                    AssetHandler::LoadResult result =
                        m_assetHandler->LoadAssetDataFromStream(asset, m_dataStream, m_loadParams.m_assetLoadFilterCB);
                    loadedSuccessfully = (result == AssetHandler::LoadResult::LoadComplete);
                }
            }

            return loadedSuccessfully;
        }

    private:
        AZStd::shared_ptr<AssetDataStream> m_dataStream;
        AssetLoadParameters m_loadParams{};
        AZ::IO::IStreamerTypes::RequestStatus m_requestState{ AZ::IO::IStreamerTypes::RequestStatus::Pending};
        bool m_isReload{ false };
        bool m_signalLoaded{ false };
    };


     /**
      * Utility class to wait when a blocking load is requested for an asset that's already loading asynchronously.
      * Uses the BlockingAssetLoadBus to detect completion, and a semaphore to signal it.
      */

    class WaitForAsset
        : public BlockingAssetLoadBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(WaitForAsset, ThreadPoolAllocator, 0);


        WaitForAsset(const Asset<AssetData>& assetToWaitFor, bool shouldDispatchEvents)
            : m_assetData(assetToWaitFor)
            , m_shouldDispatchEvents(shouldDispatchEvents)
        {
            // Track all blocking requests with the AssetManager.  This enables load jobs to potentially get routed
            // to the thread that's currently blocking waiting on the load job to complete.
            AssetManager::Instance().AddBlockingRequest(m_assetData.GetId(), this);
        }

        ~WaitForAsset() override
        {
            // Stop tracking the blocking request, which will ensure that load jobs won't be provided to this instance
            // for processing.
            AssetManager::Instance().RemoveBlockingRequest(m_assetData.GetId(), this);

            // It shouldn't be possible to destroy a blocking load request before the load job that it's blocked on
            // has been processed, so assert if it ever happens, but make sure to process it just in case.
            if (m_loadJob)
            {
                // (If a valid case is ever found where this can occur, it should be safe to remove the assert)
                AZ_Assert(false, "Blocking load request is being deleted before it could process the blocking load.");
                ProcessLoadJob();
            }
        }

        // Provides a blocked load with a LoadJob to process while it's blocking.
        // Returns true if it can be queued, false if it can't.
        bool QueueAssetLoadJob(LoadAssetJob* loadJob)
        {
            if(m_shouldDispatchEvents)
            {
                // Any load job that is going to be dispatching events should not accept additional work since dispatching events
                // can lead to more code that's blocking on an asset load which prevents us from finishing the dispatch
                // and doing the assigned work.
                // Specifically, if dispatching leads to a second block call, the load job will be assigned to the first block call,
                // which will never be completed until the second block call is finished.  If both blocks are on the same asset,
                // we end up deadlocked.
                return false;
            }

            AZStd::scoped_lock<AZStd::mutex> mutexLock(m_loadJobMutex);

            AZ_Assert(!m_loadJob, "Trying to process multiple load jobs for the same asset with the same blocking handler.");
            if (!m_loadJob)
            {
                m_loadJob = loadJob;
                m_waitEvent.release();
                return true;
            }

            return false;
        }

        void OnLoadComplete() override
        {
            Finish();
        }

        void OnLoadCanceled([[maybe_unused]] const AssetId assetId) override
        {
            Finish();
        }

        void WaitUntilReady()
        {
            BusConnect(m_assetData.GetId());

            Wait();

            BusDisconnect(m_assetData.GetId());
        }

    protected:
        void Wait()
        {
            AZ_PROFILE_SCOPE(AzCore, "WaitForAsset - %s", m_assetData.GetHint().c_str());

            // Continue to loop until the load completes.  (Most of the time in the loop will be spent in a thread-blocking state)
            while (!m_loadCompleted)
            {
                if (m_shouldDispatchEvents)
                {
                    // The event will wake up either when the load finishes, a load job is queued for processing, or every
                    // N milliseconds to see if it should dispatch events.
                    constexpr int MaxWaitBetweenDispatchMs = 1;
                    while (!m_waitEvent.try_acquire_for(AZStd::chrono::milliseconds(MaxWaitBetweenDispatchMs)))
                    {
                        AssetManager::Instance().DispatchEvents();
                    }
                }
                else
                {

                    // Don't wake up until a load job is queued for processing or the load is entirely finished.
                    m_waitEvent.acquire();
                }

                // Check to see if any load jobs have been provided for this thread to process.
                // (Load jobs will attempt to reuse blocked threads before spinning off new job threads)
                ProcessLoadJob();
            }

            // Pump the AssetBus function queue once more after the load has completed in case additional
            // functions have been queued between the last call to DispatchEvents and the completion
            // of the current load job
            if (m_shouldDispatchEvents)
            {
                AssetManager::Instance().DispatchEvents();
            }
        }

        void Finish()
        {
            AZ_PROFILE_FUNCTION(AzCore);
            m_loadCompleted = true;
            m_waitEvent.release();
        }

        bool ProcessLoadJob()
        {
            AZStd::scoped_lock<AZStd::mutex> mutexLock(m_loadJobMutex);
            bool jobProcessed = false;

            if (m_loadJob)
            {
                m_loadJob->Process();
                if (m_loadJob->IsAutoDelete())
                {
                    delete m_loadJob;
                }
                m_loadJob = nullptr;
                jobProcessed = true;
            }

            return jobProcessed;
        }

        Asset<AssetData> m_assetData;
        AZStd::binary_semaphore m_waitEvent;
        const bool m_shouldDispatchEvents{ false };
        LoadAssetJob* m_loadJob{ nullptr };
        AZStd::mutex m_loadJobMutex;
        AZStd::atomic_bool m_loadCompleted{ false };
    };


    /*
     * This class processes async AssetDatabase save jobs
     */
    class SaveAssetJob
        : public AssetDatabaseAsyncJob
    {
    public:
        AZ_CLASS_ALLOCATOR(SaveAssetJob, ThreadPoolAllocator, 0);

        SaveAssetJob(JobContext* jobContext, AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
            : AssetDatabaseAsyncJob(jobContext, true, owner, asset, assetHandler)
        {
        }

        ~SaveAssetJob() override
        {
        }

        void Process() override
        {
            SaveAsset();
        }

        void SaveAsset()
        {
            auto asset = m_asset.GetStrongReference();
            AZ_PROFILE_FUNCTION(AzCore);
            bool isSaved = false;
            AssetStreamInfo saveInfo = m_owner->GetSaveStreamInfoForAsset(asset.GetId(), asset.GetType());
            if (saveInfo.IsValid())
            {
                IO::FileIOStream stream(saveInfo.m_streamName.c_str(), saveInfo.m_streamFlags);
                stream.Seek(saveInfo.m_dataOffset, IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
                isSaved = m_assetHandler->SaveAssetData(asset, &stream);
            }
            // queue broadcast message for delivery on game thread
            AssetBus::QueueEvent(asset.GetId(), &AssetBus::Events::OnAssetSaved, asset, isSaved);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Globals
    EnvironmentVariable<AssetManager*> AssetManager::s_assetDB = nullptr;
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // AssetDatabaseJob
    // [4/3/2014]
    //=========================================================================
    AssetDatabaseJob::AssetDatabaseJob(AssetManager* owner, const Asset<AssetData>& asset, AssetHandler* assetHandler)
    {
        m_owner = owner;
        m_asset = AssetInternal::WeakAsset<AssetData>(asset);
        m_assetHandler = assetHandler;
        owner->AddJob(this);
    }

    //=========================================================================
    // ~AssetDatabaseJob
    // [4/3/2014]
    //=========================================================================
    AssetDatabaseJob::~AssetDatabaseJob()
    {
        // Make sure that the asset reference is cleared out prior to removing the job registration.
        // It's possible that clearing this reference will trigger the cleanup code for the asset, so if we wait for the
        // destructor to clear it *after* the RemoveJob call, then HasActiveJobsOrStreamerRequests() will be able to return
        // false even though the job is still executing asset-related code.
        m_asset = {};
        m_owner->RemoveJob(this);
    }

    //=========================================================================
    // Create
    // [6/12/2012]
    //=========================================================================
    bool AssetManager::Create(const Descriptor& desc)
    {
        AZ_Assert(!s_assetDB || !s_assetDB.Get(), "AssetManager already created!");

        if (!s_assetDB)
        {
            s_assetDB = Environment::CreateVariable<AssetManager*>(kAssetDBInstanceVarName);
        }
        if (!s_assetDB.Get())
        {
            s_assetDB.Set(aznew AssetManager(desc));
        }

        return true;
    }

    //=========================================================================
    // Destroy
    // [6/12/2012]
    //=========================================================================
    void AssetManager::Destroy()
    {
        AZ_Assert(s_assetDB, "AssetManager not created!");
        delete (*s_assetDB);
        *s_assetDB = nullptr;
    }

    //=========================================================================
    // IsReady
    //=========================================================================
    bool AssetManager::IsReady()
    {
        if (!s_assetDB)
        {
            s_assetDB = Environment::FindVariable<AssetManager*>(kAssetDBInstanceVarName);
        }

        return s_assetDB && *s_assetDB;
    }

    //=========================================================================
    // Instance
    //=========================================================================
    AssetManager& AssetManager::Instance()
    {
        if (!s_assetDB)
        {
            s_assetDB = Environment::FindVariable<AssetManager*>(kAssetDBInstanceVarName);
        }

        AZ_Assert(s_assetDB && *s_assetDB, "AssetManager not created!");
        return *(*s_assetDB);
    }

    bool AssetManager::SetInstance(AssetManager* assetManager)
    {
        if (!s_assetDB)
        {
            s_assetDB = Environment::CreateVariable<AssetManager*>(kAssetDBInstanceVarName);
        }

        // The old instance needs to be null or else it will leak on the assignment.
        AZ_Assert(!(*s_assetDB),
            "AssetManager::SetInstance was called without first destroying the old instance and setting it to nullptr.  "
            "This will cause the previous AssetManager instance to leak." );

        (*s_assetDB) = assetManager;
        return true;
    }

    //=========================================================================
    // AssetDatabase
    // [6/12/2012]
    //=========================================================================
    AssetManager::AssetManager(const AssetManager::Descriptor& desc)
        : m_mainThreadId(AZStd::this_thread::get_id())
        , m_debugAssetEvents(AZ::Interface<IDebugAssetEvent>::Get())
    {
        (void)desc;

        AssetManagerBus::Handler::BusConnect();
    }

    //=========================================================================
    // ~AssetManager
    // [6/12/2012]
    //=========================================================================
    AssetManager::~AssetManager()
    {
        PrepareShutDown();

        // Acquire the asset lock to make sure nobody else is trying to do anything fancy with assets
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);

        while (!m_handlers.empty())
        {
            AssetHandlerMap::iterator it = m_handlers.begin();
            AssetHandler* handler = it->second;
            UnregisterHandler(handler);
            delete handler;
        }

        AssetManagerBus::Handler::BusDisconnect();
    }

    //=========================================================================
    // DispatchEvents
    // [04/02/2014]
    //=========================================================================
    void AssetManager::DispatchEvents()
    {
        AZ_PROFILE_FUNCTION(AzCore);
        AssetManagerNotificationBus::Broadcast(&AssetManagerNotificationBus::Events::OnAssetEventsDispatchBegin);
        while (AssetBus::QueuedEventCount())
        {
            AssetBus::ExecuteQueuedEvents();
        }
        AssetManagerNotificationBus::Broadcast(&AssetManagerNotificationBus::Events::OnAssetEventsDispatchEnd);
    }

    //=========================================================================
    void AssetManager::SetAssetInfoUpgradingEnabled(bool enable)
    {
        m_assetInfoUpgradingEnabled = enable;
    }

    bool AssetManager::GetAssetInfoUpgradingEnabled() const
    {
#if defined(_RELEASE)
        // in release ("FINAL") builds, we never do this.
        return false;
#else
        return m_assetInfoUpgradingEnabled;
#endif
    }

    bool AssetManager::ShouldCancelAllActiveJobs() const
    {
        return m_cancelAllActiveJobs;
    }

    void AssetManager::SetParallelDependentLoadingEnabled(bool enable)
    {
        m_enableParallelDependentLoading = enable;
    }

    bool AssetManager::GetParallelDependentLoadingEnabled() const
    {
        return m_enableParallelDependentLoading;
    }

    void AssetManager::PrepareShutDown()
    {
        m_cancelAllActiveJobs = true;

        // We want to ensure that no active load jobs are in flight and
        // therefore we need to wait till all jobs have completed. Please note that jobs get deleted automatically once they complete.
        WaitForActiveJobsAndStreamerRequestsToFinish();

        m_ownedAssetContainerLookup.clear();
        m_ownedAssetContainers.clear();
        m_assetContainers.clear();

        // Ensure that there are no queued events on the AssetBus
        DispatchEvents();
    }

    void AssetManager::WaitForActiveJobsAndStreamerRequestsToFinish()
    {
        while (HasActiveJobsOrStreamerRequests())
        {
            DispatchEvents();
            AZStd::this_thread::yield();
        }
    }

    //=========================================================================
    // RegisterHandler
    // [7/9/2014]
    //=========================================================================
    void AssetManager::RegisterHandler(AssetHandler* handler, const AssetType& assetType)
    {
        AZ_Error("AssetDatabase", handler != nullptr, "Attempting to register a null asset handler!");
        if (handler)
        {
            if (m_handlers.insert(AZStd::make_pair(assetType, handler)).second)
            {
                handler->m_nHandledTypes++;
            }
            else
            {
                AZ_Error("AssetDatabase", false, "Asset type %s already has a handler registered! New registration ignored!", assetType.ToString<AZStd::string>().c_str());
            }
        }
    }

    //=========================================================================
    // UnregisterHandler
    // [7/9/2014]
    //=========================================================================
    void AssetManager::UnregisterHandler(AssetHandler* handler)
    {
        AZ_Error("AssetDatabase", handler != nullptr, "Attempting to unregister a null asset handler!");
        if (handler)
        {
            for (AssetHandlerMap::iterator it = m_handlers.begin(); it != m_handlers.end(); /*++it*/)
            {
                if (it->second == handler)
                {
                    // When unregistering asset handlers, it's possible that there are still some load jobs that have "finished" but
                    // haven't destroyed themselves yet by the time the asset handler gets unregistered.  LoadAssetJob contains a weak
                    // asset reference that doesn't clear until the job is destroyed, which happens *after* the OnAssetReady
                    // notification is triggered.  If the thread gets swapped out between the OnAssetReady and the job destruction,
                    // the job will still be holding onto an asset reference for this asset handler, and it will trigger the
                    // error below.  To ensure that this case doesn't happen, we will instead call
                    // WaitForActiveJobsAndStreamerRequestsToFinish() to make sure that any in-process jobs have completely cleaned
                    // themselves up before proceeding forward.
                    // One example of this pattern occurs in unit tests, where the test loads an asset, validates it, destroys the
                    // asset, and unregisters the handler, all in rapid succession.  This would extremely infrequently
                    // (~1 per 5000 runs) trigger the error case if we didn't wait for the jobs to finish here.
                    WaitForActiveJobsAndStreamerRequestsToFinish();

                    {
                        // this scope is used to control the scope of the lock.
                        AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
                        for (const auto &assetEntry : m_assets)
                        {
                            // is the handler that handles this type, this handler we're removing?
                            if (assetEntry.second->m_registeredHandler == handler)
                            {
                                AZ_Error("AssetManager", false, "Asset handler for %s is being removed, when assetid %s is still loaded!\n",
                                            assetEntry.second->GetType().ToString<AZ::OSString>().c_str(),
                                            assetEntry.second->GetId().ToString<AZ::OSString>().c_str()); // this will write the name IF AVAILABLE
                                assetEntry.second->UnregisterWithHandler();
                            }
                        }
                    }
                    it = m_handlers.erase(it);
                    handler->m_nHandledTypes--;
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    //=========================================================================
    // RegisterCatalog
    // [8/27/2012]
    //=========================================================================
    void AssetManager::RegisterCatalog(AssetCatalog* catalog, const AssetType& assetType)
    {
        AZ_Error("AssetDatabase", catalog != nullptr, "Attempting to register a null catalog!");
        if (catalog)
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> l(m_catalogMutex);
            if (m_catalogs.insert(AZStd::make_pair(assetType, catalog)).second == false)
            {
                AZ_Error("AssetDatabase", false, "Asset type %s already has a catalog registered! New registration ignored!", assetType.ToString<AZStd::string>().c_str());
            }
        }
    }

    //=========================================================================
    // UnregisterCatalog
    // [8/27/2012]
    //=========================================================================
    void AssetManager::UnregisterCatalog(AssetCatalog* catalog)
    {
        AZ_Error("AssetDatabase", catalog != nullptr, "Attempting to unregister a null catalog!");
        if (catalog)
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> l(m_catalogMutex);
            for (AssetCatalogMap::iterator iter = m_catalogs.begin(); iter != m_catalogs.end(); )
            {
                if (iter->second == catalog)
                {
                    iter = m_catalogs.erase(iter);
                }
                else
                {
                    ++iter;
                }

            }
        }
    }

    //=========================================================================
    // GetHandledAssetTypes
    // [6/27/2016]
    //=========================================================================
    void AssetManager::GetHandledAssetTypes(AssetCatalog* catalog, AZStd::vector<AssetType>& assetTypes)
    {
        for (AssetCatalogMap::iterator iter = m_catalogs.begin(); iter != m_catalogs.end(); iter++)
        {
            if (iter->second == catalog)
            {
                assetTypes.push_back(iter->first);
            }
        }
    }

    void AssetManager::SuspendAssetRelease()
    {
        ++m_suspendAssetRelease;
    }

    void AssetManager::ResumeAssetRelease()
    {
        if(--m_suspendAssetRelease != 0)
        {
            return;
        }

        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
        // First, release any containers that were loading this asset
        for (auto asset = m_assets.begin();asset != m_assets.end();)
        {
            if (asset->second->m_useCount == 0)
            {
                auto releaseAsset = asset->second;
                ++asset;
                ReleaseAssetContainersForAsset(releaseAsset);
            }
            else
            {
                ++asset;
            }
        }

        // Second, release the assets themselves

        AZStd::vector<AssetData*> assetsToRelease;

        for(auto&& asset : m_assets)
        {
            if(asset.second->m_weakUseCount == 0)
            {
                // Keep a separate list of assets to release, because releasing them will modify the m_assets list that we're
                // currently looping on.
                assetsToRelease.push_back(asset.second);
            }
        }

        for(auto&& asset : assetsToRelease)
        {
            bool removeFromHash = asset->IsRegisterReadonlyAndShareable();
            // default creation token implies that the asset was not created by the asset manager and therefore it cannot be in the asset map.
            removeFromHash = asset->m_creationToken == s_defaultCreationToken ? false : removeFromHash;

            ReleaseAsset(asset, asset->GetId(), asset->GetType(), removeFromHash, asset->m_creationToken);
        }
    }

    AssetData::AssetStatus AssetManager::BlockUntilLoadComplete(const Asset<AssetData>& asset)
    {
        if(asset.GetStatus() == AssetData::AssetStatus::NotLoaded)
        {
            AZ_Error("AssetManager", false, "BlockUntilLoadComplete must be called after an asset has been queued for load.  Asset %s (%s) is not queued for load",
                asset.GetHint().c_str(), asset.GetId().ToString<AZStd::string>().c_str());
        }
        else if(!asset.IsReady())
        {
            // If this is the main thread we'll need to call DispatchEvents to make sure the events we're waiting on actually fire
            // since the main thread is typically responsible for calling DispatchEvents elsewhere
            const bool shouldDispatch = AZStd::this_thread::get_id() == m_mainThreadId;

            // Wait for the asset and all queued dependencies to finish loading.
            WaitForAsset blockingWait(asset, shouldDispatch);

            blockingWait.WaitUntilReady();
        }

        return asset.GetStatus();
    }

    //=========================================================================
    // FindAsset
    //=========================================================================
    Asset<AssetData> AssetManager::FindAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior)
    {
        // Look up the asset id in the catalog, and use the result of that instead.
        // If assetId is a legacy id, assetInfo.m_assetId will be the canonical id. Otherwise, assetInfo.m_assetID == assetId.
        // This is because only canonical ids are stored in m_assets (see below).
        // Only do the look up if upgrading is enabled
        AZ::Data::AssetInfo assetInfo;
        if (GetAssetInfoUpgradingEnabled())
        {
            AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        }

        // If the catalog is not available, use the original assetId
        const AssetId& assetToFind(assetInfo.m_assetId.IsValid() ? assetInfo.m_assetId : assetId);

        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
        AssetMap::iterator it = m_assets.find(assetToFind);
        if (it != m_assets.end())
        {
            Asset<AssetData> asset(assetReferenceLoadBehavior);
            asset.SetData(it->second);

            return asset;
        }
        return Asset<AssetData>(assetReferenceLoadBehavior);
    }

    AZStd::pair<AZStd::chrono::milliseconds, AZ::IO::IStreamerTypes::Priority> GetEffectiveDeadlineAndPriority(
        const AssetHandler& handler, AssetType assetType, const AssetLoadParameters& loadParams)
    {
        AZStd::chrono::milliseconds deadline;
        AZ::IO::IStreamerTypes::Priority priority;

        handler.GetDefaultAssetLoadPriority(assetType, deadline, priority);

        if (loadParams.m_deadline)
        {
            deadline = loadParams.m_deadline.value();
        }

        if (loadParams.m_priority)
        {
            priority = loadParams.m_priority.value();
        }

        return make_pair(deadline, priority);
    }

    //=========================================================================
    // GetAsset
    // [6/19/2012]
    //=========================================================================
    Asset<AssetData> AssetManager::GetAsset(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior, const AssetLoadParameters& loadParams)
    {
        // If parallel dependent loads are disabled, just try to load the requested asset directly, and let it trigger
        // dependent loads as they're encountered.
        // Parallel dependent loads are disabled during asset building because there is no guarantee that dependency information
        // will be available and complete until after all assets are finished building.
        if(!GetParallelDependentLoadingEnabled())
        {
            return GetAssetInternal(assetId, assetType, assetReferenceLoadBehavior, loadParams);
        }

        // Otherwise, use Asset Containers to load all dependent assets in parallel.

        Asset<AssetData> asset = FindOrCreateAsset(assetId, assetType, assetReferenceLoadBehavior);

        if(!asset || (!loadParams.m_reloadMissingDependencies && asset.IsReady()))
        {
            // If the asset is already ready, just return it and skip the container
            return AZStd::move(asset);
        }

        auto container = GetAssetContainer(asset, loadParams);

        AZStd::scoped_lock lock(m_assetContainerMutex);

        m_ownedAssetContainers.insert({ container.get(), container });

        // Only insert a new entry into m_ownedAssetContainerLookup if one doesn't already exist for this container.
        // Because it's a multimap, it is possible to add duplicate entries by mistake.
        bool entryExists = false;
        auto rangeItr = m_ownedAssetContainerLookup.equal_range(assetId);
        for (auto itr = rangeItr.first; itr != rangeItr.second; ++itr)
        {
            if (itr->second == container.get())
            {
                entryExists = true;
                break;
            }
        }

        // Entry for this container doesn't exist yet, so add it.
        if (!entryExists)
        {
            m_ownedAssetContainerLookup.insert({ assetId, container.get() });
        }

        return asset;
    }

    Asset<AssetData> AssetManager::GetAssetInternal(const AssetId& assetId, [[maybe_unused]] const AssetType& assetType,
        AssetLoadBehavior assetReferenceLoadBehavior, const AssetLoadParameters& loadParams, AssetInfo assetInfo /*= () */, bool signalLoaded /*= false */)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AZ_Error("AssetDatabase", assetId.IsValid(), "GetAsset called with invalid asset Id.");
        AZ_Error("AssetDatabase", !assetType.IsNull(), "GetAsset called with invalid asset type.");
        bool assetMissing = false;

        {
            AZ_PROFILE_SCOPE(AzCore, "GetAsset: GetAssetInfo");

            // Attempt to look up asset info from catalog
            // This is so that when assetId is a legacy id, we're operating on the canonical id anyway
            if (!assetInfo.m_assetId.IsValid())
            {
                AssetCatalogRequestBus::BroadcastResult(assetInfo, &AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
            }

            // If the asset was found in the catalog, ensure the type infos match
            if (assetInfo.m_assetId.IsValid())
            {
                AZ_Warning("AssetManager", assetInfo.m_assetType == assetType,
                    "Requested asset id %s with type %s, but type is actually %s.",
                    assetId.ToString<AZStd::string>().c_str(), assetType.ToString<AZStd::string>().c_str(),
                    assetInfo.m_assetType.ToString<AZStd::string>().c_str());
            }
            else
            {
                AZ_Warning("AssetManager", false, "GetAsset called for asset which does not exist in asset catalog and cannot be loaded.  Asset may be missing, not processed or moved.  AssetId: %s",
                    assetId.ToString<AZStd::string>().c_str());

                // If asset not found, use the id and type given.  We will create a valid asset, but it will likely get an error
                // status below if the asset handler doesn't reroute it to a default asset.
                assetInfo.m_assetId = assetId;
                assetInfo.m_assetType = assetType;
                assetMissing = true;
            }
        }

        AZ_PROFILE_SCOPE(AzCore, "GetAsset: %s", assetInfo.m_relativePath.c_str());
        AZ_ASSET_NAMED_SCOPE("GetAsset: %s", assetInfo.m_relativePath.c_str());

        AZStd::shared_ptr<AssetDataStream> dataStream;
        AssetStreamInfo loadInfo;
        bool triggerAssetErrorNotification = false;
        bool wasUnloaded = false;
        AssetHandler* handler = nullptr;
        AssetData* assetData = nullptr;
        Asset<AssetData> asset; // Used to hold a reference while job is dispatched and while outside of the assetMutex lock.

        // Control the scope of the assetMutex lock
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
            bool isNewEntry = false;

            // check if asset already exists
            {
                AZ_PROFILE_SCOPE(AzCore, "GetAsset: FindAsset");

                AssetMap::iterator it = m_assets.find(assetInfo.m_assetId);
                if (it != m_assets.end())
                {
                    assetData = it->second;
                    asset.SetData(assetData);
                }
                else
                {
                    isNewEntry = true;
                }
            }

            {
                AZ_PROFILE_SCOPE(AzCore, "GetAsset: FindAssetHandler");

                // find the asset type handler
                AssetHandlerMap::iterator handlerIt = m_handlers.find(assetInfo.m_assetType);
                AZ_Error("AssetDatabase", handlerIt != m_handlers.end(), "No handler was registered for this asset [type:%s id:%s]!",
                    assetInfo.m_assetType.ToString<AZ::OSString>().c_str(), assetInfo.m_assetId.ToString<AZ::OSString>().c_str());
                if (handlerIt != m_handlers.end())
                {
                    // Create the asset ptr and insert it into our asset map.
                    handler = handlerIt->second;
                    if (isNewEntry)
                    {
                        AZ_PROFILE_SCOPE(AzCore, "GetAsset: CreateAsset");

                        assetData = handler->CreateAsset(assetInfo.m_assetId, assetInfo.m_assetType);
                        if (assetData)
                        {
                            assetData->m_assetId = assetInfo.m_assetId;
                            assetData->m_creationToken = ++m_creationTokenGenerator;
                            assetData->RegisterWithHandler(handler);
                            asset.SetData(assetData);
                        }
                        else
                        {
                            AZ_Error("AssetDatabase", false, "Failed to create asset with (id=%s, type=%s)",
                                assetInfo.m_assetId.ToString<AZ::OSString>().c_str(),
                                assetInfo.m_assetType.ToString<AZ::OSString>().c_str());
                        }
                    }
                }
            }

            if (assetData)
            {
                if (isNewEntry && assetData->IsRegisterReadonlyAndShareable())
                {
                    AZ_PROFILE_SCOPE(AzCore, "GetAsset: RegisterAsset");
                    m_assets.insert(AZStd::make_pair(assetInfo.m_assetId, assetData));
                }
                if (assetData->GetStatus() == AssetData::AssetStatus::NotLoaded)
                {
                    assetData->m_status = AssetData::AssetStatus::Queued;
                    UpdateDebugStatus(asset);
                    loadInfo = GetModifiedLoadStreamInfoForAsset(asset, handler);
                    wasUnloaded = true;

                    if (loadInfo.IsValid())
                    {
                        // Create the AssetDataStream instance here so it can claim an asset reference inside the lock (for a total
                        // count of 2 before starting the load), otherwise the refcount will be 1, and the load could be canceled
                        // before it is started, which creates state consistency issues.

                        dataStream = AZStd::make_shared<AssetDataStream>(handler->GetAssetBufferAllocator());
                    }
                    else
                    {
                        // Asset creation was successful, but asset loading isn't, so trigger the OnAssetError notification
                        triggerAssetErrorNotification = true;
                    }
                }
            }
        }

        if (!assetInfo.m_relativePath.empty())
        {
            asset.m_assetHint = assetInfo.m_relativePath;
        }

        asset.SetAutoLoadBehavior(assetReferenceLoadBehavior);

        // We delay queueing the async file I/O until we release m_assetMutex
        if (dataStream)
        {
            AZ_Assert(loadInfo.IsValid(), "Expected valid stream info when dataStream is valid.");
            constexpr bool isReload = false;
            QueueAsyncStreamLoad(asset, dataStream, loadInfo, isReload,
                handler, loadParams, signalLoaded);
        }
        else
        {
            AZ_Assert(!loadInfo.IsValid(), "Expected invalid stream info when dataStream is invalid.");

            if(!wasUnloaded && assetData && assetData->GetStatus() == AssetData::AssetStatus::Queued)
            {
                auto&& [deadline, priority] = GetEffectiveDeadlineAndPriority(*handler, assetData->GetType(), loadParams);

                RescheduleStreamerRequest(assetData->GetId(), deadline, priority);
            }

            if (triggerAssetErrorNotification)
            {
                // If the asset was missing from the catalog, we already printed an error, so we can skip printing this one.
                if (!assetMissing)
                {
                    AZ_Error("AssetDatabase", false, "Failed to retrieve required information for asset %s (%s)",
                        assetInfo.m_assetId.ToString<AZ::OSString>().c_str(),
                        assetInfo.m_relativePath.empty() ? "<unknown>" : assetInfo.m_relativePath.c_str());
                }

                PostLoad(asset, false, false, handler);
            }
        }

        return asset;
    }

    void AssetManager::UpdateDebugStatus(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        if(!m_debugAssetEvents)
        {
            m_debugAssetEvents = AZ::Interface<IDebugAssetEvent>::Get();
        }

        if(m_debugAssetEvents)
        {
            m_debugAssetEvents->AssetStatusUpdate(asset.GetId(), asset.GetStatus());
        }
    }

    Asset<AssetData> AssetManager::FindOrCreateAsset(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> asset_lock(m_assetMutex);

        Asset<AssetData> asset = FindAsset(assetId, assetReferenceLoadBehavior);

        if (!asset)
        {
            asset = CreateAsset(assetId, assetType, assetReferenceLoadBehavior);
        }

        return asset;
    }

    //=========================================================================
    // CreateAsset
    // [8/31/2012]
    //=========================================================================
    Asset<AssetData> AssetManager::CreateAsset(const AssetId& assetId, const AssetType& assetType, AssetLoadBehavior assetReferenceLoadBehavior)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> asset_lock(m_assetMutex);

        // check if asset already exist
        AssetMap::iterator it = m_assets.find(assetId);
        if (it == m_assets.end())
        {
            // find the asset type handler
            AssetHandlerMap::iterator handlerIt = m_handlers.find(assetType);
            AZ_Error("AssetDatabase", handlerIt != m_handlers.end(), "No handler was registered for this asset (id=%s, type=%s)!", assetId.ToString<AZ::OSString>().c_str(), assetType.ToString<AZ::OSString>().c_str());
            if (handlerIt != m_handlers.end())
            {
                // Create the asset ptr
                AssetHandler* handler = handlerIt->second;
                auto assetData = handler->CreateAsset(assetId, assetType);
                AZ_Error("AssetDatabase", assetData, "Failed to create asset with (id=%s, type=%s)", assetId.ToString<AZ::OSString>().c_str(), assetType.ToString<AZ::OSString>().c_str());
                if (assetData)
                {
                    assetData->m_assetId = assetId;
                    assetData->m_creationToken = ++m_creationTokenGenerator;
                    assetData->RegisterWithHandler(handler);
                    if (assetData->IsRegisterReadonlyAndShareable())
                    {
                        m_assets.insert(AZStd::make_pair(assetId, assetData));
                    }

                    Asset<AssetData> asset(assetReferenceLoadBehavior);
                    asset.SetData(assetData);

                    return asset;
                }
            }
        }
        else
        {
            AZ_Error("AssetDatabase", false, "Asset (id=%s, type=%s) already exists in the database! Asset not created!", assetId.ToString<AZ::OSString>().c_str(), assetType.ToString<AZ::OSString>().c_str());
        }
        return Asset<AssetData>(assetReferenceLoadBehavior);
    }

    //=========================================================================
    // ReleaseAsset
    //=========================================================================
    void AssetManager::ReleaseAsset(AssetData* asset, AssetId assetId, AssetType assetType, bool removeAssetFromHash, int creationToken)
    {
        AZ_Assert(asset, "Cannot release NULL AssetPtr!");

        if(m_suspendAssetRelease)
        {
            return;
        }

        bool wasInAssetsHash = false; // We do support assets that are not registered in the asset manager (with the same ID too).
        bool destroyAsset = false;

        if (removeAssetFromHash)
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> asset_lock(m_assetMutex);
            AssetMap::iterator it = m_assets.find(assetId);
            // need to check the count again in here in case
           // someone was trying to get the asset on another thread
           // Set it to -1 so only this thread will attempt to clean up the cache and delete the asset
            int expectedRefCount = 0;
            // if the assetId is not in the map or if the identifierId
            // do not match it implies that the asset has been already destroyed.
            // if the usecount is non zero it implies that we cannot destroy this asset.
            if (it != m_assets.end() && it->second->m_creationToken == creationToken && it->second->m_weakUseCount.compare_exchange_strong(expectedRefCount, -1))
            {
                wasInAssetsHash = true;
                m_assets.erase(it);
                destroyAsset = true;
            }
        }
        else
        {
            // if an asset is not shareable, it implies that that asset is not in the map
            // and therefore once its ref count goes to zero it cannot go back up again and therefore we can safely destroy it
            destroyAsset = true;
        }

        // We have to separate the code which was removing the asset from the m_asset map while being locked, but then actually destroy the asset
        // while the lock is not held since destroying the asset while holding the lock can cause a deadlock.
        if (destroyAsset)
        {
            if(m_debugAssetEvents)
            {
                m_debugAssetEvents->ReleaseAsset(assetId);
            }

            // find the asset type handler
            AssetHandlerMap::iterator handlerIt = m_handlers.find(assetType);
            if (handlerIt != m_handlers.end())
            {
                AssetHandler* handler = handlerIt->second;
                if (asset)
                {
                    handler->DestroyAsset(asset);

                    if (wasInAssetsHash)
                    {
                        AssetBus::QueueEvent(assetId, &AssetBus::Events::OnAssetUnloaded, assetId, assetType);
                    }
                }
            }
            else
            {
                AZ_Assert(false, "No handler was registered for asset of type %s but it was still in the AssetManager as %s", assetType.ToString<AZ::OSString>().c_str(), asset->GetId().ToString<AZ::OSString>().c_str());
            }
        }
    }

    void AssetManager::OnAssetUnused(AssetData* asset)
    {
        // If we're currently suspending asset releases, don't get rid of the asset containers either.
        if (m_suspendAssetRelease)
        {
            return;
        }

        ReleaseAssetContainersForAsset(asset);
    }

    void AssetManager::ReleaseAssetContainersForAsset(AssetData* asset)
    {
        // Release any containers that were loading this asset
        AZStd::scoped_lock lock(m_assetContainerMutex);

        AssetId assetId = asset->GetId();

        auto rangeItr = m_ownedAssetContainerLookup.equal_range(assetId);

        for (auto itr = rangeItr.first; itr != rangeItr.second;)
        {
            AZ_Assert(itr->second->GetContainerAssetId() == assetId,
                "Asset container is incorrectly associated with the asset being destroyed.");
            itr->second->ClearRootAsset();

            // Only remove owned asset containers if they aren't currently loading.
            // If they *are* currently loading, removing them could cause dependent asset loads that were triggered to
            // remain in a perpetual loading state.  Instead, leave the containers for now, they will get removed during
            // the OnAssetContainerReady callback.
            if (!itr->second->IsLoading())
            {
                m_ownedAssetContainers.erase(itr->second);
                itr = m_ownedAssetContainerLookup.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
    }

    //=========================================================================
    // SaveAsset
    // [9/13/2012]
    //=========================================================================
    void AssetManager::SaveAsset(const Asset<AssetData>& asset)
    {
        AssetHandler* handler;
        {
            // find the asset type handler
            AssetHandlerMap::iterator handlerIt = m_handlers.find(asset.GetType());
            AZ_Assert(handlerIt != m_handlers.end(), "No handler was registered for this asset [type:%s id:%s]!", asset.GetType().ToString<AZ::OSString>().c_str(), asset.GetId().ToString<AZ::OSString>().c_str());
            handler = handlerIt->second;
        }

        // start the data saving
        SaveAssetJob* saveJob = aznew SaveAssetJob(JobContext::GetGlobalContext(), this, asset, handler);
        saveJob->Start();
    }

    //=========================================================================
    // ReloadAsset
    //=========================================================================
    void AssetManager::ReloadAsset(const AssetId& assetId, AssetLoadBehavior assetReferenceLoadBehavior, bool isAutoReload)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
        auto assetIter = m_assets.find(assetId);

        if (assetIter == m_assets.end() || assetIter->second->IsLoading())
        {
            // Only existing assets can be reloaded.
            return;
        }

        auto reloadIter = m_reloads.find(assetId);
        if (reloadIter != m_reloads.end())
        {
            auto curStatus = reloadIter->second.GetData()->GetStatus();
            // We don't need another reload if we're in "Queued" state because that reload has not actually begun yet.
            // If it is in Loading state we want to pass by and allow the new assetData to be created and start the new reload
            // As the current load could already be stale
            if (curStatus == AssetData::AssetStatus::Queued)
            {
                return;
            }
            else if (curStatus == AssetData::AssetStatus::Loading || curStatus == AssetData::AssetStatus::StreamReady)
            {
                // Don't flood the tick bus - this value will be checked when the asset load completes
                reloadIter->second->SetRequeue(true);
                return;
            }
        }

        AssetData* newAssetData = nullptr;
        AssetHandler* handler = nullptr;

        bool preventAutoReload = isAutoReload && assetIter->second && !assetIter->second->HandleAutoReload();

        // when Asset<T>'s constructor is called (the one that takes an AssetData), it updates the AssetID
        // of the Asset<T> to be the real latest canonical assetId of the asset, so we cache that here instead of have it happen
        // implicitly and repeatedly for anything we call.
        Asset<AssetData> currentAsset(assetIter->second, AZ::Data::AssetLoadBehavior::Default);

        if (!assetIter->second->IsRegisterReadonlyAndShareable() && !preventAutoReload)
        {
            // Reloading an "instance asset" is basically a no-op.
            // We'll simply notify users to reload the asset.
            AssetBus::QueueFunction(&AssetManager::NotifyAssetReloaded, this, currentAsset);
            return;
        }
        else
        {
            AssetBus::QueueFunction(&AssetManager::NotifyAssetPreReload, this, currentAsset);
        }

        // Current AssetData has requested not to be auto reloaded
        if (preventAutoReload)
        {
            return;
        }

        // Resolve the asset handler and allocate new data for the reload.
        {
            AssetHandlerMap::iterator handlerIt = m_handlers.find(currentAsset.GetType());
            AZ_Assert(handlerIt != m_handlers.end(), "No handler was registered for this asset [type:%s id:%s]!",
                currentAsset.GetType().ToString<AZ::OSString>().c_str(), currentAsset.GetId().ToString<AZ::OSString>().c_str());
            handler = handlerIt->second;

            newAssetData = handler->CreateAsset(currentAsset.GetId(), currentAsset.GetType());
            if (newAssetData)
            {
                newAssetData->m_assetId = currentAsset.GetId();
                newAssetData->RegisterWithHandler(handler);
            }
        }

        if (newAssetData)
        {
            // For reloaded assets, we need to hold an internal reference to ensure the data
            // isn't immediately destroyed. Since reloads are not a shipping feature, we'll
            // hold this reference indefinitely, but we'll only hold the most recent one for
            // a given asset Id.

            newAssetData->m_status = AssetData::AssetStatus::Queued;
            Asset<AssetData> newAsset(newAssetData, assetReferenceLoadBehavior);

            m_reloads[newAsset.GetId()] = newAsset;

            UpdateDebugStatus(newAsset);

            AZStd::shared_ptr<AssetDataStream> dataStream;
            AssetStreamInfo loadInfo = GetModifiedLoadStreamInfoForAsset(newAsset, handler);
            constexpr bool isReload = true;
            if (loadInfo.IsValid())
            {
                // Create the AssetDataStream instance here so it can claim an asset reference inside the lock (for a total
                // count of 2 before starting the load), otherwise the refcount will be 1, and the load could be canceled
                // before it is started, which creates state consistency issues.

                dataStream = AZStd::make_shared<AssetDataStream>(handler->GetAssetBufferAllocator());
                if (dataStream)
                {
                    // Currently there isn't a clear use case for needing to adjust priority for reloads so the default load priority is used
                    constexpr bool signalLoaded = false;    // this is a reload, so don't signal dependent-asset loads
                    QueueAsyncStreamLoad(newAsset, dataStream, loadInfo, isReload,
                        handler, {}, signalLoaded);
                }
                else
                {
                    AZ_Assert(false, "Failed to create dataStream to reload asset %s (%s)",
                        newAsset.GetId().ToString<AZ::OSString>().c_str(),
                        newAsset.GetHint().c_str());
                }
            }
            else
            {
                // Asset creation was successful, but asset loading isn't, so trigger the OnAssetError notification
                AZ_Error("AssetDatabase", false, "Failed to retrieve required information for asset %s (%s)",
                    newAsset.GetId().ToString<AZ::OSString>().c_str(),
                    newAsset.GetHint().c_str());

                constexpr bool loadSucceeded = false;
                AssetManager::Instance().PostLoad(newAsset, loadSucceeded, isReload, handler);
            }

        }
    }

    //=========================================================================
    // ReloadAssetFromData
    //=========================================================================
    void AssetManager::ReloadAssetFromData(const Asset<AssetData>& asset)
    {
        bool shouldAssignAssetData = false;

        {
            AZ_Assert(asset.Get(), "Asset data for reload is missing.");
            AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
            AZ_Assert(
                m_assets.find(asset.GetId()) != m_assets.end(),
                "Unable to reload asset %s because it's not in the AssetManager's asset list.", asset.ToString<AZStd::string>().c_str());
            AZ_Assert(
                m_assets.find(asset.GetId()) == m_assets.end() ||
                    asset->RTTI_GetType() == m_assets.find(asset.GetId())->second->RTTI_GetType(),
                "New and old data types are mismatched!");

            auto found = m_assets.find(asset.GetId());
            if ((found == m_assets.end()) || (asset->RTTI_GetType() != found->second->RTTI_GetType()))
            {
                return; // this will just lead to crashes down the line and the above asserts cover this.
            }

            AssetData* newData = asset.Get();

            if (found->second != newData)
            {
                // Notify users that we are about to change asset
                AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetPreReload, asset);

                // Resolve the asset handler and account for the new asset instance.
                {
                    [[maybe_unused]] AssetHandlerMap::iterator handlerIt = m_handlers.find(newData->GetType());
                    AZ_Assert(
                        handlerIt != m_handlers.end(), "No handler was registered for this asset [type:%s id:%s]!",
                        newData->GetType().ToString<AZ::OSString>().c_str(), newData->GetId().ToString<AZ::OSString>().c_str());
                }

                shouldAssignAssetData = true;
            }
        }

        // We specifically perform this outside of the m_assetMutex lock so that the lock isn't held at the point that
        // OnAssetReload is triggered inside of AssignAssetData.  Otherwise, we open up a high potential for deadlocks.
        if (shouldAssignAssetData)
        {
            AssignAssetData(asset);
        }
    }

    //=========================================================================
    // GetHandler
    //=========================================================================
    AssetHandler* AssetManager::GetHandler(const AssetType& assetType)
    {
        auto handlerEntry = m_handlers.find(assetType);
        if (handlerEntry != m_handlers.end())
        {
            return handlerEntry->second;
        }
        return nullptr;
    }

    //=========================================================================
    // AssignAssetData
    //=========================================================================
    void AssetManager::AssignAssetData(const Asset<AssetData>& asset)
    {
        AZ_Assert(asset.Get(), "Reloaded data is missing!");

        const AssetId& assetId = asset.GetId();

        asset->m_status = AssetData::AssetStatus::Ready;
        UpdateDebugStatus(asset);

        if (asset->IsRegisterReadonlyAndShareable())
        {
            bool requeue{ false };
            {
                AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
                auto found = m_assets.find(assetId);
                AZ_Assert(found == m_assets.end() || asset.Get()->RTTI_GetType() == found->second->RTTI_GetType(),
                    "New and old data types are mismatched!");

                // if we are here it implies that we have two assets with the same asset id, and we are
                // trying to replace the old asset with the new asset which was not created using the asset manager system.
                // In this scenario if any other system have cached the old asset then the asset wont be destroyed
                // because of creation token mismatch when it's ref count finally goes to zero. Since the old asset is not shareable anymore
                // manually setting the creationToken to default creation token will ensure that the asset is destroyed correctly.
                asset.m_assetData->m_creationToken = ++m_creationTokenGenerator;
                if (found != m_assets.end())
                {
                    found->second->m_creationToken = AZ::Data::s_defaultCreationToken;
                }

                // Held references to old data are retained, but replace the entry in the DB for future requests.
                // Fire an OnAssetReloaded message so listeners can react to the new data.
                m_assets[assetId] = asset.Get();

                // Release the reload reference.
                auto reloadInfo = m_reloads.find(assetId);
                if (reloadInfo != m_reloads.end())
                {
                    requeue = reloadInfo->second->GetRequeue();
                    m_reloads.erase(reloadInfo);
                }
            }
            // Call reloaded before we can call ReloadAsset below to preserve order
            AssetBus::Event(assetId, &AssetBus::Events::OnAssetReloaded, asset);
            // Release the lock before we call reload
            if (requeue)
            {
                ReloadAsset(assetId, asset.GetAutoLoadBehavior());
            }
        }
        else
        {
            AssetBus::Event(assetId, &AssetBus::Events::OnAssetReloaded, asset);
        }
    }

    //=========================================================================
    // GetModifiedLoadStreamInfoForAsset
    //=========================================================================
    AssetStreamInfo AssetManager::GetModifiedLoadStreamInfoForAsset(const Asset<AssetData>& asset, AssetHandler* handler)
    {
        AssetStreamInfo loadInfo = GetLoadStreamInfoForAsset(asset.GetId(), asset.GetType());
        if (!loadInfo.IsValid())
        {
            // opportunity for handler to do default substitution:
            AZ::Data::AssetId fallbackId = handler->AssetMissingInCatalog(asset);
            if (fallbackId.IsValid())
            {
                loadInfo = GetLoadStreamInfoForAsset(fallbackId, asset.GetType());
            }
        }

        // Give the handler an opportunity to modify any of the load info before creating the dataStream.
        handler->GetCustomAssetStreamInfoForLoad(loadInfo);

        return loadInfo;
    }

    //=========================================================================
    // QueueAsyncStreamLoad
    //=========================================================================
    void AssetManager::QueueAsyncStreamLoad(Asset<AssetData> asset, AZStd::shared_ptr<AssetDataStream> dataStream,
        const AZ::Data::AssetStreamInfo& streamInfo, bool isReload,
        AssetHandler* handler, const AssetLoadParameters& loadParams, bool signalLoaded)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // Set up the callback that will process the asset data once the raw file load is finished.
        // The callback is declared as mutable so that we can clear weakAsset within the callback.  The refcount in weakAsset
        // can trigger an AssetManager::ReleaseAsset call.  If this occurs during lambda cleanup, it could happen at any time
        // on the file streamer thread as streamer requests get recycled, including during (or after) AssetManager shutdown.
        // By controlling when the refcount is changed, we can ensure that it occurs while the AssetManager is still active.
        auto assetDataStreamCallback = [this, loadParams, handler, dataStream, signalLoaded, isReload,
            weakAsset = AssetInternal::WeakAsset<AssetData>(asset)]
        (AZ::IO::IStreamerTypes::RequestStatus status) mutable
        {
            auto assetId = weakAsset.GetId();

            Asset<AssetData> loadingAsset = weakAsset.GetStrongReference();

            if (loadingAsset)
            {
                AZ_PROFILE_SCOPE(AzCore, "AZ::Data::LoadAssetStreamerCallback %s",
                    loadingAsset.GetHint().c_str());
                {
                    AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
                    AssetData* data = loadingAsset.Get();
                    if (data->GetStatus() != AssetData::AssetStatus::Queued)
                    {
                        AZ_Warning("AssetManager", false, "Asset %s no longer in Queued state, abandoning load", loadingAsset.GetId().ToString<AZStd::string>().c_str());
                        return;
                    }
                    data->m_status = AssetData::AssetStatus::StreamReady;
                }

                // The callback from AZ Streamer blocks the streaming thread until this function completes. To minimize the overhead,
                // do the majority of the work in a separate job.
                auto loadJob = aznew LoadAssetJob(this, loadingAsset,
                    dataStream, isReload, status, handler, loadParams, signalLoaded);

                bool jobQueued = false;

                // If there's already an active blocking request waiting for this load to complete, let that thread handle
                // the load itself instead of consuming a second thread.
                {
                    AZStd::scoped_lock<AZStd::recursive_mutex> requestLock(m_activeBlockingRequestMutex);
                    auto range = m_activeBlockingRequests.equal_range(assetId);
                    for(auto blockingRequest = range.first; blockingRequest != range.second; ++blockingRequest)
                    {
                        if(blockingRequest->second->QueueAssetLoadJob(loadJob))
                        {
                            jobQueued = true;
                            break;
                        }
                    }
                }

                if (!jobQueued)
                {
                    loadJob->Start();
                }
            }
            else
            {
                BlockingAssetLoadBus::Event(assetId, &BlockingAssetLoadBus::Events::OnLoadCanceled, assetId);
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetCanceled, assetId);
            }

            // *After* the loadJob has been created, clear our asset references and remove the active streamer requests.
            // This needs to happen after the loadJob creation to ensure that on AssetManager shutdown, there are no brief
            // windows in which requests and/or jobs are still active after we've removed our tracking of the requests and jobs.

            // Also, if the asset references don't get cleared until after the callback completes, or at some indeterminate later
            // time when the File Streamer cleans up the file requests (for the weakAsset lambda parameter), then it's possible that
            // they will trigger a ReleaseAsset call sometime after the AssetManager has begun to shut down, which can lead to
            // race conditions.

            // Make sure the streamer request is removed first before the asset is released
            // If the asset is released first it could lead to a race condition where another thread starts loading the asset
            // again and attempts to add a new streamer request with the same ID before the old one has been removed, causing
            // that load request to fail
            RemoveActiveStreamerRequest(assetId);
            weakAsset = {};
            loadingAsset.Reset();
        };

        auto&& [deadline, priority] = GetEffectiveDeadlineAndPriority(*handler, asset.GetType(), loadParams);

        // Track the load request and queue the asset data stream load.
        AddActiveStreamerRequest(asset.GetId(), dataStream);
        dataStream->Open(
            streamInfo.m_streamName,
            streamInfo.m_dataOffset,
            streamInfo.m_dataLen,
            deadline, priority, assetDataStreamCallback);
    }

    //=========================================================================
    // NotifyAssetReady
    //=========================================================================
    void AssetManager::NotifyAssetReady(Asset<AssetData> asset)
    {
        AssetData* data = asset.Get();
        AZ_Assert(data, "NotifyAssetReady: asset is missing info!");
        data->m_status = AssetData::AssetStatus::Ready;

        AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetReady, asset);
    }

    //=========================================================================
    // NotifyAssetPreReload
    //=========================================================================
    void AssetManager::NotifyAssetPreReload(Asset<AssetData> asset)
    {
        AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetPreReload, asset);
    }

    //=========================================================================
    // NotifyAssetReloaded
    //=========================================================================
    void AssetManager::NotifyAssetReloaded(Asset<AssetData> asset)
    {
        AssignAssetData(asset);
    }

    //=========================================================================
    // NotifyAssetReloaded
    //=========================================================================
    void AssetManager::NotifyAssetReloadError(Asset<AssetData> asset)
    {
        // Failed reloads have no side effects. Just notify observers (error reporting, etc).
        {
            AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);
            m_reloads.erase(asset.GetId());
        }
        AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetReloadError, asset);
    }

    //=========================================================================
    // NotifyAssetError
    //=========================================================================
    void AssetManager::NotifyAssetError(Asset<AssetData> asset)
    {
        asset.Get()->m_status = AssetData::AssetStatus::Error;
        AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetError, asset);
    }

    void AssetManager::NotifyAssetCanceled(AssetId assetId)
    {
        AssetBus::Event(assetId, &AssetBus::Events::OnAssetCanceled, assetId);
    }

    void AssetManager::NotifyAssetContainerReady(Asset<AssetData> asset)
    {
        AssetBus::Event(asset.GetId(), &AssetBus::Events::OnAssetContainerReady, asset);
    }

    //=========================================================================
    // AddJob
    // [04/02/2014]
    //=========================================================================
    void AssetManager::AddJob(AssetDatabaseJob* job)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_activeJobOrRequestMutex);

        m_activeJobs.push_back(*job);
    }

    //=========================================================================
    // ValidateAndRegisterAssetLoading
    //=========================================================================
    bool AssetManager::ValidateAndRegisterAssetLoading(const Asset<AssetData>& asset)
    {
        AssetData* data = asset.Get();
        {

            AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
            if (data)
            {
                // The purpose of this function is to validate this asset is still in a StreamReady
                // and only then continue the load.  We change status to loading if everything
                // is expected which the blocking RegisterAssetLoading call does not do because it
                // is already in loading status
                if (data->GetStatus() != AssetData::AssetStatus::StreamReady)
                {
                    // Something else has attempted to load this asset
                    return false;
                }
                data->m_status = AssetData::AssetStatus::Loading;
                UpdateDebugStatus(asset);
            }
        }

        return true;
    }

    //=========================================================================
    // RegisterAssetLoading
    //=========================================================================
    void AssetManager::RegisterAssetLoading(const Asset<AssetData>& asset)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AssetData* data = asset.Get();
        if (data)
        {
            data->m_status = AssetData::AssetStatus::Loading;
            UpdateDebugStatus(asset);
        }
    }

    //=========================================================================
    // UnregisterAssetLoadingByThread
    //=========================================================================
    void AssetManager::UnregisterAssetLoading([[maybe_unused]] const Asset<AssetData>& asset)
    {
        AZ_PROFILE_FUNCTION(AzCore);
    }

    //=========================================================================
    // RemoveJob
    // [04/02/2014]
    //=========================================================================
    void AssetManager::RemoveJob(AssetDatabaseJob* job)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_activeJobOrRequestMutex);

        m_activeJobs.erase(*job);
    }

    //=========================================================================
    // AddActiveStreamerRequest
    //=========================================================================
    void AssetManager::AddActiveStreamerRequest(AssetId assetId, AZStd::shared_ptr<AssetDataStream> readRequest)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_activeJobOrRequestMutex);

        // Track the request to allow for manual cancellation and for validating completion before AssetManager shutdown
        [[maybe_unused]] auto inserted =
            m_activeAssetDataStreamRequests.insert(AZStd::make_pair(assetId, readRequest));
        AZ_Assert(inserted.second, "Failed to insert streaming request into map for later retrieval by asset.");

    }

    void AssetManager::RescheduleStreamerRequest(AssetId assetId, AZStd::chrono::milliseconds newDeadline, AZ::IO::IStreamerTypes::Priority newPriority)
    {
        AZStd::scoped_lock lock(m_activeJobOrRequestMutex);

        auto iterator = m_activeAssetDataStreamRequests.find(assetId);

        if (iterator != m_activeAssetDataStreamRequests.end())
        {
            iterator->second->Reschedule(newDeadline, newPriority);
        }
    }

    //=========================================================================
    // RemoveActiveStreamerRequest
    //=========================================================================
    void AssetManager::RemoveActiveStreamerRequest(AssetId assetData)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_activeJobOrRequestMutex);
        m_activeAssetDataStreamRequests.erase(assetData);
    }

    //=========================================================================
    // HasActiveJobsOrStreamerRequests
    //=========================================================================
    bool AssetManager::HasActiveJobsOrStreamerRequests()
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_activeJobOrRequestMutex);

        return (!(m_activeJobs.empty() && m_activeAssetDataStreamRequests.empty()));
    }

    //=========================================================================
    // AddBlockingRequest
    //=========================================================================
    void AssetManager::AddBlockingRequest(AssetId assetId, WaitForAsset* blockingRequest)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> requestLock(m_activeBlockingRequestMutex);

        [[maybe_unused]] auto inserted = m_activeBlockingRequests.insert(AZStd::make_pair(assetId, blockingRequest));
        AZ_Assert(inserted.second, "Failed to track blocking request for asset %s", assetId.ToString<AZStd::string>().c_str());
    }

    //=========================================================================
    // RemoveBlockingRequest
    //=========================================================================
    void AssetManager::RemoveBlockingRequest(AssetId assetId, WaitForAsset* blockingRequest)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> requestLock(m_activeBlockingRequestMutex);
        [[maybe_unused]] bool requestFound = false;
        for (auto assetIdIterator = m_activeBlockingRequests.find(assetId); assetIdIterator != m_activeBlockingRequests.end(); )
        {
            if (assetIdIterator->second == blockingRequest)
            {
                m_activeBlockingRequests.erase(assetIdIterator);
                requestFound = true;
                break;
            }
            else
            {
                assetIdIterator++;
            }
        }

        AZ_Assert(requestFound, "Failed to erase blocking request for asset %s", assetId.ToString<AZStd::string>().c_str());
    }


    //=========================================================================
    // GetLoadStreamInfoForAsset()
    // [04/04/2014]
    //=========================================================================
    AssetStreamInfo AssetManager::GetLoadStreamInfoForAsset(const AssetId& assetId, const AssetType& assetType)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> catalogLock(m_catalogMutex);
        AssetCatalogMap::iterator catIt = m_catalogs.find(assetType);
        if (catIt == m_catalogs.end())
        {
            AZ_Error("Asset", false, "Asset [type:%s id:%s] with this type doesn't have a catalog!", assetType.template ToString<AZStd::string>().c_str(), assetId.ToString<AZStd::string>().c_str());
            return AssetStreamInfo();
        }
        return catIt->second->GetStreamInfoForLoad(assetId, assetType);
    }

    //=========================================================================
    // GetSaveStreamInfoForAsset()
    // [04/04/2014]
    //=========================================================================
    AssetStreamInfo AssetManager::GetSaveStreamInfoForAsset(const AssetId& assetId, const AssetType& assetType)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> catalogLock(m_catalogMutex);
        AssetCatalogMap::iterator catIt = m_catalogs.find(assetType);
        if (catIt == m_catalogs.end())
        {
            AZ_Error("Asset", false, "Asset [type:%s id:%s] with this type doesn't have a catalog!", assetType.template ToString<AZStd::string>().c_str(), assetId.ToString<AZStd::string>().c_str());
            return AssetStreamInfo();
        }
        return catIt->second->GetStreamInfoForSave(assetId, assetType);
    }

    //=========================================================================
    // OnAssetReady
    // [04/02/2014]
    //=========================================================================
    void AssetManager::OnAssetReady(const Asset<AssetData>& asset)
    {
        AZ_Assert(asset.Get(), "OnAssetReady fired for an asset with no data.");

        // Set status immediately from within the AssetManagerBus dispatch, so it's committed before anyone is notified (e.g. job to job, via AssetJobBus).
        asset.Get()->m_status = AssetData::AssetStatus::ReadyPreNotify;
        UpdateDebugStatus(asset);

        // Queue broadcast message for delivery on game thread.
        AssetBus::QueueFunction(&AssetManager::NotifyAssetReady, this, Asset<AssetData>(asset));
    }

    //=========================================================================
    // OnAssetError
    //=========================================================================
    void AssetManager::OnAssetError(const Asset<AssetData>& asset)
    {
        // Set status immediately from within the AssetManagerBus dispatch, so it's committed before anyone is notified (e.g. job to job, via AssetJobBus).
        asset.Get()->m_status = AssetData::AssetStatus::Error;
        UpdateDebugStatus(asset);

        // Queue broadcast message for delivery on game thread.
        AssetBus::QueueFunction(&AssetManager::NotifyAssetError, this, Asset<AssetData>(asset));
    }

    void AssetManager::OnAssetCanceled(AssetId assetId)
    {
        // Queue broadcast message for delivery on game thread.
        AssetBus::QueueFunction(&AssetManager::NotifyAssetCanceled, this, assetId);
    }

    void AssetManager::ReleaseOwnedAssetContainer(AssetContainer* assetContainer)
    {
        AZ_Assert(assetContainer, "Trying to release a null assetContainer pointer!");
        AZStd::scoped_lock lock(m_assetContainerMutex);
        auto rangeItr = m_ownedAssetContainerLookup.equal_range(assetContainer->GetContainerAssetId());

        for (auto itr = rangeItr.first; itr != rangeItr.second; ++itr)
        {
            if (itr->second == assetContainer)
            {
                m_ownedAssetContainerLookup.erase(itr);
                break;
            }
        }

        m_ownedAssetContainers.erase(assetContainer);
    }

    void AssetManager::OnAssetContainerReady(AssetContainer* assetContainer)
    {
        AssetBus::QueueFunction([this, assetContainer, asset = assetContainer->GetRootAsset()]()
        {
            NotifyAssetContainerReady(asset);
            ReleaseOwnedAssetContainer(assetContainer);
        });
    }

    void AssetManager::OnAssetContainerCanceled(AssetContainer* assetContainer)
    {
        AssetBus::QueueFunction([this, assetContainer]()
        {
            ReleaseOwnedAssetContainer(assetContainer);
        });
    }

    //=========================================================================
    // OnAssetReloaded
    //=========================================================================
    void AssetManager::OnAssetReloaded(const Asset<AssetData>& asset)
    {
        // Queue broadcast message for delivery on game thread.
        AssetBus::QueueFunction(&AssetManager::NotifyAssetReloaded, this, Asset<AssetData>(asset));
    }

    //=========================================================================
    // OnAssetReloadError
    //=========================================================================
    void AssetManager::OnAssetReloadError(const Asset<AssetData>& asset)
    {
        // Queue broadcast message for delivery on game thread.
        AssetBus::QueueFunction(&AssetManager::NotifyAssetReloadError, this, Asset<AssetData>(asset));
    }


    //=========================================================================
    // AssetHandler
    // [04/03/2014]
    //=========================================================================
    AssetHandler::AssetHandler()
        : m_nHandledTypes(0)
    {
    }

    //=========================================================================
    // ~AssetHandler
    // [04/03/2014]
    //=========================================================================
    AssetHandler::~AssetHandler()
    {
        if (m_nHandledTypes > 0)
        {
            AssetManager::Instance().UnregisterHandler(this);
        }

        AZ_Error("AssetDatabase", m_nHandledTypes == 0, "Asset handler is being destroyed but there are still %d asset types being handled by it!", (int)m_nHandledTypes);
    }

    //=========================================================================
    // LoadAssetDataFromStream
    //=========================================================================
    AssetHandler::LoadResult AssetHandler::LoadAssetDataFromStream(
        const Asset<AssetData>& asset,
        AZStd::shared_ptr<AssetDataStream> stream,
        const AssetFilterCB& assetLoadFilterCB)
    {
        AZ_PROFILE_SCOPE(AzCore, "AssetHandler::LoadAssetData - %s", asset.GetHint().c_str());

#ifdef AZ_ENABLE_TRACING
        auto start = AZStd::chrono::system_clock::now();
#endif

        LoadResult result = LoadAssetData(asset, stream, assetLoadFilterCB);

#ifdef AZ_ENABLE_TRACING
        auto loadMs = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(
                      AZStd::chrono::system_clock::now() - start);
        AZ_Warning("AssetDatabase", (!cl_assetLoadWarningEnable) ||
                   loadMs <= AZStd::chrono::milliseconds(cl_assetLoadWarningMsThreshold),
                   "Load time threshold exceeded: LoadAssetData call for %s took %" PRId64 " ms",
                   asset.GetHint().c_str(), loadMs.count());
#endif

        return result;
    }

    //=========================================================================
    // InitAsset
    // [04/03/2014]
    //=========================================================================
    void AssetHandler::InitAsset(const Asset<AssetData>& asset, bool loadStageSucceeded, bool isReload)
    {
        if (loadStageSucceeded)
        {
            if (isReload)
            {
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetReloaded, asset);
            }
            else
            {
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetReady, asset);
            }
        }
        else
        {
            if (!isReload)
            {
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetError, asset);
            }
            else
            {
                AssetManagerBus::Broadcast(&AssetManagerBus::Events::OnAssetReloadError, asset);
            }
        }
    }

    void AssetManager::ValidateAndPostLoad(AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadSucceeded,
                                           bool isReload, AZ::Data::AssetHandler* assetHandler)
    {
        {
            // We may need to revalidate that this asset hasn't already passed through postLoad
            AZStd::scoped_lock<AZStd::recursive_mutex> assetLock(m_assetMutex);
            if (asset->IsReady() || asset->m_status == AssetData::AssetStatus::LoadedPreReady)
            {
                return;
            }
            asset->m_status = AssetData::AssetStatus::LoadedPreReady;
            UpdateDebugStatus(asset);
        }
        PostLoad(asset, loadSucceeded, isReload, assetHandler);
    }

    void AssetManager::PostLoad(AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadSucceeded,
                                bool isReload, AZ::Data::AssetHandler* assetHandler)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        if (!assetHandler)
        {
            assetHandler = GetHandler(asset.GetType());
        }

        if (assetHandler)
        {
            // Queue the result for dispatch to main thread.
            assetHandler->InitAsset(asset, loadSucceeded, isReload);
        }
        else
        {
            AZ_Warning("AssetManager", false, "Couldn't find handler for asset %s (%s)", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetHint().c_str());
        }

        // Notify any dependent jobs.
        BlockingAssetLoadBus::Event(asset.GetId(), &BlockingAssetLoadBus::Events::OnLoadComplete);

        UnregisterAssetLoading(asset);
    }

    AZStd::shared_ptr<AssetContainer> AssetManager::GetAssetContainer(Asset<AssetData> asset, const AssetLoadParameters& loadParams)
    {
        // If we're doing a custom load through a filter just hand back a one off container
        if (loadParams.m_assetLoadFilterCB)
        {
            return CreateAssetContainer(asset, loadParams);
        }

        AZStd::scoped_lock<AZStd::recursive_mutex> containerLock(m_assetContainerMutex);
        AssetContainerKey containerKey{ asset.GetId(), loadParams };

        auto curIter = m_assetContainers.find(containerKey);
        if (curIter != m_assetContainers.end())
        {
            auto newRef = curIter->second.lock();
            if (newRef && newRef->IsValid())
            {
                return newRef;
            }
            auto newContainer = CreateAssetContainer(asset, loadParams);
            curIter->second = newContainer;
            return newContainer;
        }
        auto newContainer = CreateAssetContainer(asset, loadParams);

        m_assetContainers.insert({ containerKey, newContainer });

        return newContainer;
    }

    AZStd::shared_ptr<AssetContainer> AssetManager::CreateAssetContainer(Asset<AssetData> asset, const AssetLoadParameters& loadParams) const
    {
        return AZStd::shared_ptr<AssetContainer>( aznew AssetContainer(AZStd::move(asset), loadParams));
    }
} // namespace AZ::Data

size_t AZStd::hash<AZ::Data::AssetContainerKey>::operator()(const AZ::Data::AssetContainerKey& obj) const
{
    size_t h = 0;
    hash_combine(h, obj.m_assetId);
    hash_combine(h, obj.m_loadParameters.m_assetLoadFilterCB.operator!());
    hash_combine(h, obj.m_loadParameters.m_deadline.value_or(AZStd::chrono::milliseconds(-1)).count());
    hash_combine(h, obj.m_loadParameters.m_priority.value_or(-1));
    hash_combine(h, obj.m_loadParameters.m_dependencyRules);
    return h;
}
