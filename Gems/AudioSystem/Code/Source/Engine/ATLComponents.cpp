/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLComponents.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/FileFunc/FileFunc.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

#include <AudioFileUtils.h>
#include <ATLCommon.h>
#include <SoundCVars.h>
#include <IAudioSystemImplementation.h>

#include <MathConversion.h>

#if !defined(AUDIO_RELEASE)
    // Debug Draw
    #include <AzCore/std/string/conversions.h>
    #include <AzFramework/Entity/EntityDebugDisplayBus.h>
#endif // !AUDIO_RELEASE


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioObjectIDFactory
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TAudioObjectID AudioObjectIDFactory::s_invalidAudioObjectID = INVALID_AUDIO_OBJECT_ID;
    const TAudioObjectID AudioObjectIDFactory::s_globalAudioObjectID = GLOBAL_AUDIO_OBJECT_ID;
    const TAudioObjectID AudioObjectIDFactory::s_minValidAudioObjectID = (GLOBAL_AUDIO_OBJECT_ID + 1);
    const TAudioObjectID AudioObjectIDFactory::s_maxValidAudioObjectID = static_cast<TAudioObjectID>(-256);
    // Beyond the max ID value, allow for a range of 255 ID values which will be reserved for the audio middleware.

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // static
    TAudioObjectID AudioObjectIDFactory::GetNextID()
    {
        static TAudioObjectID s_nextId = s_minValidAudioObjectID;

        return (s_nextId <= s_maxValidAudioObjectID ? s_nextId++ : s_invalidAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioEventManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioEventManager::CAudioEventManager()
        : m_oAudioEventPool(Audio::CVars::s_AudioEventPoolSize, 1)
    #if !defined(AUDIO_RELEASE)
        , m_pDebugNameStore(nullptr)
    #endif // !AUDIO_RELEASE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioEventManager::~CAudioEventManager()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::Initialize()
    {
        const size_t numActiveAudioEvents = m_cActiveAudioEvents.size();

        for (size_t i = 0; i < m_oAudioEventPool.m_nReserveSize - numActiveAudioEvents; ++i)
        {
            const TAudioEventID nEventID = m_oAudioEventPool.GetNextID();
            IATLEventData* pNewEventData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pNewEventData, &AudioSystemImplementationRequestBus::Events::NewAudioEventData, nEventID);
            auto pNewEvent = azcreate(CATLEvent, (nEventID, eAS_AUDIO_SYSTEM_IMPLEMENTATION, pNewEventData), Audio::AudioSystemAllocator);
            m_oAudioEventPool.m_cReserved.push_back(pNewEvent);
        }

        TActiveEventMap::const_iterator Iter(m_cActiveAudioEvents.begin());
        TActiveEventMap::const_iterator const IterEnd(m_cActiveAudioEvents.end());

        for (; Iter != IterEnd; ++Iter)
        {
            CATLEvent* const pEvent = Iter->second;
            IATLEventData* pNewEventData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pNewEventData, &AudioSystemImplementationRequestBus::Events::NewAudioEventData, pEvent->GetID());
            pEvent->m_pImplData = pNewEventData;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::Release()
    {
        for (auto audioEvent : m_oAudioEventPool.m_cReserved)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEventData, audioEvent->m_pImplData);
            azdestroy(audioEvent, Audio::AudioSystemAllocator);
        }
        m_oAudioEventPool.m_cReserved.clear();

        TActiveEventMap::const_iterator IterActiveAudioEvents(m_cActiveAudioEvents.begin());
        TActiveEventMap::const_iterator const IterActiveAudioEventsEnd(m_cActiveAudioEvents.end());

        for (; IterActiveAudioEvents != IterActiveAudioEventsEnd; ++IterActiveAudioEvents)
        {
            CATLEvent* const pEvent = IterActiveAudioEvents->second;
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::ResetAudioEventData, pEvent->m_pImplData);
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEventData, pEvent->m_pImplData);
            pEvent->m_pImplData = nullptr;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::GetEvent(const EATLSubsystem eSender)
    {
        CATLEvent* pATLEvent = nullptr;

        switch (eSender)
        {
            case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
            {
                pATLEvent = GetImplInstance();
                break;
            }
            case eAS_ATL_INTERNAL:
            {
                pATLEvent = GetInternalInstance();
                break;
            }
            default:
            {
                AZ_Assert(false, "Unknown sender specified in GetEvent (%d)", eSender);
                break;
            }
        }

        if (pATLEvent)
        {
            m_cActiveAudioEvents[pATLEvent->GetID()] = pATLEvent;
        }

        return pATLEvent;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::LookupID(const TAudioEventID nID) const
    {
        auto iPlace = m_cActiveAudioEvents.begin();
        const bool bLookupResult = FindPlaceConst(m_cActiveAudioEvents, nID, iPlace);

        return bLookupResult ? iPlace->second : nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::ReleaseEvent(CATLEvent* const pEvent)
    {
        if (pEvent)
        {
            m_cActiveAudioEvents.erase(pEvent->GetID());

            switch (pEvent->m_eSender)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    ReleaseImplInstance(pEvent);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    ReleaseInternalInstance(pEvent);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown sender specified in ReleaseEvent (%d)", pEvent->m_eSender);
                    break;
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::GetImplInstance()
    {
        CATLEvent* pEvent = nullptr;

        if (!m_oAudioEventPool.m_cReserved.empty())
        {
            //have reserved instances
            pEvent = m_oAudioEventPool.m_cReserved.back();
            m_oAudioEventPool.m_cReserved.pop_back();
        }
        else
        {
            //need to get a new instance
            const TAudioEventID nNewID = m_oAudioEventPool.GetNextID();

            IATLEventData* pNewEventData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pNewEventData, &AudioSystemImplementationRequestBus::Events::NewAudioEventData, nNewID);
            pEvent = azcreate(CATLEvent, (nNewID, eAS_AUDIO_SYSTEM_IMPLEMENTATION, pNewEventData), Audio::AudioSystemAllocator);

            if (!pEvent)
            {
                AZLOG_ERROR("%s", "Failed to get a new instance of an ATLEvent from the implementation.");
            }
        }

        return pEvent;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::ReleaseImplInstance(CATLEvent* const pOldEvent)
    {
        if (pOldEvent)
        {
            pOldEvent->Clear();

            if (m_oAudioEventPool.m_cReserved.size() < m_oAudioEventPool.m_nReserveSize)
            {
                // can return the instance to the reserved pool
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::ResetAudioEventData, pOldEvent->m_pImplData);
                m_oAudioEventPool.m_cReserved.push_back(pOldEvent);
            }
            else
            {
                // the reserve pool is full, can return the instance to the implementation to dispose
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEventData, pOldEvent->m_pImplData);
                azdestroy(pOldEvent, Audio::AudioSystemAllocator);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::GetInternalInstance()
    {
        AZ_Assert(false, "GetInternalInstance was called yet it has no implementation!"); // implement when it is needed
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::ReleaseInternalInstance([[maybe_unused]] CATLEvent* const pOldEvent)
    {
        AZ_Assert(false, "ReleaseInternalInstance was called yet it has no implementation!"); // implement when it is needed
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioEventManager::GetNumActive() const
    {
        return m_cActiveAudioEvents.size();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioObjectManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioObjectManager::CAudioObjectManager([[maybe_unused]] CAudioEventManager& refAudioEventManager)
        : m_cObjectPool(Audio::CVars::s_AudioObjectPoolSize, AudioObjectIDFactory::s_minValidAudioObjectID)
        , m_fTimeSinceLastVelocityUpdateMS(0.0f)
    #if !defined(AUDIO_RELEASE)
        , m_pDebugNameStore(nullptr)
    #endif // !AUDIO_RELEASE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioObjectManager::~CAudioObjectManager()
    {
    }


    float CAudioObjectManager::s_fVelocityUpdateIntervalMS = 100.0f;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition)
    {
        AZ_PROFILE_FUNCTION(Audio);

        m_fTimeSinceLastVelocityUpdateMS += fUpdateIntervalMS;
        const bool bUpdateVelocity = m_fTimeSinceLastVelocityUpdateMS > s_fVelocityUpdateIntervalMS;

        m_raycastManager.ProcessRaycastResults(fUpdateIntervalMS);

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            CATLAudioObject* const pObject = audioObjectPair.second;

            if (pObject->HasActiveEvents())
            {
                AZ_PROFILE_SCOPE(Audio, "Inner Per-Object CAudioObjectManager::Update");

                pObject->Update(fUpdateIntervalMS, rListenerPosition);

                if (pObject->CanRunRaycasts())
                {
                    SATLSoundPropagationData propData;
                    pObject->GetObstOccData(propData);

                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetObstructionOcclusion,
                        pObject->GetImplDataPtr(),
                        propData.fObstruction,
                        propData.fOcclusion);
                }

                if (bUpdateVelocity && pObject->GetVelocityTracking())
                {
                    pObject->UpdateVelocity(m_fTimeSinceLastVelocityUpdateMS);
                }

                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::UpdateAudioObject, pObject->GetImplDataPtr());
            }
        }

        if (bUpdateVelocity)
        {
            m_fTimeSinceLastVelocityUpdateMS = 0.0f;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReserveID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName)
    {
        CATLAudioObject* const pNewObject = GetInstance();

        bool bSuccess = false;
        rAudioObjectID = INVALID_AUDIO_OBJECT_ID;

        if (pNewObject)
        {
            EAudioRequestStatus eImplResult = EAudioRequestStatus::Failure;
            AudioSystemImplementationRequestBus::BroadcastResult(
                eImplResult, &AudioSystemImplementationRequestBus::Events::RegisterAudioObject, pNewObject->GetImplDataPtr(),
                sAudioObjectName);

            if (eImplResult == EAudioRequestStatus::Success)
            {
                pNewObject->IncrementRefCount();
                rAudioObjectID = pNewObject->GetID();
                m_cAudioObjects.emplace(rAudioObjectID, pNewObject);
                bSuccess = true;
            }
            else
            {
                ReleaseInstance(pNewObject);
                bSuccess = false;
            }
        }

        return bSuccess;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReleaseID(const TAudioObjectID nAudioObjectID)
    {
        bool bSuccess = false;
        CATLAudioObject* const pOldObject = LookupID(nAudioObjectID);

        if (pOldObject)
        {
            // If the refcount is one, that means it's the "self" reference and there are no
            // active events, so we can release/recycle the object back to the pool.
            // Otherwise we can decrement the "self" reference and let outstanding events
            // naturally finish and auto-release the object.
            if (pOldObject->GetRefCount() == 1)
            {
                bSuccess = ReleaseInstance(pOldObject);
            }
            else
            {
                pOldObject->DecrementRefCount();
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObject* CAudioObjectManager::LookupID(const TAudioObjectID nID) const
    {
        CATLAudioObject* pResult = nullptr;
        auto it = m_cAudioObjects.find(nID);
        if (it != m_cAudioObjects.end())
        {
            pResult = it->second;
        }

        return pResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::ReportEventFinished(const CATLEvent* atlEvent)
    {
        if (atlEvent)
        {
            if (CATLAudioObject* audioObject = LookupID(atlEvent->m_nObjectID);
                audioObject != nullptr)
            {
                audioObject->EventFinished(atlEvent);

                // EventFinished will decr ref count (assuming the event was valid).
                // This handles a case where ReleaseID was called for the object, but there were
                // active events.  The object's "self" reference has already been decremented.
                // So if the event finishing naturally causes refcount to zero, we can recycle the object.
                if (audioObject->GetRefCount() == 0)
                {
                    ReleaseInstance(audioObject);
                }
            }
#if !defined(AUDIO_RELEASE)
            else
            {
                AZLOG_DEBUG(
                    "Removing Event %llu from object '%s' - Object no longer exists!", atlEvent->GetID(),
                    m_pDebugNameStore->LookupAudioObjectName(atlEvent->m_nObjectID));
            }
#endif // !AUDIO_RELEASE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObject* CAudioObjectManager::GetInstance()
    {
        CATLAudioObject* pObject = nullptr;

        if (!m_cObjectPool.m_cReserved.empty())
        {
            //have reserved instances
            pObject = m_cObjectPool.m_cReserved.back();
            m_cObjectPool.m_cReserved.pop_back();
        }
        else
        {
            //need to get a new instance
            const TAudioObjectID nNewID = AudioObjectIDFactory::GetNextID();
            IATLAudioObjectData* pObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pObjectData, &AudioSystemImplementationRequestBus::Events::NewAudioObjectData, nNewID);

            size_t unallocatedMemorySize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GetUnAllocatedMemory();

            const size_t minimalMemorySize = 100 * 1024;

            if (unallocatedMemorySize < minimalMemorySize)
            {
                AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
                unallocatedMemorySize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GetUnAllocatedMemory();
            }

            if (unallocatedMemorySize >= minimalMemorySize)
            {
                pObject = azcreate(CATLAudioObject, (nNewID, pObjectData), Audio::AudioSystemAllocator);
            }

            if (!pObject)
            {
                AZLOG_ERROR("%s", "Failed to get a new instance of an AudioObject from the implementation. "
                    "If this limit was reached from legitimate content creation and not a scripting error, "
                    "try increasing the Capacity of Audio::AudioSystemAllocator.");
                //failed to get a new instance from the implementation
            }
        }

        return pObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReleaseInstance(CATLAudioObject* const pOldObject)
    {
        bool bSuccess = false;
        if (pOldObject)
        {
            const TAudioObjectID nObjectID = pOldObject->GetID();
            m_cAudioObjects.erase(nObjectID);

        #if !defined(AUDIO_RELEASE)
            m_pDebugNameStore->RemoveAudioObject(nObjectID);
            pOldObject->CheckBeforeRemoval(m_pDebugNameStore);
        #endif // !AUDIO_RELEASE

            pOldObject->Clear();
            EAudioRequestStatus eResult = EAudioRequestStatus::Failure;
            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::UnregisterAudioObject, pOldObject->GetImplDataPtr());
            bSuccess = (eResult == EAudioRequestStatus::Success);

            if (m_cObjectPool.m_cReserved.size() < m_cObjectPool.m_nReserveSize)
            {
                // can return the instance to the reserved pool
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::ResetAudioObject, pOldObject->GetImplDataPtr());
                m_cObjectPool.m_cReserved.push_back(pOldObject);
            }
            else
            {
                // the reserve pool is full, can return the instance to the implementation to dispose
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, pOldObject->GetImplDataPtr());
                azdestroy(pOldObject, Audio::AudioSystemAllocator);
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::Initialize()
    {
        const size_t numRegisteredObjects = m_cAudioObjects.size();

        for (size_t i = 0; i < m_cObjectPool.m_nReserveSize - numRegisteredObjects; ++i)
        {
            const auto nObjectID = AudioObjectIDFactory::GetNextID();
            IATLAudioObjectData* pObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pObjectData, &AudioSystemImplementationRequestBus::Events::NewAudioObjectData, nObjectID);
            auto pObject = azcreate(CATLAudioObject, (nObjectID, pObjectData), Audio::AudioSystemAllocator);
            m_cObjectPool.m_cReserved.push_back(pObject);
        }

        for (const auto& audioObjectPair : m_cAudioObjects)
        {
            CATLAudioObject* const pAudioObject = audioObjectPair.second;
            IATLAudioObjectData* pObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pObjectData, &AudioSystemImplementationRequestBus::Events::NewAudioObjectData, pAudioObject->GetID());
            pAudioObject->SetImplDataPtr(pObjectData);

            char const* szAudioObjectName = nullptr;
        #if !defined(AUDIO_RELEASE)
            szAudioObjectName = m_pDebugNameStore->LookupAudioObjectName(pAudioObject->GetID());
        #endif // !AUDIO_RELEASE

            EAudioRequestStatus eResult = EAudioRequestStatus::Failure;
            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::RegisterAudioObject, pAudioObject->GetImplDataPtr(), szAudioObjectName);
            AZ_Assert(eResult == EAudioRequestStatus::Success, "RegisterAudioObject failed to register object named '%s'", szAudioObjectName);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::Release()
    {
        for (auto audioObject : m_cObjectPool.m_cReserved)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, audioObject->GetImplDataPtr());
            azdestroy(audioObject, Audio::AudioSystemAllocator);
        }

        m_cObjectPool.m_cReserved.clear();

        for (const auto& audioObjectPair : m_cAudioObjects)
        {
            CATLAudioObject* const pAudioObject = audioObjectPair.second;
            if (auto implObject = pAudioObject->GetImplDataPtr())
            {
                EAudioRequestStatus eResult = EAudioRequestStatus::Failure;
                AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::UnregisterAudioObject, implObject);
                AZ_Error("CAudioObjectManager", eResult == EAudioRequestStatus::Success, "Failed to Unregister Audio Object!");
                AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::ResetAudioObject, implObject);
                AZ_Error("CAudioObjectManager", eResult == EAudioRequestStatus::Success, "Failed to Reset Audio Object!");
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, implObject);
                pAudioObject->SetImplDataPtr(nullptr);
            }
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void AudioRaycastManager::PushAudioRaycastRequest(const AudioRaycastRequest& request)
    {
        // [Audio Thread]
        AZStd::scoped_lock lock(m_raycastRequestsMutex);
        m_raycastRequests.push_back(request);
    }

    AudioRaycastManager::AudioRaycastManager()
        : m_sceneFinishSimHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] float fixedDeltatime)
            {
                this->OnPhysicsSubtickFinished();
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Audio))
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
            sceneInterface->RegisterSceneSimulationFinishHandler(sceneHandle, m_sceneFinishSimHandler);
        }
        AudioRaycastRequestBus::Handler::BusConnect();
    }

    AudioRaycastManager::~AudioRaycastManager()
    {
        AudioRaycastRequestBus::Handler::BusDisconnect();
        m_sceneFinishSimHandler.Disconnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void AudioRaycastManager::OnPhysicsSubtickFinished()
    {
        // [Main Thread]
        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (sceneInterface != nullptr)
        {
            sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
        }
        AZ_Warning("AudioRaycastManager", sceneHandle != AzPhysics::InvalidSceneHandle, "Unable to retrive default physics scene.");

        AudioRaycastRequestQueueType processingQueue;

        // Lock and swap the main request container with a local one for processing...
        {
            AZStd::scoped_lock lock(m_raycastRequestsMutex);
            processingQueue.swap(m_raycastRequests);
        }

        AudioRaycastResultQueueType resultsQueue;
        resultsQueue.reserve(processingQueue.size());

        // Process raycasts and enter results in resultsQueue...
        for (const AudioRaycastRequest& request : processingQueue)
        {
            AZ_Assert(request.m_request.m_maxResults <= s_maxHitResultsPerRaycast,
                "Encountered audio raycast request that has maxResults set too high (%" PRIu64 ")!\n", request.m_request.m_maxResults);

            AzPhysics::SceneQueryHits hitResults;
            if (sceneInterface != nullptr)
            {
                hitResults = sceneInterface->QueryScene(sceneHandle, &request.m_request);
            }

            AZ_Error("Audio Raycast", hitResults.m_hits.size() <= s_maxHitResultsPerRaycast,
                "RayCastMultiple returned too many hits (%zu)!\n", hitResults.m_hits.size());

            resultsQueue.emplace_back(AZStd::move(hitResults.m_hits), request.m_audioObjectId, request.m_rayIndex);
        }

        // Lock and swap the local results into the target container (or move-append if necessary)...
        {
            AZStd::scoped_lock lock(m_raycastResultsMutex);
            if (m_raycastResults.empty())
            {
                m_raycastResults.swap(resultsQueue);
            }
            else
            {
                AZStd::move(resultsQueue.begin(), resultsQueue.end(), AZStd::back_inserter(m_raycastResults));
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void AudioRaycastManager::ProcessRaycastResults([[maybe_unused]] float updateIntervalMs)
    {
        // [Audio Thread]
        AZStd::scoped_lock lock(m_raycastResultsMutex);
        for (const AudioRaycastResult& result : m_raycastResults)
        {
            AudioRaycastNotificationBus::Event(result.m_audioObjectId, &AudioRaycastNotificationBus::Events::OnAudioRaycastResults, result);
        }

        m_raycastResults.clear();
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioListenerManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioListenerManager::CAudioListenerManager()
        : m_pDefaultListenerObject(nullptr)
        , m_nDefaultListenerID(AudioObjectIDFactory::GetNextID())
        , m_listenerOverrideID(INVALID_AUDIO_OBJECT_ID)
    {
        m_cListenerPool.reserve(m_numReservedListeners);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioListenerManager::~CAudioListenerManager()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::Initialize()
    {
        IATLListenerData* pNewListenerData = nullptr;

        // Default listener...
        AudioSystemImplementationRequestBus::BroadcastResult(pNewListenerData, &AudioSystemImplementationRequestBus::Events::NewDefaultAudioListenerObjectData, m_nDefaultListenerID);
        m_pDefaultListenerObject = azcreate(CATLListenerObject, (m_nDefaultListenerID, pNewListenerData), Audio::AudioSystemAllocator);
        if (m_pDefaultListenerObject)
        {
            m_cActiveListeners[m_nDefaultListenerID] = m_pDefaultListenerObject;
        }

        // Additional listeners...
        for (size_t listener = 0; listener < m_numReservedListeners; ++listener)
        {
            const TAudioObjectID listenerId = AudioObjectIDFactory::GetNextID();
            AudioSystemImplementationRequestBus::BroadcastResult(pNewListenerData, &AudioSystemImplementationRequestBus::Events::NewAudioListenerObjectData, listenerId);
            auto listenerObject = azcreate(CATLListenerObject, (listenerId, pNewListenerData), Audio::AudioSystemAllocator);
            m_cListenerPool.push_back(listenerObject);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::Release()
    {
        if (m_pDefaultListenerObject) // guard against double deletions
        {
            m_cActiveListeners.erase(m_nDefaultListenerID);

            AudioSystemImplementationRequestBus::Broadcast(
                &AudioSystemImplementationRequestBus::Events::DeleteAudioListenerObjectData, m_pDefaultListenerObject->m_pImplData);
            azdestroy(m_pDefaultListenerObject, Audio::AudioSystemAllocator);
            m_pDefaultListenerObject = nullptr;
        }

        // Release any remaining active audio listeners
        for (auto listenerPair : m_cActiveListeners)
        {
            AudioSystemImplementationRequestBus::Broadcast(
                &AudioSystemImplementationRequestBus::Events::DeleteAudioListenerObjectData, listenerPair.second->m_pImplData);
            azdestroy(listenerPair.second, Audio::AudioSystemAllocator);
        }

        m_cActiveListeners.clear();

        // Delete all remaining listeners from the audio listener pool
        for (auto listener : m_cListenerPool)
        {
            AudioSystemImplementationRequestBus::Broadcast(
                &AudioSystemImplementationRequestBus::Events::DeleteAudioListenerObjectData, listener->m_pImplData);
            azdestroy(listener, Audio::AudioSystemAllocator);
        }

        m_cListenerPool.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioListenerManager::ReserveID(TAudioObjectID& rAudioObjectID)
    {
        bool bSuccess = false;

        if (!m_cListenerPool.empty())
        {
            CATLListenerObject* pListener = m_cListenerPool.back();
            m_cListenerPool.pop_back();

            const TAudioObjectID nID = pListener->GetID();
            m_cActiveListeners.emplace(nID, pListener);

            rAudioObjectID = nID;
            bSuccess = true;
        }
        else
        {
            AZLOG_WARN("%s", "CAudioListenerManager::ReserveID - Reserved pool of pre-allocated Audio Listeners has been exhausted!");
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioListenerManager::ReleaseID(const TAudioObjectID nAudioObjectID)
    {
        bool bSuccess = false;

        CATLListenerObject* pListener = LookupID(nAudioObjectID);
        if (pListener)
        {
            m_cActiveListeners.erase(nAudioObjectID);
            m_cListenerPool.push_back(pListener);
            bSuccess = true;
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLListenerObject* CAudioListenerManager::LookupID(const TAudioObjectID nID) const
    {
        CATLListenerObject* pListenerObject = nullptr;

        TActiveListenerMap::const_iterator iPlace = m_cActiveListeners.begin();

        if (FindPlaceConst(m_cActiveListeners, nID, iPlace))
        {
            pListenerObject = iPlace->second;
        }

        return pListenerObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioListenerManager::GetNumActive() const
    {
        return m_cActiveListeners.size();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::GetDefaultListenerPosition(SATLWorldPosition& oPosition)
    {
        if (m_pDefaultListenerObject)
        {
            oPosition = m_pDefaultListenerObject->oPosition;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioListenerManager::SetOverrideListenerID(const TAudioObjectID nAudioObjectID)
    {
        // If passed ID is INVALID_AUDIO_OBJECT_ID, override is being turned off.
        TActiveListenerMap::const_iterator itBegin = m_cActiveListeners.begin();

        if (nAudioObjectID == INVALID_AUDIO_OBJECT_ID
            || FindPlaceConst(m_cActiveListeners, nAudioObjectID, itBegin))
        {
            m_listenerOverrideID = nAudioObjectID;
            return true;
        }

        return false;
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CATLXMLProcessor
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLXmlProcessor::CATLXmlProcessor(
        TATLTriggerLookup& rTriggers,
        TATLRtpcLookup& rRtpcs,
        TATLSwitchLookup& rSwitches,
        TATLEnvironmentLookup& rEnvironments,
        TATLPreloadRequestLookup& rPreloadRequests,
        CFileCacheManager& rFileCacheMgr)
        : m_rTriggers(rTriggers)
        , m_rRtpcs(rRtpcs)
        , m_rSwitches(rSwitches)
        , m_rEnvironments(rEnvironments)
        , m_rPreloadRequests(rPreloadRequests)
        , m_nTriggerImplIDCounter(AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED)
        , m_rFileCacheMgr(rFileCacheMgr)
        , m_rootPath("@products@")
    #if !defined(AUDIO_RELEASE)
        , m_pDebugNameStore(nullptr)
    #endif // !AUDIO_RELEASE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseControlsData(const char* const folderPath, const EATLDataScope dataScope)
    {
        AZStd::string searchPath;
        AZ::StringFunc::Path::Join(m_rootPath.c_str(), folderPath, searchPath);

        auto foundFiles = Audio::FindFilesInPath(searchPath, "*.xml");

        for (const auto& file : foundFiles)
        {
            AZ_Assert(AZ::IO::FileIOBase::GetInstance()->Exists(file.c_str()), "FindFiles found file '%s' but FileIO says it doesn't exist!", file.c_str());
            AZLOG_INFO("Loading Audio Controls Library: '%s'", file.c_str());

            Audio::ScopedXmlLoader xmlFileLoader(file.Native());
            if (xmlFileLoader.HasError())
            {
                continue;
            }

            const AZ::rapidxml::xml_node<char>* xmlRootNode = xmlFileLoader.GetRootNode();
            if (xmlRootNode)
            {
                auto childNode = xmlRootNode->first_node(nullptr, 0, false);
                while (childNode)
                {
                    if (azstricmp(childNode->name(), ATLXmlTags::TriggersNodeTag) == 0)
                    {
                        ParseAudioTriggers(childNode, dataScope);
                    }
                    else if (azstricmp(childNode->name(), ATLXmlTags::RtpcsNodeTag) == 0)
                    {
                        ParseAudioRtpcs(childNode, dataScope);
                    }
                    else if (azstricmp(childNode->name(), ATLXmlTags::SwitchesNodeTag) == 0)
                    {
                        ParseAudioSwitches(childNode, dataScope);
                    }
                    else if (azstricmp(childNode->name(), ATLXmlTags::EnvironmentsNodeTag) == 0)
                    {
                        ParseAudioEnvironments(childNode, dataScope);
                    }

                    childNode = childNode->next_sibling(nullptr, 0, false);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParsePreloadsData(const char* const folderPath, const EATLDataScope dataScope)
    {
        AZStd::string searchPath;
        AZ::StringFunc::Path::Join(m_rootPath.c_str(), folderPath, searchPath);

        auto foundFiles = Audio::FindFilesInPath(searchPath, "*.xml");

        for (const auto& file : foundFiles)
        {
            AZ_Assert(AZ::IO::FileIOBase::GetInstance()->Exists(file.c_str()), "FindFiles found file '%s' but FileIO says it doesn't exist!", file.c_str());
            AZLOG_INFO("Loading Audio Preloads Library: '%s'", file.c_str());

            Audio::ScopedXmlLoader xmlFileLoader(file.Native());
            if (xmlFileLoader.HasError())
            {
                continue;
            }

            const AZ::rapidxml::xml_node<char>* xmlRootNode = xmlFileLoader.GetRootNode();
            if (xmlRootNode)
            {
                auto childNode = xmlRootNode->first_node(ATLXmlTags::PreloadsNodeTag, 0, false);
                while (childNode)
                {
                    if (dataScope == eADS_LEVEL_SPECIFIC)
                    {
                        AZStd::string_view relativePath(folderPath);
                        AZStd::string levelName;
                        AZ::StringFunc::Path::GetFileName(relativePath.data(), levelName);
                        ParseAudioPreloads(childNode, dataScope, levelName.data());
                    }
                    else
                    {
                        ParseAudioPreloads(childNode, dataScope, nullptr);
                    }

                    childNode = childNode->next_sibling(ATLXmlTags::PreloadsNodeTag, 0, false);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ClearControlsData(const EATLDataScope eDataScope)
    {
        // Remove Triggers...
        for (auto itRemove = m_rTriggers.begin(); itRemove != m_rTriggers.end(); )
        {
            auto const pTrigger = itRemove->second;
            if (eDataScope == eADS_ALL || (pTrigger->GetDataScope() == eDataScope))
            {
            #if !defined(AUDIO_RELEASE)
                m_pDebugNameStore->RemoveAudioTrigger(pTrigger->GetID());
            #endif // !AUDIO_RELEASE

                DeleteAudioTrigger(pTrigger);
                itRemove = m_rTriggers.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }

        // Remove Rtpcs...
        for (auto itRemove = m_rRtpcs.begin(); itRemove != m_rRtpcs.end(); )
        {
            auto const pRtpc = itRemove->second;
            if (eDataScope == eADS_ALL || (pRtpc->GetDataScope() == eDataScope))
            {
            #if !defined(AUDIO_RELEASE)
                m_pDebugNameStore->RemoveAudioRtpc(pRtpc->GetID());
            #endif // !AUDIO_RELEASE

                DeleteAudioRtpc(pRtpc);
                itRemove = m_rRtpcs.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }

        // Remove Switches...
        for (auto itRemove = m_rSwitches.begin(); itRemove != m_rSwitches.end(); )
        {
            auto const pSwitch = itRemove->second;
            if (eDataScope == eADS_ALL || (pSwitch->GetDataScope() == eDataScope))
            {
            #if !defined(AUDIO_RELEASE)
                m_pDebugNameStore->RemoveAudioSwitch(pSwitch->GetID());
            #endif // !AUDIO_RELEASE

                DeleteAudioSwitch(pSwitch);
                itRemove = m_rSwitches.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }

        // Remove Environments...
        for (auto itRemove = m_rEnvironments.begin(); itRemove != m_rEnvironments.end(); )
        {
            auto const pEnvironment = itRemove->second;
            if (eDataScope == eADS_ALL || (pEnvironment->GetDataScope() == eDataScope))
            {
            #if !defined(AUDIO_RELEASE)
                m_pDebugNameStore->RemoveAudioEnvironment(pEnvironment->GetID());
            #endif // !AUDIO_RELEASE

                DeleteAudioEnvironment(pEnvironment);
                itRemove = m_rEnvironments.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioPreloads(const AZ::rapidxml::xml_node<char>* preloadsXmlRoot, const EATLDataScope dataScope, const char* const folderName)
    {
        auto preloadNode = preloadsXmlRoot->first_node(ATLXmlTags::ATLPreloadRequestTag, 0, false);
        while (preloadNode)
        {
            TAudioPreloadRequestID preloadRequestId = ATLInternalControlIDs::GlobalPreloadRequestID;
            const char* preloadRequestName = ATLInternalControlNames::GlobalPreloadRequestName;
            bool autoLoad = false;

            auto loadTypeAttr = preloadNode->first_attribute(ATLXmlTags::ATLTypeAttribute, 0, false);
            auto nameAttr = preloadNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);

            if (loadTypeAttr && azstricmp(loadTypeAttr->value(), ATLXmlTags::ATLDataLoadType) == 0)
            {
                autoLoad = true;
                if (dataScope == eADS_LEVEL_SPECIFIC)
                {
                    preloadRequestName = folderName;
                    preloadRequestId = AudioStringToID<TAudioPreloadRequestID>(preloadRequestName);
                }
            }
            else if (nameAttr)
            {
                preloadRequestName = nameAttr->value();
                preloadRequestId = AudioStringToID<TAudioPreloadRequestID>(preloadRequestName);
            }

            if (preloadRequestId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
            {
                // Legacy support - Try legacy first here because it contains more nodes than the newer format...
                auto fileEntryIds = LegacyParseFileEntries(preloadNode, dataScope, autoLoad);

                if (fileEntryIds.empty())
                {
                    fileEntryIds = ParseFileEntries(preloadNode, dataScope, autoLoad);
                }

                auto it = m_rPreloadRequests.find(preloadRequestId);
                if (it == m_rPreloadRequests.end())
                {
                    auto preloadRequest = azcreate(CATLPreloadRequest, (preloadRequestId, dataScope, autoLoad, fileEntryIds), Audio::AudioSystemAllocator);
                    m_rPreloadRequests[preloadRequestId] = preloadRequest;

                #if !defined(AUDIO_RELEASE)
                    m_pDebugNameStore->AddAudioPreloadRequest(preloadRequestId, preloadRequestName);
                #endif // !AUDIO_RELEASE
                }
                else if (!fileEntryIds.empty())
                {
                    it->second->m_cFileEntryIDs.insert(it->second->m_cFileEntryIDs.end(), fileEntryIds.begin(), fileEntryIds.end());
                }
            }

            preloadNode = preloadNode->next_sibling(ATLXmlTags::ATLPreloadRequestTag, 0, false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLPreloadRequest::TFileEntryIDs CATLXmlProcessor::LegacyParseFileEntries(const AZ::rapidxml::xml_node<char>* preloadNode, const EATLDataScope dataScope, bool autoLoad)
    {
        CATLPreloadRequest::TFileEntryIDs fileEntryIds;

        auto platformsNode = preloadNode->first_node(ATLXmlTags::ATLPlatformsTag, 0, false);
        auto configGroupNode = preloadNode->first_node(ATLXmlTags::ATLConfigGroupTag, 0, false);
        if (platformsNode && configGroupNode)
        {
            const char* configGroupName = nullptr;
            auto platformNode = platformsNode->first_node(ATLXmlTags::PlatformNodeTag, 0, false);
            while (platformNode)
            {
                auto platformAttr = platformNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
                if (platformAttr)
                {
                    if (azstricmp(platformAttr->value(), ATLXmlTags::PlatformName) == 0 || azstricmp(platformAttr->value(), ATLXmlTags::PlatformCodeName) == 0)
                    {
                        auto configGroupAttr = platformNode->first_attribute(ATLXmlTags::ATLConfigGroupAttribute, 0, false);
                        if (configGroupAttr)
                        {
                            configGroupName = configGroupAttr->value();
                            break;
                        }
                    }
                }

                platformNode = platformNode->next_sibling(ATLXmlTags::PlatformNodeTag, 0, false);
            }

            if (configGroupName)
            {
                while (configGroupNode)
                {
                    auto configGroupAttr = configGroupNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
                    if (configGroupAttr && azstricmp(configGroupAttr->value(), configGroupName) == 0)
                    {
                        // Found a config group associated with this platform...
                        auto fileNode = configGroupNode->first_node(nullptr, 0, false);
                        while (fileNode)
                        {
                            TAudioFileEntryID fileEntryId = m_rFileCacheMgr.TryAddFileCacheEntry(fileNode, dataScope, autoLoad);
                            if (fileEntryId != INVALID_AUDIO_FILE_ENTRY_ID)
                            {
                                fileEntryIds.push_back(fileEntryId);
                            }

                            fileNode = fileNode->next_sibling(nullptr, 0, false);
                        }

                        // No need to continue looking through the config groups once a match is found...
                        break;
                    }

                    configGroupNode = configGroupNode->next_sibling(ATLXmlTags::ATLConfigGroupTag, 0, false);
                }
            }
        }

        return fileEntryIds;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLPreloadRequest::TFileEntryIDs CATLXmlProcessor::ParseFileEntries(const AZ::rapidxml::xml_node<char>* preloadNode, const EATLDataScope dataScope, bool autoLoad)
    {
        CATLPreloadRequest::TFileEntryIDs fileEntryIds;
        auto fileNode = preloadNode->first_node(nullptr, 0, false);
        while (fileNode)
        {
            TAudioFileEntryID fileEntryId = m_rFileCacheMgr.TryAddFileCacheEntry(fileNode, dataScope, autoLoad);
            if (fileEntryId != INVALID_AUDIO_FILE_ENTRY_ID)
            {
                fileEntryIds.push_back(fileEntryId);
            }

            fileNode = fileNode->next_sibling(nullptr, 0, false);
        }

        return fileEntryIds;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ClearPreloadsData(const EATLDataScope eDataScope)
    {
        for (auto itRemove = m_rPreloadRequests.begin(); itRemove != m_rPreloadRequests.end(); )
        {
            auto const pRequest = itRemove->second;
            if (eDataScope == eADS_ALL || (pRequest->GetDataScope() == eDataScope))
            {
            #if !defined(AUDIO_RELEASE)
                m_pDebugNameStore->RemoveAudioPreloadRequest(pRequest->GetID());
            #endif // !AUDIO_RELEASE

                DeleteAudioPreloadRequest(pRequest);
                itRemove = m_rPreloadRequests.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioEnvironments(const AZ::rapidxml::xml_node<char>* environmentsXmlRoot, const EATLDataScope dataScope)
    {
        auto environmentNode = environmentsXmlRoot->first_node(ATLXmlTags::ATLEnvironmentTag, 0, false);
        while (environmentNode)
        {
            auto environmentAttr = environmentNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
            const char* atlEnvironmentName = nullptr;
            if (environmentAttr)
            {
                atlEnvironmentName = environmentAttr->value();
            }
            const auto atlEnvironmentId = AudioStringToID<TAudioEnvironmentID>(atlEnvironmentName);

            if ((atlEnvironmentId != INVALID_AUDIO_ENVIRONMENT_ID) && (m_rEnvironments.find(atlEnvironmentId) == m_rEnvironments.end()))
            {
                CATLAudioEnvironment::TImplPtrVec envImpls;

                auto environmentImplNode = environmentNode->first_node(nullptr, 0, false);
                while (environmentImplNode)
                {
                    IATLEnvironmentImplData* environmentImplData = nullptr;
                    EATLSubsystem receiver = eAS_NONE;

                    if (azstricmp(environmentImplNode->name(), ATLXmlTags::ATLEnvironmentRequestTag) == 0)
                    {
                        environmentImplData = NewAudioEnvironmentImplDataInternal(environmentImplNode);
                        receiver = eAS_ATL_INTERNAL;
                    }
                    else
                    {
                        AudioSystemImplementationRequestBus::BroadcastResult(environmentImplData, &AudioSystemImplementationRequestBus::Events::NewAudioEnvironmentImplData, environmentImplNode);
                        receiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                    }

                    if (environmentImplData)
                    {
                        auto environmentImpl = azcreate(CATLEnvironmentImpl, (receiver, environmentImplData), Audio::AudioSystemAllocator);
                        envImpls.push_back(environmentImpl);
                    }

                    environmentImplNode = environmentImplNode->next_sibling(nullptr, 0, false);
                }

                if (!envImpls.empty())
                {
                    auto newEnvironment = azcreate(CATLAudioEnvironment, (atlEnvironmentId, dataScope, envImpls), Audio::AudioSystemAllocator);
                    m_rEnvironments[atlEnvironmentId] = newEnvironment;

                #if !defined(AUDIO_RELEASE)
                    m_pDebugNameStore->AddAudioEnvironment(atlEnvironmentId, atlEnvironmentName);
                #endif // !AUDIO_RELEASE
                }
            }

            environmentNode = environmentNode->next_sibling(ATLXmlTags::ATLEnvironmentTag, 0, false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioTriggers(const AZ::rapidxml::xml_node<char>* triggersXmlRoot, const EATLDataScope dataScope)
    {
        auto triggerNode = triggersXmlRoot->first_node(ATLXmlTags::ATLTriggerTag, 0, false);
        while (triggerNode)
        {
            auto triggerAttr = triggerNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
            const char* atlTriggerName = nullptr;
            if (triggerAttr)
            {
                atlTriggerName = triggerAttr->value();
            }
            const auto atlTriggerId = AudioStringToID<TAudioControlID>(atlTriggerName);

            if ((atlTriggerId != INVALID_AUDIO_CONTROL_ID) && (m_rTriggers.find(atlTriggerId) == m_rTriggers.end()))
            {
                CATLTrigger::TImplPtrVec triggerImpls;

                auto triggerImplNode = triggerNode->first_node(nullptr, 0, false);
                while (triggerImplNode)
                {
                    IATLTriggerImplData* triggerImplData = nullptr;
                    EATLSubsystem receiver = eAS_NONE;

                    if (azstricmp(triggerImplNode->name(), ATLXmlTags::ATLTriggerRequestTag) == 0)
                    {
                        triggerImplData = NewAudioTriggerImplDataInternal(triggerImplNode);
                        receiver = eAS_ATL_INTERNAL;
                    }
                    else
                    {
                        AudioSystemImplementationRequestBus::BroadcastResult(triggerImplData, &AudioSystemImplementationRequestBus::Events::NewAudioTriggerImplData, triggerImplNode);
                        receiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                    }

                    if (triggerImplData)
                    {
                        auto triggerImpl = azcreate(CATLTriggerImpl, (++m_nTriggerImplIDCounter, atlTriggerId, receiver, triggerImplData), Audio::AudioSystemAllocator);
                        triggerImpls.push_back(triggerImpl);
                    }

                    triggerImplNode = triggerImplNode->next_sibling(nullptr, 0, false);
                }

                if (!triggerImpls.empty())
                {
                    auto newTrigger = azcreate(CATLTrigger, (atlTriggerId, dataScope, triggerImpls), Audio::AudioSystemAllocator);
                    m_rTriggers[atlTriggerId] = newTrigger;

                #if !defined(AUDIO_RELEASE)
                    m_pDebugNameStore->AddAudioTrigger(atlTriggerId, atlTriggerName);
                #endif // !AUDIO_RELEASE
                }
            }

            triggerNode = triggerNode->next_sibling(ATLXmlTags::ATLTriggerTag, 0, false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioSwitches(const AZ::rapidxml::xml_node<char>* switchesXmlRoot, const EATLDataScope dataScope)
    {
        auto switchNode = switchesXmlRoot->first_node(ATLXmlTags::ATLSwitchTag, 0, false);
        while (switchNode)
        {
            auto switchAttr = switchNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
            const char* atlSwitchName = nullptr;
            if (switchAttr)
            {
                atlSwitchName = switchAttr->value();
            }
            const auto atlSwitchId = AudioStringToID<TAudioControlID>(atlSwitchName);

            if ((atlSwitchId != INVALID_AUDIO_CONTROL_ID) && (m_rSwitches.find(atlSwitchId) == m_rSwitches.end()))
            {
                auto newSwitch = azcreate(CATLSwitch, (atlSwitchId, dataScope), Audio::AudioSystemAllocator);

            #if !defined(AUDIO_RELEASE)
                m_pDebugNameStore->AddAudioSwitch(atlSwitchId, atlSwitchName);
            #endif // !AUDIO_RELEASE

                auto stateNode = switchNode->first_node(ATLXmlTags::ATLSwitchStateTag, 0, false);
                while (stateNode)
                {
                    auto stateAttr = stateNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
                    const char* atlStateName = nullptr;
                    if (stateAttr)
                    {
                        atlStateName = stateAttr->value();
                    }
                    const auto atlStateId = AudioStringToID<TAudioSwitchStateID>(atlStateName);

                    if (atlStateId != INVALID_AUDIO_SWITCH_STATE_ID)
                    {
                        CATLSwitchState::TImplPtrVec switchStateImplVec;
                        auto stateImplNode = stateNode->first_node(nullptr, 0, false);
                        while (stateImplNode)
                        {
                            IATLSwitchStateImplData* newSwitchStateImplData = nullptr;
                            EATLSubsystem receiver = eAS_NONE;
                            const char* stateImplTag = stateImplNode->name();
                            if (azstricmp(stateImplTag, ATLXmlTags::ATLSwitchRequestTag) == 0)
                            {
                                newSwitchStateImplData = NewAudioSwitchStateImplDataInternal(stateImplNode);
                                receiver = eAS_ATL_INTERNAL;
                            }
                            else
                            {
                                AudioSystemImplementationRequestBus::BroadcastResult(newSwitchStateImplData, &AudioSystemImplementationRequestBus::Events::NewAudioSwitchStateImplData, stateImplNode);
                                receiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                            }

                            if (newSwitchStateImplData)
                            {
                                auto switchStateImpl = azcreate(CATLSwitchStateImpl, (receiver, newSwitchStateImplData), Audio::AudioSystemAllocator);
                                switchStateImplVec.push_back(switchStateImpl);
                            }

                            stateImplNode = stateImplNode->next_sibling(nullptr, 0, false);
                        }

                        auto newState = azcreate(CATLSwitchState, (atlSwitchId, atlStateId, switchStateImplVec), Audio::AudioSystemAllocator);
                        newSwitch->cStates[atlStateId] = newState;

                    #if !defined(AUDIO_RELEASE)
                        m_pDebugNameStore->AddAudioSwitchState(atlSwitchId, atlStateId, atlStateName);
                    #endif // !AUDIO_RELEASE
                    }

                    stateNode = stateNode->next_sibling(ATLXmlTags::ATLSwitchStateTag, 0, false);
                }

                m_rSwitches[atlSwitchId] = newSwitch;
            }

            switchNode = switchNode->next_sibling(ATLXmlTags::ATLSwitchTag, 0, false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioRtpcs(const AZ::rapidxml::xml_node<char>* rtpcsXmlRoot, const EATLDataScope dataScope)
    {
        auto rtpcNode = rtpcsXmlRoot->first_node(ATLXmlTags::ATLRtpcTag, 0, false);
        while (rtpcNode)
        {
            auto rtpcAttr = rtpcNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
            const char* atlRtpcName = rtpcAttr->value();
            const auto atlRtpcId = AudioStringToID<TAudioControlID>(atlRtpcName);

            if ((atlRtpcId != INVALID_AUDIO_CONTROL_ID) && (m_rRtpcs.find(atlRtpcId) == m_rRtpcs.end()))
            {
                CATLRtpc::TImplPtrVec rtpcImpls;

                auto rtpcImplNode = rtpcNode->first_node(nullptr, 0, false);
                while (rtpcImplNode)
                {
                    IATLRtpcImplData* rtpcImplData = nullptr;
                    EATLSubsystem receiver = eAS_NONE;

                    if (azstricmp(rtpcImplNode->name(), ATLXmlTags::ATLRtpcRequestTag) == 0)
                    {
                        rtpcImplData = NewAudioRtpcImplDataInternal(rtpcImplNode);
                        receiver = eAS_ATL_INTERNAL;
                    }
                    else
                    {
                        AudioSystemImplementationRequestBus::BroadcastResult(rtpcImplData, &AudioSystemImplementationRequestBus::Events::NewAudioRtpcImplData, rtpcImplNode);
                        receiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                    }

                    if (rtpcImplData)
                    {
                        auto rtpcImpl = azcreate(CATLRtpcImpl, (receiver, rtpcImplData), Audio::AudioSystemAllocator);
                        rtpcImpls.push_back(rtpcImpl);
                    }

                    rtpcImplNode = rtpcImplNode->next_sibling(nullptr, 0, false);
                }

                if (!rtpcImpls.empty())
                {
                    auto newRtpc = azcreate(CATLRtpc, (atlRtpcId, dataScope, rtpcImpls), Audio::AudioSystemAllocator);
                    m_rRtpcs[atlRtpcId] = newRtpc;

                #if !defined(AUDIO_RELEASE)
                    m_pDebugNameStore->AddAudioRtpc(atlRtpcId, atlRtpcName);
                #endif // !AUDIO_RELEASE
                }
            }

            rtpcNode = rtpcNode->next_sibling(ATLXmlTags::ATLRtpcTag, 0, false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLTriggerImplData* CATLXmlProcessor::NewAudioTriggerImplDataInternal([[maybe_unused]] const AZ::rapidxml::xml_node<char>* triggerXmlRoot)
    {
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLRtpcImplData* CATLXmlProcessor::NewAudioRtpcImplDataInternal([[maybe_unused]] const AZ::rapidxml::xml_node<char>* rtpcXmlRoot)
    {
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLSwitchStateImplData* CATLXmlProcessor::NewAudioSwitchStateImplDataInternal(const AZ::rapidxml::xml_node<char>* switchXmlRoot)
    {
        SATLSwitchStateImplData_internal* switchStateImpl = nullptr;
        auto switchNameAttr = switchXmlRoot->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
        if (switchNameAttr)
        {
            const char* internalSwitchName = switchNameAttr->value();

            auto valueNode = switchXmlRoot->first_node(ATLXmlTags::ATLValueTag, 0, false);
            if (valueNode)
            {
                auto stateNameAttr = valueNode->first_attribute(ATLXmlTags::ATLNameAttribute, 0, false);
                if (stateNameAttr)
                {
                    const char* internalStateName = stateNameAttr->value();

                    const auto internalSwitchId = AudioStringToID<TAudioControlID>(internalSwitchName);
                    const auto internalStateId = AudioStringToID<TAudioSwitchStateID>(internalStateName);

                    if (internalSwitchId != INVALID_AUDIO_CONTROL_ID && internalStateId != INVALID_AUDIO_SWITCH_STATE_ID)
                    {
                        switchStateImpl = azcreate(SATLSwitchStateImplData_internal, (internalSwitchId, internalStateId), Audio::AudioSystemAllocator);
                    }
                }
            }
        }

        return switchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLEnvironmentImplData* CATLXmlProcessor::NewAudioEnvironmentImplDataInternal([[maybe_unused]] const AZ::rapidxml::xml_node<char>* environmentXmlRoot)
    {
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioTrigger(CATLTrigger* const pOldTrigger)
    {
        if (pOldTrigger)
        {
            for (auto const triggerImpl : pOldTrigger->m_cImplPtrs)
            {
                if (triggerImpl->GetReceiver() == eAS_ATL_INTERNAL)
                {
                    azdestroy(triggerImpl->m_pImplData, Audio::AudioSystemAllocator);
                }
                else
                {
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioTriggerImplData, triggerImpl->m_pImplData);
                }

                azdestroy(triggerImpl, Audio::AudioSystemAllocator);
            }

            azdestroy(pOldTrigger, Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioRtpc(CATLRtpc* const pOldRtpc)
    {
        if (pOldRtpc)
        {
            for (auto const rtpcImpl : pOldRtpc->m_cImplPtrs)
            {
                if (rtpcImpl->GetReceiver() == eAS_ATL_INTERNAL)
                {
                    azdestroy(rtpcImpl->m_pImplData, Audio::AudioSystemAllocator);
                }
                else
                {
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioRtpcImplData, rtpcImpl->m_pImplData);
                }

                azdestroy(rtpcImpl, Audio::AudioSystemAllocator);
            }

            azdestroy(pOldRtpc, Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioSwitch(CATLSwitch* const pOldSwitch)
    {
        if (pOldSwitch)
        {
            for (auto& statePair : pOldSwitch->cStates)
            {
                auto switchState = statePair.second;
                if (switchState)
                {
                    for (auto const stateImpl : switchState->m_cImplPtrs)
                    {
                        if (stateImpl->GetReceiver() == eAS_ATL_INTERNAL)
                        {
                            azdestroy(stateImpl->m_pImplData, Audio::AudioSystemAllocator);
                        }
                        else
                        {
                            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioSwitchStateImplData, stateImpl->m_pImplData);
                        }

                        azdestroy(stateImpl, Audio::AudioSystemAllocator);
                    }

                    azdestroy(switchState, Audio::AudioSystemAllocator);
                }
            }

            azdestroy(pOldSwitch, Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioPreloadRequest(CATLPreloadRequest* const pOldPreloadRequest)
    {
        if (pOldPreloadRequest)
        {
            const EATLDataScope eScope = pOldPreloadRequest->GetDataScope();
            for (auto preloadFileId : pOldPreloadRequest->m_cFileEntryIDs)
            {
                m_rFileCacheMgr.TryRemoveFileCacheEntry(preloadFileId, eScope);
            }

            azdestroy(pOldPreloadRequest, Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioEnvironment(CATLAudioEnvironment* const pOldEnvironment)
    {
        if (pOldEnvironment)
        {
            for (auto const environmentImpl : pOldEnvironment->m_cImplPtrs)
            {
                if (environmentImpl->GetReceiver() == eAS_ATL_INTERNAL)
                {
                    azdestroy(environmentImpl->m_pImplData, Audio::AudioSystemAllocator);
                }
                else
                {
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEnvironmentImplData, environmentImpl->m_pImplData);
                }

                azdestroy(environmentImpl, Audio::AudioSystemAllocator);
            }

            azdestroy(pOldEnvironment, Audio::AudioSystemAllocator);
        }
    }


#if !defined(AUDIO_RELEASE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::DrawDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, float fPosX, float fPosY) const
    {
        static const AZ::Color headerColor{ 1.0f, 1.0f, 1.0f, 0.9f };
        static const AZ::Color itemPlayingColor{ 0.3f, 0.6f, 0.3f, 0.9f };
        static const AZ::Color itemLoadingColor{ 0.9f, 0.2f, 0.2f, 0.9f };
        static const AZ::Color itemOtherColor{ 0.8f, 0.8f, 0.8f, 0.9f };

        const float textSize = 0.8f;
        auto headerStr = AZStd::fixed_string<32>::format("Audio Events [%zu]", m_cActiveAudioEvents.size());
        debugDisplay.SetColor(headerColor);
        debugDisplay.Draw2dTextLabel(fPosX, fPosY, textSize, headerStr.c_str());
        fPosX += 20.0f;
        fPosY += 17.0f;

        auto triggerFilter = static_cast<AZ::CVarFixedString>(Audio::CVars::s_AudioTriggersDebugFilter);
        AZStd::to_lower(triggerFilter.begin(), triggerFilter.end());

        for (auto& audioEventPair : m_cActiveAudioEvents)
        {
            auto const atlEvent = audioEventPair.second;

            AZStd::string triggerName(m_pDebugNameStore->LookupAudioTriggerName(atlEvent->m_nTriggerID));
            AZStd::to_lower(triggerName.begin(), triggerName.end());

            if (AudioDebugDrawFilter(triggerName, triggerFilter))
            {
                if (atlEvent->IsPlaying())
                {
                    debugDisplay.SetColor(itemPlayingColor);
                }
                else if (atlEvent->m_audioEventState == eAES_LOADING)
                {
                    debugDisplay.SetColor(itemLoadingColor);
                }
                else
                {
                    debugDisplay.SetColor(itemOtherColor);
                }

                AZStd::string str = AZStd::string::format(
                    "%s (%llu): %s (%llu)", m_pDebugNameStore->LookupAudioObjectName(atlEvent->m_nObjectID), atlEvent->m_nObjectID,
                    triggerName.c_str(), atlEvent->GetID());
                debugDisplay.Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

                fPosY += 16.0f;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::SetDebugNameStore(const CATLDebugNameStore* const pDebugNameStore)
    {
        m_pDebugNameStore = pDebugNameStore;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioObjectManager::GetNumAudioObjects() const
    {
        return m_cAudioObjects.size();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioObjectManager::GetNumActiveAudioObjects() const
    {
        size_t nNumActiveAudioObjects = 0;

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            auto const audioObject = audioObjectPair.second;

            if (audioObject->HasActiveEvents())
            {
                ++nNumActiveAudioObjects;
            }
        }

        return nNumActiveAudioObjects;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::DrawPerObjectDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& rListenerPos) const
    {
        auto audioObjectFilter = static_cast<AZ::CVarFixedString>(Audio::CVars::s_AudioObjectsDebugFilter);
        AZStd::to_lower(audioObjectFilter.begin(), audioObjectFilter.end());

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            auto const audioObject = audioObjectPair.second;

            AZStd::string audioObjectName(m_pDebugNameStore->LookupAudioObjectName(audioObject->GetID()));
            AZStd::to_lower(audioObjectName.begin(), audioObjectName.end());

            bool bDraw = AudioDebugDrawFilter(audioObjectName, audioObjectFilter);

            bDraw = bDraw && (!Audio::CVars::s_ShowActiveAudioObjectsOnly || audioObject->HasActiveEvents());

            if (bDraw)
            {
                audioObject->DrawDebugInfo(debugDisplay, rListenerPos, m_pDebugNameStore);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::DrawDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, float fPosX, float fPosY) const
    {
        static const AZ::Color headerColor{ 1.0f, 1.0f, 1.0f, 0.9f };
        static const AZ::Color itemActiveColor{ 0.3f, 0.6f, 0.3f, 0.9f };
        static const AZ::Color itemInactiveColor{ 0.8f, 0.8f, 0.8f, 0.9f };
        static const AZ::Color overloadColor{ 1.0f, 0.3f, 0.3f, 0.9f };

        AZStd::string str;
        size_t activeObjects = 0;
        size_t aliveObjects = m_cAudioObjects.size();
        size_t remainingObjects = (m_cObjectPool.m_nReserveSize > aliveObjects ? m_cObjectPool.m_nReserveSize - aliveObjects : 0);
        const float fHeaderPosY = fPosY;
        const float textSize = 0.8f;
        fPosX += 20.0f;
        fPosY += 17.0f;

        auto audioObjectFilter = static_cast<AZ::CVarFixedString>(Audio::CVars::s_AudioObjectsDebugFilter);
        AZStd::to_lower(audioObjectFilter.begin(), audioObjectFilter.end());

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            auto const audioObject = audioObjectPair.second;

            AZStd::string audioObjectName(m_pDebugNameStore->LookupAudioObjectName(audioObject->GetID()));
            AZStd::to_lower(audioObjectName.begin(), audioObjectName.end());

            bool bDraw = AudioDebugDrawFilter(audioObjectName, audioObjectFilter);
            bool hasActiveEvents = audioObject->HasActiveEvents();
            bDraw = bDraw && (!Audio::CVars::s_ShowActiveAudioObjectsOnly || hasActiveEvents);

            if (bDraw)
            {
                const AZ::Vector3 position(audioObject->GetPosition().GetPositionVec());
                SATLSoundPropagationData propData;
                audioObject->GetObstOccData(propData);

                str = AZStd::string::format(
                    "[%.2f  %.2f  %.2f] (ID: %llu  Obst: %.2f  Occl: %.2f): %s",
                    position.GetX(), position.GetY(), position.GetZ(), audioObject->GetID(),
                    propData.fObstruction, propData.fOcclusion, audioObjectName.c_str());
                debugDisplay.SetColor(hasActiveEvents ? itemActiveColor : itemInactiveColor);
                debugDisplay.Draw2dTextLabel(fPosX, fPosY, textSize, str.c_str());

                fPosY += 16.0f;
            }

            if (hasActiveEvents)
            {
                ++activeObjects;
            }
        }

        static const char* headerFormat = "Audio Objects [Active : %3zu | Alive: %3zu | Pool: %3zu | Remaining: %3zu]";
        const bool overloaded = (m_cAudioObjects.size() > m_cObjectPool.m_nReserveSize);
        str = AZStd::string::format(headerFormat, activeObjects, aliveObjects,
            m_cObjectPool.m_nReserveSize, remainingObjects);
        debugDisplay.SetColor(overloaded ? overloadColor : headerColor);
        debugDisplay.Draw2dTextLabel(fPosX, fHeaderPosY, textSize, str.c_str());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore)
    {
        m_pDebugNameStore = pDebugNameStore;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore)
    {
        m_pDebugNameStore = pDebugNameStore;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::SetRootPath(const char* path)
    {
        if (path && path[0] != '\0')
        {
            m_rootPath = path;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::DrawDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        static const AZ::Color audioListenerColor(0.2f, 0.6f, 0.9f, 0.9f);
        static const AZ::Color xAxisColor(1.f, 0.f, 0.f, 0.9f);
        static const AZ::Color yAxisColor(0.f, 1.f, 0.f, 0.9f);
        static const AZ::Color zAxisColor(0.f, 0.f, 1.f, 0.9f);

        if (m_pDefaultListenerObject)
        {
            AZ::Vector3 vListenerPos = m_pDefaultListenerObject->oPosition.GetPositionVec();

            // Draw Axes...
            debugDisplay.SetColor(xAxisColor);
            debugDisplay.DrawLine(vListenerPos, vListenerPos + m_pDefaultListenerObject->oPosition.GetRightVec());
            debugDisplay.SetColor(yAxisColor);
            debugDisplay.DrawLine(vListenerPos, vListenerPos + m_pDefaultListenerObject->oPosition.GetForwardVec());
            debugDisplay.SetColor(zAxisColor);
            debugDisplay.DrawLine(vListenerPos, vListenerPos + m_pDefaultListenerObject->oPosition.GetUpVec());

            // Draw Sphere...
            const float radius = 0.05f; // 0.15 meters
            debugDisplay.SetColor(audioListenerColor);
            debugDisplay.DrawWireSphere(vListenerPos, radius);
        }
    }

#endif // !AUDIO_RELEASE

} // namespace Audio
