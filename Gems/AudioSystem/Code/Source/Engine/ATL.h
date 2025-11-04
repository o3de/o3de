/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ATLEntities.h>
#include <ATLComponents.h>
#include <FileCacheManager.h>

#include <ISystem.h>

#if !defined(AUDIO_RELEASE)
namespace AzFramework
{
    class DebugDisplayRequests;
}
#endif

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioTranslationLayer
        : public ISystemEventListener
    {
    public:
        CAudioTranslationLayer();
        ~CAudioTranslationLayer() override;

        CAudioTranslationLayer(const CAudioTranslationLayer& rOther) = delete;          //copy protection
        CAudioTranslationLayer& operator=(const CAudioTranslationLayer& rOther) = delete; //copy protection

        // ISystemEventListener
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
        // ~ISystemEventListener

        bool Initialize();
        bool ShutDown();
        void Update();

        void ProcessRequest(AudioRequestVariant&& request);

        TAudioControlID GetAudioTriggerID(const char* const sAudioTriggerName) const;
        TAudioControlID GetAudioRtpcID(const char* const sAudioRtpcName) const;
        TAudioControlID GetAudioSwitchID(const char* const sAudioSwitchName) const;
        TAudioSwitchStateID GetAudioSwitchStateID(const TAudioControlID nSwitchID, const char* const sAudioSwitchStateName) const;
        TAudioPreloadRequestID GetAudioPreloadRequestID(const char* const sAudioPreloadRequestName) const;
        TAudioEnvironmentID GetAudioEnvironmentID(const char* const sAudioEnvironmentName) const;

        bool ReserveAudioObjectID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName);
        bool ReleaseAudioObjectID(const TAudioObjectID nAudioObjectID);

        bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID);
        bool ReleaseAudioListenerID(const TAudioObjectID nAudioObjectID);
        bool SetAudioListenerOverrideID(const TAudioObjectID nAudioObjectID);

        bool CanProcessRequests() const { return (m_nFlags & eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN) == 0; }

        EAudioRequestStatus ParseControlsData(const char* const pFolderPath, const EATLDataScope eDataScope);
        EAudioRequestStatus ClearControlsData(const EATLDataScope eDataScope);

        const AZStd::string& GetControlsImplSubPath() const;

        TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig);
        void DestroyAudioSource(TAudioSourceId sourceId);

    private:
        EAudioRequestStatus InitializeImplComponent();
        void ReleaseImplComponent();

        EAudioRequestStatus PrepUnprepTriggerAsync(
            CATLAudioObjectBase* const pAudioObject,
            const CATLTrigger* const pTrigger,
            const bool bPrepare);
        EAudioRequestStatus ActivateTrigger(
            CATLAudioObjectBase* const pAudioObject,
            const CATLTrigger* const pTrigger,
            void* const pOwner = nullptr,
            const SATLSourceData* pSourceData = nullptr);
        EAudioRequestStatus StopTrigger(
            CATLAudioObjectBase* const pAudioObject,
            const CATLTrigger* const pTrigger);
        EAudioRequestStatus StopAllTriggers(CATLAudioObjectBase* const pAudioObject, void* const pOwner = nullptr);
        EAudioRequestStatus SetSwitchState(
            CATLAudioObjectBase* const pAudioObject,
            const CATLSwitchState* const pState);
        EAudioRequestStatus SetRtpc(
            CATLAudioObjectBase* const pAudioObject,
            const CATLRtpc* const pRtpc,
            const float fValue);
        EAudioRequestStatus ResetRtpcs(CATLAudioObjectBase* const pAudioObject);
        EAudioRequestStatus SetEnvironment(
            CATLAudioObjectBase* const pAudioObject,
            const CATLAudioEnvironment* const pEnvironment,
            const float fAmount);
        EAudioRequestStatus ResetEnvironments(CATLAudioObjectBase* const pAudioObject);

        EAudioRequestStatus ActivateInternalTrigger(
            CATLAudioObjectBase* const pAudioObject,
            const IATLTriggerImplData* const pTriggerData,
            IATLEventData* const pEventData);
        EAudioRequestStatus StopInternalEvent(
            CATLAudioObjectBase* const pAudioObject,
            const IATLEventData* const pEventData);
        EAudioRequestStatus StopAllInternalEvents(CATLAudioObjectBase* const pAudioObject);
        EAudioRequestStatus SetInternalRtpc(
            CATLAudioObjectBase* const pAudioObject,
            const IATLRtpcImplData* const pRtpcData,
            const float fValue);
        EAudioRequestStatus SetInternalSwitchState(
            CATLAudioObjectBase* const pAudioObject,
            const IATLSwitchStateImplData* const pSwitchStateData);
        EAudioRequestStatus SetInternalEnvironment(
            CATLAudioObjectBase* const pAudioObject,
            const IATLEnvironmentImplData* const pEnvironmentImplData,
            const float fAmount);

        EAudioRequestStatus RefreshAudioSystem(
            const char* const controlsPath, const char* const levelName, TAudioPreloadRequestID levelPreloadId);

        EAudioRequestStatus MuteAll();
        EAudioRequestStatus UnmuteAll();
        EAudioRequestStatus LoseFocus();
        EAudioRequestStatus GetFocus();
        void UpdateSharedData();
        void SetImplLanguage();

        CATLAudioObjectBase* GetRequestObject(TAudioObjectID objectId);

        enum EATLInternalStates : TATLEnumFlagsType
        {
            eAIS_NONE                           = 0,
            eAIS_IS_MUTED                       = AUDIO_BIT(0),
            eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN = AUDIO_BIT(1),
        };

        // Regularly updated common data to be referenced by all components.
        SATLSharedData m_oSharedData;

        // ATLObject containers
        TATLTriggerLookup m_cTriggers;
        TATLRtpcLookup m_cRtpcs;
        TATLSwitchLookup m_cSwitches;
        TATLPreloadRequestLookup m_cPreloadRequests;
        TATLEnvironmentLookup m_cEnvironments;

        CATLGlobalAudioObject* m_pGlobalAudioObject;
        const TAudioObjectID m_nGlobalAudioObjectID;

        TAudioTriggerInstanceID m_nTriggerInstanceIDCounter;
        TAudioSourceId m_nextSourceId;

        // Components
        CAudioEventManager m_oAudioEventMgr;
        CAudioObjectManager m_oAudioObjectMgr;
        CAudioListenerManager m_oAudioListenerMgr;
        CFileCacheManager m_oFileCacheMgr;
        CATLXmlProcessor m_oXmlProcessor;

        // Utility members
        TATLEnumFlagsType m_nFlags;

        AZStd::string m_implSubPath;

        using duration_ms = AZStd::chrono::duration<float, AZStd::milli>;
        AZStd::chrono::steady_clock::time_point m_lastUpdateTime;
        duration_ms m_elapsedTime;

#if !defined(AUDIO_RELEASE)
    public:
        void DrawAudioSystemDebugInfo();
        const CATLDebugNameStore& GetDebugStore() const { return m_oDebugNameStore; }

    private:
        void DrawAudioObjectDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay);
        void DrawATLComponentDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, float posX, float posY);
        void DrawImplMemoryPoolDebugInfo(AzFramework::DebugDisplayRequests& debugDisplay, float posX, float posY);

        CATLDebugNameStore m_oDebugNameStore;
        AZStd::string m_implementationName;
#endif // !AUDIO_RELEASE
    };
} // namespace Audio
