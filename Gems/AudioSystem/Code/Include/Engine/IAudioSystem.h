/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/parallel/mutex.h>

#include <IAudioInterfacesCommonData.h>

// External forward declarations.
struct SSystemInitParams;

namespace Audio
{
    enum EATLDataScope : TATLEnumFlagsType
    {
        eADS_NONE           = 0,
        eADS_GLOBAL         = 1,
        eADS_LEVEL_SPECIFIC = 2,
        eADS_ALL            = 3,
    };

    enum EAudioControlType : TATLEnumFlagsType
    {
        eACT_NONE           = 0,
        eACT_AUDIO_OBJECT   = 1,
        eACT_TRIGGER        = 2,
        eACT_RTPC           = 3,
        eACT_SWITCH         = 4,
        eACT_SWITCH_STATE   = 5,
        eACT_PRELOAD        = 6,
        eACT_ENVIRONMENT    = 7,
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    AZ_FORCE_INLINE T AudioStringToID(const char* const source)
    {
        return static_cast<T>(AZ::Crc32(source));
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
#define AUDIO_REQUEST_TYPE(NAMESPACE, CLASS, GUID) \
        AZ_RTTI(Audio::NAMESPACE::CLASS, GUID, Audio::AudioRequestBase); \
        using CallbackType = AZStd::function<void(const CLASS&)>; \
        CallbackType m_callback;

    class AudioRequestBase
    {
    public:
        AZ_RTTI(Audio::AudioRequestBase, "{1D31EE4F-8D7F-41FD-87C0-C9BB76555570}");
        AudioRequestBase() = default;
        virtual ~AudioRequestBase() = default;
        AZ_DISABLE_COPY(AudioRequestBase);

        // The audio object to operate on, if applicable.
        // For audio requests under the Audio::Object namespace, this is applicable.
        // If the audio object id is invalid, the request will operate on the global audio object.
        TAudioObjectID m_audioObjectId{ INVALID_AUDIO_OBJECT_ID };

        EAudioRequestStatus m_status{ EAudioRequestStatus::None };
        AZ::u32 m_flags{ 0 };
    };

    namespace SystemRequest
    {
        struct Initialize
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, Initialize, "{8C777214-109E-4C44-A0F5-FCECF601C1E6}");
        };

