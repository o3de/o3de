/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformIncl.h>
#include <AudioSystemImpl_wwise.h>

#include <platform.h>
#include <AzCore/AzCore_Traits_Platform.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <IAudioSystem.h>
#include <AudioAllocators.h>
#include <AudioLogger.h>
#include <AudioSourceManager.h>
#include <AudioSystemImplCVars.h>
#include <Common_wwise.h>
#include <Config_wwise.h>
#include <FileIOHandler_wwise.h>

#include <AK/SoundEngine/Common/AkMemoryMgr.h>          // Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>             // Memory and Stream Manager interfaces
#include <AK/SoundEngine/Common/AkSoundEngine.h>        // Sound Engine
#include <AK/SpatialAudio/Common/AkSpatialAudio.h>      // Spatial Audio
#include <AK/MusicEngine/Common/AkMusicEngine.h>        // Music Engine

#include <PluginRegistration_wwise.h>                   // Registration of default set of plugins, customize this header to your needs.

#include <inttypes.h>

#if !defined(WWISE_RELEASE)
    #include <AK/Comm/AkCommunication.h>    // Communication between Wwise and the game (excluded in release build)
    #include <AK/Tools/Common/AkMonitorError.h>
    #include <AkPlatformFuncs_Platform.h>
#endif // WWISE_RELEASE

#if defined(AK_MAX_AUX_PER_OBJ)
    #define LY_MAX_AUX_PER_OBJ  AK_MAX_AUX_PER_OBJ
#else
    #define LY_MAX_AUX_PER_OBJ  (4)
#endif


namespace Audio
{
    namespace Platform
    {
        void InitializeMemory(CAudioLogger& audioLogger);
        void SetupAkSoundEngine(AkPlatformInitSettings& platformInitSettings);
    }

    /////////////////////////////////////////////////////////////////////////////////
    //                          AK MEMORY HOOKS SETUP
    namespace Wwise::MemHooks
    {
        void* Malloc(AkMemPoolId memId, size_t size)
        {
            size_t memCategory = memId & AkMemID_MASK;
            AZ_Assert(memCategory < AkMemID_NUM, "Wwise::MemHooks::Malloc - Bad AkMemPoolId passed: %zu", memCategory);
            return AZ::AllocatorInstance<AudioImplAllocator>::Get().Allocate(size, 0, 0,
                (memCategory < AkMemID_NUM) ? MemoryManagerCategories[memCategory] : nullptr);
        }

        void* Malign(AkMemPoolId memId, size_t size, AkUInt32 alignment)
        {
            size_t memCategory = memId & AkMemID_MASK;
            AZ_Assert(memCategory < AkMemID_NUM, "WWise::MemHooks::Malign - Bad AkMemPoolId passed: %zu", memCategory);
            return AZ::AllocatorInstance<AudioImplAllocator>::Get().Allocate(size, alignment, 0,
                (memCategory < AkMemID_NUM) ? MemoryManagerCategories[memCategory] : nullptr);
        }

        void* Realloc([[maybe_unused]] AkMemPoolId memId, void* address, size_t size)
        {
            return AZ::AllocatorInstance<AudioImplAllocator>::Get().ReAllocate(address, size, 0);
        }

        void* ReallocAligned([[maybe_unused]] AkMemPoolId memId, void* address, size_t size, AkUInt32 alignment)
        {
            return AZ::AllocatorInstance<AudioImplAllocator>::Get().ReAllocate(address, size, alignment);
        }

        void Free([[maybe_unused]] AkMemPoolId memId, void* address)
        {
            AZ::AllocatorInstance<AudioImplAllocator>::Get().DeAllocate(address);
        }

        size_t TotalReservedMemorySize()
        {
            return AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().Capacity();
        }

        size_t SizeOfMemory([[maybe_unused]] AkMemPoolId memId, void* address)
        {
            return AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().AllocationSize(address);
        }

    } // namespace Wwise::MemHooks

    extern CAudioLogger g_audioImplLogger_wwise;

    const char* const CAudioSystemImpl_wwise::WwiseImplSubPath = "wwise/";
    const char* const CAudioSystemImpl_wwise::WwiseGlobalAudioObjectName = "LY-GlobalAudioObject";
    const float CAudioSystemImpl_wwise::ObstructionOcclusionMin = 0.0f;
    const float CAudioSystemImpl_wwise::ObstructionOcclusionMax = 1.0f;

