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
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_AudioLoggingOptions);
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_DrawAudioDebug);
    inline Audio::Flags<AZ::u32> s_debugDrawOptions;
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_FileCacheManagerDebugFilter);
    inline Audio::Flags<AZ::u32> s_fcmDrawOptions;
#endif // !AUDIO_RELEASE

} // namespace Audio::CVars