        struct Shutdown
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, Shutdown, "{33F13AAE-3E08-4C9A-939B-2D111403DD76}");
        };

        struct ReserveObject
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, ReserveObject, "{A0FA000F-EBE1-4DCB-ADA8-9C05B27D71E9}");

            TAudioObjectID m_objectId{ INVALID_AUDIO_OBJECT_ID };   // this will receive the new id
            AZStd::string m_objectName;
        };

        struct CreateSource
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, CreateSource, "{0EE11CCC-9343-4C2B-BB17-191AA673C674}");

            CreateSource(const SAudioInputConfig& config)
                : m_sourceConfig{ config }
            {
            }

            SAudioInputConfig m_sourceConfig{};
        };

        struct DestroySource
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, DestroySource, "{14537418-E888-4078-921D-BD7D24A65269}");

            DestroySource(TAudioSourceId sourceId)
                : m_sourceId{ sourceId }
            {
            }

            TAudioSourceId m_sourceId{ INVALID_AUDIO_SOURCE_ID };
        };

        struct LoadControls
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, LoadControls, "{C47AC196-176B-42F2-9ECE-5D5FFF06BAAB}");

            AZStd::string m_controlsPath;
            EATLDataScope m_scope{ eADS_NONE };
        };

        struct UnloadControls
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, UnloadControls, "{1AAAFBF5-B5A1-4BF0-94A4-58CA2726F9BF}");

            EATLDataScope m_scope{ eADS_NONE };
        };

        struct LoadBank
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, LoadBank, "{12A333E0-1BFC-4A96-8FF9-51A677150268}");

            TAudioPreloadRequestID m_preloadRequestId{ INVALID_AUDIO_PRELOAD_REQUEST_ID };
            bool m_asyncLoad{ true };
            bool m_autoLoadOnly{ false };
        };

        struct UnloadBank
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, UnloadBank, "{974C6644-000C-4710-8EB5-35FC1CD2A7CE}");

            TAudioPreloadRequestID m_preloadRequestId{ INVALID_AUDIO_PRELOAD_REQUEST_ID };
        };

        struct UnloadBanksByScope
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, UnloadBanksByScope, "{15F9FF1D-3444-4317-AA9A-F27C369C5BBD}");

            EATLDataScope m_scope{ eADS_NONE };
        };

        struct ReloadAll
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, ReloadAll, "{318F4F8E-6733-43C3-BC2B-D6BA588854D5}");

            AZStd::string m_controlsPath;
            AZStd::string m_levelName;
            TAudioPreloadRequestID m_levelPreloadId{ INVALID_AUDIO_PRELOAD_REQUEST_ID };
        };

        struct LoseFocus
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, LoseFocus, "{8F316485-BDC4-43F2-9333-E85B107C1321}");
        };

        struct GetFocus
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, GetFocus, "{ABBB7450-767B-4B46-825C-19970C1A15DE}");
        };

        struct MuteAll
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, MuteAll, "{9D3888DC-789D-4570-AB2E-85EB21509AA5}");
        };

        struct UnmuteAll
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, UnmuteAll, "{7A3AEF49-3152-44F5-AF51-A5D050611161}");
        };

        struct StopAllAudio
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, StopAllAudio, "{5D07B532-E4F5-4D37-826F-AAA26A53DFE7}");
        };

        struct DrawDebug
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, DrawDebug, "{D85EDE10-382C-4748-B888-C442008D6401}");
        };

        struct ChangeLanguage
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, ChangeLanguage, "{7AEEEF69-9685-4473-9EDB-ADC5D467998C}");
        };

        struct SetPanningMode
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(SystemRequest, SetPanningMode, "{FEB8075C-4DE1-4117-AE61-E196800BA222}");

            PanningMode m_panningMode{ PanningMode::Speakers };
        };

    } // namespace System


    namespace ObjectRequest
    {
        struct ExecuteTrigger
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, ExecuteTrigger, "{E801D6A2-EB87-4733-AC8C-AEEE91E47847}");

            TAudioControlID m_triggerId{ INVALID_AUDIO_CONTROL_ID };
            void* m_owner{ nullptr };
        };

        struct ExecuteSourceTrigger : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, ExecuteSourceTrigger, "{FDA0D264-F66C-40A4-BD07-28986D2B8882}");

            TAudioControlID m_triggerId{ INVALID_AUDIO_CONTROL_ID };
            SAudioSourceInfo m_sourceInfo{};
            void* m_owner{ nullptr };
        };

        struct PrepareTrigger
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, PrepareTrigger, "{33B13ED8-A2DC-407D-8C5B-CEC2444822F5}");

            TAudioControlID m_triggerId{ INVALID_AUDIO_CONTROL_ID };
        };

        struct UnprepareTrigger
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, UnprepareTrigger, "{358F485D-25F2-4EBF-AA50-AE6972221535}");

            TAudioControlID m_triggerId{ INVALID_AUDIO_CONTROL_ID };
        };

        struct StopTrigger
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, StopTrigger, "{91CCADDD-03E0-47F6-BB17-33576D793175}");

            TAudioControlID m_triggerId{ INVALID_AUDIO_CONTROL_ID };
            void* m_owner{ nullptr };
        };

        struct StopAllTriggers
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, StopAllTriggers, "{7A222303-A170-4805-9A15-B82B26EFFBF8}");

            bool m_filterByOwner{ false };
            void* m_owner{ nullptr };
        };

        struct SetPosition
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, SetPosition, "{A7AAA8D1-75A7-4F94-A3ED-3D5A80429569}");

            SATLWorldPosition m_position{};
        };

        struct SetParameterValue
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, SetParameterValue, "{10D950F1-2860-4A22-B34A-CBE9586D4555}");

            TAudioControlID m_parameterId{ INVALID_AUDIO_CONTROL_ID };
            float m_value{ 0.f };
        };

        struct SetSwitchValue
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, SetSwitchValue, "{86A43DD6-8BC9-4446-970C-4078BDB46E0B}");

            TAudioControlID m_switchId{ INVALID_AUDIO_CONTROL_ID };
            TAudioSwitchStateID m_stateId{ INVALID_AUDIO_SWITCH_STATE_ID };
        };

        struct SetEnvironmentValue
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, SetEnvironmentValue, "{E9BAA202-5CBF-4313-930B-7CAB81CB7FFF}");

            TAudioEnvironmentID m_environmentId{ INVALID_AUDIO_ENVIRONMENT_ID };
            float m_value{ 0.f };
        };

        struct ResetEnvironments
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, ResetEnvironments, "{5B92F5C5-DD55-46D9-9666-57F30D0E8CA3}");
        };

        struct ResetParameters
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, ResetParameters, "{C94D1F26-9331-47B4-8A81-61C3A3526664}");
        };

        struct Release
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, Release, "{8C7995B6-AD5C-45AB-AEEE-2AA823843FBC}");
        };

        struct SetMultiplePositions
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ObjectRequest, SetMultiplePositions, "{228E557D-C8FE-457F-8983-7E88870AF89C}");

            MultiPositionParams m_params{};
        };

    } // namespace Object


    namespace CallbackRequest
    {
        struct ReportFinishedEvent
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(CallbackRequest, ReportFinishedEvent, "{4BBB0A44-AE52-4734-B832-9E9A20C447EF}");

            TAudioEventID m_eventId{ INVALID_AUDIO_EVENT_ID };
        };
    } // namespace Callback


    namespace ListenerRequest
    {
        struct SetWorldTransform
            : public AudioRequestBase
        {
            AUDIO_REQUEST_TYPE(ListenerRequest, SetWorldTransform, "{30B70579-127A-476F-9651-0BD3408DA888}");

            SATLWorldPosition m_transform{};
        };

    } // namespace Listener


    using AudioRequestVariant = AZStd::variant<
        // System Requests
        Audio::SystemRequest::Initialize,
        Audio::SystemRequest::Shutdown,
        Audio::SystemRequest::ReserveObject,
        Audio::SystemRequest::CreateSource,
        Audio::SystemRequest::DestroySource,
        Audio::SystemRequest::LoadControls,
        Audio::SystemRequest::UnloadControls,
        Audio::SystemRequest::LoadBank,
        Audio::SystemRequest::UnloadBank,
        Audio::SystemRequest::UnloadBanksByScope,
        Audio::SystemRequest::ReloadAll,
        Audio::SystemRequest::LoseFocus,
        Audio::SystemRequest::GetFocus,
        Audio::SystemRequest::MuteAll,
        Audio::SystemRequest::UnmuteAll,
        Audio::SystemRequest::StopAllAudio,
        Audio::SystemRequest::DrawDebug,
        Audio::SystemRequest::ChangeLanguage,
        Audio::SystemRequest::SetPanningMode,
        // Object Requests
        Audio::ObjectRequest::ExecuteTrigger,
        Audio::ObjectRequest::ExecuteSourceTrigger,
        Audio::ObjectRequest::PrepareTrigger,
        Audio::ObjectRequest::UnprepareTrigger,
        Audio::ObjectRequest::StopTrigger,
        Audio::ObjectRequest::StopAllTriggers,
        Audio::ObjectRequest::SetPosition,
        Audio::ObjectRequest::SetParameterValue,
        Audio::ObjectRequest::SetSwitchValue,
        Audio::ObjectRequest::SetEnvironmentValue,
        Audio::ObjectRequest::ResetParameters,
        Audio::ObjectRequest::ResetEnvironments,
        Audio::ObjectRequest::Release,
        Audio::ObjectRequest::SetMultiplePositions,
        // Callback Requests
        Audio::CallbackRequest::ReportFinishedEvent,
        // Listener Requests
        Audio::ListenerRequest::SetWorldTransform>;


    using AudioRequestsQueue = AZStd::deque<AudioRequestVariant, AZ::AZStdAlloc<AZ::SystemAllocator>>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct IAudioProxy
    {
        virtual ~IAudioProxy() = default;

        virtual void Initialize(const char* sObjectName, void* ownerOverride = nullptr, bool bInitAsync = true) = 0;
        virtual void Release() = 0;

        virtual void ExecuteTrigger(TAudioControlID nTriggerID) = 0;
        virtual void ExecuteSourceTrigger(TAudioControlID nTriggerID, const SAudioSourceInfo& rSourceInfo) = 0;
        virtual void StopAllTriggers() = 0;
        virtual void StopTrigger(TAudioControlID nTriggerID) = 0;
        virtual void SetSwitchState(TAudioControlID nSwitchID, TAudioSwitchStateID nStateID) = 0;
        virtual void SetRtpcValue(TAudioControlID nRtpcID, float fValue) = 0;
        virtual void SetObstructionCalcType(ObstructionType eObstructionType) = 0;
        virtual void SetPosition(const SATLWorldPosition& rPosition) = 0;
        virtual void SetPosition(const AZ::Vector3& rPosition) = 0;
        virtual void SetMultiplePositions(const MultiPositionParams& params) = 0;
        virtual void SetEnvironmentAmount(TAudioEnvironmentID nEnvironmentID, float fAmount) = 0;
        virtual void ResetEnvironments() = 0;
        virtual void ResetParameters() = 0;
        virtual TAudioObjectID GetAudioObjectID() const = 0;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioPreloadNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioPreloadNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - PreloadID Address, Multiple Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = TAudioPreloadRequestID;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        virtual void OnAudioPreloadCached() = 0;
        virtual void OnAudioPreloadUncached() = 0;
    };

    using AudioPreloadNotificationBus = AZ::EBus<AudioPreloadNotifications>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class IAudioSystem
    {
        // This interface is registered with AZ::Interface<>
    public:
        AZ_RTTI(IAudioSystem, "{4AF3417B-C264-4970-96C5-EBB888EA922D}");

        virtual ~IAudioSystem() = default;

        virtual bool Initialize() = 0;
        virtual void Release() = 0;
        virtual void ExternalUpdate() = 0;

        virtual void PushRequest(AudioRequestVariant&& request) = 0;
        virtual void PushRequests(AudioRequestsQueue& requests) = 0;
        virtual void PushRequestBlocking(AudioRequestVariant&& request) = 0;
        virtual void PushCallback(AudioRequestVariant&& callback) = 0;

        virtual TAudioControlID GetAudioTriggerID(const char* sAudioTriggerName) const = 0;
        virtual TAudioControlID GetAudioRtpcID(const char* sAudioRtpcName) const = 0;
        virtual TAudioControlID GetAudioSwitchID(const char* sAudioSwitchName) const = 0;
        virtual TAudioSwitchStateID GetAudioSwitchStateID(TAudioControlID nSwitchID, const char* sAudioSwitchStateName) const = 0;
        virtual TAudioPreloadRequestID GetAudioPreloadRequestID(const char* sAudioPreloadRequestName) const = 0;
        virtual TAudioEnvironmentID GetAudioEnvironmentID(const char* sAudioEnvironmentName) const = 0;

        virtual bool ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) = 0;
        virtual bool ReleaseAudioListenerID(TAudioObjectID nAudioObjectID) = 0;
        virtual bool SetAudioListenerOverrideID(TAudioObjectID nAudioObjectID) = 0;

        virtual const char* GetControlsPath() const = 0;
        virtual void UpdateControlsPath() = 0;
        virtual void RefreshAudioSystem(const char* levelName) = 0;

        virtual IAudioProxy* GetAudioProxy() = 0;
        virtual void RecycleAudioProxy(IAudioProxy* pIAudioProxy) = 0;

        virtual TAudioSourceId CreateAudioSource(const SAudioInputConfig& sourceConfig) = 0;
        virtual void DestroyAudioSource(TAudioSourceId sourceId) = 0;
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioTriggerNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioTriggerNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Address by ID, Multiple Handler, Recursive Mutex, Queued
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        using BusIdType = TriggerNotificationIdType;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        // Give default definitions, because handlers don't need to implement all these functions...
        virtual void ReportDurationInfo(
            [[maybe_unused]] TAudioControlID triggerId,
            [[maybe_unused]] TAudioEventID eventId,
            [[maybe_unused]] float duration,
            [[maybe_unused]] float estimatedDuration) {}
        virtual void ReportTriggerStarted([[maybe_unused]] TAudioControlID triggerId) {}
        virtual void ReportTriggerFinished([[maybe_unused]] TAudioControlID triggerId) {}
    };

    using AudioTriggerNotificationBus = AZ::EBus<AudioTriggerNotifications>;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioStreamingRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioStreamingRequests() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides - Single Bus, Single Handler
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = TAudioSourceId;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ReadStreamingInput>
        // Summary:
        //      Load streaming input into the ATL-specific audio input
        // Arguments:
        //      data - Buffer of data to load into the input device's internal buffer
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual AZStd::size_t ReadStreamingInput(const AudioStreamData& data) = 0;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // <title ReadStreamingMultiTrackInput>
        // Summary:
        //      Load streaming multi-track input into the ATL-specific audio input
        // Arguments:
        //      data - Buffers of multi-track data to load into the input device's internal buffer
        // Return value:
        //      The number of frames loaded into the internal buffer
        ///////////////////////////////////////////////////////////////////////////////////////////////
        virtual AZStd::size_t ReadStreamingMultiTrackInput(AudioStreamMultiTrackData& data) = 0;
    };

    using AudioStreamingRequestBus = AZ::EBus<AudioStreamingRequests>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    namespace Gem
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Audio initialization loads resources that may not be ready until after
        // component application (and AP) is fully up, so it is delayed until CSystem::Init.
        // Similarly, release is called in CSystem::ShutDown.
        class SystemBusInterface
            : public AZ::EBusTraits
        {
        public:
            virtual ~SystemBusInterface() = default;

            ///////////////////////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            ///////////////////////////////////////////////////////////////////////////////////////////
            // Interface methods
            virtual bool Initialize() = 0;
            virtual void Release() = 0;
            ///////////////////////////////////////////////////////////////////////////////////////////
        };

        // SystemRequestBus is used with AudioSystem Gem
        class SystemRequests : public SystemBusInterface {};
        using SystemRequestBus = AZ::EBus<SystemRequests>;
        // EngineRequestBus is used with AudioEngine* Gem
        class EngineRequests : public SystemBusInterface {};
        using EngineRequestBus = AZ::EBus<EngineRequests>;

    } // namespace Gem

} // namespace Audio
