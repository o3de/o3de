/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <SoundCVars.h>
#include <AudioLogger.h>
#include <AudioInternalInterfaces.h>
#include <AudioSystem_Traits_Platform.h>

#include <AzCore/Console/ConsoleTypeHelpers.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <MicrophoneBus.h>


namespace Audio::CVars
{
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
        [](const float& smoothFactor) -> void
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

    AZ_CVAR(AZ::u64, s_ATLMemorySize, AZ_TRAIT_AUDIOSYSTEM_ATL_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB of memory to be used by the ATL/Audio System.\n"
        "Usage: s_ATLMemorySize=" AZ_TRAIT_AUDIOSYSTEM_ATL_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(AZ::u64, s_FileCacheManagerMemorySize, AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The size in KiB the File Cache Manager will use for banks.\n"
        "Usage: s_FileCacheManagerMemorySize=" AZ_TRAIT_AUDIOSYSTEM_FILE_CACHE_MANAGER_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(AZ::u64, s_AudioEventPoolSize, AZ_TRAIT_AUDIOSYSTEM_AUDIO_EVENT_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The number of audio events to preallocate in a pool.\n"
        "Usage: s_AudioEventPoolSize=" AZ_TRAIT_AUDIOSYSTEM_AUDIO_EVENT_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(AZ::u64, s_AudioObjectPoolSize, AZ_TRAIT_AUDIOSYSTEM_AUDIO_OBJECT_POOL_SIZE,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "The number of audio objects to preallocate in a pool.\n"
        "Usage: s_AudioObjectPoolSize=" AZ_TRAIT_AUDIOSYSTEM_AUDIO_OBJECT_POOL_SIZE_DEFAULT_TEXT "\n");

    AZ_CVAR(float, s_PositionUpdateThreshold, 0.1f,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "An audio object needs to move by this distance in order to issue a position update to the audio system.\n"
        "Usage: s_PositionUpdateThreshold=5.0\n");

    AZ_CVAR(float, s_VelocityTrackingThreshold, 0.1f,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "An audio object needs to have its velocity changed by this amount in order to issue an 'object_speed' Rtpc update to the audio system.\n"
        "Usage: s_VelocityTrackingThreshold=0.5\n");

    AZ_CVAR(AZ::u32, s_AudioProxiesInitType, 0,
        [](const AZ::u32& initType) -> void
        {
            static constexpr AZ::u32 s_numAudioProxyInitTypes = 3;
            if (initType < s_numAudioProxyInitTypes)
            {
                s_AudioProxiesInitType = initType;
            }
        },
        AZ::ConsoleFunctorFlags::Null,
        "Overrides the initialization mode of audio proxies globally.\n"
        "0: AudioProxy-specific initiaization (Default).\n"
        "1: All AudioProxy's initialize synchronously.\n"
        "2: All AudioProxy's initialize asynchronously.\n"
        "Usage: s_AudioProxiesInitType=2\n");

    auto OnChangeAudioLanguage = []([[maybe_unused]] const AZ::CVarFixedString& language) -> void
    {
        SAudioRequest languageRequest;
        SAudioManagerRequestData<eAMRT_CHANGE_LANGUAGE> languageRequestData;

        languageRequest.pData = &languageRequestData;
        languageRequest.nFlags = Audio::eARF_PRIORITY_HIGH;
        AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, languageRequest);
    };

    AZ_CVAR(AZ::CVarFixedString, g_languageAudio, "", OnChangeAudioLanguage, AZ::ConsoleFunctorFlags::Null, "");

#if !defined(AUDIO_RELEASE)
    AZ_CVAR(bool, s_IgnoreWindowFocus, false,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Determines whether application focus should issue events to the audio system or not.\n"
        "false: Window focus event should be issued (Default).\n"
        "true: Ignore window focus events.\n"
        "Usage: s_IgnoreWindowFocus=true\n");

    AZ_CVAR(bool, s_ShowActiveAudioObjectsOnly, false,
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Determines whether active or all audio objects should be drawn when debug drawing is enabled.\n"
        "false: Draws all audio objects (Default).\n"
        "true: Draws only active audio objects.\n"
        "Usage: s_ShowActiveAudioObjectsOnly=true\n");

    AZ_CVAR(AZ::CVarFixedString, s_AudioTriggersDebugFilter, "",
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Filters debug drawing to only audio triggers that match this filter as sub-string.\n"
        "Usage: s_AudioTriggersDebugFilter=impact_hit\n");

    AZ_CVAR(AZ::CVarFixedString, s_AudioObjectsDebugFilter, "",
        nullptr, AZ::ConsoleFunctorFlags::Null,
        "Filters debug drawing to only audio objects whose name matches this filter as a sub-string.\n"
        "Usage: s_AudioObjectsDebugFilter=weapon_axe\n");

    auto OnChangeLogOptions = [](const AZ::CVarFixedString& options) -> void
    {
        if (options == "0")
        {
            Audio::Log::s_audioLogOptions = Log::Options::None;
            return;
        }

        Audio::Flags<AZ::u32> flags;
        flags.SetFlags(Log::Options::Errors, options.contains("a"));
        flags.SetFlags(Log::Options::Warnings, options.contains("b"));
        flags.SetFlags(Log::Options::Comments, options.contains("c"));
        Audio::Log::s_audioLogOptions = flags.GetRawFlags();
    };

    AZ_CVAR(AZ::CVarFixedString, s_AudioLoggingOptions, "ab",
        OnChangeLogOptions,
        AZ::ConsoleFunctorFlags::Null,
        "Toggles the log level of audio related messages.\n"
        "Usage: s_AudioLoggingOptions=abc (flags can be combined)\n"
        "Default: ab (Errors & Warnings)\n"
        "a: Errors\n"
        "b: Warnings\n"
        "c: Comments\n");

    auto OnChangeDebugDrawOptions = [](const AZ::CVarFixedString& options) -> void
    {
        s_debugDrawOptions.ClearAllFlags();
        if (options == "0")
        {
            return;
        }

        s_debugDrawOptions.SetFlags(DebugDraw::Options::DrawObjects, options.contains("a"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ObjectLabels, options.contains("b"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ObjectTriggers, options.contains("c"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ObjectStates, options.contains("d"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ObjectRtpcs, options.contains("e"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ObjectEnvironments, options.contains("f"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::DrawRays, options.contains("g"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::RayLabels, options.contains("h"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::DrawListener, options.contains("i"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ActiveEvents, options.contains("v"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::ActiveObjects, options.contains("w"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::FileCacheInfo, options.contains("x"));
        s_debugDrawOptions.SetFlags(DebugDraw::Options::MemoryInfo, options.contains("y"));
    };

    AZ_CVAR(AZ::CVarFixedString, s_DrawAudioDebug, "",
        OnChangeDebugDrawOptions,
        AZ::ConsoleFunctorFlags::IsCheat,
        "Draws AudioTranslationLayer related debug data to the screen.\n"
        "Usage: s_DrawAudioDebug=abcde (flags can be combined)\n"
        "0: Turn off.\n"
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
        "y: Show memory usage info for the audio engine.\n");

    auto OnChangeFileCacheManagerFilterOptions = [](const AZ::CVarFixedString& options) -> void
    {
        s_fcmDrawOptions.ClearAllFlags();
        if (options == "0")
        {
            return;
        }

        s_fcmDrawOptions.SetFlags(FileCacheManagerDebugDraw::Options::Global, options.contains("a"));
        s_fcmDrawOptions.SetFlags(FileCacheManagerDebugDraw::Options::LevelSpecific, options.contains("b"));
        s_fcmDrawOptions.SetFlags(FileCacheManagerDebugDraw::Options::UseCounted, options.contains("c"));
        s_fcmDrawOptions.SetFlags(FileCacheManagerDebugDraw::Options::Loaded, options.contains("d"));
    };

    AZ_CVAR(AZ::CVarFixedString, s_FileCacheManagerDebugFilter, "",
        OnChangeFileCacheManagerFilterOptions,
        AZ::ConsoleFunctorFlags::IsCheat,
        "Allows for filtered display of the file cache entries such as Globals, Level Specifics, Use Counted and so on.\n"
        "Usage: s_FileCacheManagerDebugFilter [0ab...] (flags can be combined)\n"
        "Default: 0 (all)\n"
        "a: Globals\n"
        "b: Level Specifics\n"
        "c: Use Counted\n"
        "d: Currently Loaded\n");

    static void s_ExecuteTrigger(const AZ::ConsoleCommandContainer& args);
    static void s_StopTrigger(const AZ::ConsoleCommandContainer& args);
    static void s_SetRtpc(const AZ::ConsoleCommandContainer& args);
    static void s_SetSwitchState(const AZ::ConsoleCommandContainer& args);
    static void s_LoadPreload(const AZ::ConsoleCommandContainer& args);
    static void s_UnloadPreload(const AZ::ConsoleCommandContainer& args);
    static void s_PlayFile(const AZ::ConsoleCommandContainer& args);
    static void s_Microphone(const AZ::ConsoleCommandContainer& args);
    static void s_PlayExternalSource(const AZ::ConsoleCommandContainer& args);
    static void s_SetPanningMode(const AZ::ConsoleCommandContainer& args);

    AZ_CONSOLEFREEFUNC(s_ExecuteTrigger, AZ::ConsoleFunctorFlags::IsCheat,
        "Execute an Audio Trigger.\n"
        "The first argument is the name of the AudioTrigger to be executed, the second argument is an optional AudioObject ID.\n"
        "If the second argument is provided, the AudioTrigger is executed on the AudioObject with the given ID,\n"
        "otherwise, the AudioTrigger is executed on the GlobalAudioObject\n"
        "Usage: s_ExecuteTrigger Play_chicken_idle 605 or s_ExecuteTrigger MuteDialog\n");

    AZ_CONSOLEFREEFUNC(s_StopTrigger, AZ::ConsoleFunctorFlags::IsCheat,
        "Stops an Audio Trigger.\n"
        "The first argument is the name of the AudioTrigger to be stopped, the second argument is an optional AudioObject ID.\n"
        "If the second argument is provided, the AudioTrigger is stopped on the AudioObject with the given ID,\n"
        "otherwise, the AudioTrigger is stopped on the GlobalAudioObject\n"
        "Usage: s_StopTrigger Play_chicken_idle 605 or s_StopTrigger MuteDialog\n");

    AZ_CONSOLEFREEFUNC(s_SetRtpc, AZ::ConsoleFunctorFlags::IsCheat,
        "Set an Audio RTPC value.\n"
        "The first argument is the name of the AudioRtpc to be set, the second argument is the float value to be set,"
        "the third argument is an optional AudioObject ID.\n"
        "If the third argument is provided, the AudioRtpc is set on the AudioObject with the given ID,\n"
        "otherwise, the AudioRtpc is set on the GlobalAudioObject\n"
        "Usage: s_SetRtpc character_speed  0.0  601 or s_SetRtpc volume_music 1.0\n");

    AZ_CONSOLEFREEFUNC(s_SetSwitchState, AZ::ConsoleFunctorFlags::IsCheat,
        "Set an Audio Switch to a provided State.\n"
        "The first argument is the name of the AudioSwitch to, the second argument is the name of the SwitchState to be set,"
        "the third argument is an optional AudioObject ID.\n"
        "If the third argument is provided, the AudioSwitch is set on the AudioObject with the given ID,\n"
        "otherwise, the AudioSwitch is set on the GlobalAudioObject\n"
        "Usage: s_SetSwitchState SurfaceType concrete 601 or s_SetSwitchState weather rain\n");

    AZ_CONSOLEFREEFUNC(s_LoadPreload, AZ::ConsoleFunctorFlags::IsCheat,
        "Load an Audio Preload to the FileCacheManager.\n"
        "The first argument is the name of the ATL preload.\n"
        "Usage: s_LoadPreload GlobalBank\n");

    AZ_CONSOLEFREEFUNC(s_UnloadPreload, AZ::ConsoleFunctorFlags::IsCheat,
        "Unload an Audio Preload from the FileCacheManager.\n"
        "The first argument is the name of the ATL Prelaod.\n"
        "Usage: s_UnloadPreload GlobalBank\n");

    AZ_CONSOLEFREEFUNC(s_PlayFile, AZ::ConsoleFunctorFlags::IsCheat,
        "Play an audio file directly.\n"
        "First argument is the name of the file to play.  Only .wav and .pcm (raw) files are supported right now.\n"
        "Second argument is the name of the audio trigger to use."
        "Usage: s_PlayFile \"sounds\\wwise\\external_sources\\sfx\\my_file.wav\" Play_audio_input_2D\n");

    AZ_CONSOLEFREEFUNC(s_Microphone, AZ::ConsoleFunctorFlags::IsCheat,
        "Turn on/off microphone input.\n"
        "First argument is 0 or 1 to turn off or on the Microphone, respectively.\n"
        "Second argument is the name of the ATL trigger to use (when turning microphone on) for Audio Input.\n"
        "Usage: s_Microphone 1 Play_audio_input_2D\n"
        "Usage: s_Microphone 0\n");

    AZ_CONSOLEFREEFUNC(s_PlayExternalSource, AZ::ConsoleFunctorFlags::IsCheat,
        "Execute an 'External Source' audio trigger.\n"
        "The first argument is the name of the audio trigger to execute.\n"
        "The second argument is the collection Id.\n"
        "The third argument is the language Id.\n"
        "The fourth argument is the file Id.\n"
        "Usage: s_PlayExternalSource Play_external_VO 0 0 1\n");

    AZ_CONSOLEFREEFUNC(s_SetPanningMode, AZ::ConsoleFunctorFlags::IsCheat,
        "Set the Panning mode to either 'speakers' or 'headphones'.\n"
        "Speakers will have a 60 degree angle from the listener to the L/R speakers.\n"
        "Headphones will have a 180 degree angle from the listener to the L/R speakers.\n"
        "Usage: s_SetPanningMode speakers    (default)\n"
        "Usage: s_SetPanningMode headphones\n");

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_ExecuteTrigger(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 1 || args.size() == 2)
        {
            AZStd::string triggerName(args[0]);
            TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName.c_str());

            if (triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                TAudioObjectID objectId = INVALID_AUDIO_OBJECT_ID;
                if (args.size() == 2)
                {
                    if (!AZ::ConsoleTypeHelpers::StringToValue<TAudioObjectID>(objectId, args[1]))
                    {
                        g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %.*s", AZ_STRING_ARG(args[1]));
                        return;
                    }
                }

                SAudioRequest request;
                SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> requestData(triggerId, 0.0f);

                request.nAudioObjectID = objectId;
                request.nFlags = eARF_PRIORITY_NORMAL;
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown trigger name: %.*s", AZ_STRING_ARG(args[0]));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_ExecuteTrigger TriggerName [Optional Object ID]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_StopTrigger(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 1 || args.size() == 2)
        {
            AZStd::string triggerName(args[0]);
            TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName.c_str());

            if (triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                TAudioObjectID objectId = INVALID_AUDIO_OBJECT_ID;
                if (args.size() == 2)
                {
                    if (!AZ::ConsoleTypeHelpers::StringToValue<TAudioObjectID>(objectId, args[1]))
                    {
                        g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %.*s", AZ_STRING_ARG(args[1]));
                        return;
                    }
                }

                SAudioRequest request;
                SAudioObjectRequestData<eAORT_STOP_TRIGGER> requestData(triggerId);

                request.nAudioObjectID = objectId;
                request.nFlags = eARF_PRIORITY_NORMAL;
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown trigger name: %.*s", AZ_STRING_ARG(args[0]));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_StopTrigger TriggerName [Optional Object ID]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_SetRtpc(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 2 || args.size() == 3)
        {
            AZStd::string rtpcName(args[0]);
            TAudioControlID rtpcId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(rtpcId, &AudioSystemRequestBus::Events::GetAudioRtpcID, rtpcName.c_str());

            if (rtpcId != INVALID_AUDIO_CONTROL_ID)
            {
                float value = 0.f;
                if (!AZ::ConsoleTypeHelpers::StringToValue<float>(value, args[1]))
                {
                    g_audioLogger.Log(eALT_ERROR, "Invalid float number: %.*s", AZ_STRING_ARG(args[1]));
                    return;
                }

                TAudioObjectID objectId = INVALID_AUDIO_OBJECT_ID;
                if (args.size() == 3)
                {
                    if (!AZ::ConsoleTypeHelpers::StringToValue<TAudioObjectID>(objectId, args[2]))
                    {
                        g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %.*s", AZ_STRING_ARG(args[2]));
                        return;
                    }
                }

                SAudioRequest request;
                SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> requestData(rtpcId, value);

                request.nAudioObjectID = objectId;
                request.nFlags = eARF_PRIORITY_NORMAL;
                request.pData = &requestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown Rtpc name: %.*s", AZ_STRING_ARG(args[0]));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_SetRtpc RtpcName RtpcValue [Optional Object ID]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_SetSwitchState(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 2 || args.size() == 3)
        {
            AZStd::string switchName(args[0]);
            TAudioControlID switchId = INVALID_AUDIO_CONTROL_ID;
            AudioSystemRequestBus::BroadcastResult(switchId, &AudioSystemRequestBus::Events::GetAudioSwitchID, switchName.c_str());

            if (switchId != INVALID_AUDIO_CONTROL_ID)
            {
                AZStd::string stateName(args[1]);
                TAudioSwitchStateID stateId = INVALID_AUDIO_SWITCH_STATE_ID;
                AudioSystemRequestBus::BroadcastResult(
                    stateId, &AudioSystemRequestBus::Events::GetAudioSwitchStateID, switchId, stateName.c_str());

                if (stateId != INVALID_AUDIO_SWITCH_STATE_ID)
                {
                    TAudioObjectID objectId = INVALID_AUDIO_OBJECT_ID;
                    if (args.size() == 3)
                    {
                        if (!AZ::ConsoleTypeHelpers::StringToValue<TAudioObjectID>(objectId, args[2]))
                        {
                            g_audioLogger.Log(eALT_ERROR, "Invalid Object ID: %.*s", AZ_STRING_ARG(args[2]));
                            return;
                        }
                    }

                    SAudioRequest request;
                    SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> requestData(switchId, stateId);

                    request.nAudioObjectID = objectId;
                    request.nFlags = eARF_PRIORITY_NORMAL;
                    request.pData = &requestData;

                    AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "Invalid Switch State name: %.*s", AZ_STRING_ARG(args[1]));
                }
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown Switch name: %.*s", AZ_STRING_ARG(args[0]));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_SetSwitchState SwitchName SwitchStateName [Optional Object ID]");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_LoadPreload(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 1)
        {
            AZStd::string preloadName(args[0]);
            TAudioPreloadRequestID preloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
            AudioSystemRequestBus::BroadcastResult(preloadId, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName.c_str());
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
                g_audioLogger.Log(eALT_ERROR, "Preload named %.*s not found", AZ_STRING_ARG(args[0]));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_LoadPreload PreloadName");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_UnloadPreload(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 1)
        {
            AZStd::string preloadName(args[0]);
            TAudioPreloadRequestID preloadId = INVALID_AUDIO_PRELOAD_REQUEST_ID;
            AudioSystemRequestBus::BroadcastResult(preloadId, &AudioSystemRequestBus::Events::GetAudioPreloadRequestID, preloadName.c_str());
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
                g_audioLogger.Log(eALT_ERROR, "Preload named %.*s not found", AZ_STRING_ARG(args[0]));
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_UnloadPreload PreloadName");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_PlayFile(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() >= 2)
        {
            AZStd::string filename(args[0]);

            AZStd::string fileext;
            AZStd::to_lower(fileext.begin(), fileext.end());
            AZ::StringFunc::Path::GetExtension(filename.c_str(), fileext, false);

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
                SAudioInputConfig audioInputConfig(audioInputType, filename.c_str());

                if (audioInputType == AudioInputSourceType::PcmFile)
                {
                    if (args.size() == 4)
                    {
                        audioInputConfig.m_bitsPerSample = 16;
                        if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u32>(audioInputConfig.m_numChannels, args[2]))
                        {
                            g_audioLogger.Log(eALT_ERROR, "Invalid number of channels '%.*s'", AZ_STRING_ARG(args[2]));
                            return;
                        }
                        if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u32>(audioInputConfig.m_sampleRate, args[3]))
                        {
                            g_audioLogger.Log(eALT_ERROR, "Invalid sample rate '%.*s'", AZ_STRING_ARG(args[3]));
                            return;
                        }
                        audioInputConfig.m_sampleType = AudioInputSampleType::Int;
                    }
                    else
                    {
                        g_audioLogger.Log(
                            eALT_ERROR, "Using PCM file, additional parameters needed: [NumChannels] [SampleRate] (e.g. 2 16000)");
                        return;
                    }
                }

                TAudioSourceId sourceId = INVALID_AUDIO_SOURCE_ID;
                AudioSystemRequestBus::BroadcastResult(sourceId, &AudioSystemRequestBus::Events::CreateAudioSource, audioInputConfig);

                if (sourceId != INVALID_AUDIO_SOURCE_ID)
                {
                    TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
                    AudioSystemRequestBus::BroadcastResult(
                        triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, args[1].data());

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
                        g_audioLogger.Log(eALT_ERROR, "Failed to find the trigger named %.*s", AZ_STRING_ARG(args[1]));
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "Unable to create a new audio source");
                }
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR, "Audio files with extension '.%s' are unsupported", fileext.c_str());
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Usage: s_PlayFile \"path\\to\\myfile.wav\" Play_audio_input_2D");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_Microphone(const AZ::ConsoleCommandContainer& args)
    {
        static bool micState = false; // mic is off initially
        static TAudioSourceId micSourceId = INVALID_AUDIO_SOURCE_ID;
        static TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;

        if (args.size() == 2)
        {
            AZ::u32 state;
            if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u32>(state, args[0]))
            {
                g_audioLogger.Log(eALT_ERROR, "Invalid number passed '%.*s', should be 0 or 1", AZ_STRING_ARG(args[0]));
                return;
            }

            AZStd::string triggerName(args[1]);

            if (state == 1 && !micState && micSourceId == INVALID_AUDIO_SOURCE_ID && triggerId == INVALID_AUDIO_CONTROL_ID)
            {
                g_audioLogger.Log(eALT_ALWAYS, "Turning on Microhpone with %s\n", triggerName.c_str());
                bool success = true;

                AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName.c_str());
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
                        // micConfig.m_sampleRate = 16000;
                        // micConfig.m_bitsPerSample = 16;
                        // micConfig.m_sampleType = AudioInputSampleType::Int;

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
                    g_audioLogger.Log(eALT_ERROR, "Failed to find the trigger named %s", triggerName.c_str());
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
        else if (args.size() == 1)
        {
            AZ::u32 state;
            if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u32>(state, args[0]))
            {
                g_audioLogger.Log(eALT_ERROR, "Invalid number passed '%.*s', should be 0 or 1", AZ_STRING_ARG(args[0]));
                return;
            }

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
            g_audioLogger.Log(eALT_ERROR, "Usage: s_Microphone 1 Play_audio_input_2D\nUsage: s_Microphone 0");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void s_PlayExternalSource(const AZ::ConsoleCommandContainer& args)
    {
        // This cookie value is a hash on the name of the External Source.
        // By default when you add an External Source to a sound, it gives the name 'External_Source' and has this hash.
        // Apparently it can be changed in the Wwise project, so it's unfortunately content-dependent.  But there's no easy
        // way to extract that info in this context.
        static constexpr AZ::u64 externalSourceCookieValue = 618371124ull;

        TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;

        if (args.size() == 4)
        {
            AZStd::string triggerName(args[0]);
            AudioSystemRequestBus::BroadcastResult(triggerId, &AudioSystemRequestBus::Events::GetAudioTriggerID, triggerName.c_str());
            if (triggerId == INVALID_AUDIO_CONTROL_ID)
            {
                g_audioLogger.Log(eALT_ERROR, "Failed to find the trigger named '%s'\n", triggerName.c_str());
                return;
            }

            AZ::u64 collection;
            if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u64>(collection, args[1]))
            {
                g_audioLogger.Log(eALT_ERROR, "Invalid collection number passed '%.*s'", AZ_STRING_ARG(args[1]));
                return;
            }
            AZ::u64 language;
            if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u64>(language, args[2]))
            {
                g_audioLogger.Log(eALT_ERROR, "Invalid language number passed '%.*s'", AZ_STRING_ARG(args[2]));
                return;
            }
            AZ::u64 file;
            if (!AZ::ConsoleTypeHelpers::StringToValue<AZ::u64>(file, args[3]))
            {
                g_audioLogger.Log(eALT_ERROR, "Invalid file number passed '%.*s'", AZ_STRING_ARG(args[3]));
                return;
            }

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
    static void s_SetPanningMode(const AZ::ConsoleCommandContainer& args)
    {
        if (args.size() == 1)
        {
            PanningMode panningMode;
            AZStd::string mode(args[0]);
            AZStd::to_lower(mode.begin(), mode.end());
            if (mode == "speakers")
            {
                panningMode = PanningMode::Speakers;
                g_audioLogger.Log(eALT_COMMENT, "Setting Panning Mode to 'Speakers'\n");
            }
            else if (mode == "headphones")
            {
                panningMode = PanningMode::Headphones;
                g_audioLogger.Log(eALT_COMMENT, "Setting Panning Mode to 'Headphones'\n");
            }
            else
            {
                g_audioLogger.Log(eALT_ERROR,
                    "Panning mode '%.*s' is invalid.  Please specify either 'speakers' or 'headphones'\n",
                    AZ_STRING_ARG(args[0]));
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
            g_audioLogger.Log(eALT_ERROR, "Usage: s_SetPanningMode (speakers|headphones)\n");
        }
    }

#endif // !AUDIO_RELEASE

} // namespace Audio::CVars
