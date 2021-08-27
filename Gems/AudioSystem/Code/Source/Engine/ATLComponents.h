/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/XML/rapidxml.h>

#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

#include <IAudioSystem.h>
#include <ATLAudioObject.h>
#include <ATLEntities.h>
#include <ATLUtils.h>
#include <AudioInternalInterfaces.h>
#include <FileCacheManager.h>


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioObjectIDFactory
    {
    public:
        static TAudioObjectID GetNextID();

        static const TAudioObjectID s_invalidAudioObjectID;
        static const TAudioObjectID s_globalAudioObjectID;
        static const TAudioObjectID s_minValidAudioObjectID;
        static const TAudioObjectID s_maxValidAudioObjectID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioEventManager
    {
    public:
        CAudioEventManager();
        ~CAudioEventManager();

        CAudioEventManager(const CAudioEventManager&) = delete;         // not defined; calls will fail at compile time
        CAudioEventManager& operator=(const CAudioEventManager&) = delete; // not defined; calls will fail at compile time

        void Initialize();
        void Release();
        void Update(const float fUpdateIntervalMS);

        CATLEvent* GetEvent(const EATLSubsystem eSender);
        CATLEvent* LookupID(const TAudioEventID nID) const;
        void ReleaseEvent(CATLEvent* const pEvent);

        size_t GetNumActive() const;

    private:
        CATLEvent* GetImplInstance();
        void ReleaseImplInstance(CATLEvent* const pOldEvent);
        CATLEvent* GetInternalInstance();
        void ReleaseInternalInstance(CATLEvent* const pOldEvent);

        using TActiveEventMap = ATLMapLookupType<TAudioEventID, CATLEvent*>;
        TActiveEventMap m_cActiveAudioEvents;

        CInstanceManager<CATLEvent, TAudioEventID> m_oAudioEventPool;

#if !defined(AUDIO_RELEASE)
    public:
        void SetDebugNameStore(const CATLDebugNameStore* const pDebugNameStore);
        void DrawDebugInfo(IRenderAuxGeom& rAuxGeom, float fPosX, float fPosY) const;

    private:
        const CATLDebugNameStore* m_pDebugNameStore;
#endif // !AUDIO_RELEASE
    };



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioRaycastManager
        : public AudioRaycastRequestBus::Handler
    {
    public:
        AudioRaycastManager();
        ~AudioRaycastManager() override;

        // AudioRaycastRequestBus::Handler interface
        void PushAudioRaycastRequest(const AudioRaycastRequest& request) override;

        void OnPhysicsSubtickFinished();

        // Additional functionality related to processing raycasts...
        void ProcessRaycastResults(float updateIntervalMs);

        using AudioRaycastRequestQueueType = AZStd::vector<AudioRaycastRequest>;
        using AudioRaycastResultQueueType = AZStd::vector<AudioRaycastResult>;

    protected:
        AZStd::mutex m_raycastRequestsMutex;
        AZStd::mutex m_raycastResultsMutex;
        AudioRaycastRequestQueueType m_raycastRequests;
        AudioRaycastResultQueueType m_raycastResults;

        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioObjectManager
    {
    public:
        CAudioObjectManager(CAudioEventManager& refAudioEventManager);
        ~CAudioObjectManager();

        CAudioObjectManager(const CAudioObjectManager&) = delete;           // not defined; calls will fail at compile time
        CAudioObjectManager& operator=(const CAudioObjectManager&) = delete; // not defined; calls will fail at compile time

        void Initialize();
        void Release();
        void Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition);

        bool ReserveID(TAudioObjectID& rAudioObjectID);
        bool ReleaseID(const TAudioObjectID nAudioObjectID);
        CATLAudioObject* LookupID(const TAudioObjectID nAudioObjectID) const;

        void ReportStartedEvent(const CATLEvent* const pEvent);
        void ReportFinishedEvent(const CATLEvent* const pEvent, const bool bSuccess);


        using TActiveObjectMap = ATLMapLookupType<TAudioObjectID, CATLAudioObject*>;

    #if !defined(AUDIO_RELEASE)
        bool ReserveID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName);
        void SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore);
        size_t GetNumAudioObjects() const;
        size_t GetNumActiveAudioObjects() const;
        const TActiveObjectMap& GetActiveAudioObjects() const
        {
            return m_cAudioObjects;
        }
        void DrawPerObjectDebugInfo(IRenderAuxGeom& rAuxGeom, const AZ::Vector3& rListenerPos) const;
        void DrawDebugInfo(IRenderAuxGeom& rAuxGeom, float fPosX, float fPosY) const;

    private:
        CATLDebugNameStore* m_pDebugNameStore;

    #endif // !AUDIO_RELEASE

    private:
        static float s_fVelocityUpdateIntervalMS;

        CATLAudioObject* GetInstance();
        bool ReleaseInstance(CATLAudioObject* const pOldObject);

        TActiveObjectMap m_cAudioObjects;
        CInstanceManager<CATLAudioObject, TAudioObjectID> m_cObjectPool;
        float m_fTimeSinceLastVelocityUpdateMS;

        AudioRaycastManager m_raycastManager;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioListenerManager
    {
    public:
        CAudioListenerManager();
        ~CAudioListenerManager();

        CAudioListenerManager(const CAudioListenerManager&) = delete;           // not defined; calls will fail at compile time
        CAudioListenerManager& operator=(const CAudioListenerManager&) = delete; // not defined; calls will fail at compile time

        void Initialize();
        void Release();
        void Update(const float fUpdateIntervalMS);

        bool ReserveID(TAudioObjectID& rAudioObjectID);
        bool ReleaseID(const TAudioObjectID nAudioObjectID);
        CATLListenerObject* LookupID(const TAudioObjectID nAudioObjectID) const;

        size_t GetNumActive() const;
        void GetDefaultListenerPosition(SATLWorldPosition& oPosition);
        TAudioObjectID GetDefaultListenerID() const
        {
            return m_nDefaultListenerID;
        }
        bool SetOverrideListenerID(const TAudioObjectID nAudioObjectID);
        TAudioObjectID GetOverrideListenerID() const
        {
            return m_listenerOverrideID;
        }

#if !defined(AUDIO_RELEASE)
        void DrawDebugInfo(IRenderAuxGeom& rAuxGeom) const;
#endif // !AUDIO_RELEASE

    private:
        using TListenerPtrContainer = AZStd::vector<CATLListenerObject*, Audio::AudioSystemStdAllocator>;
        using TActiveListenerMap = ATLMapLookupType<TAudioObjectID, CATLListenerObject*>;

        TActiveListenerMap m_cActiveListeners;
        TListenerPtrContainer m_cListenerPool;

        CATLListenerObject* m_pDefaultListenerObject;
        const TAudioObjectID m_nDefaultListenerID;
        TAudioObjectID m_listenerOverrideID;

        // No longer have a maximum, but we can create a number of additional listeners at startup.
        // TODO: Control this by a cvar
        const size_t m_numReservedListeners = 8;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioEventListenerManager
    {
    public:
        CAudioEventListenerManager();
        ~CAudioEventListenerManager();

        CAudioEventListenerManager(const CAudioEventListenerManager& other) = delete;               // Copy protection
        CAudioEventListenerManager& operator=(const CAudioEventListenerManager& other) = delete;    // Copy protection

        void AddRequestListener(const SAudioEventListener& listener);
        void RemoveRequestListener(const SAudioEventListener& listener);
        void NotifyListener(const SAudioRequestInfo* const pRequestInfo);

#if !defined(AUDIO_RELEASE)
        size_t GetNumEventListeners() const
        {
            return m_cListeners.size();
        }
#endif // !AUDIO_RELEASE

    private:
        using TListenerArray = AZStd::vector<SAudioEventListener, Audio::AudioSystemStdAllocator>;
        TListenerArray m_cListeners;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLXmlProcessor
    {
    public:
        CATLXmlProcessor(
            TATLTriggerLookup& rTriggers,
            TATLRtpcLookup& rRtpcs,
            TATLSwitchLookup& rSwitches,
            TATLEnvironmentLookup& rEnvironments,
            TATLPreloadRequestLookup& rPreloadRequests,
            CFileCacheManager& rFileCacheMgr);

        ~CATLXmlProcessor();

        void Initialize();
        void Release();

        void ParseControlsData(const char* const sFolderPath, const EATLDataScope eDataScope);
        void ClearControlsData(const EATLDataScope eDataScope);
        void ParsePreloadsData(const char* const sFolderPath, const EATLDataScope eDataScope);
        void ClearPreloadsData(const EATLDataScope eDataScope);

    private:
        void ParseAudioTriggers(const AZ::rapidxml::xml_node<char>* triggersXmlRoot, const EATLDataScope dataScope);
        void ParseAudioSwitches(const AZ::rapidxml::xml_node<char>* switchesXmlRoot, const EATLDataScope dataScope);
        void ParseAudioRtpcs(const AZ::rapidxml::xml_node<char>* rtpcsXmlRoot, const EATLDataScope dataScope);
        void ParseAudioPreloads(const AZ::rapidxml::xml_node<char>* preloadsXmlRoot, const EATLDataScope dataScope, const char* const folderName);
        void ParseAudioEnvironments(const AZ::rapidxml::xml_node<char>* environmentsXmlRoot, const EATLDataScope dataScope);

        CATLPreloadRequest::TFileEntryIDs LegacyParseFileEntries(const AZ::rapidxml::xml_node<char>* preloadNode, const EATLDataScope dataScope, bool autoLoad);
        CATLPreloadRequest::TFileEntryIDs ParseFileEntries(const AZ::rapidxml::xml_node<char>* preloadNode, const EATLDataScope dataScope, bool autoLoad);

        IATLTriggerImplData* NewAudioTriggerImplDataInternal(const AZ::rapidxml::xml_node<char>* triggerXmlRoot);
        IATLRtpcImplData* NewAudioRtpcImplDataInternal(const AZ::rapidxml::xml_node<char>* rtpcXmlRoot);
        IATLSwitchStateImplData* NewAudioSwitchStateImplDataInternal(const AZ::rapidxml::xml_node<char>* switchXmlRoot);
        IATLEnvironmentImplData* NewAudioEnvironmentImplDataInternal(const AZ::rapidxml::xml_node<char>* environmentXmlRoot);

        void DeleteAudioTrigger(CATLTrigger* const pOldTrigger);
        void DeleteAudioRtpc(CATLRtpc* const pOldRtpc);
        void DeleteAudioSwitch(CATLSwitch* const pOldSwitch);
        void DeleteAudioPreloadRequest(CATLPreloadRequest* const pOldPreloadRequest);
        void DeleteAudioEnvironment(CATLAudioEnvironment* const pOldEnvironment);

        TATLTriggerLookup& m_rTriggers;
        TATLRtpcLookup& m_rRtpcs;
        TATLSwitchLookup& m_rSwitches;
        TATLEnvironmentLookup& m_rEnvironments;
        TATLPreloadRequestLookup& m_rPreloadRequests;

        CFileCacheManager& m_rFileCacheMgr;

        TAudioTriggerImplID m_nTriggerImplIDCounter;
        AZStd::string m_rootPath;

#if !defined(AUDIO_RELEASE)
    public:
        void SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore);
        void SetRootPath(const char* path);

    private:
        CATLDebugNameStore* m_pDebugNameStore;
#endif // !AUDIO_RELEASE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSharedData
    {
        SATLSharedData();
        ~SATLSharedData();

        SATLWorldPosition m_oActiveListenerPosition;
    };
} // namespace Audio
