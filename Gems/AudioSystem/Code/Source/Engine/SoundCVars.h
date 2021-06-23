/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioSystem.h>
#include <AzCore/Console/IConsole.h>

struct IConsoleCmdArgs;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AZ CVars (new)
    AZ_CVAR_EXTERNED(bool, s_EnableRaycasts);
    AZ_CVAR_EXTERNED(float, s_RaycastMinDistance);
    AZ_CVAR_EXTERNED(float, s_RaycastMaxDistance);
    AZ_CVAR_EXTERNED(float, s_RaycastCacheTimeMs);
    AZ_CVAR_EXTERNED(float, s_RaycastSmoothFactor);

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CSoundCVars
    {
    public:
        CSoundCVars();
        ~CSoundCVars();

        CSoundCVars(const CSoundCVars&) = delete;           // Copy protection
        CSoundCVars& operator=(const CSoundCVars&) = delete; // Copy protection

        void RegisterVariables();
        void UnregisterVariables();

        int m_nATLPoolSize;
        int m_nFileCacheManagerSize;
        int m_nAudioObjectPoolSize;
        int m_nAudioEventPoolSize;
        int m_nAudioProxiesInitType;


        float m_fPositionUpdateThreshold;
        float m_fVelocityTrackingThreshold;

        float m_audioListenerTranslationZOffset;
        float m_audioListenerTranslationPercentage;

    #if !defined(AUDIO_RELEASE)
        int m_nIgnoreWindowFocus;
        int m_nDrawAudioDebug;
        int m_nFileCacheManagerDebugFilter;
        int m_nAudioLoggingOptions;
        int m_nShowActiveAudioObjectsOnly;
        ICVar* m_pAudioTriggersDebugFilter;
        ICVar* m_pAudioObjectsDebugFilter;

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
