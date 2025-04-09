/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ATLUtils.h>
#include <AzCore/Console/IConsole.h>

namespace Audio::CVars
{
    AZ_CVAR_EXTERNED(AZ::u32, sys_audio_disable);

    AZ_CVAR_EXTERNED(AZ::u64, s_ATLMemorySize);
    AZ_CVAR_EXTERNED(AZ::u64, s_FileCacheManagerMemorySize);
    AZ_CVAR_EXTERNED(AZ::u64, s_AudioObjectPoolSize);
    AZ_CVAR_EXTERNED(AZ::u64, s_AudioEventPoolSize);

    AZ_CVAR_EXTERNED(bool, s_EnableRaycasts);
    AZ_CVAR_EXTERNED(float, s_RaycastMinDistance);
    AZ_CVAR_EXTERNED(float, s_RaycastMaxDistance);
    AZ_CVAR_EXTERNED(float, s_RaycastCacheTimeMs);
    AZ_CVAR_EXTERNED(float, s_RaycastSmoothFactor);

    AZ_CVAR_EXTERNED(float, s_PositionUpdateThreshold);
    AZ_CVAR_EXTERNED(float, s_VelocityTrackingThreshold);
    AZ_CVAR_EXTERNED(AZ::u32, s_AudioProxiesInitType);

    AZ_CVAR_EXTERNED(AZ::CVarFixedString, g_languageAudio);

#if !defined(AUDIO_RELEASE)
    AZ_CVAR_EXTERNED(bool, s_IgnoreWindowFocus);
    AZ_CVAR_EXTERNED(bool, s_ShowActiveAudioObjectsOnly);
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_AudioTriggersDebugFilter);
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_AudioObjectsDebugFilter);
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_DrawAudioDebug);
    inline Audio::Flags<AZ::u32> s_debugDrawOptions;
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_FileCacheManagerDebugFilter);
    inline Audio::Flags<AZ::u32> s_fcmDrawOptions;
#endif // !AUDIO_RELEASE

} // namespace Audio::CVars


#if !defined(AUDIO_RELEASE)
// Flags for the debug draw cvars
namespace Audio::DebugDraw
{
    enum class Options
    {
        None = 0,
        DrawObjects = (1 << 0),
        ObjectLabels = (1 << 1),
        ObjectTriggers = (1 << 2),
        ObjectStates = (1 << 3),
        ObjectRtpcs = (1 << 4),
        ObjectEnvironments = (1 << 5),
        DrawRays = (1 << 6),
        RayLabels = (1 << 7),
        DrawListener = (1 << 8),
        ActiveEvents = (1 << 9),
        ActiveObjects = (1 << 10),
        FileCacheInfo = (1 << 11),
        MemoryInfo = (1 << 12),
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(Audio::DebugDraw::Options);

} // namespace Audio::DebugDraw

namespace Audio::FileCacheManagerDebugDraw
{
    enum class Options
    {
        All = 0,
        Global = (1 << 0),
        LevelSpecific = (1 << 1),
        UseCounted = (1 << 2),
        Loaded = (1 << 3),
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(Audio::FileCacheManagerDebugDraw::Options);

} // namespace Audio::FileCacheManagerDebugDraw
#endif // !AUDIO_RELEASE
