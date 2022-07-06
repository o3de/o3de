/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CryENGINE system core-handle all subsystems


#include "CrySystem_precompiled.h"
#include "System.h"
#include <time.h>
#include <AzCore/Console/Console.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Debug/Budget.h>
#include <CryPath.h>
#include <CrySystemBus.h>
#include <CryCommon/IFont.h>
#include <CryCommon/MiniQueue.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Debug/IEventLogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Logging/MissingAssetLogger.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzCore/Interface/Interface.h>

AZ_DEFINE_BUDGET(CrySystem);

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEM_CPP_SECTION_1 1
#define SYSTEM_CPP_SECTION_2 2
#define SYSTEM_CPP_SECTION_3 3
#define SYSTEM_CPP_SECTION_4 4
#define SYSTEM_CPP_SECTION_5 5
#define SYSTEM_CPP_SECTION_6 6
#define SYSTEM_CPP_SECTION_7 7
#define SYSTEM_CPP_SECTION_8 8
#define SYSTEM_CPP_SECTION_9 9
#endif

#if defined(_RELEASE) && AZ_LEGACY_CRYSYSTEM_TRAIT_USE_EXCLUDEUPDATE_ON_CONSOLE
//exclude some not needed functionality for release console builds
#define EXCLUDE_UPDATE_ON_CONSOLE
#endif

#ifdef WIN32
#include <AzCore/PlatformIncl.h>

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSystem* pSystem = 0;
    if (gEnv)
    {
        pSystem = static_cast<CSystem*>(gEnv->pSystem);
    }
    if (pSystem && !pSystem->IsQuitting())
    {
        LRESULT result = 0;
        bool bAny = false;
        for (std::vector<IWindowMessageHandler*>::const_iterator it = pSystem->m_windowMessageHandlers.begin(); it != pSystem->m_windowMessageHandlers.end(); ++it)
        {
            IWindowMessageHandler* pHandler = *it;
            LRESULT maybeResult = 0xDEADDEAD;
            if (pHandler->HandleMessage(hWnd, uMsg, wParam, lParam, &maybeResult))
            {
                assert(maybeResult != 0xDEADDEAD && "Message handler indicated a resulting value, but no value was written");
                if (bAny)
                {
                    assert(result == maybeResult && "Two window message handlers tried to return different result values");
                }
                else
                {
                    bAny = true;
                    result = maybeResult;
                }
            }
        }
        if (bAny)
        {
            // One of the registered handlers returned something
            return result;
        }
    }

    // Handle with the default procedure
    assert(IsWindowUnicode(hWnd) && "Window should be Unicode when compiling with UNICODE");
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
#endif

#if defined(LINUX) && !defined(ANDROID)
#include <execinfo.h> // for backtrace
#endif

#if defined(ANDROID)
#include <unwind.h>  // for _Unwind_Backtrace and _Unwind_GetIP
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(System_cpp)
#endif

#include <IRenderer.h>
#include <IMovieSystem.h>
#include <ILog.h>
#include <IAudioSystem.h>

#include <LoadScreenBus.h>

#include <AzFramework/Archive/Archive.h>
#include "XConsole.h"
#include "Log.h"

#include "XML/xml.h"

#include "LocalizedStringManager.h"
#include "XML/XmlUtils.h"
#include "SystemEventDispatcher.h"

#include "RemoteConsole/RemoteConsole.h"

#include <PNoise3.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Input/Buses/Requests/InputSystemRequestBus.h>

#ifdef WIN32

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

#include <process.h>
#include <malloc.h>
#endif

#include <ILevelSystem.h>

#include <AzFramework/IO/LocalFileIO.h>

// Define global cvars.
SSystemCVars g_cvars;

#include <AzCore/Module/Environment.h>
#include <AzCore/Component/ComponentApplication.h>
#include "AZCoreLogSink.h"

