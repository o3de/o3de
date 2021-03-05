/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <SoundCVars.h>
#include <AudioLogger.h>
#include <AudioSystem_Traits_Platform.h>

#include <AzCore/StringFunc/StringFunc.h>

#include <ISystem.h>
#include <IConsole.h>
#include <MicrophoneBus.h>


namespace Audio
{
    extern CAudioLogger g_audioLogger;


    // CVar: s_EnableRaycasts
    // Usage: s_EnableRaycasts=true (false)
    AZ_CVAR(bool, s_EnableRaycasts, true,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Set to true/false to globally enable/disable raycasting for audio occlusion & obstruction.");

    // CVar: s_RaycastMinDistance
    // Usage: s_RaycastMinDistance=0.5
    // Note: This callback defines an "absolute" minimum constant that the value of the CVar should not go below.
    //       We clamp the value to this minimum if it goes below this.
    AZ_CVAR(float, s_RaycastMinDistance, 0.5f,
        [](const float& minDist) -> void
        {
            if (minDist >= s_RaycastMaxDistance)
            {
                AZ_Warning("SoundCVars", false,
                    "CVar 's_RaycastMinDistance' (%f) needs to be less than 's_RaycastMaxDistance' (%f).\n"
                    "Audio raycasts won't run until the distance range is fixed.\n",
                    static_cast<float>(s_RaycastMinDistance), static_cast<float>(s_RaycastMaxDistance));
            }

            static constexpr float s_absoluteMinRaycastDistance = 0.1f;
            s_RaycastMinDistance = AZ::GetMax(minDist, s_absoluteMinRaycastDistance);
            AZ_Warning("SoundCVars", s_RaycastMinDistance == minDist,
                "CVar 's_RaycastMinDistance' will be clamped to an absolute minimum value of %f.\n", s_absoluteMinRaycastDistance);
        },
        AZ::ConsoleFunctorFlags::Null,
        "Raycasts for obstruction/occlusion are not sent for sounds whose distance to the listener is less than this value.");

    // CVar: s_RaycastMaxDistance
    // Usage: s_RaycastMaxDistance=100.0
    // Note: This callback defines an "absolute" maximum constant that the value of the CVar should not go above.
    //       We clamp the value to this maximum if it goes above this.
    AZ_CVAR(float, s_RaycastMaxDistance, 100.f,
        [](const float& maxDist) -> void
        {
            if (maxDist <= s_RaycastMinDistance)
            {
                AZ_Warning("SoundCVars", false,
                    "CVar 's_RaycastMaxDistance' (%f) needs to be greater than 's_RaycastMinDistance' (%f).\n"
                    "Audio raycasts won't run until the distance range is fixed.\n",
                    static_cast<float>(s_RaycastMaxDistance), static_cast<float>(s_RaycastMinDistance));
            }

            static constexpr float s_absoluteMaxRaycastDistance = 1000.f;
            s_RaycastMaxDistance = AZ::GetMin(maxDist, s_absoluteMaxRaycastDistance);
            AZ_Warning("SoundCVars", s_RaycastMaxDistance == maxDist,
                "CVar 's_RaycastMaxDistance' will be clamped to an absolute maximum value of %f.\n", s_absoluteMaxRaycastDistance);
        },
        AZ::ConsoleFunctorFlags::Null,
        "Raycasts for obstruction/occlusion are not sent for sounds whose distance to the listener is greater than this value.");

    // CVar: s_RaycastCacheTimeMs
    // Usage: s_RaycastCacheTimeMs=250.0
    // Note: This callback defines an "absolute" minimum constant that the value of the CVar should not go below.
    //       We clamp the value to this minimum if it goes below this.
    AZ_CVAR(float, s_RaycastCacheTimeMs, 250.f,
        [](const float& cacheTimeMs) -> void
        {
            static constexpr float s_absoluteMinRaycastCacheTimeMs = 1 / 60.f;
            s_RaycastCacheTimeMs = AZ::GetMax(cacheTimeMs, s_absoluteMinRaycastCacheTimeMs);
            AZ_Warning("SoundCVars", cacheTimeMs == s_RaycastCacheTimeMs,
                "CVar 's_RaycastCacheTimeMs' will be clamped to an absolute minimum of %f.\n", s_absoluteMinRaycastCacheTimeMs);
        },
        AZ::ConsoleFunctorFlags::Null,
        "Physics raycast results are given this amount of time before they are considered dirty and need to be recast.");

    // CVar: s_RaycastSmoothFactor
    // Usage: s_RaycastSmoothFactor=5.0
    AZ_CVAR(float, s_RaycastSmoothFactor, 7.f,
        [](const float& smoothFactor) ->void
        {
            static constexpr float s_absoluteMinRaycastSmoothFactor = 0.f;
            static constexpr float s_absoluteMaxRaycastSmoothFactor = 10.f;
            s_RaycastSmoothFactor = AZ::GetClamp(smoothFactor, s_absoluteMinRaycastSmoothFactor, s_absoluteMaxRaycastSmoothFactor);
            AZ_Warning("SoundCVars", s_RaycastSmoothFactor == smoothFactor,
                "CVar 's_RaycastSmoothFactor' was be clamped to an absolute range of [%f, %f].\n",
                s_absoluteMinRaycastSmoothFactor, s_absoluteMaxRaycastSmoothFactor);
        },
        AZ::ConsoleFunctorFlags::Null,
        "How slowly the smoothing of obstruction/occlusion values should smooth to target: delta / (smoothFactor^2 + 1).  "
        "Low values will smooth faster, high values will smooth slower.");

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CSoundCVars::CSoundCVars()
        : m_nATLPoolSize(0)
        , m_nFileCacheManagerSize(0)
        , m_nAudioObjectPoolSize(0)
        , m_nAudioEventPoolSize(0)
        , m_nAudioProxiesInitType(0)
        , m_fPositionUpdateThreshold(0.0f)
        , m_fVelocityTrackingThreshold(0.0f)
        , m_audioListenerTranslationPercentage(0.f)
        , m_audioListenerTranslationZOffset(0.f)

    #if !defined(AUDIO_RELEASE)
        , m_nIgnoreWindowFocus(0)
        , m_nDrawAudioDebug(0)
        , m_nFileCacheManagerDebugFilter(0)
        , m_nAudioLoggingOptions(0)
        , m_nShowActiveAudioObjectsOnly(0)
        , m_pAudioTriggersDebugFilter(nullptr)
        , m_pAudioObjectsDebugFilter(nullptr)
    #endif // !AUDIO_RELEASE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CSoundCVars::~CSoundCVars()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::RegisterVariables()
    {
        m_nATLPoolSize          = AZ_TRAIT_AUDIOSYSTEM_ATL_POOL_SIZE;
        m_nAudioEventPoolSize   = AZ_TRAIT_AUDIOSYSTEM_AUDIO_EVENT_POOL_SIZE;
        m_nAudioObjectPoolSize  = AZ_TRAIT_AUDIOSYSTEM_AUDIO_OBJECT_POOL_SIZE;
        m_nFileCacheManagerSize = AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_SIZE;

        // Common Cross-Platform Defaults
        m_nAudioProxiesInitType         = 0;
        m_fPositionUpdateThreshold      = 0.1f;
        m_fVelocityTrackingThreshold    = 0.1f;

        REGISTER_CVAR2("s_ATLPoolSize", &m_nATLPoolSize, m_nATLPoolSize, VF_REQUIRE_APP_RESTART,
            "Specifies the size (in KiB) of the memory pool to be used by the ATL.\n"
            "Usage: s_ATLPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_AUDIOSYSTEM_ATL_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_AudioEventPoolSize", &m_nAudioEventPoolSize, m_nAudioEventPoolSize, VF_REQUIRE_APP_RESTART,
            "Sets the number of preallocated audio events.\n"
            "Usage: s_AudioEventPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_AUDIOSYSTEM_AUDIO_EVENT_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_AudioObjectPoolSize", &m_nAudioObjectPoolSize, m_nAudioObjectPoolSize, VF_REQUIRE_APP_RESTART,
            "Sets the number of preallocated audio objects and corresponding audio proxies.\n"
            "Usage: s_AudioObjectPoolSize [0/...]\n"
            "Default: " AZ_TRAIT_AUDIOSYSTEM_AUDIO_OBJECT_POOL_SIZE_DEFAULT_TEXT "\n");

        REGISTER_CVAR2("s_FileCacheManagerSize", &m_nFileCacheManagerSize, m_nFileCacheManagerSize, VF_REQUIRE_APP_RESTART,
            "Sets the size in KiB the AFCM will allocate on the heap.\n"
            "Usage: s_FileCacheManagerSize [0/...]\n"
            "Default: " AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_SIZE_DEFAULT_TEXT "\n");


        REGISTER_CVAR2("s_PositionUpdateThreshold", &m_fPositionUpdateThreshold, m_fPositionUpdateThreshold, VF_CHEAT | VF_CHEAT_NOCHECK,
            "An audio object has to move by at least this amount to issue a position update request to the audio system.\n"
            "This kind of optimization should ideally be done by the parent system so this is here for convenience.\n"
            "Usage: s_PositionUpdateThreshold [0/...]\n"
            "Default: 0.1 (10 cm)\n");

        REGISTER_CVAR2("s_VelocityTrackingThreshold", &m_fVelocityTrackingThreshold, m_fVelocityTrackingThreshold, VF_CHEAT | VF_CHEAT_NOCHECK,
            "An audio object has to change its velocity by at least this amount to issue an \"object_speed\" RTPC update request to the audio system.\n"
            "Usage: s_VelocityTrackingThreshold [0/...]\n"
            "Default: 0.1 (10 cm/s)\n");

        REGISTER_CVAR2("s_AudioProxiesInitType", &m_nAudioProxiesInitType, m_nAudioProxiesInitType, VF_NULL,
            "Can override AudioProxies' init type on a global scale.\n"
            "If set it determines whether AudioProxies initialize synchronously or asynchronously.\n"
            "This is a performance type cvar as asynchronously initializing AudioProxies\n"
            "will have a greatly reduced impact on the calling thread.\n"
            "Be aware though that when set to initialize asynchronously that audio will play back delayed.\n"
            "By how much will greatly depend on the audio thread's work load.\n"
            "0: AudioProxy specific initialization.\n"
            "1: All AudioProxies initialize synchronously.\n"
            "2: All AudioProxies initialize asynchronously.\n"
            "Usage: s_AudioProxiesInitType [0/1/2]\n"
            "Default: 0\n");

        REGISTER_CVAR2("s_AudioListenerTranslationZOffset", &m_audioListenerTranslationZOffset, 0.f, VF_NULL,
            "Use this to specify a Z-Offset (\"Up\") for the audio listener's position.\n"
            "Usage: s_AudioListenerTranslationZOffset 1.3\n"
            "Default: 0.0\n");

        REGISTER_CVAR2("s_AudioListenerTranslationPercentage", &m_audioListenerTranslationPercentage, 0.f, VF_NULL,
            "Use this to specify a percentage of translation of the audio listener between two points\n"
            "(usually these are the camera's location and player's location).\n"
            "Usage: s_AudioListenerTranslationPercentage [0.0..1.0]\n"
            "Default: 0.0\n");

#if !defined(AUDIO_RELEASE)
        REGISTER_COMMAND("s_ExecuteTrigger", CmdExecuteTrigger, VF_CHEAT,
            "Execute an Audio Trigger.\n"
            "The first argument is the name of the AudioTrigger to be executed, the second argument is an optional AudioObject ID.\n"
            "If the second argument is provided, the AudioTrigger is executed on the AudioObject with the given ID,\n"
            "otherwise, the AudioTrigger is executed on the GlobalAudioObject\n"
            "Usage: s_ExecuteTrigger Play_chicken_idle 605 or s_ExecuteTrigger MuteDialog\n");

        REGISTER_COMMAND("s_StopTrigger", CmdStopTrigger, VF_CHEAT,
            "Execute an Audio Trigger.\n"
            "The first argument is the name of the AudioTrigger to be stopped, the second argument is an optional AudioObject ID.\n"
            "If the second argument is provided, the AudioTrigger is stopped on the AudioObject with the given ID,\n"
            "otherwise, the AudioTrigger is stopped on the GlobalAudioObject\n"
            "Usage: s_StopTrigger Play_chicken_idle 605 or s_StopTrigger MuteDialog\n");

        REGISTER_COMMAND("s_SetRtpc", CmdSetRtpc, VF_CHEAT,
            "Set an Audio RTPC value.\n"
            "The first argument is the name of the AudioRtpc to be set, the second argument is the float value to be set,"
            "the third argument is an optional AudioObject ID.\n"
            "If the third argument is provided, the AudioRtpc is set on the AudioObject with the given ID,\n"
            "otherwise, the AudioRtpc is set on the GlobalAudioObject\n"
            "Usage: s_SetRtpc character_speed  0.0  601 or s_SetRtpc volume_music 1.0\n");

        REGISTER_COMMAND("s_SetSwitchState", CmdSetSwitchState, VF_CHEAT,
            "Set an Audio Switch to a provided State.\n"
            "The first argument is the name of the AudioSwitch to, the second argument is the name of the SwitchState to be set,"
            "the third argument is an optional AudioObject ID.\n"
            "If the third argument is provided, the AudioSwitch is set on the AudioObject with the given ID,\n"
            "otherwise, the AudioSwitch is set on the GlobalAudioObject\n"
            "Usage: s_SetSwitchState SurfaceType concrete 601 or s_SetSwitchState weather rain\n");

        REGISTER_COMMAND("s_LoadPreload", CmdLoadPreload, VF_CHEAT,
            "Load an Audio Preload to the FileCacheManager.\n"
            "The first argument is the name of the ATL preload.\n"
            "Usage: s_LoadPreload GlobalBank\n");

        REGISTER_COMMAND("s_UnloadPreload", CmdUnloadPreload, VF_CHEAT,
            "Unload an Audio Preload from the FileCacheManager.\n"
            "The first argument is the name of the ATL Prelaod.\n"
            "Usage: s_UnloadPreload GlobalBank\n");

        REGISTER_COMMAND("s_PlayFile", CmdPlayFile, VF_CHEAT,
            "Play an audio file directly.  Uses Audio Input Source (Wwise).\n"
            "First argument is the name of the file to play.  Only .wav and .pcm (raw) files are supported right now.\n"
            "Second argument is the name of the audio trigger to use."
            "Usage: s_PlayFile \"sounds\\wwise\\external_sources\\sfx\\my_file.wav\" Play_audio_input_2D\n"
            );

        REGISTER_COMMAND("s_Microphone", CmdMicrophone, VF_CHEAT,
            "Turn on/off microphone input.  Uses Audio Input Source (Wwise).\n"
            "First argument is 0 or 1 to turn off or on the Microphone, respectively.\n"
            "Second argument is the name of the ATL trigger to use (when turning microphone on) for Audio Input.\n"
            "Usage: s_Microphone 1 Play_audio_input_2D\n"
            "Usage: s_Microphone 0\n"
        );

        REGISTER_COMMAND("s_PlayExternalSource", CmdPlayExternalSource, VF_CHEAT,
            "Execute an 'External Source' audio trigger.\n"
            "The first argument is the name of the audio trigger to execute.\n"
            "The second argument is the collection Id.\n"
            "The third argument is the language Id.\n"
            "The fourth argument is the file Id.\n"
            "Usage: s_PlayExternalSource Play_ext_vo 0 0 1\n"
        );

        REGISTER_COMMAND("s_SetPanningMode", CmdSetPanningMode, VF_CHEAT,
            "Set the Panning mode to either 'speakers' or 'headphones'.\n"
            "Speakers will have a 60 degree angle from the listener to the L/R speakers.\n"
            "Headphones will have a 180 degree angle from the listener to the L/R speakers.\n"
            "Usage: s_SetPanningMode speakers    (default)\n"
            "Usage: s_SetPanningMode headphones\n"
        );

        REGISTER_CVAR2("s_IgnoreWindowFocus", &m_nIgnoreWindowFocus, 0, VF_DEV_ONLY,
            "If set to 1, the sound system will continue playing when the Editor or Game window loses focus.\n"
            "Usage: s_IgnoreWindowFocus [0/1]\n"
            "Default: 0 (off)\n");

        REGISTER_CVAR2("s_DrawAudioDebug", &m_nDrawAudioDebug, 0, VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
            "Draws AudioTranslationLayer related debug data to the screen.\n"
            "Usage: s_DrawAudioDebug [0ab...] (flags can be combined)\n"
            "0: No audio debug info on the screen.\n"
            "a: Draw spheres around active audio objects.\n"
            "b: Show text labels for active audio objects.\n"
            "c: Show trigger names for active audio objects.\n"
            "d: Show current states for active audio objects.\n"
            "e: Show RTPC values for active audio objects.\n"
            "f: Show Environment amounts for active audio objects.\n"
            "g: Draw occlusion rays.\n"
            "h: Show occlusion ray labels.\n"
            "i: Draw sphere around active audio listener.\n"
            "v: List active Events.\n"
            "w: List active Audio Objects.\n"
            "x: Show FileCache Manager debug info.\n"
            "y: Show memory pool usage info for the audio impl.\n"
            );

        REGISTER_CVAR2("s_FileCacheManagerDebugFilter", &m_nFileCacheManagerDebugFilter, 0, VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
            "Allows for filtered display of the different AFCM entries such as Globals, Level Specifics, Game Hints and so on.\n"
            "Usage: s_FileCacheManagerDebugFilter [0ab...] (flags can be combined)\n"
            "Default: 0 (all)\n"
            "a: Globals\n"
            "b: Level Specifics\n"
            "c: Game Hints\n"
            "d: Currently Loaded\n");

        REGISTER_CVAR2("s_AudioLoggingOptions", &m_nAudioLoggingOptions, AlphaBits("ab"), VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
            "Toggles the logging of audio related messages.\n"
            "Usage: s_AudioLoggingOptions [ab...] (flags can be combined)\n"
            "Default: ab (Errors & Warnings)\n"
            "a: Errors\n"
            "b: Warnings\n"
            "c: Comments\n");

        REGISTER_CVAR2("s_ShowActiveAudioObjectsOnly", &m_nShowActiveAudioObjectsOnly, 1, VF_DEV_ONLY,
            "When drawing audio object names on the screen this cvar can be used to choose between all registered audio objects or only those that reference active audio triggers.\n"
            "Usage: s_ShowActiveAudioObjectsOnly [0/1]\n"
            "Default: 1 (active only)\n");

        m_pAudioTriggersDebugFilter = REGISTER_STRING("s_AudioTriggersDebugFilter", "", 0,
            "Allows for filtered display of audio triggers by a search string.\n"
            "Usage: s_AudioTriggersDebugFilter laser\n"
            "Default: \"\" (all)\n");

        m_pAudioObjectsDebugFilter = REGISTER_STRING("s_AudioObjectsDebugFilter", "", 0,
            "Allows for filtered display of audio objects by a search string.\n"
            "Usage: s_AudioObjectsDebugFilter spaceship.\n"
            "Default: \"\" (all)\n");

#endif // !AUDIO_RELEASE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::UnregisterVariables()
    {
        UNREGISTER_CVAR("s_ATLPoolSize");


        UNREGISTER_CVAR("s_PositionUpdateThreshold");
        UNREGISTER_CVAR("s_VelocityTrackingThreshold");
        UNREGISTER_CVAR("s_FileCacheManagerSize");
        UNREGISTER_CVAR("s_AudioObjectPoolSize");
        UNREGISTER_CVAR("s_AudioEventPoolSize");
        UNREGISTER_CVAR("s_AudioProxiesInitType");
        UNREGISTER_CVAR("s_AudioListenerTranslationYOffset");
        UNREGISTER_CVAR("s_AudioListenerTranslationPercentage");

#if !defined(AUDIO_RELEASE)
        UNREGISTER_COMMAND("s_ExecuteTrigger");
        UNREGISTER_COMMAND("s_StopTrigger");
        UNREGISTER_COMMAND("s_SetRtpc");
        UNREGISTER_COMMAND("s_SetSwitchState");
        UNREGISTER_COMMAND("s_LoadPreload");
        UNREGISTER_COMMAND("s_UnloadPreload");
        UNREGISTER_COMMAND("s_PlayFile");
        UNREGISTER_COMMAND("s_PlayExternalSource");
        UNREGISTER_COMMAND("s_SetPanningMode");
        UNREGISTER_CVAR("s_IgnoreWindowFocus");
        UNREGISTER_CVAR("s_DrawAudioDebug");
        UNREGISTER_CVAR("s_FileCacheManagerDebugFilter");
        UNREGISTER_CVAR("s_AudioLoggingOptions");
        UNREGISTER_CVAR("s_ShowActiveAudioObjectsOnly");
        UNREGISTER_CVAR("s_AudioTriggersDebugFilter");
        UNREGISTER_CVAR("s_AudioObjectsDebugFilter");
#endif // !AUDIO_RELEASE
    }


#if !defined(AUDIO_RELEASE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdExecuteTrigger(IConsoleCmdArgs* pCmdArgs)
    {
        const int nArgCount = pCmdArgs->GetArgCount();

        if ((nArgCount == 2) || (nArgCount == 3))
        {
            TAudioControlID nTriggerID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(nTriggerID, &AudioSystemRequestBus::Events::GetAudioTriggerID, pCmdArgs->GetArg(1));

            if (nTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                TAudioObjectID nObjectID = INVALID_AUDIO_OBJECT_ID;
                if (nArgCount == 3)
                {
                    const int nTempID = atoi(pCmdArgs->GetArg(2));
                    if (nTempID > 0)
                    {
                        nObjectID = static_cast<TAudioObjectID>(nTempID);
                    }
                    else
                    {
                        g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %s", pCmdArgs->GetArg(2));
                    }
                }

                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> oRequestData(nTriggerID, 0.0f);

                oRequest.nAudioObjectID = nObjectID;
                oRequest.nFlags = eARF_PRIORITY_NORMAL;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown trigger name: %s", pCmdArgs->GetArg(1));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_ExecuteTrigger [TriggerName] [[Optional Object ID]]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdStopTrigger(IConsoleCmdArgs* pCmdArgs)
    {
        const int nArgCount = pCmdArgs->GetArgCount();

        if ((nArgCount == 2) || (nArgCount == 3))
        {
            TAudioControlID nTriggerID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(nTriggerID, &AudioSystemRequestBus::Events::GetAudioTriggerID, pCmdArgs->GetArg(1));

            if (nTriggerID != INVALID_AUDIO_CONTROL_ID)
            {
                TAudioObjectID nObjectID = INVALID_AUDIO_OBJECT_ID;
                if (nArgCount == 3)
                {
                    const int nTempID = atoi(pCmdArgs->GetArg(2));
                    if (nTempID > 0)
                    {
                        nObjectID = static_cast<TAudioObjectID>(nTempID);
                    }
                    else
                    {
                        g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %s", pCmdArgs->GetArg(2));
                    }
                }

                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_STOP_TRIGGER> oRequestData(nTriggerID);

                oRequest.nAudioObjectID = nObjectID;
                oRequest.nFlags = eARF_PRIORITY_NORMAL;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown trigger name: %s", pCmdArgs->GetArg(1));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_StopTrigger [TriggerName] [[Optional Object ID]]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdSetRtpc(IConsoleCmdArgs* pCmdArgs)
    {
        const int nArgCount = pCmdArgs->GetArgCount();

        if ((nArgCount == 3) || (nArgCount == 4))
        {
            TAudioControlID nRtpcID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(nRtpcID, &AudioSystemRequestBus::Events::GetAudioRtpcID, pCmdArgs->GetArg(1));

            if (nRtpcID != INVALID_AUDIO_CONTROL_ID)
            {
                double fValue = atof(pCmdArgs->GetArg(2));

                TAudioObjectID nObjectID = INVALID_AUDIO_OBJECT_ID;
                if (nArgCount == 4)
                {
                    const int nTempID = atoi(pCmdArgs->GetArg(3));
                    if (nTempID > 0)
                    {
                        nObjectID = static_cast<TAudioObjectID>(nTempID);
                    }
                    else
                    {
                        g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %s", pCmdArgs->GetArg(3));
                        return;
                    }
                }

                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> oRequestData(nRtpcID, static_cast<float>(fValue));

                oRequest.nAudioObjectID = nObjectID;
                oRequest.nFlags = eARF_PRIORITY_NORMAL;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown Rtpc name: %s", pCmdArgs->GetArg(1));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_SetRtpc [RtpcName] [RtpcValue] [[Optional Object ID]]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdSetSwitchState(IConsoleCmdArgs* pCmdArgs)
    {
        const int nArgCount = pCmdArgs->GetArgCount();

        if ((nArgCount == 3) || (nArgCount == 4))
        {
            TAudioControlID nSwitchID = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(nSwitchID, &AudioSystemRequestBus::Events::GetAudioSwitchID, pCmdArgs->GetArg(1));

            if (nSwitchID != INVALID_AUDIO_CONTROL_ID)
            {
                TAudioSwitchStateID nSwitchStateID = INVALID_AUDIO_SWITCH_STATE_ID;
                AudioSystemRequestBus::BroadcastResult(nSwitchStateID, &AudioSystemRequestBus::Events::GetAudioSwitchStateID, nSwitchID, pCmdArgs->GetArg(2));

                if (nSwitchStateID != INVALID_AUDIO_SWITCH_STATE_ID)
                {
                    TAudioObjectID nObjectID = INVALID_AUDIO_OBJECT_ID;
                    if (nArgCount == 4)
                    {
                        const int nTempID = atoi(pCmdArgs->GetArg(3));
                        if (nTempID > 0)
                        {
                            nObjectID = static_cast<TAudioObjectID>(nTempID);
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %s", pCmdArgs->GetArg(3));
                            return;
                        }
                    }

                    SAudioRequest oRequest;
                    SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> oRequestData(nSwitchID, nSwitchStateID);

                    oRequest.nAudioObjectID = nObjectID;
                    oRequest.nFlags = eARF_PRIORITY_NORMAL;
                    oRequest.pData = &oRequestData;

                    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "Invalid  Switch State name: %s", pCmdArgs->GetArg(2));
                }
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown Switch name: %s", pCmdArgs->GetArg(1));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_SetSwitchState [SwitchName] [SwitchStateName] [[Optional Object ID]]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdLoadPreload(IConsoleCmdArgs* cmdArgs)
    {
        const int argCount = cmdArgs->GetArgCount();

        if (argCount == 2)
        {
            const char* preloadName = cmdArgs->GetArg(1);

            TAudioPreloadRequestID preloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
            AudioSystemRequestBus::BroadcastResult(preloadId, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName);
            if (preloadId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
            {
                SAudioRequest request;
                SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> requestData(preloadId);
                request.nFlags = eARF_PRIORITY_NORMAL;
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Preload named %s not found", preloadName);
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_LoadPreload [PreloadName]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdUnloadPreload(IConsoleCmdArgs* cmdArgs)
    {
        const int argCount = cmdArgs->GetArgCount();

        if (argCount == 2)
        {
            const char* preloadName = cmdArgs->GetArg(1);

            TAudioPreloadRequestID preloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
            AudioSystemRequestBus::BroadcastResult(preloadId, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName);
            if (preloadId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
            {
                SAudioRequest request;
                SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST> requestData(preloadId);
                request.nFlags = eARF_PRIORITY_NORMAL;
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Preload named %s not found", preloadName);
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_UnloadPreload [PreloadName]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdPlayFile(IConsoleCmdArgs* cmdArgs)
    {
        const int argCount = cmdArgs->GetArgCount();

        if (argCount >= 3)
        {
            const char* filename = cmdArgs->GetArg(1);

            AZStd::string fileext;
            AZ::StringFunc::Path::GetExtension(filename, fileext, false);

            AudioInputSourceType audioInputType = AudioInputSourceType::Unsupported;

            // Use file extension to guess the file type
            if (fileext.compare("wav") == 0)
            {
                audioInputType = AudioInputSourceType::WavFile;
            }
            else if (fileext.compare("pcm") == 0)
            {
                audioInputType = AudioInputSourceType::PcmFile;
            }

            if (audioInputType != AudioInputSourceType::Unsupported)
            {
                // Setup the configuration...
                SAudioInputConfig audioInputConfig(audioInputType, filename);

                if (audioInputType == AudioInputSourceType::PcmFile)
                {
                    if (argCount == 5)
                    {
                        audioInputConfig.m_bitsPerSample = 16;
                        audioInputConfig.m_numChannels = std::strtoul(cmdArgs->GetArg(3), nullptr, 10);
                        audioInputConfig.m_sampleRate = std::strtoul(cmdArgs->GetArg(4), nullptr, 10);
                        audioInputConfig.m_sampleType = AudioInputSampleType::Int;
                    }
                    else
                    {
                        g_audioLogger.Log(eALT_ERROR, "Using PCM file, additional parameters needed: [NumChannels] [SampleRate] (e.g. 2 16000)");
                        return;
                    }
                }

                TAudioSourceId sourceId = INVALID_AUDIO_SOURCE_ID;
                AudioSystemRequestBus::BroadcastResult(sourceId, &AudioSystemRequestBus::Events::CreateAudioSource, audioInputConfig);

                if (sourceId != INVALID_AUDIO_SOURCE_ID)
                {
                    TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
                    AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, cmdArgs->GetArg(2));

                    if (triggerId != INVALID_AUDIO_CONTROL_ID)
                    {
                        SAudioRequest request;
                        SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER> requestData(triggerId, sourceId);
                        request.nFlags = eARF_PRIORITY_NORMAL;
                        request.pData = &requestData;

                        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
                    }
                    else
                    {
                        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::DestroyAudioSource, sourceId);
                        g_audioLogger.Log(eALT_ERROR, "Failed to find the trigger named %s", cmdArgs->GetArg(2));
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "Unable to create a new audio source");
                }
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Audio files with extension .%s are unsupported", fileext.c_str());
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_PlayFile \"path\\to\\myfile.wav\" \"Play_audio_input_2D\"");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdMicrophone(IConsoleCmdArgs* pCmdArgs)
    {
        static bool micState = false;   // mic is off initially
        static TAudioSourceId micSourceId = INVALID_AUDIO_SOURCE_ID;
        static TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;

        const int argCount = pCmdArgs->GetArgCount();

        if (argCount == 3)
        {
            int state = std::strtol(pCmdArgs->GetArg(1), nullptr, 10);

            const char* triggerName = pCmdArgs->GetArg(2);

            if (state == 1 && !micState && micSourceId == INVALID_AUDIO_SOURCE_ID && triggerId == INVALID_AUDIO_CONTROL_ID)
            {
                g_audioLogger.Log(eALT_ALWAYS, "Turning on Microhpone with %s\n", triggerName);
                bool success = true;

                AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
                if (triggerId != INVALID_AUDIO_CONTROL_ID)
                {
                    // Start the mic session
                    bool startedMic = false;
                    MicrophoneRequestBus::BroadcastResult(startedMic, &MicrophoneRequestBus::Events::StartSession);
                    if (startedMic)
                    {
                        SAudioInputConfig micConfig;
                        MicrophoneRequestBus::BroadcastResult(micConfig, &MicrophoneRequestBus::Events::GetFormatConfig);

                        // If you want to test resampling, set the values here before you create an Audio Source.
                        // In this case, we would be specifying 16kHz, 16-bit integers.
                        //micConfig.m_sampleRate = 16000;
                        //micConfig.m_bitsPerSample = 16;
                        //micConfig.m_sampleType = AudioInputSampleType::Int;

                        AudioSystemRequestBus::BroadcastResult(micSourceId, &AudioSystemRequestBus::Events::CreateAudioSource, micConfig);

                        if (micSourceId != INVALID_AUDIO_SOURCE_ID)
                        {
                            SAudioRequest request;
                            SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER> requestData(triggerId, micSourceId);
                            request.nFlags = eARF_PRIORITY_NORMAL;
                            request.pData = &requestData;

                            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
                        }
                        else
                        {
                            success = false;
                            g_audioLogger.Log(eALT_ERROR, "Failed to create a new audio source for the microphone");
                        }
                    }
                    else
                    {
                        success = false;
                        g_audioLogger.Log(eALT_ERROR, "Failed to start the microphone session");
                    }
                }
                else
                {
                    success = false;
                    g_audioLogger.Log(eALT_ERROR, "Failed to find the trigger named %s", triggerName);
                }

                if (success)
                {
                    micState = true;
                }
                else
                {
                    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::DestroyAudioSource, micSourceId);
                    MicrophoneRequestBus::Broadcast(&MicrophoneRequestBus::Events::EndSession);
                    micSourceId = INVALID_AUDIO_SOURCE_ID;
                    triggerId = INVALID_AUDIO_CONTROL_ID;
                }
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Error encountered while turning on/off microphone");
            }
        }
        else if (argCount == 2)
        {
            int state = std::strtol(pCmdArgs->GetArg(1), nullptr, 10);

            if (state == 0 && micState && micSourceId != INVALID_AUDIO_SOURCE_ID && triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                g_audioLogger.Log(eALT_ALWAYS, "Turning off Microphone\n");

                // Stop the trigger (may not be necessary)
                SAudioRequest request;
                SAudioObjectRequestData<eAORT_STOP_TRIGGER> requestData(triggerId);
                request.nFlags = eARF_PRIORITY_NORMAL;
                request.pData = &requestData;
                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);

                // Destroy the audio source, end the mic session, and reset state...
                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::DestroyAudioSource, micSourceId);
                MicrophoneRequestBus::Broadcast(&MicrophoneRequestBus::Events::EndSession);
                micSourceId = INVALID_AUDIO_SOURCE_ID;
                triggerId = INVALID_AUDIO_CONTROL_ID;

                micState = false;
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Error encountered while turning on/off microphone");
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_Microphone 1 Play_audio_input_2D  /  s_Microphone 0");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdPlayExternalSource(IConsoleCmdArgs* pCmdArgs)
    {
        // This cookie value is a hash on the name of the External Source.
        // By default when you add an External Source to a sound, it gives the name 'External_Source' and has this hash.
        // Apparently it can be changed in the Wwise project, so it's unfortunately content-dependent.  But there's no easy
        // way to extract that info in this context.
        const AZ::u64 externalSourceCookieValue = 618371124ull;

        TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;

        if (pCmdArgs->GetArgCount() == 5)
        {
            const char* triggerName = pCmdArgs->GetArg(1);
            AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName);
            if (triggerId == INVALID_AUDIO_CONTROL_ID)
            {
                g_audioLogger.Log(eALT_ERROR, "Failed to find the trigger named '%s'\n", triggerName);
                return;
            }

            int collection = std::strtol(pCmdArgs->GetArg(2), nullptr, 10);
            int language = std::strtol(pCmdArgs->GetArg(3), nullptr, 10);
            int file = std::strtol(pCmdArgs->GetArg(4), nullptr, 10);

            SAudioSourceInfo sourceInfo(externalSourceCookieValue, file, language, collection, eACT_PCM);

            SAudioRequest request;
            SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER> requestData(triggerId, sourceInfo);
            request.nFlags = eARF_PRIORITY_NORMAL;
            request.pData = &requestData;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_PlayExternalSource Play_ext_vo 0 0 1");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CSoundCVars::CmdSetPanningMode(IConsoleCmdArgs* pCmdArgs)
    {
        if (pCmdArgs->GetArgCount() == 2)
        {
            PanningMode panningMode;
            const char* mode = pCmdArgs->GetArg(1);
            if (azstricmp(mode, "speakers") == 0)
            {
                panningMode = PanningMode::Speakers;
                g_audioLogger.Log(eALT_COMMENT, "Setting Panning Mode to 'Speakers'.\n");
            }
            else if (azstricmp(mode, "headphones") == 0)
            {
                panningMode = PanningMode::Headphones;
                g_audioLogger.Log(eALT_COMMENT, "Setting Panning Mode to 'Headphones'.\n");
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Panning mode '%s' is invalid.  Please specify either 'speakers' or 'headphones'\n", mode);
                return;
            }

            SAudioRequest request;
            SAudioManagerRequestData<eAMRT_SET_AUDIO_PANNING_MODE> requestData(panningMode);
            request.nFlags = eARF_PRIORITY_NORMAL;
            request.pData = &requestData;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_SetPanningMode speakers\nUsage: s_SetPanningMode headphones");
        }
    }

#endif // !AUDIO_RELEASE

} // namespace Audio
