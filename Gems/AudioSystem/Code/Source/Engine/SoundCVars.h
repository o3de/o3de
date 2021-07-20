/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioSystem.h>
#include <AzCore/Console/IConsole.h>

struct IConsoleCmdArgs;

namespace Audio::CVars
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
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

#if !defined(AUDIO_RELEASE)
    AZ_CVAR_EXTERNED(bool, s_IgnoreWindowFocus);
    AZ_CVAR_EXTERNED(bool, s_ShowActiveAudioObjectsOnly);
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_AudioTriggersDebugFilter);
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, s_AudioObjectsDebugFilter);
#endif // !AUDIO_RELEASE

} // namespace Audio::CVars

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CSoundCVars
    {
    public:
        CSoundCVars();
        ~CSoundCVars() = default;

        CSoundCVars(const CSoundCVars&) = delete;           // Copy protection
        CSoundCVars& operator=(const CSoundCVars&) = delete; // Copy protection

        void RegisterVariables();
        void UnregisterVariables();

    #if !defined(AUDIO_RELEASE)
        int m_nDrawAudioDebug;
        int m_nFileCacheManagerDebugFilter;
        int m_nAudioLoggingOptions;

    private:
        static void CmdExecuteTrigger(IConsoleCmdArgs* pCmdArgs);
        static void CmdStopTrigger(IConsoleCmdArgs* pCmdArgs);
        static void CmdSetRtpc(IConsoleCmdArgs* pCmdArgs);
        static void CmdSetSwitchState(IConsoleCmdArgs* pCmdArgs);
        static void CmdLoadPreload(IConsoleCmdArgs* pCmdArgs);
        static void CmdUnloadPreload(IConsoleCmdArgs* pCmdArgs);
        static void CmdPlayFile(IConsoleCmdArgs* pCmdArgs);
        static void CmdMicrophone(IConsoleCmdArgs* pCmdArgs);
        static void CmdPlayExternalSource(IConsoleCmdArgs* pCmdArgs);
        static void CmdSetPanningMode(IConsoleCmdArgs* pCmdArgs);
    #endif // !AUDIO_RELEASE
    };

    extern CSoundCVars g_audioCVars;
} // namespace Audio