    static bool audioDeviceInitializationEvent = false;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AK callbacks
    void WwiseEventCallback(AkCallbackType callbackType, AkCallbackInfo* callbackInfo)
    {
        if (callbackType == AK_EndOfEvent)
        {
            auto const eventData = static_cast<SATLEventData_wwise*>(callbackInfo->pCookie);

            if (eventData)
            {
                SAudioRequest request;
                SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> requestData(eventData->nATLID, true);
                request.nFlags = eARF_THREAD_SAFE_PUSH;
                request.pData = &requestData;

                AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, request);

                if (eventData->nSourceId != INVALID_AUDIO_SOURCE_ID)
                {
                    AkPlayingID playingId = AudioSourceManager::Get().FindPlayingSource(eventData->nSourceId);
                    AudioSourceManager::Get().DeactivateSource(playingId);
                }
            }
        }
        else if (callbackType == AK_Duration)
        {
            auto durationInfo = static_cast<AkDurationCallbackInfo*>(callbackInfo);
            auto const eventData = static_cast<SATLEventData_wwise*>(callbackInfo->pCookie);
            if (durationInfo && eventData)
            {
                AudioTriggerNotificationBus::QueueEvent(eventData->m_triggerId, &AudioTriggerNotificationBus::Events::ReportDurationInfo,
                    eventData->nATLID, durationInfo->fDuration, durationInfo->fEstimatedDuration);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void AudioDeviceCallback(
        [[maybe_unused]] AK::IAkGlobalPluginContext* context,
        [[maybe_unused]] AkUniqueID audioDeviceSharesetId,
        [[maybe_unused]] AkUInt32 deviceId,
        AK::AkAudioDeviceEvent deviceEvent,
        [[maybe_unused]] AKRESULT inAkResult
    )
    {
        if (deviceEvent == AK::AkAudioDeviceEvent_Initialization)
        {
            audioDeviceInitializationEvent = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void PrepareEventCallback(
        AkUniqueID akEventId,
        [[maybe_unused]] const void* bankPtr,
        AKRESULT loadResult,
        [[maybe_unused]] AkMemPoolId memPoolId,
        void* cookie)
    {
        auto const eventData = static_cast<SATLEventData_wwise*>(cookie);

        if (eventData)
        {
            eventData->nAKID = akEventId;

            SAudioRequest request;
            SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> requestData(eventData->nATLID, loadResult ==  AK_Success);
            request.nFlags = eARF_THREAD_SAFE_PUSH;
            request.pData = &requestData;

            AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, request);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(WWISE_RELEASE)
    static void ErrorMonitorCallback(
        AK::Monitor::ErrorCode in_eErrorCode,   ///< Error code number value
        const AkOSChar* in_pszError,            ///< Message or error string to be displayed
        AK::Monitor::ErrorLevel in_eErrorLevel, ///< Specifies whether it should be displayed as a message or an error
        AkPlayingID in_playingID,               ///< Related Playing ID if applicable, AK_INVALID_PLAYING_ID otherwise
        AkGameObjectID in_gameObjID             ///< Related Game Object ID if applicable, AK_INVALID_GAME_OBJECT otherwise
        )
    {
        char* errorStr = nullptr;
        CONVERT_OSCHAR_TO_CHAR(in_pszError, errorStr);
        g_audioImplLogger_wwise.Log(
            ((in_eErrorLevel & AK::Monitor::ErrorLevel_Error) != 0) ? eALT_ERROR : eALT_COMMENT,
            "<Wwise> %s ErrorCode: %d PlayingID: %u GameObjID: %llu", errorStr, in_eErrorCode, in_playingID, in_gameObjID);
    }
#endif // WWISE_RELEASE

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static int GetAssetType(const SATLSourceData* sourceData)
    {
        if (!sourceData)
        {
            return eAAT_NONE;
        }

        return sourceData->m_sourceInfo.m_codecType == eACT_STREAM_PCM
               ? eAAT_STREAM
               : eAAT_SOURCE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static int GetAkCodecID(EAudioCodecType codecType)
    {
        switch (codecType)
        {
        case eACT_AAC:
            return AKCODECID_AAC;
        case eACT_ADPCM:
            return AKCODECID_ADPCM;
        case eACT_PCM:
            return AKCODECID_PCM;
        case eACT_VORBIS:
            return AKCODECID_VORBIS;
        case eACT_XMA:
            return AKCODECID_XMA;
        case eACT_XWMA:
            return AKCODECID_XWMA;
        case eACT_STREAM_PCM:
        default:
            AZ_Assert(codecType, "Codec not supported");
            return AKCODECID_VORBIS;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_wwise::CAudioSystemImpl_wwise(const char* assetsPlatformName)
        : m_globalGameObjectID(static_cast<AkGameObjectID>(GLOBAL_AUDIO_OBJECT_ID))
        , m_defaultListenerGameObjectID(AK_INVALID_GAME_OBJECT)
        , m_initBankID(AK_INVALID_BANK_ID)
#if !defined(WWISE_RELEASE)
        , m_isCommSystemInitialized(false)
#endif // !WWISE_RELEASE
    {
        if (assetsPlatformName && assetsPlatformName[0] != '\0')
        {
            m_assetsPlatform = assetsPlatformName;
        }

        Platform::InitializeMemory(g_audioImplLogger_wwise);

        SetBankPaths();

#if !defined(WWISE_RELEASE)
        m_fullImplString = AZStd::string::format("%s (%s)", WWISE_IMPL_VERSION_STRING, m_soundbankFolder.c_str());

        // Set up memory categories for debug tracking, do this early before initializing Wwise so they are available
        // before the any allocations through hooks occur.
        g_audioImplLogger_wwise.Log(eALT_COMMENT, "Memory Categories:\n");
        m_debugMemoryInfo.reserve(AkMemID_NUM + 1);

        for (AZ::u32 memId = 0; memId < AkMemID_NUM; ++memId)
        {
            AudioImplMemoryPoolInfo memInfo;
            azstrcpy(memInfo.m_poolName, sizeof(memInfo.m_poolName), Wwise::MemoryManagerCategories[memId]);
            memInfo.m_poolId = memId;

            m_debugMemoryInfo.push_back(memInfo);

            g_audioImplLogger_wwise.Log(eALT_COMMENT, "Memory category ID: %u - '%s'\n", memId, Wwise::MemoryManagerCategories[memId]);
        }

        // Add one more category for global stats.
        AudioImplMemoryPoolInfo memInfo;
        azstrcpy(memInfo.m_poolName, sizeof(memInfo.m_poolName), "Global");
        m_debugMemoryInfo.push_back(memInfo);
#endif // !WWISE_RELEASE

        AudioSystemImplementationRequestBus::Handler::BusConnect();
        AudioSystemImplementationNotificationBus::Handler::BusConnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_wwise::~CAudioSystemImpl_wwise()
    {
        AudioSystemImplementationRequestBus::Handler::BusDisconnect();
        AudioSystemImplementationNotificationBus::Handler::BusDisconnect();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationNotificationBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemLoseFocus()
    {
    #if AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
        AKRESULT akResult = AK::SoundEngine::Suspend();
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to Suspend, AKRESULT = %d\n", akResult);
        }
    #endif // AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemGetFocus()
    {
    #if AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
        AKRESULT akResult = AK::SoundEngine::WakeupFromSuspend();
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to WakeupFromSuspend, AKRESULT = %d\n", akResult);
        }
    #endif // AZ_TRAIT_AUDIOENGINEWWISE_AUDIOSYSTEMIMPL_USE_SUSPEND
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemMuteAll()
    {
        // With Wwise we drive this via events.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemUnmuteAll()
    {
        // With Wwise we drive this via events.
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::OnAudioSystemRefresh()
    {
        AKRESULT akResult = AK_Fail;

        if (m_initBankID != AK_INVALID_BANK_ID)
        {
            akResult = AK::SoundEngine::UnloadBank(m_initBankID, nullptr);

            if (!IS_WWISE_OK(akResult))
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to unload %s, returned the AKRESULT: %d", Wwise::InitBank, akResult);
                AZ_Assert(false, "<Wwise> Failed to unload %s!", Wwise::InitBank);
            }
        }

        AkOSChar* initBankName = nullptr;
        CONVERT_CHAR_TO_OSCHAR(Wwise::InitBank, initBankName);
        akResult = AK::SoundEngine::LoadBank(initBankName, m_initBankID);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to load %s, returned the AKRESULT: %d", Wwise::InitBank, akResult);
            m_initBankID = AK_INVALID_BANK_ID;
            AZ_Assert(false, "<Wwise> Failed to load %s!", Wwise::InitBank);
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationRequestBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::Update([[maybe_unused]] const float updateIntervalMS)
    {
        AZ_PROFILE_FUNCTION(Audio);

        if (AK::SoundEngine::IsInitialized())
        {
    #if !defined(WWISE_RELEASE)
            AKRESULT akResult = AK_Fail;
            static bool enableOutputCapture = false;

            if (Wwise::Cvars::s_EnableOutputCapture && !enableOutputCapture)
            {
                // This file ends up in the cache folder
                // Need to disable this on LTX, it produces garbage output.  But there's no way to "IsLTX()" yet.
                akResult = AK::SoundEngine::StartOutputCapture(AKTEXT("../wwise_audio_capture.wav"));
                AZ_Assert(IS_WWISE_OK(akResult), "AK::SoundEngine::StartOutputCapture failed!");
                enableOutputCapture = Wwise::Cvars::s_EnableOutputCapture;
            }
            else if (!Wwise::Cvars::s_EnableOutputCapture && enableOutputCapture)
            {
                akResult = AK::SoundEngine::StopOutputCapture();
                AZ_Assert(IS_WWISE_OK(akResult), "AK::SoundEngine::StopOutputCapture failed!");
                enableOutputCapture = Wwise::Cvars::s_EnableOutputCapture;
            }

            if (audioDeviceInitializationEvent)
            {
                AkChannelConfig channelConfig = AK::SoundEngine::GetSpeakerConfiguration();
                int surroundSpeakers = channelConfig.uNumChannels;
                int lfeSpeakers = 0;
                if (AK::HasLFE(channelConfig.uChannelMask))
                {
                    --surroundSpeakers;
                    ++lfeSpeakers;
                }
                m_speakerConfigString = AZStd::string::format("Output: %d.%d", surroundSpeakers, lfeSpeakers);
                m_fullImplString = AZStd::string::format("%s (%s)  %s", WWISE_IMPL_VERSION_STRING, m_soundbankFolder.c_str(), m_speakerConfigString.c_str());

                audioDeviceInitializationEvent = false;
            }
    #endif // !WWISE_RELEASE

            AK::SoundEngine::RenderAudio();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::Initialize()
    {
        // If something fails so severely during initialization that we need to fall back to a 'Null' implementation
        // we will need to shut down what has been initialized so far. Therefore make sure to call Shutdown() before returning eARS_FAILURE!

        AkMemSettings akMemSettings;
        AK::MemoryMgr::GetDefaultSettings(akMemSettings);
        akMemSettings.pfMalloc = Wwise::MemHooks::Malloc;
        akMemSettings.pfMalign = Wwise::MemHooks::Malign;
        akMemSettings.pfRealloc = Wwise::MemHooks::Realloc;
        akMemSettings.pfReallocAligned = Wwise::MemHooks::ReallocAligned;
        akMemSettings.pfFree = Wwise::MemHooks::Free;
        akMemSettings.pfTotalReservedMemorySize = Wwise::MemHooks::TotalReservedMemorySize;
        akMemSettings.pfSizeOfMemory = Wwise::MemHooks::SizeOfMemory;
        akMemSettings.uMemAllocationSizeLimit = Wwise::Cvars::s_PrimaryMemorySize << 10;

        AKRESULT akResult = AK::MemoryMgr::Init(&akMemSettings);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::MemoryMgr::Init() returned AKRESULT %d", akResult);
            ShutDown();
            return eARS_FAILURE;
        }

        akResult = AK::SoundEngine::RegisterAudioDeviceStatusCallback(AudioDeviceCallback);
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::RegisterAudioDeviceStatusCallback failed!\n");
        }

        AkStreamMgrSettings akStreamSettings;
        AK::StreamMgr::GetDefaultSettings(akStreamSettings);

        if (AK::StreamMgr::Create(akStreamSettings) == nullptr)
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::StreamMgr::Create() failed!\n");
            ShutDown();
            return eARS_FAILURE;
        }

        akResult = m_fileIOHandler.Init(Wwise::Cvars::s_StreamDeviceMemorySize << 10);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "m_fileIOHandler.Init() returned AKRESULT %d", akResult);
            ShutDown();
            return eARS_FAILURE;
        }

        m_fileIOHandler.SetBankPath(m_soundbankFolder.c_str());

        AkInitSettings akInitSettings;
        AK::SoundEngine::GetDefaultInitSettings(akInitSettings);
        akInitSettings.uCommandQueueSize = aznumeric_cast<AkUInt32>(Wwise::Cvars::s_CommandQueueMemorySize << 10);
#if !defined(WWISE_RELEASE)
        akInitSettings.uMonitorQueuePoolSize = aznumeric_cast<AkUInt32>(Wwise::Cvars::s_MonitorQueueMemorySize << 10);
#endif // !WWISE_RELEASE
        akInitSettings.bEnableGameSyncPreparation = false;

#if AZ_TRAIT_AUDIOENGINEWWISE_DEFAULT_SPEAKER_CONFIGURATION
        akInitSettings.settingsMainOutput.channelConfig.SetStandardOrAnonymous(
            AK::ChannelMaskToNumChannels(AZ_TRAIT_AUDIOENGINEWWISE_DEFAULT_SPEAKER_CONFIGURATION),
            AZ_TRAIT_AUDIOENGINEWWISE_DEFAULT_SPEAKER_CONFIGURATION);
#endif

        AkPlatformInitSettings akPlatformInitSettings;
        AK::SoundEngine::GetDefaultPlatformInitSettings(akPlatformInitSettings);

        Platform::SetupAkSoundEngine(akPlatformInitSettings);

        akResult = AK::SoundEngine::Init(&akInitSettings, &akPlatformInitSettings);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::SoundEngine::Init() returned AKRESULT %d", akResult);
            ShutDown();
            return eARS_FAILURE;
        }

        AkMusicSettings akMusicSettings;
        AK::MusicEngine::GetDefaultInitSettings(akMusicSettings);

        akResult = AK::MusicEngine::Init(&akMusicSettings);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::MusicEngine::Init() returned AKRESULT %d", akResult);
            ShutDown();
            return eARS_FAILURE;
        }

        AkSpatialAudioInitSettings akSpatialAudioSettings;
        akResult = AK::SpatialAudio::Init(akSpatialAudioSettings);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::SpatialAudio::Init() returned AKRESULT %d", akResult);
            ShutDown();
            return eARS_FAILURE;
        }

#if !defined(WWISE_RELEASE)
        if (Audio::Wwise::Cvars::s_EnableCommSystem)
        {
            m_isCommSystemInitialized = true;
            AkCommSettings akCommSettings;
            AK::Comm::GetDefaultInitSettings(akCommSettings);

            akResult = AK::Comm::Init(akCommSettings);

            if (!IS_WWISE_OK(akResult))
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible\n", akResult);
                m_isCommSystemInitialized = false;
            }

            akResult = AK::Monitor::SetLocalOutput(AK::Monitor::ErrorLevel_All, ErrorMonitorCallback);

            if (!IS_WWISE_OK(akResult))
            {
                AK::Comm::Term();
                g_audioImplLogger_wwise.Log(eALT_ERROR, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", akResult);
                m_isCommSystemInitialized = false;
            }
        }
#endif // !WWISE_RELEASE

        // Initialize the AudioSourceManager
        AudioSourceManager::Get().Initialize();

        // Register the Global Audio Object used for the events that don't need a location in the game world
        akResult = AK::SoundEngine::RegisterGameObj(m_globalGameObjectID, WwiseGlobalAudioObjectName);

        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::RegisterGameObject() failed for '%s' with AKRESULT %d", WwiseGlobalAudioObjectName, akResult);
        }

        // Load init.bnk before making the system available to the users
        AkOSChar* initBankName = nullptr;
        CONVERT_CHAR_TO_OSCHAR(Wwise::InitBank, initBankName);

        akResult = AK::SoundEngine::LoadBank(initBankName, m_initBankID);
        if (!IS_WWISE_OK(akResult))
        {
            // This does not qualify for a fallback to the 'Null' audio implementation!
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to load %s, returned AKRESULT %d", Wwise::InitBank, akResult);
            m_initBankID = AK_INVALID_BANK_ID;
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ShutDown()
    {
        AKRESULT akResult = AK_Fail;

#if !defined(WWISE_RELEASE)
        if (m_isCommSystemInitialized)
        {
            AK::Comm::Term();

            akResult = AK::Monitor::SetLocalOutput(0, nullptr);

            if (!IS_WWISE_OK(akResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", akResult);
            }

            m_isCommSystemInitialized = false;
        }
#endif // !WWISE_RELEASE

        akResult = AK::SoundEngine::UnregisterAudioDeviceStatusCallback();
        if (akResult != AK_Success)
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::UnregisterAudioDeviceStatusCallback failed!\n");
        }

        // Shutdown the AudioSourceManager
        AudioSourceManager::Get().Shutdown();

        AK::MusicEngine::Term();

        if (AK::SoundEngine::IsInitialized())
        {
            // UnRegister the DummyGameObject
            akResult = AK::SoundEngine::UnregisterGameObj(m_globalGameObjectID);

            if (!IS_WWISE_OK(akResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "AK::SoundEngine::UnregisterGameObject() failed for '%s' with AKRESULT %d", WwiseGlobalAudioObjectName, akResult);
            }

            akResult = AK::SoundEngine::ClearBanks();

            if (!IS_WWISE_OK(akResult))
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Failed to clear sound banks!");
            }

            AK::SoundEngine::Term();
        }

        // Terminate the streaming device and streaming manager
        // CAkFilePackageLowLevelIOBlocking::Term() destroys its associated streaming device
        // that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
        if (AK::IAkStreamMgr::Get())
        {
            m_fileIOHandler.ShutDown();
            AK::IAkStreamMgr::Get()->Destroy();
        }

        // Terminate the Memory Manager
        if (AK::MemoryMgr::IsInitialized())
        {
            AK::MemoryMgr::Term();
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::Release()
    {
        // Deleting this object and destroying the allocator has been moved to AudioEngineWwiseSystemComponent
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::StopAllSounds()
    {
        AK::SoundEngine::StopAll();
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::RegisterAudioObject(
        IATLAudioObjectData* const audioObjectData,
        const char* const objectName)
    {
        if (audioObjectData)
        {
            auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

            const AKRESULT akResult = AK::SoundEngine::RegisterGameObj(implObjectData->nAKID, objectName);

            const bool akSuccess = IS_WWISE_OK(akResult);

            if (!akSuccess)
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise RegisterGameObj failed with AKRESULT: %d", akResult);
            }

            return BoolToARS(akSuccess);
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise RegisterGameObj failed, audioObjectData was null");
            return eARS_FAILURE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnregisterAudioObject(IATLAudioObjectData* const audioObjectData)
    {
        if (audioObjectData)
        {
            auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

            const AKRESULT akResult = AK::SoundEngine::UnregisterGameObj(implObjectData->nAKID);

            const bool akSuccess = IS_WWISE_OK(akResult);

            if (!akSuccess)
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise UnregisterGameObj failed with AKRESULT: %d", akResult);
            }

            return BoolToARS(akSuccess);
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise UnregisterGameObj failed, audioObjectData was null");
            return eARS_FAILURE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ResetAudioObject(IATLAudioObjectData* const audioObjectData)
    {
        if (audioObjectData)
        {
            auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

            implObjectData->cEnvironmentImplAmounts.clear();
            implObjectData->bNeedsToUpdateEnvironments = false;

            return eARS_SUCCESS;
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_WARNING, "Resetting Audio object failed, audioObjectData was null");
            return eARS_FAILURE;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UpdateAudioObject(IATLAudioObjectData* const audioObjectData)
    {
        AZ_PROFILE_FUNCTION(Audio);

        EAudioRequestStatus result = eARS_FAILURE;

        if (audioObjectData)
        {
            auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

            if (implObjectData->bNeedsToUpdateEnvironments)
            {
                result = PostEnvironmentAmounts(implObjectData);
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepareTriggerSync(
        [[maybe_unused]] IATLAudioObjectData* const audioObjectData,
        const IATLTriggerImplData* const triggerData)
    {
        return PrepUnprepTriggerSync(triggerData, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnprepareTriggerSync(
        [[maybe_unused]] IATLAudioObjectData* const audioObjectData,
        const IATLTriggerImplData* const triggerData)
    {
        return PrepUnprepTriggerSync(triggerData, false);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepareTriggerAsync(
        [[maybe_unused]] IATLAudioObjectData* const audioObjectData,
        const IATLTriggerImplData* const triggerData,
        IATLEventData* const eventData)
    {
        return PrepUnprepTriggerAsync(triggerData, eventData, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnprepareTriggerAsync(
        [[maybe_unused]] IATLAudioObjectData* const audioObjectData,
        const IATLTriggerImplData* const triggerData,
        IATLEventData* const eventData)
    {
        return PrepUnprepTriggerAsync(triggerData, eventData, false);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ActivateTrigger(
        IATLAudioObjectData* const audioObjectData,
        const IATLTriggerImplData* const triggerData,
        IATLEventData* const eventData,
        const SATLSourceData* const sourceData)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);
        auto const implTriggerData = static_cast<const SATLTriggerImplData_wwise*>(triggerData);
        auto const implEventData = static_cast<SATLEventData_wwise*>(eventData);

        if (implObjectData && implTriggerData && implEventData)
        {
            AkGameObjectID akObjectId = AK_INVALID_GAME_OBJECT;

            if (implObjectData->bHasPosition)
            {
                akObjectId = implObjectData->nAKID;
                PostEnvironmentAmounts(implObjectData);
            }
            else
            {
                akObjectId = m_globalGameObjectID;
            }

            AkPlayingID akPlayingId = AK_INVALID_PLAYING_ID;
            switch (GetAssetType(sourceData))
            {
                case eAAT_SOURCE:
                {
                    AZ_Assert(sourceData, "SourceData not provided for source type!");
                    // format: "external/{collection_id}/{language_id}/{file_id}.wem"
                    auto filePath = AZStd::string::format("%s" "/%" PRIu64 "/%" PRIu64 "/%" PRIu64 ".wem",
                        Audio::Wwise::ExternalSourcesPath,
                        sourceData->m_sourceInfo.m_collectionId,
                        sourceData->m_sourceInfo.m_languageId,
                        sourceData->m_sourceInfo.m_fileId);

                    AkOSChar* finalFilePath = nullptr;
                    CONVERT_CHAR_TO_OSCHAR(filePath.c_str(), finalFilePath);

                    AkExternalSourceInfo sources[1];
                    sources[0].iExternalSrcCookie = static_cast<AkUInt32>(sourceData->m_sourceInfo.m_sourceId);
                    sources[0].szFile = finalFilePath;
                    sources[0].idCodec = GetAkCodecID(sourceData->m_sourceInfo.m_codecType);

                    akPlayingId = AK::SoundEngine::PostEvent(
                        implTriggerData->nAKID,
                        akObjectId,
                        AK_EndOfEvent | AK_Duration,
                        &WwiseEventCallback,
                        implEventData,
                        1,
                        sources);

                    if (akPlayingId != AK_INVALID_PLAYING_ID)
                    {
                        implEventData->audioEventState = eAES_PLAYING;
                        implEventData->nAKID = akPlayingId;
                        result = eARS_SUCCESS;
                    }
                    else
                    {
                        // if Posting an Event failed, try to prepare it, if it isn't prepared already
                        g_audioImplLogger_wwise.Log(eALT_WARNING, "Failed to Post Wwise event %u with external source '%s'",
                            implTriggerData->nAKID, filePath.c_str());
                    }
                    break;
                }

                case eAAT_STREAM:
                    [[fallthrough]];
                case eAAT_NONE:
                    [[fallthrough]];
                default:
                {
                    akPlayingId = AK::SoundEngine::PostEvent(
                        implTriggerData->nAKID,
                        akObjectId,
                        AK_EndOfEvent | AK_Duration,
                        &WwiseEventCallback,
                        implEventData);

                    if (akPlayingId != AK_INVALID_PLAYING_ID)
                    {
                        if (sourceData)
                        {
                            TAudioSourceId sourceId = sourceData->m_sourceInfo.m_sourceId;
                            if (sourceId != INVALID_AUDIO_SOURCE_ID)
                            {
                                // Activate the audio input source (associates sourceId w/ playingId)...
                                AudioSourceManager::Get().ActivateSource(sourceId, akPlayingId);
                                implEventData->nSourceId = sourceId;
                            }
                        }

                        implEventData->audioEventState = eAES_PLAYING;
                        implEventData->nAKID = akPlayingId;
                        result = eARS_SUCCESS;
                    }
                    else
                    {
                        // if Posting an Event failed, try to prepare it, if it isn't prepared already
                        g_audioImplLogger_wwise.Log(eALT_WARNING, "Failed to Post Wwise event %u", implTriggerData->nAKID);
                    }
                    break;
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Wwise implementation of ActivateTrigger.");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::StopEvent(
        [[maybe_unused]] IATLAudioObjectData* const audioObjectData,
        const IATLEventData* const eventData)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implEventData = static_cast<const SATLEventData_wwise*>(eventData);

        if (implEventData)
        {
            switch (implEventData->audioEventState)
            {
                case eAES_PLAYING:
                {
                    AK::SoundEngine::StopPlayingID(implEventData->nAKID, 10);
                    result = eARS_SUCCESS;
                    break;
                }
                default:
                {
                    g_audioImplLogger_wwise.Log(eALT_ERROR, "Stopping an event of this type is not supported yet");
                    break;
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid EventData passed to the Wwise implementation of StopEvent.");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::StopAllEvents(IATLAudioObjectData* const audioObjectData)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

        if (implObjectData)
        {
            const AkGameObjectID akObjectId = implObjectData->bHasPosition ? implObjectData->nAKID : m_globalGameObjectID;

            AK::SoundEngine::StopAll(akObjectId);

            result = eARS_SUCCESS;
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of StopAllEvents.");
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetPosition(
        IATLAudioObjectData* const audioObjectData,
        const SATLWorldPosition& worldPosition)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

        if (implObjectData)
        {
            AkSoundPosition akSoundPos;
            ATLTransformToAkTransform(worldPosition, akSoundPos);

            const AKRESULT akResult = AK::SoundEngine::SetPosition(implObjectData->nAKID, akSoundPos);
            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetPosition failed with AKRESULT: %d", akResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetPosition.");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetMultiplePositions(
        IATLAudioObjectData* const audioObjectData,
        const MultiPositionParams& multiPositionParams)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

        if (implObjectData)
        {
            AZStd::vector<AkSoundPosition> akPositions;
            AZStd::for_each(multiPositionParams.m_positions.begin(), multiPositionParams.m_positions.end(),
                [&akPositions](const auto& position)
                {
                    akPositions.emplace_back(AZVec3ToAkTransform(position));
                }
            );

            AK::SoundEngine::MultiPositionType type = AK::SoundEngine::MultiPositionType_MultiDirections; // default 'Blended'

            if (multiPositionParams.m_type == MultiPositionBehaviorType::Separate)
            {
                type = AK::SoundEngine::MultiPositionType_MultiSources;
            }

            const AKRESULT akResult = AK::SoundEngine::SetMultiplePositions(implObjectData->nAKID, akPositions.data(), static_cast<AkUInt16>(akPositions.size()), type);
            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetMultiplePositions failed with AKRESULT: %d\n", akResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetMultiplePositions.");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetEnvironment(
        IATLAudioObjectData* const audioObjectData,
        const IATLEnvironmentImplData* const environmentData,
        const float amount)
    {
        static const float s_envEpsilon = 0.0001f;

        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);
        auto const implEnvironmentData = static_cast<const SATLEnvironmentImplData_wwise*>(environmentData);

        if (implObjectData && implEnvironmentData)
        {
            switch (implEnvironmentData->eType)
            {
                case eWAET_AUX_BUS:
                {
                    float currentAmount = -1.f;
                    auto it = implObjectData->cEnvironmentImplAmounts.find(implEnvironmentData->nAKBusID);
                    if (it != implObjectData->cEnvironmentImplAmounts.end())
                    {
                        currentAmount = it->second;
                    }

                    if (currentAmount == -1.f || !AZ::IsClose(currentAmount, amount, s_envEpsilon))
                    {
                        implObjectData->cEnvironmentImplAmounts[implEnvironmentData->nAKBusID] = amount;
                        implObjectData->bNeedsToUpdateEnvironments = true;
                    }

                    result = eARS_SUCCESS;
                    break;
                }
                case eWAET_RTPC:
                {
                    auto akRtpcValue = static_cast<AkRtpcValue>(implEnvironmentData->fMult * amount + implEnvironmentData->fShift);

                    const AKRESULT akResult = AK::SoundEngine::SetRTPCValue(implEnvironmentData->nAKRtpcID, akRtpcValue, implObjectData->nAKID);

                    if (IS_WWISE_OK(akResult))
                    {
                        result = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the Rtpc %u to value %f on object %u in SetEnvironement()",
                            implEnvironmentData->nAKRtpcID,
                            akRtpcValue,
                            implObjectData->nAKID);
                    }
                    break;
                }
                default:
                {
                    AZ_Assert(false, "<Wwise> Unknown AudioEnvironmentImplementation type!");
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or EnvironmentData passed to the Wwise implementation of SetEnvironment");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetRtpc(
        IATLAudioObjectData* const audioObjectData,
        const IATLRtpcImplData* const rtpcData,
        const float value)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);
        auto const implRtpcData = static_cast<const SATLRtpcImplData_wwise*>(rtpcData);

        if (implObjectData && implRtpcData)
        {
            auto akRtpcValue = static_cast<AkRtpcValue>(implRtpcData->m_fMult * value + implRtpcData->m_fShift);

            const AKRESULT akResult = AK::SoundEngine::SetRTPCValue(implRtpcData->nAKID, akRtpcValue, implObjectData->nAKID);

            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise failed to set the Rtpc %llu to value %f on object %llu",
                    implRtpcData->nAKID,
                    static_cast<AkRtpcValue>(value),
                    implObjectData->nAKID);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of SetRtpc");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetSwitchState(
        IATLAudioObjectData* const audioObjectData,
        const IATLSwitchStateImplData* const switchStateData)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);
        auto const implSwitchStateData = static_cast<const SATLSwitchStateImplData_wwise*>(switchStateData);

        if (implObjectData && implSwitchStateData)
        {
            switch (implSwitchStateData->eType)
            {
                case eWST_SWITCH:
                {
                    const AkGameObjectID akObjectId = implObjectData->bHasPosition ? implObjectData->nAKID : m_globalGameObjectID;

                    const AKRESULT akResult = AK::SoundEngine::SetSwitch(
                            implSwitchStateData->nAKSwitchID,
                            implSwitchStateData->nAKStateID,
                            akObjectId);

                    if (IS_WWISE_OK(akResult))
                    {
                        result = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the switch group %u to state %u on object %llu",
                            implSwitchStateData->nAKSwitchID,
                            implSwitchStateData->nAKStateID,
                            akObjectId);
                    }
                    break;
                }
                case eWST_STATE:
                {
                    const AKRESULT akResult = AK::SoundEngine::SetState(
                            implSwitchStateData->nAKSwitchID,
                            implSwitchStateData->nAKStateID);

                    if (IS_WWISE_OK(akResult))
                    {
                        result = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the state group %u to state %u",
                            implSwitchStateData->nAKSwitchID,
                            implSwitchStateData->nAKStateID);
                    }
                    break;
                }
                case eWST_RTPC:
                {
                    const AkGameObjectID akObjectId = implObjectData->nAKID;

                    const AKRESULT akResult = AK::SoundEngine::SetRTPCValue(
                            implSwitchStateData->nAKSwitchID,
                            static_cast<AkRtpcValue>(implSwitchStateData->fRtpcValue),
                            akObjectId);

                    if (IS_WWISE_OK(akResult))
                    {
                        result = eARS_SUCCESS;
                    }
                    else
                    {
                        g_audioImplLogger_wwise.Log(
                            eALT_WARNING,
                            "Wwise failed to set the Rtpc %u to value %f on object %llu",
                            implSwitchStateData->nAKSwitchID,
                            static_cast<AkRtpcValue>(implSwitchStateData->fRtpcValue),
                            akObjectId);
                    }
                    break;
                }
                case eWST_NONE:
                {
                    break;
                }
                default:
                {
                    g_audioImplLogger_wwise.Log(eALT_WARNING, "Unknown EWwiseSwitchType: %u", implSwitchStateData->eType);
                    AZ_Assert(false, "<Wwise> Unknown EWwiseSwitchType");
                    break;
                }
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of SetRtpc");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetObstructionOcclusion(
        IATLAudioObjectData* const audioObjectData,
        const float obstruction,
        const float occlusion)
    {
        if (obstruction < ObstructionOcclusionMin || obstruction > ObstructionOcclusionMax)
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "Obstruction value %f is out of range, Obstruction should be between %f and %f.",
                obstruction, ObstructionOcclusionMin, ObstructionOcclusionMax);
        }

        if (occlusion < ObstructionOcclusionMin || occlusion > ObstructionOcclusionMax)
        {
            g_audioImplLogger_wwise.Log(
                eALT_WARNING,
                "Occlusion value %f is out of range, Occlusion should be between %f and %f.",
                occlusion, ObstructionOcclusionMin, ObstructionOcclusionMax);
        }

        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

        if (implObjectData)
        {
            const AKRESULT akResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
                    implObjectData->nAKID,
                    m_defaultListenerGameObjectID,  // only set the obstruction/occlusion for the default listener for now
                    static_cast<AkReal32>(obstruction),
                    static_cast<AkReal32>(occlusion));

            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise failed to set Obstruction %f and Occlusion %f on object %llu",
                    obstruction,
                    occlusion,
                    implObjectData->nAKID);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetObjectObstructionAndOcclusion");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::SetListenerPosition(
        IATLListenerData* const listenerData,
        const SATLWorldPosition& newPosition)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implListenerData = static_cast<SATLListenerData_wwise*>(listenerData);

        if (implListenerData)
        {
            AkListenerPosition akListenerPos;
            ATLTransformToAkTransform(newPosition, akListenerPos);

            const AKRESULT akResult = AK::SoundEngine::SetPosition(implListenerData->nAKListenerObjectId, akListenerPos);

            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise SetListenerPosition failed with AKRESULT: %u", akResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid ATLListenerData passed to the Wwise implementation of SetListenerPosition");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ResetRtpc(IATLAudioObjectData* const audioObjectData, const IATLRtpcImplData* const rtpcData)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);
        auto const implRtpcDat = static_cast<const SATLRtpcImplData_wwise*>(rtpcData);

        if (implObjectData && implRtpcDat)
        {
            const AKRESULT akResult = AK::SoundEngine::ResetRTPCValue(implRtpcDat->nAKID, implObjectData->nAKID);

            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise failed to reset the Rtpc %u on object %llu",
                    implRtpcDat->nAKID,
                    implObjectData->nAKID);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of ResetRtpc");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::RegisterInMemoryFile(SATLAudioFileEntryInfo* const fileEntryInfo)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        if (fileEntryInfo)
        {
            auto const implFileEntryData = static_cast<SATLAudioFileEntryData_wwise*>(fileEntryInfo->pImplData);

            if (implFileEntryData)
            {
                AkBankID akBankId = AK_INVALID_BANK_ID;

                const AKRESULT akResult = AK::SoundEngine::LoadBankMemoryView(
                    fileEntryInfo->pFileData,
                    aznumeric_cast<AkUInt32>(fileEntryInfo->nSize),
                    akBankId);

                if (IS_WWISE_OK(akResult))
                {
                    implFileEntryData->nAKBankID = akBankId;
                    result = eARS_SUCCESS;
                }
                else
                {
                    implFileEntryData->nAKBankID = AK_INVALID_BANK_ID;
                    g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to load bank '%s'\n", fileEntryInfo->sFileName);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioFileEntryData passed to RegisterInMemoryFile");
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::UnregisterInMemoryFile(SATLAudioFileEntryInfo* const fileEntryInfo)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        if (fileEntryInfo)
        {
            auto const implFileEntryData = static_cast<SATLAudioFileEntryData_wwise*>(fileEntryInfo->pImplData);

            if (implFileEntryData)
            {
                const AKRESULT akResult = AK::SoundEngine::UnloadBank(implFileEntryData->nAKBankID, fileEntryInfo->pFileData);

                if (IS_WWISE_OK(akResult))
                {
                    result = eARS_SUCCESS;
                }
                else
                {
                    g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to unload bank '%s'\n", fileEntryInfo->sFileName);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_ERROR, "Invalid AudioFileEntryData passed to UnregisterInMemoryFile");
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::ParseAudioFileEntry(const AZ::rapidxml::xml_node<char>* audioFileEntryNode, SATLAudioFileEntryInfo* const fileEntryInfo)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        if (audioFileEntryNode && azstricmp(audioFileEntryNode->name(), WwiseXmlTags::WwiseFileTag) == 0 && fileEntryInfo)
        {
            const char* audioFileEntryName = nullptr;
            auto fileEntryNameAttr = audioFileEntryNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (fileEntryNameAttr)
            {
                audioFileEntryName = fileEntryNameAttr->value();
            }

            bool isLocalized = false;
            auto localizedAttr = audioFileEntryNode->first_attribute(WwiseXmlTags::WwiseLocalizedAttribute, 0, false);

            // Legacy Preload support
            if (!localizedAttr)
            {
                localizedAttr = audioFileEntryNode->first_attribute(WwiseXmlTags::Legacy::WwiseLocalizedAttribute, 0, false);
            }

            if (localizedAttr)
            {
                if (azstricmp(localizedAttr->value(), "true") == 0)
                {
                    isLocalized = true;
                }
            }

            if (audioFileEntryName && audioFileEntryName[0] != '\0')
            {
                fileEntryInfo->bLocalized = isLocalized;
                fileEntryInfo->sFileName = audioFileEntryName;
                fileEntryInfo->nMemoryBlockAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;
                fileEntryInfo->pImplData = azcreate(SATLAudioFileEntryData_wwise, (), Audio::AudioImplAllocator, "ATLAudioFileEntryData_wwise");
                result = eARS_SUCCESS;
            }
            else
            {
                fileEntryInfo->sFileName = nullptr;
                fileEntryInfo->nMemoryBlockAlignment = 0;
                fileEntryInfo->pImplData = nullptr;
            }
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioFileEntryData(IATLAudioFileEntryData* const oldAudioFileEntry)
    {
        azdestroy(oldAudioFileEntry, Audio::AudioImplAllocator, SATLAudioFileEntryData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetAudioFileLocation(SATLAudioFileEntryInfo* const fileEntryInfo)
    {
        const char* location = nullptr;

        if (fileEntryInfo)
        {
            location = fileEntryInfo->bLocalized ? m_localizedSoundbankFolder.c_str() : m_soundbankFolder.c_str();
        }

        return location;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLAudioObjectData_wwise* CAudioSystemImpl_wwise::NewGlobalAudioObjectData(const TAudioObjectID objectId)
    {
        AZ_UNUSED(objectId);
        auto newObjectData = azcreate(SATLAudioObjectData_wwise, (AK_INVALID_GAME_OBJECT, false), Audio::AudioImplAllocator, "ATLAudioObjectData_wwise-Global");
        return newObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLAudioObjectData_wwise* CAudioSystemImpl_wwise::NewAudioObjectData(const TAudioObjectID objectId)
    {
        auto newObjectData = azcreate(SATLAudioObjectData_wwise, (static_cast<AkGameObjectID>(objectId), true), Audio::AudioImplAllocator, "ATLAudioObjectData_wwise");
        return newObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioObjectData(IATLAudioObjectData* const oldObjectData)
    {
        azdestroy(oldObjectData, Audio::AudioImplAllocator, SATLAudioObjectData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLListenerData_wwise* CAudioSystemImpl_wwise::NewDefaultAudioListenerObjectData(const TATLIDType listenerId)
    {
        auto newObjectData = azcreate(SATLListenerData_wwise, (static_cast<AkGameObjectID>(listenerId)), Audio::AudioImplAllocator, "ATLListenerData_wwise-Default");
        if (newObjectData)
        {
            auto listenerName = AZStd::string::format("DefaultAudioListener(%" PRIu64 ")", static_cast<AZ::u64>(newObjectData->nAKListenerObjectId));
            AKRESULT akResult = AK::SoundEngine::RegisterGameObj(newObjectData->nAKListenerObjectId, listenerName.c_str());
            if (IS_WWISE_OK(akResult))
            {
                akResult = AK::SoundEngine::SetDefaultListeners(&newObjectData->nAKListenerObjectId, 1);
                if (IS_WWISE_OK(akResult))
                {
                    m_defaultListenerGameObjectID = newObjectData->nAKListenerObjectId;
                }
                else
                {
                    g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in SetDefaultListeners to set AkGameObjectID %llu as default with AKRESULT: %u", newObjectData->nAKListenerObjectId, akResult);
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in RegisterGameObj registering a DefaultAudioListener with AKRESULT: %u", akResult);
            }
        }

        return newObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLListenerData_wwise* CAudioSystemImpl_wwise::NewAudioListenerObjectData(const TATLIDType listenerId)
    {
        auto newObjectData = azcreate(SATLListenerData_wwise, (static_cast<AkGameObjectID>(listenerId)), Audio::AudioImplAllocator, "ATLListenerData_wwise");
        if (newObjectData)
        {
            auto listenerName = AZStd::string::format("AudioListener(%" PRIu64 ")", static_cast<AZ::u64>(newObjectData->nAKListenerObjectId));
            AKRESULT akResult = AK::SoundEngine::RegisterGameObj(newObjectData->nAKListenerObjectId, listenerName.c_str());
            if (!IS_WWISE_OK(akResult))
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in RegisterGameObj registering an AudioListener with AKRESULT: %u", akResult);
            }
        }

        return newObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioListenerObjectData(IATLListenerData* const oldListenerData)
    {
        auto listenerData = static_cast<SATLListenerData_wwise*>(oldListenerData);
        if (listenerData)
        {
            AKRESULT akResult = AK::SoundEngine::UnregisterGameObj(listenerData->nAKListenerObjectId);
            if (IS_WWISE_OK(akResult))
            {
                if (listenerData->nAKListenerObjectId == m_defaultListenerGameObjectID)
                {
                    m_defaultListenerGameObjectID = AK_INVALID_GAME_OBJECT;
                }
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING, "Wwise failed in UnregisterGameObj unregistering an AudioListener(%llu) with AKRESULT: %u", listenerData->nAKListenerObjectId, akResult);
            }
        }

        azdestroy(oldListenerData, Audio::AudioImplAllocator, SATLListenerData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLEventData_wwise* CAudioSystemImpl_wwise::NewAudioEventData(const TAudioEventID eventId)
    {
        auto newObjectData = azcreate(SATLEventData_wwise, (eventId), Audio::AudioImplAllocator, "ATLEventData_wwise");
        return newObjectData;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioEventData(IATLEventData* const oldEventData)
    {
        azdestroy(oldEventData, Audio::AudioImplAllocator, SATLEventData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::ResetAudioEventData(IATLEventData* const eventData)
    {
        auto const implEventData = static_cast<SATLEventData_wwise*>(eventData);

        if (implEventData)
        {
            implEventData->audioEventState = eAES_NONE;
            implEventData->nAKID = AK_INVALID_UNIQUE_ID;
            implEventData->nSourceId = INVALID_AUDIO_SOURCE_ID;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLTriggerImplData* CAudioSystemImpl_wwise::NewAudioTriggerImplData(const AZ::rapidxml::xml_node<char>* audioTriggerNode)
    {
        SATLTriggerImplData_wwise* newTriggerImpl = nullptr;

        if (audioTriggerNode && azstricmp(audioTriggerNode->name(), WwiseXmlTags::WwiseEventTag) == 0)
        {
            auto eventNameAttr = audioTriggerNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (eventNameAttr)
            {
                const char* eventName = eventNameAttr->value();
                const AkUniqueID akId = AK::SoundEngine::GetIDFromString(eventName);

                if (akId != AK_INVALID_UNIQUE_ID)
                {
                    newTriggerImpl = azcreate(SATLTriggerImplData_wwise, (akId), Audio::AudioImplAllocator, "ATLTriggerImplData_wwise");
                }
            }
        }

        return newTriggerImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioTriggerImplData(IATLTriggerImplData* const oldTriggerImplData)
    {
        azdestroy(oldTriggerImplData, Audio::AudioImplAllocator, SATLTriggerImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLRtpcImplData* CAudioSystemImpl_wwise::NewAudioRtpcImplData(const AZ::rapidxml::xml_node<char>* audioRtpcNode)
    {
        SATLRtpcImplData_wwise* newRtpcImpl = nullptr;

        AkRtpcID akRtpcId = AK_INVALID_RTPC_ID;
        float mult = 1.f;
        float shift = 0.f;

        ParseRtpcImpl(audioRtpcNode, akRtpcId, mult, shift);

        if (akRtpcId != AK_INVALID_RTPC_ID)
        {
            newRtpcImpl = azcreate(SATLRtpcImplData_wwise, (akRtpcId, mult, shift), Audio::AudioImplAllocator, "ATLRtpcImplData_wwise");
        }

        return newRtpcImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioRtpcImplData(IATLRtpcImplData* const oldRtpcImplData)
    {
        azdestroy(oldRtpcImplData, Audio::AudioImplAllocator, SATLRtpcImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLSwitchStateImplData* CAudioSystemImpl_wwise::NewAudioSwitchStateImplData(const AZ::rapidxml::xml_node<char>* audioSwitchNode)
    {
        SATLSwitchStateImplData_wwise* newSwitchImpl = nullptr;
        const char* nodeName = audioSwitchNode->name();

        if (azstricmp(nodeName, WwiseXmlTags::WwiseSwitchTag) == 0)
        {
            newSwitchImpl = ParseWwiseSwitchOrState(audioSwitchNode, eWST_SWITCH);
        }
        else if (azstricmp(nodeName, WwiseXmlTags::WwiseStateTag) == 0)
        {
            newSwitchImpl = ParseWwiseSwitchOrState(audioSwitchNode, eWST_STATE);
        }
        else if (azstricmp(nodeName, WwiseXmlTags::WwiseRtpcSwitchTag) == 0)
        {
            newSwitchImpl = ParseWwiseRtpcSwitch(audioSwitchNode);
        }

        return newSwitchImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioSwitchStateImplData(IATLSwitchStateImplData* const oldSwitchStateImplData)
    {
        azdestroy(oldSwitchStateImplData, Audio::AudioImplAllocator, SATLSwitchStateImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLEnvironmentImplData* CAudioSystemImpl_wwise::NewAudioEnvironmentImplData(const AZ::rapidxml::xml_node<char>* audioEnvironmentNode)
    {
        SATLEnvironmentImplData_wwise* newEnvironmentImpl = nullptr;

        if (azstricmp(audioEnvironmentNode->name(), WwiseXmlTags::WwiseAuxBusTag) == 0)
        {
            auto auxBusNameAttr = audioEnvironmentNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (auxBusNameAttr)
            {
                const char* auxBusName = auxBusNameAttr->value();
                const AkUniqueID akBusId = AK::SoundEngine::GetIDFromString(auxBusName);

                if (akBusId != AK_INVALID_AUX_ID)
                {
                    newEnvironmentImpl = azcreate(SATLEnvironmentImplData_wwise, (eWAET_AUX_BUS, static_cast<AkAuxBusID>(akBusId)), Audio::AudioImplAllocator, "ATLEnvironmentImplData_wwise");
                }
            }
        }
        else if (azstricmp(audioEnvironmentNode->name(), WwiseXmlTags::WwiseRtpcTag) == 0)
        {
            AkRtpcID akRtpcId = AK_INVALID_RTPC_ID;
            float mult = 1.f;
            float shift = 0.f;
            ParseRtpcImpl(audioEnvironmentNode, akRtpcId, mult, shift);

            if (akRtpcId != AK_INVALID_RTPC_ID)
            {
                newEnvironmentImpl = azcreate(SATLEnvironmentImplData_wwise, (eWAET_RTPC, akRtpcId, mult, shift), Audio::AudioImplAllocator, "ATLEnvironmentImplData_wwise");
            }
        }

        return newEnvironmentImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DeleteAudioEnvironmentImplData(IATLEnvironmentImplData* const oldEnvironmentImplData)
    {
        azdestroy(oldEnvironmentImplData, Audio::AudioImplAllocator, SATLEnvironmentImplData_wwise);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetImplementationNameString() const
    {
#if !defined(WWISE_RELEASE)
        return m_fullImplString.c_str();
#else
        return nullptr;
#endif // !WWISE_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const
    {
        memoryInfo.nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().Capacity();
        memoryInfo.nPrimaryPoolUsedSize = memoryInfo.nPrimaryPoolSize - AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().GetUnAllocatedMemory();
        memoryInfo.nPrimaryPoolAllocations = 0;
        memoryInfo.nSecondaryPoolSize = 0;
        memoryInfo.nSecondaryPoolUsedSize = 0;
        memoryInfo.nSecondaryPoolAllocations = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AudioImplMemoryPoolInfo> CAudioSystemImpl_wwise::GetMemoryPoolInfo()
    {
#if !defined(WWISE_RELEASE)
        // Update memory category info...
        for (auto& memInfo : m_debugMemoryInfo)
        {
            if (memInfo.m_poolId < 0)
            {
                break;
            }

            AK::MemoryMgr::CategoryStats categoryStats;
            AK::MemoryMgr::GetCategoryStats(memInfo.m_poolId, categoryStats);

            memInfo.m_memoryUsed = static_cast<AZ::u32>(categoryStats.uUsed);
            memInfo.m_peakUsed = static_cast<AZ::u32>(categoryStats.uPeakUsed);
            memInfo.m_numAllocs = categoryStats.uAllocs;
            memInfo.m_numFrees = categoryStats.uFrees;
        }

        AK::MemoryMgr::GlobalStats globalStats;
        AK::MemoryMgr::GetGlobalStats(globalStats);

        auto& memInfo = m_debugMemoryInfo.back();
        memInfo.m_memoryReserved = static_cast<AZ::u32>(globalStats.uReserved);
        memInfo.m_memoryUsed = static_cast<AZ::u32>(globalStats.uUsed);
        memInfo.m_peakUsed = static_cast<AZ::u32>(globalStats.uMax);

        // return the memory infos...
        return m_debugMemoryInfo;
#else
        return AZStd::vector<AudioImplMemoryPoolInfo>();
#endif // !WWISE_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystemImpl_wwise::CreateAudioSource(const SAudioInputConfig& sourceConfig)
    {
        return AudioSourceManager::Get().CreateSource(sourceConfig);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::DestroyAudioSource(TAudioSourceId sourceId)
    {
        AudioSourceManager::Get().DestroySource(sourceId);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::SetPanningMode(PanningMode mode)
    {
        AkPanningRule panningRule;
        switch (mode)
        {
        case PanningMode::Speakers:
            panningRule = AkPanningRule_Speakers;
            break;
        case PanningMode::Headphones:
            panningRule = AkPanningRule_Headphones;
            break;
        default:
            return;
        }

        AKRESULT akResult = AK::SoundEngine::SetPanningRule(panningRule);
        if (!IS_WWISE_OK(akResult))
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR, "Wwise failed to set Panning Rule to [%s]\n", panningRule == AkPanningRule_Speakers ? "Speakers" : "Headphones");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::ParseWwiseSwitchOrState(const AZ::rapidxml::xml_node<char>* node, EWwiseSwitchType type)
    {
        SATLSwitchStateImplData_wwise* switchStateImpl = nullptr;

        auto switchNameAttr = node->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
        if (switchNameAttr)
        {
            const char* switchName = switchNameAttr->value();

            auto valueNode = node->first_node(WwiseXmlTags::WwiseValueTag, 0, false);
            if (valueNode)
            {
                auto valueNameAttr = valueNode->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
                if (valueNameAttr)
                {
                    const char* stateName = valueNameAttr->value();

                    const AkUniqueID akSGroupId = AK::SoundEngine::GetIDFromString(switchName);
                    const AkUniqueID akSNameId = AK::SoundEngine::GetIDFromString(stateName);

                    if (akSGroupId != AK_INVALID_UNIQUE_ID && akSNameId != AK_INVALID_UNIQUE_ID)
                    {
                        switchStateImpl = azcreate(SATLSwitchStateImplData_wwise, (type, akSGroupId, akSNameId), Audio::AudioImplAllocator, "ATLSwitchStateImplData_wwise");
                    }
                }
            }
        }

        return switchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLSwitchStateImplData_wwise* CAudioSystemImpl_wwise::ParseWwiseRtpcSwitch(const AZ::rapidxml::xml_node<char>* node)
    {
        SATLSwitchStateImplData_wwise* switchStateImpl = nullptr;

        if (node && azstricmp(node->name(), WwiseXmlTags::WwiseRtpcSwitchTag) == 0)
        {
            auto rtpcNameAttr = node->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (rtpcNameAttr)
            {
                const char* rtpcName = rtpcNameAttr->value();
                float rtpcValue = 0.f;

                auto rtpcValueAttr = node->first_attribute(WwiseXmlTags::WwiseValueAttribute, 0, false);
                if (rtpcValueAttr)
                {
                    rtpcValue = AZStd::stof(AZStd::string(rtpcValueAttr->value()));

                    const AkUniqueID akRtpcId = AK::SoundEngine::GetIDFromString(rtpcName);
                    if (akRtpcId != AK_INVALID_RTPC_ID)
                    {
                        switchStateImpl = azcreate(SATLSwitchStateImplData_wwise, (eWST_RTPC, akRtpcId, akRtpcId, rtpcValue), Audio::AudioImplAllocator, "ATLSwitchStateImplData_wwise");
                    }
                }
            }
        }

        return switchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::ParseRtpcImpl(const AZ::rapidxml::xml_node<char>* node, AkRtpcID& akRtpcId, float& mult, float& shift)
    {
        if (node && azstricmp(node->name(), WwiseXmlTags::WwiseRtpcTag) == 0)
        {
            auto rtpcAttr = node->first_attribute(WwiseXmlTags::WwiseNameAttribute, 0, false);
            if (rtpcAttr)
            {
                const char* rtpcName = rtpcAttr->value();
                akRtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(rtpcName));

                if (akRtpcId != AK_INVALID_RTPC_ID)
                {
                    auto multAttr = node->first_attribute(WwiseXmlTags::WwiseMutiplierAttribute, 0, false);
                    if (multAttr)
                    {
                        mult = AZStd::stof(AZStd::string(multAttr->value()));
                    }

                    auto shiftAttr = node->first_attribute(WwiseXmlTags::WwiseShiftAttribute, 0, false);
                    if (shiftAttr)
                    {
                        shift = AZStd::stof(AZStd::string(shiftAttr->value()));
                    }
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepUnprepTriggerSync(
        const IATLTriggerImplData* const triggerData,
        bool prepare)
    {
        EAudioRequestStatus result = eARS_FAILURE;

        auto const implTriggerData = static_cast<const SATLTriggerImplData_wwise*>(triggerData);

        if (implTriggerData)
        {
            AkUniqueID akUniqueId = implTriggerData->nAKID;

            const AKRESULT akResult = AK::SoundEngine::PrepareEvent(
                    prepare ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
                    &akUniqueId,
                    1);

            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise PrepareEvent with %s failed for Wwise event %u with AKRESULT: %u",
                    prepare ? "Preparation_Load" : "Preparation_Unload",
                    akUniqueId,
                    akResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR,
                "Invalid ATLTriggerData or EventData passed to the Wwise implementation of %sTriggerSync",
                prepare ? "Prepare" : "Unprepare");
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PrepUnprepTriggerAsync(
        [[maybe_unused]] const IATLTriggerImplData* const triggerData,
        [[maybe_unused]] IATLEventData* const eventData,
        [[maybe_unused]] bool prepare)
    {
        EAudioRequestStatus result = eARS_FAILURE;

#if 0   // Turned off, PrepareEvent is not supported yet.
        auto const implTriggerData = static_cast<const SATLTriggerImplData_wwise*>(triggerData);
        auto const implEventData = static_cast<SATLEventData_wwise*>(eventData);

        if (implTriggerData && implEventData)
        {
            AkUniqueID akUniqueId = implTriggerData->nAKID;

            const AKRESULT akResult = AK::SoundEngine::PrepareEvent(
                    prepare ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
                    &akUniqueId,
                    1,
                    &PrepareEventCallback,
                    implEventData);

            if (IS_WWISE_OK(akResult))
            {
                implEventData->nAKID = akUniqueId;
                implEventData->audioEventState = eAES_UNLOADING;

                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(
                    eALT_WARNING,
                    "Wwise PrepareEvent with %s failed for Wwise event %u with AKRESULT: %u",
                    prepare ? "Preparation_Load" : "Preparation_Unload",
                    akUniqueId,
                    akResult);
            }
        }
        else
        {
            g_audioImplLogger_wwise.Log(eALT_ERROR,
                "Invalid ATLTriggerData or EventData passed to the Wwise implementation of %sTriggerAsync",
                prepare ? "Prepare" : "Unprepare");
        }
#endif

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::SetBankPaths()
    {
        // Default...
        // "sounds/wwise/"
        AZStd::string bankPath = Audio::Wwise::DefaultBanksPath;

        // "sounds/wwise/wwise_config.json"
        AZStd::string configFile = bankPath + Audio::Wwise::ConfigFile;

        if (AZ::IO::FileIOBase::GetInstance()
            && AZ::IO::FileIOBase::GetInstance()->Exists(configFile.c_str()))
        {
            Audio::Wwise::ConfigurationSettings configSettings;
            if (configSettings.Load(configFile))
            {
                for (const auto& platformMap : configSettings.m_platformMappings)
                {
                    // May need to do a series of checks compare the data in the config settings to what's actually in the file system.
                    // This is the most straightforward platform check.
                    if (azstricmp(platformMap.m_enginePlatform.c_str(), AZ_TRAIT_OS_PLATFORM_NAME) == 0)
                    {
                        AZStd::string platformPath;
                        // "sounds/wwise/windows"
                        AZ::StringFunc::AssetDatabasePath::Join(bankPath.c_str(), platformMap.m_bankSubPath.c_str(), platformPath);

                        AZStd::string initBankPath;
                        // "sounds/wwise/windows/init.bnk"
                        AZ::StringFunc::AssetDatabasePath::Join(platformPath.c_str(), Audio::Wwise::InitBank, initBankPath);
                        if (AZ::IO::FileIOBase::GetInstance()->Exists(initBankPath.c_str()))
                        {
                            if (!platformPath.ends_with(AZ_CORRECT_DATABASE_SEPARATOR))
                            {
                                platformPath.push_back(AZ_CORRECT_DATABASE_SEPARATOR);
                            }
                            bankPath = AZStd::move(platformPath);
                            break;
                        }
                    }
                }
            }
        }

        m_soundbankFolder = bankPath;
        m_localizedSoundbankFolder = bankPath;

        Audio::Wwise::SetBanksRootPath(m_soundbankFolder);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioSystemImpl_wwise::SEnvPairCompare::operator()(const AZStd::pair<const AkAuxBusID, float>& pair1, const AZStd::pair<const AkAuxBusID, float>& pair2) const
    {
        return (pair1.second > pair2.second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_wwise::PostEnvironmentAmounts(IATLAudioObjectData* const audioObjectData)
    {
        EAudioRequestStatus result = eARS_FAILURE;
        auto const implObjectData = static_cast<SATLAudioObjectData_wwise*>(audioObjectData);

        if (implObjectData)
        {
            AkAuxSendValue akAuxSendValues[LY_MAX_AUX_PER_OBJ];
            AZ::u32 auxCount = 0;

            SATLAudioObjectData_wwise::TEnvironmentImplMap::iterator envPair = implObjectData->cEnvironmentImplAmounts.begin();
            const SATLAudioObjectData_wwise::TEnvironmentImplMap::const_iterator envBegin = implObjectData->cEnvironmentImplAmounts.begin();
            const SATLAudioObjectData_wwise::TEnvironmentImplMap::const_iterator envEnd = implObjectData->cEnvironmentImplAmounts.end();

            if (implObjectData->cEnvironmentImplAmounts.size() <= LY_MAX_AUX_PER_OBJ)
            {
                for (; envPair != envEnd; ++auxCount)
                {
                    const float amount = envPair->second;

                    akAuxSendValues[auxCount].auxBusID = envPair->first;
                    akAuxSendValues[auxCount].fControlValue = amount;
                    akAuxSendValues[auxCount].listenerID = m_defaultListenerGameObjectID;  // TODO: Expand api to allow specify listeners

                    // If an amount is zero, we still want to send it to the middleware, but we also want to remove it from the map.
                    if (amount == 0.0f)
                    {
                        envPair = implObjectData->cEnvironmentImplAmounts.erase(envPair);
                    }
                    else
                    {
                        ++envPair;
                    }
                }
            }
            else
            {
                // sort the environments in order of decreasing amounts and take the first LY_MAX_AUX_PER_OBJ worth
                using TEnvPairSet = AZStd::set<SATLAudioObjectData_wwise::TEnvironmentImplMap::value_type, SEnvPairCompare, Audio::AudioImplStdAllocator>;
                TEnvPairSet envPairs(envBegin, envEnd);

                TEnvPairSet::const_iterator sortedEnvPair = envPairs.begin();
                const TEnvPairSet::const_iterator sortedEnvEnd = envPairs.end();

                for (; (sortedEnvPair != sortedEnvEnd) && (auxCount < LY_MAX_AUX_PER_OBJ); ++sortedEnvPair, ++auxCount)
                {
                    akAuxSendValues[auxCount].auxBusID = sortedEnvPair->first;
                    akAuxSendValues[auxCount].fControlValue = sortedEnvPair->second;
                    akAuxSendValues[auxCount].listenerID = m_defaultListenerGameObjectID;      // TODO: Expand api to allow specify listeners
                }

                // remove all Environments with 0.0 amounts
                while (envPair != envEnd)
                {
                    if (envPair->second == 0.0f)
                    {
                        envPair = implObjectData->cEnvironmentImplAmounts.erase(envPair);
                    }
                    else
                    {
                        ++envPair;
                    }
                }
            }

            AZ_Assert(auxCount <= LY_MAX_AUX_PER_OBJ, "WwiseImpl PostEnvironmentAmounts - Exceeded the allowed number of aux environments that can be set!");

            const AKRESULT akResult = AK::SoundEngine::SetGameObjectAuxSendValues(implObjectData->nAKID, akAuxSendValues, auxCount);

            if (IS_WWISE_OK(akResult))
            {
                result = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_wwise.Log(eALT_WARNING,
                    "Wwise SetGameObjectAuxSendValues failed on object %llu with AKRESULT: %u",
                    implObjectData->nAKID,
                    akResult);
            }

            implObjectData->bNeedsToUpdateEnvironments = false;
        }

        return result;
    }

    //////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_wwise::GetImplSubPath() const
    {
        return WwiseImplSubPath;
    }

    //////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_wwise::SetLanguage(const char* const language)
    {
        if (language)
        {
            AZStd::string languageSubfolder(language);

            languageSubfolder += "/";

            m_localizedSoundbankFolder = m_soundbankFolder;
            m_localizedSoundbankFolder.append(languageSubfolder);

            m_fileIOHandler.SetLanguageFolder(languageSubfolder.c_str());
        }
    }

} // namespace Audio