namespace
{
    float GetMovieFrameDeltaTime()
    {
        // Use GetRealTickDeltaTimeUs for CryMovie, because it should not be affected by pausing game time
        const AZ::TimeUs delta = AZ::GetRealTickDeltaTimeUs();
        return AZ::TimeUsToSeconds(delta);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// System Implementation.
//////////////////////////////////////////////////////////////////////////
CSystem::CSystem()
{
    CrySystemRequestBus::Handler::BusConnect();

    m_systemGlobalState = ESYSTEM_GLOBAL_STATE_UNKNOWN;
    m_iHeight = 0;
    m_iWidth = 0;
    m_iColorBits = 0;
    // CRT ALLOCATION threshold

    m_bIsAsserting = false;
    m_pSystemEventDispatcher = new CSystemEventDispatcher(); // Must be first.

    if (m_pSystemEventDispatcher)
    {
        m_pSystemEventDispatcher->RegisterListener(this);
    }

    //////////////////////////////////////////////////////////////////////////
    // Clear environment.
    //////////////////////////////////////////////////////////////////////////
    memset(&m_env, 0, sizeof(m_env));

    //////////////////////////////////////////////////////////////////////////
    // Initialize global environment interface pointers.
    m_env.pSystem = this;
    m_env.bIgnoreAllAsserts = false;
    m_env.bNoAssertDialog = false;

    //////////////////////////////////////////////////////////////////////////

    m_sysNoUpdate = NULL;
    m_pCmdLine = NULL;
    m_pLevelSystem = NULL;
    m_pLocalizationManager = NULL;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(System_cpp)
#endif

    m_pUserCallback = NULL;
    m_sys_firstlaunch = NULL;

    //  m_sys_filecache = NULL;
    m_gpu_particle_physics = NULL;

    m_bInitializedSuccessfully = false;
    m_bRelaunch = false;
    m_iLoadingMode = 0;
    m_bTestMode = false;
    m_bEditor = false;
    m_bPreviewMode = false;
    m_bIgnoreUpdates = false;
    m_bNoCrashDialog = false;
    m_bNoErrorReportWindow = false;

    m_bInDevMode = false;
    m_bGameFolderWritable = false;

    m_bPaused = false;
    m_bNoUpdate = false;
    m_iApplicationInstance = -1;


    m_pXMLUtils = new CXmlUtils(this);

    if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
    {
        m_initedOSAllocator = true;
        AZ::AllocatorInstance<AZ::OSAllocator>::Create();
    }
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        m_initedSysAllocator = true;
        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        AZ::Debug::Trace::Instance().Init();
    }

    m_eRuntimeState = ESYSTEM_EVENT_LEVEL_UNLOAD;

    m_bHasRenderedErrorMessage = false;

#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    RegisterWindowMessageHandler(this);
#endif

    m_ConfigPlatform = CONFIG_INVALID_PLATFORM;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
    ShutDown();

#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    UnregisterWindowMessageHandler(this);
#endif

    CRY_ASSERT(m_windowMessageHandlers.empty() && "There exists a dangling window message handler somewhere");

    SAFE_DELETE(m_pXMLUtils);
    SAFE_DELETE(m_pSystemEventDispatcher);

    AZCoreLogSink::Disconnect();
    if (m_initedSysAllocator)
    {
        AZ::Debug::Trace::Instance().Destroy();
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
    if (m_initedOSAllocator)
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }

    m_env.pSystem = 0;
    gEnv = 0;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
IRemoteConsole* CSystem::GetIRemoteConsole()
{
    return CRemoteConsole::GetInst();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetDevMode(bool bEnable)
{
    m_bInDevMode = bEnable;
}


///////////////////////////////////////////////////
void CSystem::ShutDown()
{
    CryLogAlways("System Shutdown");

    // don't broadcast OnCrySystemShutdown unless
    // we'd previously broadcast OnCrySystemInitialized
    if (m_bInitializedSuccessfully)
    {
        EBUS_EVENT(CrySystemEventBus, OnCrySystemShutdown, *this);
    }

    if (m_pUserCallback)
    {
        m_pUserCallback->OnShutdown();
    }

    if (GetIRemoteConsole()->IsStarted())
    {
        GetIRemoteConsole()->Stop();
    }

    if (m_sys_firstlaunch)
    {
        m_sys_firstlaunch->Set("0");
    }

    if ((m_bEditor) && (m_env.pConsole))
    {
        // restore the old saved cvars
        if (m_env.pConsole->GetCVar("r_Width"))
        {
            m_env.pConsole->GetCVar("r_Width")->Set(m_iWidth);
        }
        if (m_env.pConsole->GetCVar("r_Height"))
        {
            m_env.pConsole->GetCVar("r_Height")->Set(m_iHeight);
        }
        if (m_env.pConsole->GetCVar("r_ColorBits"))
        {
            m_env.pConsole->GetCVar("r_ColorBits")->Set(m_iColorBits);
        }
    }

    if (m_bEditor && !m_bRelaunch)
    {
        SaveConfiguration();
    }

    // Dispatch the full-shutdown event in case this is not a fast-shutdown.
    if (m_pSystemEventDispatcher != NULL)
    {
        m_pSystemEventDispatcher->OnSystemEvent(ESYSTEM_EVENT_FULL_SHUTDOWN, 0, 0);
    }

    SAFE_RELEASE(m_env.pMovieSystem);
    SAFE_RELEASE(m_env.pCryFont);
    if (m_env.pConsole)
    {
        ((CXConsole*)m_env.pConsole)->FreeRenderResources();
    }
    SAFE_RELEASE(m_pLevelSystem);

    if (m_env.pLog)
    {
        m_env.pLog->UnregisterConsoleVariables();
    }

    GetIRemoteConsole()->UnregisterConsoleVariables();

    // Release console variables.

    SAFE_RELEASE(m_sys_firstlaunch);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(System_cpp)
#endif

    SAFE_DELETE(m_pLocalizationManager);

    delete m_pCmdLine;
    m_pCmdLine = 0;

    // Audio System Shutdown!
    // Shut down audio as late as possible but before the streaming system and console get released!
    Audio::Gem::AudioSystemGemRequestBus::Broadcast(&Audio::Gem::AudioSystemGemRequestBus::Events::Release);

    // Shut down console as late as possible and after audio!
    SAFE_RELEASE(m_env.pConsole);

    // Log must be last thing released.
    if (m_env.pLog)
    {
        m_env.pLog->FlushAndClose();
    }
    SAFE_RELEASE(m_env.pLog);   // creates log backup

    ShutdownFileSystem();

    EBUS_EVENT(CrySystemEventBus, OnCrySystemPostShutdown);
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
void CSystem::Quit()
{
    CryLogAlways("CSystem::Quit invoked from thread %" PRI_THREADID " (main is %" PRI_THREADID ")", GetCurrentThreadId(), gEnv->mMainThreadId);

    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);

    // If this was set from anywhere but the main thread, bail and let the main thread handle shutdown
    if (GetCurrentThreadId() != gEnv->mMainThreadId)
    {
        return;
    }

    if (m_pUserCallback)
    {
        m_pUserCallback->OnQuit();
    }

    gEnv->pLog->FlushAndClose();

    // Latest possible place to flush any pending messages to disk before the forceful termination.
    if (auto logger = AZ::Interface<AZ::Debug::IEventLogger>::Get(); logger)
    {
        logger->Flush();
    }

#ifdef WIN32
    //Post a WM_QUIT message to the Win32 api which causes the message loop to END
    //This is not the same as handling a WM_DESTROY event which destroys a window
    //but keeps the message loop alive.
    PostQuitMessage(0);
#endif
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::IsQuitting() const
{
    bool wasExitMainLoopRequested = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(wasExitMainLoopRequested, &AzFramework::ApplicationRequests::WasExitMainLoopRequested);
    return wasExitMainLoopRequested;
}

//////////////////////////////////////////////////////////////////////////
ISystem* CSystem::GetCrySystem()
{
    return this;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfNeeded()
{
    static bool firstCall = true;

    typedef MiniQueue<CTimeValue, 32> PrevNow;
    static PrevNow prevNow;
    if (firstCall)
    {
        const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
        const double timeSec = AZ::TimeMsToSecondsDouble(timeMs);
        m_lastTickTime = CTimeValue(timeSec);
        prevNow.Push(m_lastTickTime);
        firstCall = false;
        return;
    }

    const float maxRate = m_svDedicatedMaxRate->GetFVal();
    const float minTime = 1.0f / maxRate;
    const AZ::TimeMs nowTimeMs = AZ::GetRealElapsedTimeMs();
    const double nowTimeSec = AZ::TimeMsToSecondsDouble(nowTimeMs);
    const CTimeValue now = CTimeValue(nowTimeSec);
    const float elapsed = (now - m_lastTickTime).GetSeconds();

    if (prevNow.Full())
    {
        prevNow.Pop();
    }
    prevNow.Push(now);

    static bool allowStallCatchup = true;
    if (elapsed > minTime && allowStallCatchup)
    {
        allowStallCatchup = false;
        const AZ::TimeMs lastTimeMs = AZ::GetRealElapsedTimeMs();
        const double lastTimeSec = AZ::TimeMsToSecondsDouble(lastTimeMs);
        m_lastTickTime = CTimeValue(lastTimeSec);
        return;
    }
    allowStallCatchup = true;

    float totalElapsed = (now - prevNow.Front()).GetSeconds();
    float wantSleepTime = AZStd::clamp(minTime * (prevNow.Size() - 1) - totalElapsed, 0.0f, (minTime - elapsed) * 0.9f);
    static float sleepTime = 0;
    sleepTime = (15 * sleepTime + wantSleepTime) / 16;
    int sleepMS = (int)(1000.0f * sleepTime + 0.5f);
    if (sleepMS > 0)
    {
        AZ_PROFILE_FUNCTION(CrySystem);
        Sleep(sleepMS);
    }

    const AZ::TimeMs lastTimeMs = AZ::GetRealElapsedTimeMs();
    const double lastTimeSec = AZ::TimeMsToSecondsDouble(lastTimeMs);
    m_lastTickTime = CTimeValue(lastTimeSec);
}

extern DWORD g_idDebugThreads[];
extern int g_nDebugThreads;
int prev_sys_float_exceptions = -1;

//////////////////////////////////////////////////////////////////////
bool CSystem::UpdatePreTickBus(int updateFlags, int nPauseMode)
{
    // If we detect the quit flag at the start of Update, that means it was set
    // from another thread, and we should quit immediately. Otherwise, it will
    // be set by game logic or the console during Update and we will quit later
    if (IsQuitting())
    {
        Quit();
        return false;
    }

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    // do the dedicated sleep earlier than the frame profiler to avoid having it counted
    if (gEnv->IsDedicated())
    {
        SleepIfNeeded();
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE

#ifdef WIN32
    // enable/disable SSE fp exceptions (#nan and /0)
    // need to do it each frame since sometimes they are being reset
    _mm_setcsr(_mm_getcsr() & ~0x280 | (g_cvars.sys_float_exceptions > 0 ? 0 : 0x280));
#endif //WIN32

    AZ_PROFILE_FUNCTION(CrySystem);

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (m_pUserCallback)
    {
        m_pUserCallback->OnUpdate();
    }

    //////////////////////////////////////////////////////////////////////////
    // Enable/Disable floating exceptions.
    //////////////////////////////////////////////////////////////////////////
    prev_sys_float_exceptions += 1 + g_cvars.sys_float_exceptions & prev_sys_float_exceptions >> 31;
    if (prev_sys_float_exceptions != g_cvars.sys_float_exceptions)
    {
        prev_sys_float_exceptions = g_cvars.sys_float_exceptions;

        EnableFloatExceptions(g_cvars.sys_float_exceptions);
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE
       //////////////////////////////////////////////////////////////////////////

    if (m_env.pLog)
    {
        m_env.pLog->Update();
    }

#ifdef USE_REMOTE_CONSOLE
    GetIRemoteConsole()->Update();
#endif
    if (nPauseMode != 0)
    {
        m_bPaused = true;
    }
    else
    {
        m_bPaused = false;
    }

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (m_bIgnoreUpdates)
    {
        return true;
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE

    //static bool sbPause = false;
    //bool bPause = false;
    bool bNoUpdate = false;
#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (m_sysNoUpdate && m_sysNoUpdate->GetIVal())
    {
        bNoUpdate = true;
    }

    m_bNoUpdate = bNoUpdate;
#endif //EXCLUDE_UPDATE_ON_CONSOLE

    //check if we are quitting from the game
    if (IsQuitting())
    {
        Quit();
        return (false);
    }

    //limit frame rate if vsync is turned off
    //for consoles this is done inside renderthread to be vsync dependent
    {
        static ICVar* pSysMaxFPS = NULL;
        static ICVar* pVSync = NULL;

        if (pSysMaxFPS == NULL && gEnv && gEnv->pConsole)
        {
            pSysMaxFPS = gEnv->pConsole->GetCVar("sys_MaxFPS");
        }
        if (pVSync == NULL && gEnv && gEnv->pConsole)
        {
            pVSync = gEnv->pConsole->GetCVar("r_Vsync");
        }

        if (pSysMaxFPS && pVSync)
        {
            int32 maxFPS = pSysMaxFPS->GetIVal();
            uint32 vSync = pVSync->GetIVal();

            if (maxFPS == 0 && vSync == 0)
            {
                ILevelSystem* pLvlSys = GetILevelSystem();
                const bool inLevel = pLvlSys && pLvlSys->IsLevelLoaded();
                maxFPS = !inLevel || IsPaused() ? 60 : 0;
            }

            if (maxFPS > 0 && vSync == 0)
            {
                const float safeMarginFPS = 0.5f;//save margin to not drop below 30 fps
                static AZ::TimeMs sTimeLast = AZ::GetRealElapsedTimeMs();
                const AZ::TimeMs timeFrameMax(static_cast<AZ::TimeMs>(
                    (int64)(1000.f / ((float)maxFPS + safeMarginFPS))
                    ));
                const AZ::TimeMs timeLast = timeFrameMax + sTimeLast;
                while (timeLast > AZ::GetRealElapsedTimeMs())
                {
                    CrySleep(0);
                }
                sTimeLast = AZ::GetRealElapsedTimeMs();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////
    //update console system
    if (m_env.pConsole)
    {
        m_env.pConsole->Update();
    }

    if (IsQuitting())
    {
        Quit();
        return false;
    }

    // Run movie system pre-update
    if (!bNoUpdate)
    {
        UpdateMovieSystem(updateFlags, GetMovieFrameDeltaTime(), true);
    }

    return !IsQuitting();
}

//////////////////////////////////////////////////////////////////////
bool CSystem::UpdatePostTickBus(int updateFlags, int /*nPauseMode*/)
{
    const AZ::TimeMs updateStartTimeMs = AZ::GetRealElapsedTimeMs();
    const double updateStartTimeSec = AZ::TimeMsToSecondsDouble(updateStartTimeMs);
    const CTimeValue updateStart(updateStartTimeSec);

    // Run movie system post-update
    if (!m_bNoUpdate)
    {
        UpdateMovieSystem(updateFlags, GetMovieFrameDeltaTime(), false);
    }

    //////////////////////////////////////////////////////////////////////
    // Update sound system
    if (!m_bNoUpdate)
    {
        UpdateAudioSystems();
    }

    //Now update frame statistics
    const AZ::TimeMs curTimeMs = AZ::GetRealElapsedTimeMs();
    const double curTimeSec = AZ::TimeMsToSecondsDouble(curTimeMs);
    const CTimeValue cur_time(curTimeSec);

    CTimeValue a_second(g_cvars.sys_update_profile_time);
    std::vector< std::pair<CTimeValue, float> >::iterator it = m_updateTimes.begin();
    for (std::vector< std::pair<CTimeValue, float> >::iterator eit = m_updateTimes.end(); it != eit; ++it)
    {
        if ((cur_time - it->first) < a_second)
        {
            break;
        }
    }

    {
        if (it != m_updateTimes.begin())
        {
            m_updateTimes.erase(m_updateTimes.begin(), it);
        }

        float updateTime = (cur_time - updateStart).GetMilliSeconds();
        m_updateTimes.push_back(std::make_pair(cur_time, updateTime));
    }

    m_pSystemEventDispatcher->Update();

    if (!gEnv->IsEditing() && m_eRuntimeState == ESYSTEM_EVENT_LEVEL_GAMEPLAY_START)
    {
        gEnv->pCryPak->DisableRuntimeFileAccess(true);
    }

    // Also broadcast for anyone else that needs to draw global debug to do so now
    AzFramework::DebugDisplayEventBus::Broadcast(&AzFramework::DebugDisplayEvents::DrawGlobalDebugInfo);

    return !IsQuitting();
}


bool CSystem::UpdateLoadtime()
{
    return !IsQuitting();
}

void CSystem::UpdateAudioSystems()
{
    if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get(); audioSystem != nullptr)
    {
        audioSystem->ExternalUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetUpdateStats(SSystemUpdateStats& stats)
{
    if (m_updateTimes.empty())
    {
        stats = SSystemUpdateStats();
    }
    else
    {
        stats.avgUpdateTime = 0;
        stats.maxUpdateTime = -FLT_MAX;
        stats.minUpdateTime = +FLT_MAX;
        for (std::vector< std::pair<CTimeValue, float> >::const_iterator it = m_updateTimes.begin(), eit = m_updateTimes.end(); it != eit; ++it)
        {
            const float t = it->second;
            stats.avgUpdateTime += t;
            stats.maxUpdateTime = max(stats.maxUpdateTime, t);
            stats.minUpdateTime = min(stats.minUpdateTime, t);
        }
        stats.avgUpdateTime /= m_updateTimes.size();
    }
}


//////////////////////////////////////////////////////////////////////////
void CSystem::UpdateMovieSystem(const int updateFlags, const float fFrameTime, const bool bPreUpdate)
{
    if (m_env.pMovieSystem && !(updateFlags & ESYSUPDATE_EDITOR) && g_cvars.sys_trackview)
    {
        float fMovieFrameTime = fFrameTime;

        if (fMovieFrameTime > g_cvars.sys_maxTimeStepForMovieSystem)
        {
            fMovieFrameTime = g_cvars.sys_maxTimeStepForMovieSystem;
        }

        if (bPreUpdate)
        {
            m_env.pMovieSystem->PreUpdate(fMovieFrameTime);
        }
        else
        {
            m_env.pMovieSystem->PostUpdate(fMovieFrameTime);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// XML stuff
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::CreateXmlNode(const char* sNodeName, bool bReuseStrings, bool bIsProcessingInstruction)
{
    return new CXmlNode(sNodeName, bReuseStrings, bIsProcessingInstruction);
}

//////////////////////////////////////////////////////////////////////////
IXmlUtils* CSystem::GetXmlUtils()
{
    return m_pXMLUtils;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFromFile(const char* sFilename, bool bReuseStrings)
{
    return m_pXMLUtils->LoadXmlFromFile(sFilename, bReuseStrings);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings, bool bSuppressWarnings)
{
    return m_pXMLUtils->LoadXmlFromBuffer(buffer, size, bReuseStrings, bSuppressWarnings);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::CheckLogVerbosity(int verbosity)
{
    if (verbosity <= m_env.pLog->GetVerbosityLevel())
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Warning(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    WarningV(module, severity, flags, file, format, args);
    va_end(args);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ShowMessage(const char* text, const char* caption, unsigned int uType)
{
    if (m_pUserCallback)
    {
        m_pUserCallback->ShowMessage(text, caption, uType);
    }
    else
    {
        CryMessageBox(text, caption, uType);
    }
}

inline const char* ValidatorModuleToString(EValidatorModule module)
{
    switch (module)
    {
    case VALIDATOR_MODULE_RENDERER:
        return "Renderer";
    case VALIDATOR_MODULE_3DENGINE:
        return "3DEngine";
    case VALIDATOR_MODULE_ASSETS:
        return "Assets";
    case VALIDATOR_MODULE_SYSTEM:
        return "System";
    case VALIDATOR_MODULE_AUDIO:
        return "Audio";
    case VALIDATOR_MODULE_MOVIE:
        return "Movie";
    case VALIDATOR_MODULE_EDITOR:
        return "Editor";
    case VALIDATOR_MODULE_NETWORK:
        return "Network";
    case VALIDATOR_MODULE_PHYSICS:
        return "Physics";
    case VALIDATOR_MODULE_ONLINE:
        return "Online";
    case VALIDATOR_MODULE_FEATURETESTS:
        return "FeatureTests";
    case VALIDATOR_MODULE_SHINE:
        return "UI";
    }
    return "";
}

//////////////////////////////////////////////////////////////////////////
void CSystem::WarningV(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args)
{
    // Fran: No logging in a testing environment
    if (m_env.pLog == 0)
    {
        return;
    }

    const char* sModuleFilter = m_env.pLog->GetModuleFilter();
    if (sModuleFilter && *sModuleFilter != 0)
    {
        const char* sModule = ValidatorModuleToString(module);
        if (strlen(sModule) > 1 || AZ::StringFunc::Find(sModule, sModuleFilter) == AZStd::string::npos)
        {
            // Filter out warnings from other modules.
            return;
        }
    }

    bool bDbgBreak = false;
    if (severity == VALIDATOR_ERROR_DBGBRK)
    {
        bDbgBreak = true;
        severity = VALIDATOR_ERROR; // change it to a standard VALIDATOR_ERROR for simplicity in the rest of the system
    }

    IMiniLog::ELogType ltype = ILog::eComment;
    switch (severity)
    {
    case    VALIDATOR_ERROR:
        ltype = ILog::eError;
        break;
    case    VALIDATOR_WARNING:
        ltype = ILog::eWarning;
        break;
    case    VALIDATOR_COMMENT:
        ltype = ILog::eComment;
        break;
    default:
        break;
    }

    AZStd::fixed_string<MAX_WARNING_LENGTH> fmt;
    vsnprintf_s(fmt.data(), MAX_WARNING_LENGTH, MAX_WARNING_LENGTH - 1, format, args);

    if (file && *file)
    {
        fmt += " [File=";
        fmt += file;
        fmt += "]";

    }
    m_env.pLog->LogWithType(ltype, flags | VALIDATOR_FLAG_SKIP_VALIDATOR, "%s", fmt.c_str());

    if (bDbgBreak && g_cvars.sys_error_debugbreak)
    {
        AZ::Debug::Trace::Break();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedPath(const char* sLanguage, AZStd::string& sLocalizedPath)
{
    // Omit the trailing slash!
    AZStd::string sLocalizationFolder(PathUtil::GetLocalizationFolder());
    sLocalizationFolder.pop_back();

    int locFormat = 0;
    LocalizationManagerRequestBus::BroadcastResult(locFormat, &LocalizationManagerRequestBus::Events::GetLocalizationFormat);
    if(locFormat == 1)
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + ".loc.agsxml";
    }
    else
    {
        if (AZ::StringFunc::Equal(sLocalizationFolder, "Languages", false))
        {
            sLocalizedPath = sLocalizationFolder + "/" + sLanguage + "_xml.pak";
        }
        else
        {
            sLocalizedPath = AZStd::string("Localized/") + sLanguage + "_xml.pak";
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedAudioPath(const char* sLanguage, AZStd::string& sLocalizedPath)
{
    // Omit the trailing slash!
    AZStd::string sLocalizationFolder(PathUtil::GetLocalizationFolder());
    sLocalizationFolder.pop_back();

    if (AZ::StringFunc::Equal(sLocalizationFolder, "Languages", false))
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + ".pak";
    }
    else
    {
        sLocalizedPath = AZStd::string("Localized/") + sLanguage + ".pak";
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguagePak(const char* sLanguage)
{
    AZStd::string sLocalizedPath;
    GetLocalizedPath(sLanguage, sLocalizedPath);
    m_env.pCryPak->ClosePacks({ sLocalizedPath.c_str(), sLocalizedPath.size() });
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguageAudioPak(const char* sLanguage)
{
    AZStd::string sLocalizedPath;
    GetLocalizedAudioPath(sLanguage, sLocalizedPath);
    m_env.pCryPak->ClosePacks({ sLocalizedPath.c_str(), sLocalizedPath.size() });
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Relaunch(bool bRelaunch)
{
    if (m_sys_firstlaunch)
    {
        m_sys_firstlaunch->Set("0");
    }

    m_bRelaunch = bRelaunch;
    SaveConfiguration();
}

//////////////////////////////////////////////////////////////////////////
ILocalizationManager* CSystem::GetLocalizationManager()
{
    return m_pLocalizationManager;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteCommandLine(bool deferred)
{
    if (m_executedCommandLine)
    {
        return;
    }

    m_executedCommandLine = true;

    // execute command line arguments e.g. +g_gametype ASSAULT +LoadLevel "testy"

    ICmdLine* pCmdLine = GetICmdLine();
    assert(pCmdLine);

    const int iCnt = pCmdLine->GetArgCount();
    for (int i = 0; i < iCnt; ++i)
    {
        const ICmdLineArg* pCmd = pCmdLine->GetArg(i);

        if (pCmd->GetType() == eCLAT_Post)
        {
            AZStd::string sLine = pCmd->GetName();
            {
                if (pCmd->GetValue())
                {
                    sLine += AZStd::string(" ") + pCmd->GetValue();
                }

                GetILog()->Log("Executing command from command line: \n%s\n", sLine.c_str()); // - the actual command might be executed much later (e.g. level load pause)
                GetIConsole()->ExecuteString(sLine.c_str(), false, deferred);
            }
        }
    }

    //gEnv->pConsole->ExecuteString("sys_RestoreSpec test*"); // to get useful debugging information about current spec settings to the log file
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetConfigPlatform(const ESystemConfigPlatform platform)
{
    m_ConfigPlatform = platform;
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigPlatform CSystem::GetConfigPlatform() const
{
    return m_ConfigPlatform;
}

//////////////////////////////////////////////////////////////////////////
CPNoise3* CSystem::GetNoiseGen()
{
    static CPNoise3 m_pNoiseGen;
    return &m_pNoiseGen;
}

//////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageCVarChanged(ICVar* language)
{
    if (language && language->GetType() == CVAR_STRING)
    {
        CSystem* pSys = static_cast<CSystem*>(gEnv->pSystem);
        if (pSys && pSys->GetLocalizationManager())
        {
            const char* lang = language->GetString();

            // Hook up Localization initialization
            int locFormat = 0;
            LocalizationManagerRequestBus::BroadcastResult(locFormat, &LocalizationManagerRequestBus::Events::GetLocalizationFormat);
            if (locFormat == 0)
            {
                const char* locLanguage = nullptr;
                LocalizationManagerRequestBus::BroadcastResult(locLanguage, &LocalizationManagerRequestBus::Events::GetLanguage);
            pSys->OpenLanguagePak(lang);
            }

            LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::SetLanguage, lang);
            LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::ReloadData);

            if (gEnv->pCryFont)
            {
                gEnv->pCryFont->OnLanguageChanged();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OnLocalizationFolderCVarChanged(ICVar* const pLocalizationFolder)
{
    if (pLocalizationFolder && pLocalizationFolder->GetType() == CVAR_STRING)
    {
        CSystem* const pSystem = static_cast<CSystem* const>(gEnv->pSystem);

        if (pSystem != NULL && gEnv->pCryPak != NULL)
        {
            CLocalizedStringsManager* const pLocalizationManager = static_cast<CLocalizedStringsManager* const>(pSystem->GetLocalizationManager());

            if (pLocalizationManager)
            {
                // Get what is currently loaded
                CLocalizedStringsManager::TLocalizationTagVec tagVec;
                pLocalizationManager->GetLoadedTags(tagVec);

                // Release the old localization data.
                CLocalizedStringsManager::TLocalizationTagVec::const_iterator end = tagVec.end();
                for (CLocalizedStringsManager::TLocalizationTagVec::const_iterator it = tagVec.begin(); it != end; ++it)
                {
                    pLocalizationManager->ReleaseLocalizationDataByTag(it->c_str());
                }

                // Close the paks situated in the previous localization folder.
                pSystem->CloseLanguagePak(pLocalizationManager->GetLanguage());
                pSystem->CloseLanguageAudioPak(pSystem->m_currentLanguageAudio.c_str());

                // Set the new localization folder.
                gEnv->pCryPak->SetLocalizationFolder(pLocalizationFolder->GetString());

                // Now open the paks situated in the new localization folder.
                pSystem->OpenLanguagePak(pLocalizationManager->GetLanguage());
                pSystem->OpenLanguageAudioPak(pSystem->m_currentLanguageAudio.c_str());

                // And load the new data.
                for (CLocalizedStringsManager::TLocalizationTagVec::const_iterator it = tagVec.begin(); it != end; ++it)
                {
                    pLocalizationManager->LoadLocalizationDataByTag(it->c_str());
                }
            }
        }
    }
}

// Catch changes to assert verbosity and update the global used to track it
void CSystem::SetAssertLevel(int _assertlevel)
{
    AZ::EnvironmentVariable<int> assertVerbosityLevel = AZ::Environment::FindVariable<int>("assertVerbosityLevel");
    if (assertVerbosityLevel)
    {
        assertVerbosityLevel.Set(_assertlevel);
    }
}

void CSystem::OnAssertLevelCvarChanged(ICVar* pArgs)
{
    SetAssertLevel(pArgs->GetIVal());
}

void CSystem::SetLogLevel(int _loglevel)
{
    AZ::EnvironmentVariable<int> logVerbosityLevel = AZ::Environment::FindVariable<int>("sys_LogLevel");
    if (logVerbosityLevel.IsConstructed())
    {
        logVerbosityLevel.Set(_loglevel);
    }
}

void CSystem::OnLogLevelCvarChanged(ICVar* pArgs)
{
    if (pArgs)
    {
        SetLogLevel(pArgs->GetIVal());
    }
}


void CSystem::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN:
    case ESYSTEM_EVENT_LEVEL_UNLOAD:
        gEnv->pCryPak->DisableRuntimeFileAccess(false);
    case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
        m_eRuntimeState = event;
        break;
    }
}

ESystemGlobalState CSystem::GetSystemGlobalState(void)
{
    return m_systemGlobalState;
}

const char* CSystem::GetSystemGlobalStateName(const ESystemGlobalState systemGlobalState)
{
    static const char* const s_systemGlobalStateNames[] = {
        "UNKNOWN",                  // ESYSTEM_GLOBAL_STATE_UNKNOWN,
        "INIT",                     // ESYSTEM_GLOBAL_STATE_INIT,
        "RUNNING",                  // ESYSTEM_GLOBAL_STATE_RUNNING,
        "LEVEL_LOAD_PREPARE",       // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PREPARE,
        "LEVEL_LOAD_START",         // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START,
        "LEVEL_LOAD_MATERIALS",     // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_MATERIALS,
        "LEVEL_LOAD_OBJECTS",       // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_OBJECTS,
        "LEVEL_LOAD_STATIC_WORLD",  // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_STATIC_WORLD,
        "LEVEL_LOAD_PRECACHE",      // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PRECACHE,
        "LEVEL_LOAD_TEXTURES",      // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_TEXTURES,
        "LEVEL_LOAD_END",           // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_END,
        "LEVEL_LOAD_COMPLETE"       // ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE
    };
    const size_t numElements = sizeof(s_systemGlobalStateNames) / sizeof(s_systemGlobalStateNames[0]);
    const size_t index = (size_t)systemGlobalState;
    if (index >= numElements)
    {
        return "INVALID INDEX";
    }
    return s_systemGlobalStateNames[index];
}

void CSystem::SetSystemGlobalState(const ESystemGlobalState systemGlobalState)
{
    static AZ::TimeMs s_startTime = AZ::Time::ZeroTimeMs;
    if (systemGlobalState != m_systemGlobalState)
    {
        const AZ::TimeMs endTime = AZ::GetRealElapsedTimeMs();
        [[maybe_unused]] const double numSeconds = AZ::TimeMsToSecondsDouble(endTime - s_startTime);
        CryLog("SetGlobalState %d->%d '%s'->'%s' %3.1f seconds",
            m_systemGlobalState, systemGlobalState,
            CSystem::GetSystemGlobalStateName(m_systemGlobalState), CSystem::GetSystemGlobalStateName(systemGlobalState),
            numSeconds);
        s_startTime = AZ::GetRealElapsedTimeMs();
    }
    m_systemGlobalState = systemGlobalState;

#if AZ_LOADSCREENCOMPONENT_ENABLED
    if (m_systemGlobalState == ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE)
    {
        EBUS_EVENT(LoadScreenBus, Stop);
    }
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RegisterWindowMessageHandler(IWindowMessageHandler* pHandler)
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    assert(pHandler && !stl::find(m_windowMessageHandlers, pHandler) && "This IWindowMessageHandler is already registered");
    m_windowMessageHandlers.push_back(pHandler);
#else
    CRY_ASSERT(false && "This platform does not support window message handlers");
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::UnregisterWindowMessageHandler(IWindowMessageHandler* pHandler)
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    [[maybe_unused]] bool bRemoved = stl::find_and_erase(m_windowMessageHandlers, pHandler);
    assert(pHandler && bRemoved && "This IWindowMessageHandler was not registered");
#else
    CRY_ASSERT(false && "This platform does not support window message handlers");
#endif
}

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32)
bool CSystem::HandleMessage([[maybe_unused]] HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    static bool sbInSizingModalLoop;
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    *pResult = 0;
    switch (uMsg)
    {
    // System event translation
    case WM_CLOSE:
        /*
            Trigger CSystem to call Quit() the next time
            it calls Update(). HandleMessages can get messages
            pumped to it from SyncMainWithRender which would
            be called recurively by Quit(). Doing so would
            cause the render thread to deadlock and the main
            thread to spin in SRenderThread::WaitFlushFinishedCond.
        */
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
        return false;
    case WM_MOVE:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE, x, y);
        return false;
    case WM_SIZE:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE, x, y);
        switch (wParam)
        {
        case SIZE_MINIMIZED:
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnMinimized);
            break;
        case SIZE_MAXIMIZED:
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnMaximized);
            break;
        case SIZE_RESTORED:
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnRestored);
            break;
        }
        return false;
    case WM_WINDOWPOSCHANGED:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_POS_CHANGED, 1, 0);
        return false;
    case WM_STYLECHANGED:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_STYLE_CHANGED, 1, 0);
        return false;
    case WM_ACTIVATE:
        // Pass HIWORD(wParam) as well to indicate whether this window is minimized or not
        // HIWORD(wParam) != 0 is minimized, HIWORD(wParam) == 0 is not minimized
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_ACTIVATE, LOWORD(wParam) != WA_INACTIVE, HIWORD(wParam));
        return true;
    case WM_SETFOCUS:
        EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnSetFocus);
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 1, 0);
        return false;
    case WM_KILLFOCUS:
        EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnKillFocus);
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 0, 0);
        return false;
    case WM_INPUTLANGCHANGE:
        GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LANGUAGE_CHANGE, wParam, lParam);
        return false;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_SCREENSAVE)
        {
            // Check if screen saver is allowed
            IConsole* const pConsole = gEnv->pConsole;
            const ICVar* const pVar = pConsole ? pConsole->GetCVar("sys_screensaver_allowed") : 0;
            return pVar && pVar->GetIVal() == 0;
        }
        return false;

    // Mouse activation
    case WM_MOUSEACTIVATE:
        *pResult = MA_ACTIVATEANDEAT;
        return true;

    // Hardware mouse counters
    case WM_ENTERSIZEMOVE:
        sbInSizingModalLoop = true;
    // Fall through intended
    case WM_ENTERMENULOOP:
    {
        return true;
    }
    case WM_CAPTURECHANGED:
    // If WM_CAPTURECHANGED is received after WM_ENTERSIZEMOVE (ie, moving/resizing begins).
    // but no matching WM_EXITSIZEMOVE is received (this can happen if the window is not actually moved).
    // we still need to decrement the hardware mouse counter that was incremented when WM_ENTERSIZEMOVE was seen.
    // So in this case, we effectively treat WM_CAPTURECHANGED as if it was the WM_EXITSIZEMOVE message.
    // This behavior has only been reproduced the window is deactivated during the modal loop (ie, breakpoint triggered and focus moves to VS).
    case WM_EXITSIZEMOVE:
        if (!sbInSizingModalLoop)
        {
            return false;
        }
        sbInSizingModalLoop = false;
    // Fall through intended
    case WM_EXITMENULOOP:
    {
        return (uMsg != WM_CAPTURECHANGED);
    }
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    {
        const bool bAlt = (lParam & (1 << 29)) != 0;
        if (bAlt && wParam == VK_F4)
        {
            return false;     // Pass though ALT+F4
        }
        // Prevent game from entering menu loop!  Editor does allow menu loop.
        return !m_bEditor;
    }
    case WM_INPUT:
    {
        UINT rawInputSize;
        const UINT rawInputHeaderSize = sizeof(RAWINPUTHEADER);
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &rawInputSize, rawInputHeaderSize);

        AZStd::array<BYTE, sizeof(RAWINPUT)> rawInputBytesArray;
        LPBYTE rawInputBytes = rawInputBytesArray.data();

        [[maybe_unused]] const UINT bytesCopied = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);
        CRY_ASSERT(bytesCopied == rawInputSize);

        [[maybe_unused]] RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
        CRY_ASSERT(rawInput);

        AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputEvent, *rawInput);

        return false;
    }
    case WM_DEVICECHANGE:
    {
        if (wParam == 0x0007) // DBT_DEVNODES_CHANGED
        {
            AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputDeviceChangeEvent);
        }
        return true;
    }
    case WM_CHAR:
    {
        const unsigned short codeUnitUTF16 = static_cast<unsigned short>(wParam);
        AzFramework::RawInputNotificationBusWindows::Broadcast(&AzFramework::RawInputNotificationsWindows::OnRawInputCodeUnitUTF16Event, codeUnitUTF16);
        return true;
    }

    // Any other event doesn't interest us
    default:
        return false;
    }
}

#endif

ILevelSystem* CSystem::GetILevelSystem()
{
    return m_pLevelSystem;
}

#undef EXCLUDE_UPDATE_ON_CONSOLE

