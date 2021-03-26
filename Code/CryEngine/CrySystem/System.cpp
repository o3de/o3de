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

// Description : CryENGINE system core-handle all subsystems


#include "CrySystem_precompiled.h"
#include "System.h"
#include <time.h>
#include <AzCore/Console/Console.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/SystemFile.h>
#include "CryLibrary.h"
#include <CryPath.h>
#include <CrySystemBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Debug/IEventLogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Logging/MissingAssetLogger.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/API/AtomActiveInterface.h>
#include <AzCore/Interface/Interface.h>


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
#define WIN32_LEAN_AND_MEAN
// If app hasn't chosen, set to work with Windows 98, Windows Me, Windows 2000, Windows XP and beyond
#include <windows.h>

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSystem* pSystem = 0;
    if (gEnv)
    {
        pSystem = static_cast<CSystem*>(gEnv->pSystem);
    }
    if (pSystem && !pSystem->IsQuitting())
    {
        LRESULT result;
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
#if defined(UNICODE) || defined(_UNICODE)
    assert(IsWindowUnicode(hWnd) && "Window should be Unicode when compiling with UNICODE");
#else
    if (!IsWindowUnicode(hWnd))
    {
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
#endif
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


#include <I3DEngine.h>
#include <IRenderer.h>
#include <IMovieSystem.h>
#include <ServiceNetwork.h>
#include <ILog.h>
#include <IAudioSystem.h>
#include <IProcess.h>
#include <INotificationNetwork.h>
#include <ISoftCodeMgr.h>
#include "VisRegTest.h"
#include <LyShine/ILyShine.h>
#include <ITimeOfDay.h>

#include <LoadScreenBus.h>

#include <AzFramework/Archive/Archive.h>
#include "XConsole.h"
#include "Log.h"
#include "CrySizerStats.h"
#include "CrySizerImpl.h"
#include "NotificationNetwork.h"
#include "ProfileLog.h"

#include "XML/xml.h"
#include "XML/ReadWriteXMLSink.h"

#include "StreamEngine/StreamEngine.h"
#include "PhysRenderer.h"

#include "LocalizedStringManager.h"
#include "XML/XmlUtils.h"
#include "Serialization/ArchiveHost.h"
#include "SystemEventDispatcher.h"
#include "ServerThrottle.h"
#include "ILocalMemoryUsage.h"
#include "ResourceManager.h"
#include "HMDBus.h"
#include "OverloadSceneManager/OverloadSceneManager.h"
#include <IThreadManager.h>

#include "IZLibCompressor.h"
#include "IZlibDecompressor.h"
#include "ILZ4Decompressor.h"
#include "IZStdDecompressor.h"
#include "zlib.h"
#include "RemoteConsole/RemoteConsole.h"

#include <PNoise3.h>
#include <StringUtils.h>
#include "CryWaterMark.h"
WATERMARKDATA(_m);

#include "ImageHandler.h"
#include <LyShine/Bus/UiCursorBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Input/Buses/Requests/InputSystemRequestBus.h>

#ifdef WIN32

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>

// To enable profiling with vtune (https://software.intel.com/en-us/intel-vtune-amplifier-xe), make sure the line below is not commented out
//#define  PROFILE_WITH_VTUNE

#include <process.h>
#include <malloc.h>
#endif

#if USE_STEAM
#include "Steamworks/public/steam/steam_api.h"
#endif

#include <ILevelSystem.h>

#include <CrtDebugStats.h>
#include <AzFramework/IO/LocalFileIO.h>

// profilers api.
VTuneFunction VTResume = NULL;
VTuneFunction VTPause = NULL;

// Define global cvars.
SSystemCVars g_cvars;

#include "ITextModeConsole.h"

extern int CryMemoryGetAllocatedSize();

// these heaps are used by underlying System structures
// to allocate, accordingly, small (like elements of std::set<..*>) and big (like memory for reading files) objects
// hopefully someday we'll have standard MT-safe heap
//CMTSafeHeap g_pakHeap;
CMTSafeHeap* g_pPakHeap = 0;// = &g_pakHeap;

//////////////////////////////////////////////////////////////////////////
#include "Validator.h"

#include <IViewSystem.h>

#include <AzCore/Module/Environment.h>
#include <AzCore/Component/ComponentApplication.h>
#include "AZCoreLogSink.h"

#if defined(ANDROID)
namespace
{
    struct Callstack
    {
        Callstack()
            : addrs(NULL)
            , ignore(0)
            , count(0)
        {
        }
        Callstack(void** addrs, size_t ignore, size_t count)
        {
            this->addrs = addrs;
            this->ignore = ignore;
            this->count = count;
        }
        void** addrs;
        size_t ignore;
        size_t count;
    };

    static _Unwind_Reason_Code trace_func(struct _Unwind_Context* context, void* arg)
    {
        Callstack* cs = static_cast<Callstack*>(arg);
        if (cs->count)
        {
            void* ip = (void*) _Unwind_GetIP(context);
            if (ip)
            {
                if (cs->ignore)
                {
                    cs->ignore--;
                }
                else
                {
                    cs->addrs[0] = ip;
                    cs->addrs++;
                    cs->count--;
                }
            }
        }
        return _URC_NO_REASON;
    }

    static int Backtrace(void** addrs, size_t ignore, size_t size)
    {
        Callstack cs(addrs, ignore, size);
        _Unwind_Backtrace(trace_func, (void*) &cs);
        return size - cs.count;
    }
}
#endif

#if defined(CVARS_WHITELIST)
struct SCVarsWhitelistConfigSink
    : public ILoadConfigurationEntrySink
{
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup)
    {
        ICVarsWhitelist* pCVarsWhitelist = gEnv->pSystem->GetCVarsWhiteList();
        bool whitelisted = (pCVarsWhitelist) ? pCVarsWhitelist->IsWhiteListed(szKey, false) : true;
        if (whitelisted)
        {
            gEnv->pConsole->LoadConfigVar(szKey, szValue);
        }
    }
} g_CVarsWhitelistConfigSink;
#endif // defined(CVARS_WHITELIST)

/////////////////////////////////////////////////////////////////////////////////
// System Implementation.
//////////////////////////////////////////////////////////////////////////
CSystem::CSystem(SharedEnvironmentInstance* pSharedEnvironment)
    : m_imageHandler(std::make_unique<ImageHandler>())
{
    CrySystemRequestBus::Handler::BusConnect();

    if (!pSharedEnvironment)
    {
        CryFatalError("No shared environment instance provided. "
            "Cross-module sharing of EBuses and allocators "
            "is not possible.");
    }

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

#ifdef WIN32
    m_hInst = NULL;
    m_hWnd = NULL;
#endif

    //////////////////////////////////////////////////////////////////////////
    // Clear environment.
    //////////////////////////////////////////////////////////////////////////
    memset(&m_env, 0, sizeof(m_env));

    //////////////////////////////////////////////////////////////////////////
    // Initialize global environment interface pointers.
    m_env.pSystem = this;
    m_env.pTimer = &m_Time;
    m_env.pNameTable = &m_nameTable;
    m_env.bServer = false;
    m_env.bMultiplayer = false;
    m_env.bHostMigrating = false;
    m_env.bIgnoreAllAsserts = false;
    m_env.bNoAssertDialog = false;
    m_env.bTesting = false;

    m_env.pSharedEnvironment = pSharedEnvironment;

    m_env.SetFMVIsPlaying(false);
    m_env.SetCutsceneIsPlaying(false);

    m_env.szDebugStatus[0] = '\0';

#if !defined(CONSOLE)
    m_env.SetIsClient(false);
#endif
    //////////////////////////////////////////////////////////////////////////

    m_pStreamEngine = NULL;
    m_PhysThread = 0;

    m_pIFont = NULL;
    m_pIFontUi = NULL;
    m_pVisRegTest = NULL;
    m_rWidth = NULL;
    m_rHeight = NULL;
    m_rWidthAndHeightAsFractionOfScreenSize = NULL;
    m_rMaxWidth = NULL;
    m_rMaxHeight = NULL;
    m_rColorBits = NULL;
    m_rDepthBits = NULL;
    m_cvSSInfo = NULL;
    m_rStencilBits = NULL;
    m_rFullscreen = NULL;
    m_rDriver = NULL;
    m_sysNoUpdate = NULL;
    m_pMemoryManager = NULL;
    m_pProcess = NULL;

    m_pValidator = NULL;
    m_pCmdLine = NULL;
    m_pDefaultValidator = NULL;
    m_pLevelSystem = NULL;
    m_pViewSystem = NULL;
    m_pIZLibCompressor = NULL;
    m_pIZLibDecompressor = NULL;
    m_pILZ4Decompressor = NULL;
    m_pIZStdDecompressor = nullptr;
    m_pLocalizationManager = NULL;
    m_sys_physics_CPU = 0;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(System_cpp)
#endif
    m_sys_min_step = 0;
    m_sys_max_step = 0;

    m_pNotificationNetwork = NULL;

    m_cvAIUpdate = NULL;

    m_pUserCallback = NULL;
#if defined(CVARS_WHITELIST)
    m_pCVarsWhitelist = NULL;
    m_pCVarsWhitelistConfigSink = &g_CVarsWhitelistConfigSink;
#endif // defined(CVARS_WHITELIST)
    m_sys_memory_debug = NULL;
    m_sysWarnings = NULL;
    m_sysKeyboard = NULL;
    m_sys_GraphicsQuality = NULL;
    m_sys_firstlaunch = NULL;
    m_sys_enable_budgetmonitoring = NULL;
    m_sys_preload = NULL;

    //  m_sys_filecache = NULL;
    m_gpu_particle_physics = NULL;
    m_pCpu = NULL;

    m_bInitializedSuccessfully = false;
    m_bShaderCacheGenMode = false;
    m_bRelaunch = false;
    m_iLoadingMode = 0;
    m_bTestMode = false;
    m_bEditor = false;
    m_bPreviewMode = false;
    m_bIgnoreUpdates = false;
    m_bNoCrashDialog = false;
    m_bNoErrorReportWindow = false;

#ifndef _RELEASE
    m_checkpointLoadCount = 0;
    m_loadOrigin = eLLO_Unknown;
    m_hasJustResumed = false;
    m_expectingMapCommand = false;
#endif

    // no mem stats at the moment
    m_pMemStats = NULL;
    m_pSizer = NULL;
    m_pCVarQuit = NULL;

    m_bForceNonDevMode = false;
    m_bWasInDevMode = false;
    m_bInDevMode = false;
    m_bGameFolderWritable = false;

    m_bDrawConsole = true;
    m_bDrawUI = true;

    m_nServerConfigSpec = CONFIG_VERYHIGH_SPEC;
    m_nMaxConfigSpec = CONFIG_VERYHIGH_SPEC;

    //m_hPhysicsThread = INVALID_HANDLE_VALUE;
    //m_hPhysicsActive = INVALID_HANDLE_VALUE;
    //m_bStopPhysics = 0;
    //m_bPhysicsActive = 0;

    m_pProgressListener = 0;

    m_bPaused = false;
    m_bNoUpdate = false;
    m_nUpdateCounter = 0;
    m_iApplicationInstance = -1;


    m_pXMLUtils = new CXmlUtils(this);
    m_pArchiveHost = Serialization::CreateArchiveHost();
    m_pMemoryManager = CryGetIMemoryManager();
    m_pThreadTaskManager = new CThreadTaskManager;
    m_pResourceManager = new CResourceManager;
    m_pTextModeConsole = NULL;

    InitThreadSystem();

    m_pMiniGUI = NULL;
    m_pPerfHUD = NULL;

    g_pPakHeap = new CMTSafeHeap;

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

    m_UpdateTimesIdx = 0U;
    m_bNeedDoWorkDuringOcclusionChecks = false;

    m_eRuntimeState = ESYSTEM_EVENT_LEVEL_UNLOAD;

    m_bHasRenderedErrorMessage = false;
    m_bIsSteamInitialized = false;

    m_pDataProbe = nullptr;
#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    RegisterWindowMessageHandler(this);
#endif

    m_ConfigPlatform = CONFIG_INVALID_PLATFORM;

    AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CSystem::~CSystem()
{
    AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
    ShutDown();

#if AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER
    UnregisterWindowMessageHandler(this);
#endif

    CRY_ASSERT(m_windowMessageHandlers.empty() && "There exists a dangling window message handler somewhere");

    SAFE_DELETE(m_pVisRegTest);
    SAFE_DELETE(m_pXMLUtils);
    SAFE_DELETE(m_pArchiveHost);
    SAFE_DELETE(m_pThreadTaskManager);
    SAFE_DELETE(m_pResourceManager);
    SAFE_DELETE(m_pSystemEventDispatcher);
    //  SAFE_DELETE(m_pMemoryManager);

    if (gEnv && gEnv->pThreadManager)
    {
        gEnv->pThreadManager->UnRegisterThirdPartyThread("Main");
    }
    ShutDownThreadSystem();

    SAFE_DELETE(g_pPakHeap);

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

    AZ::Environment::Detach();

    m_env.pSystem = 0;
    gEnv = 0;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Release()
{
    //Disconnect the render bus
    AZ::RenderNotificationsBus::Handler::BusDisconnect();

    delete this;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::FreeLib(AZStd::unique_ptr<AZ::DynamicModuleHandle>& hLibModule)
{
    if (hLibModule)
    {
        if (hLibModule->IsLoaded())
        {
            hLibModule->Unload();
        }
        hLibModule.release();
    }
}

//////////////////////////////////////////////////////////////////////////
IStreamEngine* CSystem::GetStreamEngine()
{
    return m_pStreamEngine;
}

//////////////////////////////////////////////////////////////////////////
IRemoteConsole* CSystem::GetIRemoteConsole()
{
    return CRemoteConsole::GetInst();
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetForceNonDevMode(const bool bValue)
{
    m_bForceNonDevMode = bValue;
    if (bValue)
    {
        SetDevMode(false);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::GetForceNonDevMode() const
{
    return m_bForceNonDevMode;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetDevMode(bool bEnable)
{
    if (bEnable)
    {
        m_bWasInDevMode = true;
    }
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

    // clean up properly the console
    if (m_pTextModeConsole)
    {
        m_pTextModeConsole->OnShutdown();
    }

    SAFE_DELETE(m_pTextModeConsole);

    KillPhysicsThread();

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

    // Shutdown any running VR devices.
    EBUS_EVENT(AZ::VR::HMDInitRequestBus, Shutdown);


    //////////////////////////////////////////////////////////////////////////
    // Clear 3D Engine resources.
    if (m_env.p3DEngine)
    {
        m_env.p3DEngine->UnloadLevel();
    }
    //////////////////////////////////////////////////////////////////////////

    // Shutdown resource manager.
    m_pResourceManager->Shutdown();

    if (gEnv && gEnv->pLyShine)
    {
        gEnv->pLyShine->Release();
        gEnv->pLyShine = nullptr;
    }

    SAFE_DELETE(m_env.pResourceCompilerHelper);

    SAFE_RELEASE(m_env.pMovieSystem);
    SAFE_DELETE(m_env.pServiceNetwork);
    SAFE_RELEASE(m_env.pLyShine);
    SAFE_RELEASE(m_env.pCryFont);
    SAFE_RELEASE(m_env.p3DEngine); // depends on EntitySystem
    if (m_env.pConsole)
    {
        ((CXConsole*)m_env.pConsole)->FreeRenderResources();
    }
    SAFE_RELEASE(m_pIZLibCompressor);
    SAFE_RELEASE(m_pIZLibDecompressor);
    SAFE_RELEASE(m_pILZ4Decompressor);
    SAFE_RELEASE(m_pIZStdDecompressor);
    SAFE_RELEASE(m_pViewSystem);
    SAFE_RELEASE(m_pLevelSystem);

    //Can't kill renderer before we delete CryFont, 3DEngine, etc
    if (GetIRenderer())
    {
        GetIRenderer()->ShutDown();
        SAFE_RELEASE(m_env.pRenderer);
    }

    if (m_env.pLog)
    {
        m_env.pLog->UnregisterConsoleVariables();
    }

    GetIRemoteConsole()->UnregisterConsoleVariables();

    // Release console variables.

    SAFE_RELEASE(m_pCVarQuit);
    SAFE_RELEASE(m_rWidth);
    SAFE_RELEASE(m_rHeight);
    SAFE_RELEASE(m_rWidthAndHeightAsFractionOfScreenSize);
    SAFE_RELEASE(m_rMaxWidth);
    SAFE_RELEASE(m_rMaxHeight);
    SAFE_RELEASE(m_rColorBits);
    SAFE_RELEASE(m_rDepthBits);
    SAFE_RELEASE(m_cvSSInfo);
    SAFE_RELEASE(m_rStencilBits);
    SAFE_RELEASE(m_rFullscreen);
    SAFE_RELEASE(m_rDriver);

    SAFE_RELEASE(m_sysWarnings);
    SAFE_RELEASE(m_sysKeyboard);
    SAFE_RELEASE(m_sys_GraphicsQuality);
    SAFE_RELEASE(m_sys_firstlaunch);
    SAFE_RELEASE(m_sys_enable_budgetmonitoring);
    SAFE_RELEASE(m_sys_physics_CPU);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(System_cpp)
#endif

    SAFE_RELEASE(m_sys_min_step);
    SAFE_RELEASE(m_sys_max_step);

    SAFE_RELEASE(m_pNotificationNetwork);

    SAFE_DELETE(m_env.pSoftCodeMgr);
    SAFE_DELETE(m_pMemStats);
    SAFE_DELETE(m_pSizer);
    SAFE_DELETE(m_pDefaultValidator);
    m_pValidator = nullptr;

    SAFE_DELETE(m_env.pOverloadSceneManager);

    SAFE_DELETE(m_pLocalizationManager);

    //DebugStats(false, false);//true);
    //CryLogAlways("");
    //CryLogAlways("release mode memory manager stats:");
    //DumpMMStats(true);

    SAFE_DELETE(m_pCpu);

    delete m_pCmdLine;
    m_pCmdLine = 0;

    // Audio System Shutdown!
    // Shut down audio as late as possible but before the streaming system and console get released!
    Audio::Gem::AudioSystemGemRequestBus::Broadcast(&Audio::Gem::AudioSystemGemRequestBus::Events::Release);

    // Shut down the streaming system and console as late as possible and after audio!
    SAFE_DELETE(m_pStreamEngine);
    SAFE_RELEASE(m_env.pConsole);

    // Log must be last thing released.
    SAFE_RELEASE(m_env.pProfileLogSystem);
    if (m_env.pLog)
    {
        m_env.pLog->FlushAndClose();
    }
    SAFE_RELEASE(m_env.pLog);   // creates log backup

    ShutdownFileSystem();

#if defined(MAP_LOADING_SLICING)
    delete gEnv->pSystemScheduler;
#endif // defined(MAP_LOADING_SLICING)

    ShutdownModuleLibraries();

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

    if (GetIRenderer())
    {
        GetIRenderer()->RestoreGamma();
    }

    gEnv->pLog->FlushAndClose();

    // Latest possible place to flush any pending messages to disk before the forceful termination.
    if (auto logger = AZ::Interface<AZ::Debug::IEventLogger>::Get(); logger)
    {
        logger->Flush();
    }

    /*
    * TODO: This call to _exit, _Exit, TerminateProcess etc. needs to
    * eventually be removed. This causes an extremely early exit before we
    * actually perform cleanup. When this gets called most managers are
    * simply never deleted and we leave it to the OS to clean up our mess
    * which is just really bad practice. However there are LOTS of issues
    * with shutdown at the moment. Removing this will simply cause
    * a crash when either the Editor or Launcher initiate shutdown. Both
    * applications crash differently too. Bugs will be logged about those
    * issues.
    */
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(System_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32) || defined(WIN64)
    TerminateProcess(GetCurrentProcess(), m_env.retCode);
#else
    exit(m_env.retCode);
#endif

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
void CSystem::SetIProcess(IProcess* process)
{
    m_pProcess = process;
    //if (m_pProcess)
    //m_pProcess->SetPMessage("");
}

//////////////////////////////////////////////////////////////////////////
ISystem* CSystem::GetCrySystem()
{
    return this;
}

//////////////////////////////////////////////////////////////////////////
// Physics thread task
//////////////////////////////////////////////////////////////////////////
class CPhysicsThreadTask
    : public IThreadTask
{
public:

    CPhysicsThreadTask()
    {
        m_bStopRequested = 0;
        m_bIsActive = 0;
        m_stepRequested = 0;
        m_bProcessing = 0;
        m_doZeroStep = 0;
        m_lastStepTimeTaken = 0U;
        m_lastWaitTimeTaken = 0U;
    }

    //////////////////////////////////////////////////////////////////////////
    // IThreadTask implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnUpdate()
    {
        Run();
        // At the end.. delete the task
        delete this;
    }
    virtual void Stop()
    {
        Cancel();
    }
    virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
    //////////////////////////////////////////////////////////////////////////

    virtual void Run()
    {
        m_bStopRequested = 0;
        m_bIsActive = 1;

        float step, timeTaken, kSlowdown = 1.0f;
        int nSlowFrames = 0;
        int64 timeStart;
#ifdef ENABLE_LW_PROFILERS
        LARGE_INTEGER stepStart, stepEnd;
#endif
        LARGE_INTEGER waitStart, waitEnd;
        uint64 yieldBegin = 0U;
        MarkThisThreadForDebugging("Physics");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_5
#include AZ_RESTRICTED_FILE(System_cpp)
#endif
        while (true)
        {
            QueryPerformanceCounter(&waitStart);
            m_FrameEvent.Wait(); // Wait untill new frame
            QueryPerformanceCounter(&waitEnd);
            m_lastWaitTimeTaken = waitEnd.QuadPart - waitStart.QuadPart;

            if (m_bStopRequested)
            {
                UnmarkThisThreadFromDebugging();
                return;
            }
            bool stepped = false;
#ifdef ENABLE_LW_PROFILERS
            QueryPerformanceCounter(&stepStart);
#endif
            while ((step = m_stepRequested) > 0 || m_doZeroStep)
            {
                stepped = true;
                m_stepRequested = 0;
                m_bProcessing = 1;
                m_doZeroStep = 0;

                if (kSlowdown != 1.0f)
                {
                    step = max(1, FtoI(step * kSlowdown * 50 - 0.5f)) * 0.02f;
                }
                timeStart = CryGetTicks();
                timeTaken = gEnv->pTimer->TicksToSeconds(CryGetTicks() - timeStart);
                if (timeTaken > step * 0.9f)
                {
                    if (++nSlowFrames > 5)
                    {
                        kSlowdown = step * 0.9f / timeTaken;
                    }
                }
                else
                {
                    kSlowdown = 1.0f, nSlowFrames = 0;
                }
                m_bProcessing = 0;
                //int timeSleep = (int)((m_timeTarget-gEnv->pTimer->GetAsyncTime()).GetMilliSeconds()*0.9f);
                //Sleep(max(0,timeSleep));
            }
            if (!stepped)
            {
                Sleep(0);
            }
            m_FrameDone.Set();
#ifdef ENABLE_LW_PROFILERS
            QueryPerformanceCounter(&stepEnd);
            m_lastStepTimeTaken = stepEnd.QuadPart - stepStart.QuadPart;
#endif
        }
    }
    virtual void Cancel()
    {
        Pause();
        m_bStopRequested = 1;
        m_FrameEvent.Set();
        m_bIsActive = 0;
    }

    int Pause()
    {
        if (m_bIsActive)
        {
            AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::System);
            m_bIsActive = 0;
            while (m_bProcessing)
            {
                ;
            }
            return 1;
        }
        return 0;
    }
    int Resume()
    {
        if (!m_bIsActive)
        {
            m_bIsActive = 1;
            return 1;
        }
        return 0;
    }
    int IsActive() { return m_bIsActive; }
    int RequestStep(float dt)
    {
        if (m_bIsActive && dt > FLT_EPSILON)
        {
            m_stepRequested += dt;
            if (dt <= 0.0f)
            {
                m_doZeroStep = 1;
            }
            m_FrameEvent.Set();
        }

        return m_bProcessing;
    }
    float GetRequestedStep() { return m_stepRequested; }

    uint64 LastStepTaken() const
    {
        return m_lastStepTimeTaken;
    }

    uint64 LastWaitTime() const
    {
        return m_lastWaitTimeTaken;
    }

    void EnsureStepDone()
    {
        FRAME_PROFILER("SysUpdate:PhysicsEnsureDone", gEnv->pSystem, PROFILE_SYSTEM);
        if (m_bIsActive)
        {
            while (m_stepRequested > 0.0f || m_bProcessing)
            {
                m_FrameDone.Wait();
            }
        }
    }

protected:

    volatile int m_bStopRequested;
    volatile int m_bIsActive;
    volatile float m_stepRequested;
    volatile int m_bProcessing;
    volatile int m_doZeroStep;
    volatile uint64 m_lastStepTimeTaken;
    volatile uint64 m_lastWaitTimeTaken;

    CryEvent m_FrameEvent;
    CryEvent m_FrameDone;

    SThreadTaskInfo m_TaskInfo;
};

void CSystem::CreatePhysicsThread()
{
    if (!m_PhysThread)
    {
        //////////////////////////////////////////////////////////////////////////
        SThreadTaskParams threadParams;
        threadParams.name = "Physics";
        threadParams.nFlags = THREAD_TASK_BLOCKING;
        threadParams.nStackSizeKB = PHYSICS_STACK_SIZE >> 10;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_6
#include AZ_RESTRICTED_FILE(System_cpp)
#endif

        {
            m_PhysThread = new CPhysicsThreadTask;
            GetIThreadTaskManager()->RegisterTask(m_PhysThread, threadParams);
        }
    }

}

void CSystem::KillPhysicsThread()
{
    if (m_PhysThread)
    {
        GetIThreadTaskManager()->UnregisterTask(m_PhysThread);
        m_PhysThread = 0;
    }
}

///////////////////////////////////////////////////////////////////////////
// AzFramework::Terrain::TerrainDataNotificationBus START
void CSystem::OnTerrainDataCreateBegin()
{
    KillPhysicsThread();
}

void CSystem::OnTerrainDataDestroyBegin()
{
    OnTerrainDataCreateBegin();
}

// AzFramework::Terrain::TerrainDataNotificationBus END
///////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
int CSystem::SetThreadState(ESubsystem subsys, bool bActive)
{
    switch (subsys)
    {
    case ESubsys_Physics:
    {
        if (m_PhysThread)
        {
            return bActive ? ((CPhysicsThreadTask*)m_PhysThread)->Resume() : ((CPhysicsThreadTask*)m_PhysThread)->Pause();
        }
    }
    break;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfInactive()
{
    // ProcessSleep()
    if (m_bDedicatedServer || m_bEditor || gEnv->bMultiplayer)
    {
        return;
    }

#if defined(WIN32)
    if (!AZ::Interface<AzFramework::AtomActiveInterface>::Get())
    {
        WIN_HWND hRendWnd = GetIRenderer()->GetHWND();
        if (!hRendWnd)
        {
            return;
        }

        AZ_TRACE_METHOD();
        // Loop here waiting for window to be activated.
        for (int nLoops = 0; nLoops < 5; nLoops++)
        {
            WIN_HWND hActiveWnd = ::GetActiveWindow();
            if (hActiveWnd == hRendWnd)
            {
                break;
            }

            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
            Sleep(5);
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SleepIfNeeded()
{
    FUNCTION_PROFILER_FAST(this, PROFILE_SYSTEM, g_bProfilerEnabled);

    ITimer* const pTimer = gEnv->pTimer;
    static bool firstCall = true;

    typedef MiniQueue<CTimeValue, 32> PrevNow;
    static PrevNow prevNow;
    if (firstCall)
    {
        m_lastTickTime = pTimer->GetAsyncTime();
        prevNow.Push(m_lastTickTime);
        firstCall = false;
        return;
    }

    const float maxRate = m_svDedicatedMaxRate->GetFVal();
    const float minTime = 1.0f / maxRate;
    CTimeValue now = pTimer->GetAsyncTime();
    float elapsed = (now - m_lastTickTime).GetSeconds();

    if (prevNow.Full())
    {
        prevNow.Pop();
    }
    prevNow.Push(now);

    static bool allowStallCatchup = true;
    if (elapsed > minTime && allowStallCatchup)
    {
        allowStallCatchup = false;
        m_lastTickTime = pTimer->GetAsyncTime();
        return;
    }
    allowStallCatchup = true;

    float totalElapsed = (now - prevNow.Front()).GetSeconds();
    float wantSleepTime = CLAMP(minTime * (prevNow.Size() - 1) - totalElapsed, 0, (minTime - elapsed) * 0.9f);
    static float sleepTime = 0;
    sleepTime = (15 * sleepTime + wantSleepTime) / 16;
    int sleepMS = (int)(1000.0f * sleepTime + 0.5f);
    if (sleepMS > 0)
    {
        AZ_PROFILE_FUNCTION_IDLE(AZ::Debug::ProfileCategory::System);
        Sleep(sleepMS);
    }

    m_lastTickTime = pTimer->GetAsyncTime();
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

    RenderBegin();

#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    // do the dedicated sleep earlier than the frame profiler to avoid having it counted
    if (gEnv->IsDedicated())
    {
#if defined(MAP_LOADING_SLICING)
        gEnv->pSystemScheduler->SchedulingSleepIfNeeded();
#else
        SleepIfNeeded();
#endif // defined(MAP_LOADING_SLICING)
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE

    gEnv->pOverloadSceneManager->Update();

#ifdef WIN32
    // enable/disable SSE fp exceptions (#nan and /0)
    // need to do it each frame since sometimes they are being reset
    _mm_setcsr(_mm_getcsr() & ~0x280 | (g_cvars.sys_float_exceptions > 0 ? 0 : 0x280));
#endif //WIN32

    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_SYSTEM);
    AZ_TRACE_METHOD();

    m_nUpdateCounter++;
#ifndef EXCLUDE_UPDATE_ON_CONSOLE
    if (!m_sDelayedScreeenshot.empty())
    {
        gEnv->pRenderer->ScreenShot(m_sDelayedScreeenshot.c_str());
        m_sDelayedScreeenshot.clear();
    }

    // Check if game needs to be sleeping when not active.
    SleepIfInactive();

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
        UpdateFPExceptionsMaskForThreads();
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

    if (gEnv->pLocalMemoryUsage != NULL)
    {
        gEnv->pLocalMemoryUsage->OnUpdate();
    }

    if (!gEnv->IsEditor() && gEnv->pRenderer)
    {
        // If the dimensions of the render target change,
        // or are different from the camera defaults,
        // we need to update the camera frustum.
        CCamera& viewCamera = GetViewCamera();
        const int renderTargetWidth = m_rWidth->GetIVal();
        const int renderTargetHeight = m_rHeight->GetIVal();

        if (renderTargetWidth != viewCamera.GetViewSurfaceX() ||
            renderTargetHeight != viewCamera.GetViewSurfaceZ())
        {
            viewCamera.SetFrustum(renderTargetWidth,
                renderTargetHeight,
                viewCamera.GetFov(),
                viewCamera.GetNearPlane(),
                viewCamera.GetFarPlane(),
                gEnv->pRenderer->GetPixelAspectRatio());
        }
    }
    if (nPauseMode != 0)
    {
        m_bPaused = true;
    }
    else
    {
        m_bPaused = false;
    }

#ifdef PROFILE_WITH_VTUNE
    if (m_bInDevMode)
    {
        if (VTPause != NULL && VTResume != NULL)
        {
            static bool bVtunePaused = true;

            const AzFramework::InputChannel* inputChannelScrollLock = AzFramework::InputChannelRequests::FindInputChannel(AzFramework::InputDeviceKeyboard::Key::WindowsSystemScrollLock);
            const bool bPaused = (inputChannelScrollLock ? inputChannelScrollLock->IsActive() : false);

            {
                if (bVtunePaused && !bPaused)
                {
                    GetIProfilingSystem()->VTuneResume();
                }
                if (!bVtunePaused && bPaused)
                {
                    GetIProfilingSystem()->VTunePause();
                }
                bVtunePaused = bPaused;
            }
        }
    }
#endif //PROFILE_WITH_VTUNE

#ifdef SOFTCODE_SYSTEM_ENABLED
    if (m_env.pSoftCodeMgr)
    {
        m_env.pSoftCodeMgr->PollForNewModules();
    }
#endif

    if (m_pStreamEngine)
    {
        FRAME_PROFILER("StreamEngine::Update()", this, PROFILE_SYSTEM);
        m_pStreamEngine->Update();
    }

    if (g_cvars.az_streaming_stats)
    {
        SDrawTextInfo ti;
        ti.flags = eDrawText_FixedSize | eDrawText_2D | eDrawText_Monospace;
        ti.xscale = ti.yscale = 1.2f;

        const int viewportHeight = GetViewCamera().GetViewSurfaceZ();

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_8
#include AZ_RESTRICTED_FILE(System_cpp)
#else
    float y = static_cast<float>(viewportHeight) - 85.0f;
#endif

        AZStd::vector<AZ::IO::Statistic> stats;
        AZ::Interface<AZ::IO::IStreamer>::Get()->CollectStatistics(stats);

        for (AZ::IO::Statistic& stat : stats)
        {
            switch (stat.GetType())
            {
            case AZ::IO::Statistic::Type::FloatingPoint:
                gEnv->pRenderer->DrawTextQueued(Vec3(10, y, 1.0f), ti,
                    AZStd::string::format("%s/%s: %.3f", stat.GetOwner().data(), stat.GetName().data(), stat.GetFloatValue()).c_str());
                break;
            case AZ::IO::Statistic::Type::Integer:
                gEnv->pRenderer->DrawTextQueued(Vec3(10, y, 1.0f), ti,
                    AZStd::string::format("%s/%s: %lli", stat.GetOwner().data(), stat.GetName().data(), stat.GetIntegerValue()).c_str());
                break;
            case AZ::IO::Statistic::Type::Percentage:
                gEnv->pRenderer->DrawTextQueued(Vec3(10, y, 1.0f), ti,
                    AZStd::string::format("%s/%s: %.2f (percent)", stat.GetOwner().data(), stat.GetName().data(), stat.GetPercentage()).c_str());
                break;
            default:
                gEnv->pRenderer->DrawTextQueued(Vec3(10, y, 1.0f), ti, AZStd::string::format("Unsupported stat type: %i", static_cast<int>(stat.GetType())).c_str());
                break;
            }
            y -= 12.0f;
            if (y < 0.0f)
            {
                //Exit the loop because there's no purpose on rendering text outside of the visible area.
                break;
            }
        }
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
    //check what is the current process
    IProcess* pProcess = GetIProcess();
    if (!pProcess)
    {
        return (true); //should never happen
    }
    if (m_sysNoUpdate && m_sysNoUpdate->GetIVal())
    {
        bNoUpdate = true;
        updateFlags = ESYSUPDATE_IGNORE_PHYSICS;
    }

    //if ((pProcess->GetFlags() & PROC_MENU) || (m_sysNoUpdate && m_sysNoUpdate->GetIVal()))
    //  bPause = true;
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
        FRAME_PROFILER_LEGACYONLY("FRAME_CAP", gEnv->pSystem, PROFILE_SYSTEM);
        AZ_TRACE_METHOD_NAME("FrameLimiter");
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
                const bool inLevel = pLvlSys && pLvlSys->GetCurrentLevel() != 0;
                maxFPS = !inLevel || IsPaused() ? 60 : 0;
            }

            if (maxFPS > 0 && vSync == 0)
            {
                CTimeValue timeFrameMax;
                const float safeMarginFPS = 0.5f;//save margin to not drop below 30 fps
                static CTimeValue sTimeLast = gEnv->pTimer->GetAsyncTime();
                timeFrameMax.SetMilliSeconds((int64)(1000.f / ((float)maxFPS + safeMarginFPS)));
                const CTimeValue timeLast = timeFrameMax + sTimeLast;
                while (timeLast.GetValue() > gEnv->pTimer->GetAsyncTime().GetValue())
                {
                    CrySleep(0);
                }
                sTimeLast = gEnv->pTimer->GetAsyncTime();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////
    //update time subsystem
    m_Time.UpdateOnFrameStart();

    if (m_env.p3DEngine)
    {
        m_env.p3DEngine->OnFrameStart();
    }

    //////////////////////////////////////////////////////////////////////
    // update rate limiter for dedicated server
    if (m_pServerThrottle.get())
    {
        m_pServerThrottle->Update();
    }

    //////////////////////////////////////////////////////////////////////////
    if (m_env.pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
    {
        EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, UpdateInternalState);
    }

    //////////////////////////////////////////////////////////////////////
    //update console system
    if (m_env.pConsole)
    {
        FRAME_PROFILER("SysUpdate:Console", this, PROFILE_SYSTEM);
        m_env.pConsole->Update();
    }

    if (IsQuitting())
    {
        Quit();
        return false;
    }

#ifndef EXCLUDE_UPDATE_ON_CONSOLE

    //////////////////////////////////////////////////////////////////////
    //update notification network system
    if (m_pNotificationNetwork)
    {
        FRAME_PROFILER("SysUpdate:NotificationNetwork", this, PROFILE_SYSTEM);
        m_pNotificationNetwork->Update();
    }
#endif //EXCLUDE_UPDATE_ON_CONSOLE
       //////////////////////////////////////////////////////////////////////
       //update sound system Part 1 if in Editor / in Game Mode Viewsystem updates the Listeners
    if (!m_env.IsEditorGameMode())
    {
        if ((updateFlags & ESYSUPDATE_EDITOR) != 0 && !bNoUpdate && nPauseMode != 1)
        {
            // updating the Listener Position in a first separate step.
            // Updating all views here is a bit of a workaround, since we need
            // to ensure that sound listeners owned by inactive views are also
            // marked as inactive. Ideally that should happen when exiting game mode.
            if (GetIViewSystem())
            {
                FRAME_PROFILER("SysUpdate:UpdateSoundListeners", this, PROFILE_SYSTEM);
                GetIViewSystem()->UpdateSoundListeners();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Update Threads Task Manager.
    //////////////////////////////////////////////////////////////////////////
    {
        FRAME_PROFILER("SysUpdate:ThreadTaskManager", this, PROFILE_SYSTEM);
        m_pThreadTaskManager->OnUpdate();
    }

    //////////////////////////////////////////////////////////////////////////
    // Update Resource Manager.
    //////////////////////////////////////////////////////////////////////////
    {
        FRAME_PROFILER("SysUpdate:ResourceManager", this, PROFILE_SYSTEM);
        m_pResourceManager->Update();
    }

    //////////////////////////////////////////////////////////////////////
    // update physic system
    //static float time_zero = 0;
    if (m_sys_physics_CPU->GetIVal() > 0 && !gEnv->IsDedicated())
    {
        CreatePhysicsThread();
    }
    else
    {
        KillPhysicsThread();
    }

    static int g_iPausedPhys = 0;

    CPhysicsThreadTask* pPhysicsThreadTask = ((CPhysicsThreadTask*)m_PhysThread);
    if (!pPhysicsThreadTask)
    {
        FRAME_PROFILER_LEGACYONLY("SysUpdate:AllAIAndPhysics", this, PROFILE_SYSTEM);
        AZ_TRACE_METHOD_NAME("SysUpdate::AllAIAndPhysics");

        //////////////////////////////////////////////////////////////////////
        // update entity system (a little bit) before physics
        if (nPauseMode != 1)
        {
            if (!bNoUpdate)
            {
                EBUS_EVENT(CrySystemEventBus, OnCrySystemPrePhysicsUpdate);
            }
        }

        // intermingle physics/AI updates so that if we get a big timestep (frame rate glitch etc) the
        // AI gets to steer entities before they travel over cliffs etc.
        const float maxTimeStep = 0.25f;
        int maxSteps = 1;
        float fCurTime = m_Time.GetCurrTime();
        float timeToDo = m_Time.GetFrameTime();//fCurTime - fPrevTime;
        if (m_env.bMultiplayer)
        {
            timeToDo = m_Time.GetRealFrameTime();
        }



        while (timeToDo > 0.0001f && maxSteps-- > 0)
        {
            float thisStep = min(maxTimeStep, timeToDo);
            timeToDo -= thisStep;


            EBUS_EVENT(CrySystemEventBus, OnCrySystemPostPhysicsUpdate);
        }

    }
    else
    {

        // In multithreaded physics mode, post physics fires after physics events are dispatched on the main thread.
        EBUS_EVENT(CrySystemEventBus, OnCrySystemPostPhysicsUpdate);

        //////////////////////////////////////////////////////////////////////
        // update entity system (a little bit) before physics
        if (nPauseMode != 1)
        {
            if (!bNoUpdate)
            {
                EBUS_EVENT(CrySystemEventBus, OnCrySystemPrePhysicsUpdate);
            }
        }

    }

    // Use UI timer for CryMovie, because it should not be affected by pausing game time
    const float fMovieFrameTime = m_Time.GetFrameTime(ITimer::ETIMER_UI);

    // Run movie system pre-update
    if (!bNoUpdate)
    {
        FRAME_PROFILER("SysUpdate:UpdateMovieSystem", this, PROFILE_SYSTEM);
        UpdateMovieSystem(updateFlags, fMovieFrameTime, true);
    }

    return !IsQuitting();
}

//////////////////////////////////////////////////////////////////////
bool CSystem::UpdatePostTickBus(int updateFlags, int nPauseMode)
{
    CTimeValue updateStart = gEnv->pTimer->GetAsyncTime();

    // Run movie system post-update
    if (!m_bNoUpdate)
    {
        const float fMovieFrameTime = m_Time.GetFrameTime(ITimer::ETIMER_UI);
        FRAME_PROFILER("SysUpdate:UpdateMovieSystem", this, PROFILE_SYSTEM);
        UpdateMovieSystem(updateFlags, fMovieFrameTime, false);
    }

    //////////////////////////////////////////////////////////////////////
    //update process (3D engine)
    if (!(updateFlags & ESYSUPDATE_EDITOR) && !m_bNoUpdate && m_env.p3DEngine)
    {
        FRAME_PROFILER("SysUpdate:Update3DEngine", this, PROFILE_SYSTEM);

        if (ITimeOfDay* pTOD = m_env.p3DEngine->GetTimeOfDay())
        {
            pTOD->Tick();
        }

        if (m_env.p3DEngine)
        {
            m_env.p3DEngine->Tick();  // clear per frame temp data
        }
        if (m_pProcess && (m_pProcess->GetFlags() & PROC_3DENGINE))
        {
            if ((nPauseMode != 1))
            {
                if (!IsEquivalent(m_ViewCamera.GetPosition(), Vec3(0, 0, 0), VEC_EPSILON))
                {
                    if (m_env.p3DEngine)
                    {
                        //                  m_env.p3DEngine->SetCamera(m_ViewCamera);
                        m_pProcess->Update();
                    }
                }
            }
        }
        else
        {
            if (m_pProcess)
            {
                m_pProcess->Update();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////
    //update sound system part 2
    if (!g_cvars.sys_deferAudioUpdateOptim && !m_bNoUpdate)
    {
        FRAME_PROFILER("SysUpdate:UpdateAudioSystems", this, PROFILE_SYSTEM);
        UpdateAudioSystems();
    }
    else
    {
        m_bNeedDoWorkDuringOcclusionChecks = true;
    }

    //Now update frame statistics
    CTimeValue cur_time = gEnv->pTimer->GetAsyncTime();

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

    UpdateUpdateTimes();

    {
        FRAME_PROFILER("SysUpdate - SystemEventDispatcher::Update", this, PROFILE_SYSTEM);
        m_pSystemEventDispatcher->Update();
    }

    if (!gEnv->IsEditing() && m_eRuntimeState == ESYSTEM_EVENT_LEVEL_GAMEPLAY_START)
    {
        gEnv->pCryPak->DisableRuntimeFileAccess(true);
    }

    // If it's in editing mode (in editor) the render is done in RenderViewport so we skip rendering here.
    if (!gEnv->IsEditing() && gEnv->pRenderer && gEnv->p3DEngine)
    {
        if (GetIViewSystem())
        {
            GetIViewSystem()->Update(min(gEnv->pTimer->GetFrameTime(), 0.1f));
        }

        if (gEnv->pLyShine)
        {
            // Tell the UI system the size of the viewport we are rendering to - this drives the
            // canvas size for full screen UI canvases. It needs to be set before either pLyShine->Update or
            // pLyShine->Render are called. It must match the viewport size that the input system is using.
            AZ::Vector2 viewportSize;
            viewportSize.SetX(static_cast<float>(gEnv->pRenderer->GetOverlayWidth()));
            viewportSize.SetY(static_cast<float>(gEnv->pRenderer->GetOverlayHeight()));
            gEnv->pLyShine->SetViewportSize(viewportSize);

            bool isUiPaused = gEnv->pTimer->IsTimerPaused(ITimer::ETIMER_UI);
            if (!isUiPaused)
            {
                gEnv->pLyShine->Update(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_UI));
            }
        }

        // Begin occlusion job after setting the correct camera.
        gEnv->p3DEngine->PrepareOcclusion(GetViewCamera());

        CrySystemNotificationBus::Broadcast(&CrySystemNotifications::OnPreRender);

        // Also broadcast for anyone else that needs to draw global debug to do so now
        AzFramework::DebugDisplayEventBus::Broadcast(&AzFramework::DebugDisplayEvents::DrawGlobalDebugInfo);

        Render();

        gEnv->p3DEngine->EndOcclusion();

        CrySystemNotificationBus::Broadcast(&CrySystemNotifications::OnPostRender);

        RenderEnd();

        gEnv->p3DEngine->SyncProcessStreamingUpdate();

        if (NeedDoWorkDuringOcclusionChecks())
        {
            DoWorkDuringOcclusionChecks();
        }

        // Sync the work that must be done in the main thread by the end of frame.
        gEnv->pRenderer->GetGenerateShadowRendItemJobExecutor()->WaitForCompletion();
        gEnv->pRenderer->GetGenerateRendItemJobExecutor()->WaitForCompletion();
    }

    return !IsQuitting();
}


bool CSystem::UpdateLoadtime()
{
    return !IsQuitting();
}

void CSystem::DoWorkDuringOcclusionChecks()
{
    if (g_cvars.sys_deferAudioUpdateOptim && !m_bNoUpdate)
    {
        UpdateAudioSystems();
        m_bNeedDoWorkDuringOcclusionChecks = false;
    }
}

void CSystem::UpdateAudioSystems()
{
    AZ_TRACE_METHOD();
    FRAME_PROFILER_LEGACYONLY("SysUpdate:Audio", this, PROFILE_SYSTEM);
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::ExternalUpdate);
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
    LOADING_TIME_PROFILE_SECTION_ARGS(sFilename);

    return m_pXMLUtils->LoadXmlFromFile(sFilename, bReuseStrings);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSystem::LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings, bool bSuppressWarnings)
{
    LOADING_TIME_PROFILE_SECTION
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
int CSystem::ShowMessage(const char* text, const char* caption, unsigned int uType)
{
    if (m_pUserCallback)
    {
        return m_pUserCallback->ShowMessage(text, caption, uType);
    }
    return CryMessageBox(text, caption, uType);
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
        if (strlen(sModule) > 1 || CryStringUtils::stristr(sModule, sModuleFilter) == 0)
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
    char szBuffer[MAX_WARNING_LENGTH];
    vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, format, args);

    if (file && *file)
    {
        CryFixedStringT<MAX_WARNING_LENGTH> fmt = szBuffer;
        fmt += " [File=";
        fmt += file;
        fmt += "]";

        m_env.pLog->LogWithType(ltype, flags | VALIDATOR_FLAG_SKIP_VALIDATOR, "%s", fmt.c_str());
    }
    else
    {
        m_env.pLog->LogWithType(ltype, flags | VALIDATOR_FLAG_SKIP_VALIDATOR, "%s", szBuffer);
    }

    //if(file)
    //m_env.pLog->LogWithType( ltype, "  ... caused by file '%s'",file);

    if (m_pValidator && (flags & VALIDATOR_FLAG_SKIP_VALIDATOR) == 0)
    {
        SValidatorRecord record;
        record.file = file;
        record.text = szBuffer;
        record.module = module;
        record.severity = severity;
        record.flags = flags;
        record.assetScope = m_env.pLog->GetAssetScopeString();
        m_pValidator->Report(record);
    }

    if (bDbgBreak && g_cvars.sys_error_debugbreak)
    {
        AZ::Debug::Trace::Break();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedPath(const char* sLanguage, string& sLocalizedPath)
{
    // Omit the trailing slash!
    string sLocalizationFolder(string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    int locFormat = 0;
    LocalizationManagerRequestBus::BroadcastResult(locFormat, &LocalizationManagerRequestBus::Events::GetLocalizationFormat);
    if(locFormat == 1)
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + ".loc.agsxml";
    }
    else
    {
    if (sLocalizationFolder.compareNoCase("Languages") != 0)
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + "_xml.pak";
    }
    else
    {
        sLocalizedPath = string("Localized/") + sLanguage + "_xml.pak";
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::GetLocalizedAudioPath(const char* sLanguage, string& sLocalizedPath)
{
    // Omit the trailing slash!
    string sLocalizationFolder(string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    if (sLocalizationFolder.compareNoCase("Languages") != 0)
    {
        sLocalizedPath = sLocalizationFolder + "/" + sLanguage + ".pak";
    }
    else
    {
        sLocalizedPath = string("Localized/") + sLanguage + ".pak";
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguagePak(const char* sLanguage)
{
    string sLocalizedPath;
    GetLocalizedPath(sLanguage, sLocalizedPath);
    m_env.pCryPak->ClosePacks({ sLocalizedPath.c_str(), sLocalizedPath.size() });
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CloseLanguageAudioPak(const char* sLanguage)
{
    string sLocalizedPath;
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
ICrySizer* CSystem::CreateSizer()
{
    return new CrySizerImpl;
}

//////////////////////////////////////////////////////////////////////////
uint32 CSystem::GetUsedMemory()
{
    return CryMemoryGetAllocatedSize();
}

//////////////////////////////////////////////////////////////////////////
ILocalizationManager* CSystem::GetLocalizationManager()
{
    return m_pLocalizationManager;
}

//////////////////////////////////////////////////////////////////////////
IThreadTaskManager* CSystem::GetIThreadTaskManager()
{
    return m_pThreadTaskManager;
}

//////////////////////////////////////////////////////////////////////////
IResourceManager* CSystem::GetIResourceManager()
{
    return m_pResourceManager;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::debug_GetCallStackRaw(void** callstack, uint32& callstackLength)
{
    uint32 callstackCapacity = callstackLength;
    uint32 nNumStackFramesToSkip = 1;

    memset(callstack, 0, sizeof(void*) * callstackLength);

#if !defined(ANDROID)
    callstackLength = 0;
#endif

#if AZ_LEGACY_CRYSYSTEM_TRAIT_CAPTURESTACK
    if (callstackCapacity > 0x40)
    {
        callstackCapacity = 0x40;
    }
    callstackLength = RtlCaptureStackBackTrace(nNumStackFramesToSkip, callstackCapacity, callstack, NULL);
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_7
#include AZ_RESTRICTED_FILE(System_cpp)
#endif

    if (callstackLength > 0)
    {
        std::reverse(callstack, callstack + callstackLength);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ExecuteCommandLine(bool deferred)
{
    if (m_executedCommandLine)
    {
        return;
    }

    m_executedCommandLine = true;

    // auto detect system spec (overrides profile settings)
    if (m_pCmdLine->FindArg(eCLAT_Pre, "autodetect"))
    {
        AutoDetectSpec(false);
    }

    // execute command line arguments e.g. +g_gametype ASSAULT +map "testy"

    ICmdLine* pCmdLine = GetICmdLine();
    assert(pCmdLine);

    const int iCnt = pCmdLine->GetArgCount();
    for (int i = 0; i < iCnt; ++i)
    {
        const ICmdLineArg* pCmd = pCmdLine->GetArg(i);

        if (pCmd->GetType() == eCLAT_Post)
        {
            string sLine = pCmd->GetName();

#if defined(CVARS_WHITELIST)
            if (!GetCVarsWhiteList() || GetCVarsWhiteList()->IsWhiteListed(sLine, false))
#endif
            {
                if (pCmd->GetValue())
                {
                    sLine += string(" ") + pCmd->GetValue();
                }

                GetILog()->Log("Executing command from command line: \n%s\n", sLine.c_str()); // - the actual command might be executed much later (e.g. level load pause)
                GetIConsole()->ExecuteString(sLine.c_str(), false, deferred);
            }
#if defined(CVARS_WHITELIST)
            else if (gEnv->IsDedicated())
            {
                GetILog()->LogError("Failed to execute command: '%s' as it is not whitelisted\n", sLine.c_str());
            }
#endif
        }
    }

    //gEnv->pConsole->ExecuteString("sys_RestoreSpec test*"); // to get useful debugging information about current spec settings to the log file
}

void CSystem::DumpMemoryCoverage()
{
    m_MemoryFragmentationProfiler.DumpMemoryCoverage();
}

ITextModeConsole* CSystem::GetITextModeConsole()
{
    if (m_bDedicatedServer)
    {
        return m_pTextModeConsole;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigSpec CSystem::GetConfigSpec(bool bClient)
{
    if (bClient)
    {
        if (m_sys_GraphicsQuality)
        {
            return (ESystemConfigSpec)m_sys_GraphicsQuality->GetIVal();
        }
        return CONFIG_VERYHIGH_SPEC; // highest spec.
    }
    else
    {
        return m_nServerConfigSpec;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSystem::SetConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform, bool bClient)
{
    if (bClient)
    {
        if (m_sys_GraphicsQuality)
        {
            SetConfigPlatform(platform);
            m_sys_GraphicsQuality->Set(static_cast<int>(spec));
        }
    }
    else
    {
        m_nServerConfigSpec = spec;
    }
}

//////////////////////////////////////////////////////////////////////////
ESystemConfigSpec CSystem::GetMaxConfigSpec() const
{
    return m_nMaxConfigSpec;
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

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::VTuneResume()
{
#ifdef PROFILE_WITH_VTUNE
    if (VTResume)
    {
        CryLogAlways("VTune Resume");
        VTResume();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CProfilingSystem::VTunePause()
{
#ifdef PROFILE_WITH_VTUNE
    if (VTPause)
    {
        VTPause();
        CryLogAlways("VTune Pause");
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
sUpdateTimes& CSystem::GetCurrentUpdateTimeStats()
{
    return m_UpdateTimes[m_UpdateTimesIdx];
}

//////////////////////////////////////////////////////////////////////////
const sUpdateTimes* CSystem::GetUpdateTimeStats(uint32& index, uint32& num)
{
    index = m_UpdateTimesIdx;
    num = NUM_UPDATE_TIMES;
    return m_UpdateTimes;
}

void CSystem::UpdateUpdateTimes()
{
    sUpdateTimes& sample = m_UpdateTimes[m_UpdateTimesIdx];
    if (m_PhysThread)
    {
        static uint64 lastPhysTime = 0U;
        static uint64 lastMainTime = 0U;
        static uint64 lastYields = 0U;
        static uint64 lastPhysWait = 0U;
        uint64 physTime = 0, mainTime = 0;
        uint32 yields = 0;
        physTime = ((CPhysicsThreadTask*)m_PhysThread)->LastStepTaken();
        mainTime = CryGetTicks() - lastMainTime;
        lastMainTime = mainTime;
        lastPhysWait = ((CPhysicsThreadTask*)m_PhysThread)->LastWaitTime();
        sample.PhysStepTime = physTime;
        sample.SysUpdateTime = mainTime;
        sample.PhysYields = yields;
        sample.physWaitTime = lastPhysWait;
    }
    ++m_UpdateTimesIdx;
    if (m_UpdateTimesIdx >= NUM_UPDATE_TIMES)
    {
        m_UpdateTimesIdx = 0;
    }
}


#ifndef _RELEASE
void CSystem::GetCheckpointData(ICheckpointData& data)
{
    data.m_totalLoads = m_checkpointLoadCount;
    data.m_loadOrigin = m_loadOrigin;
}

void CSystem::IncreaseCheckpointLoadCount()
{
    if (!m_hasJustResumed)
    {
        ++m_checkpointLoadCount;
    }

    m_hasJustResumed = false;
}

void CSystem::SetLoadOrigin(LevelLoadOrigin origin)
{
    switch (origin)
    {
    case eLLO_NewLevel: // Intentional fall through
    case eLLO_Level2Level:
        m_expectingMapCommand = true;
        break;

    case eLLO_Resumed:
        m_hasJustResumed = true;
        break;

    case eLLO_MapCmd:
        if (m_expectingMapCommand)
        {
            // We knew a map command was coming, so don't process this.
            m_expectingMapCommand = false;
            return;
        }
        break;
    }

    m_loadOrigin = origin;
    m_checkpointLoadCount = 0;
}
#endif

bool CSystem::SteamInit()
{
#if USE_STEAM
    if (m_bIsSteamInitialized)
    {
        return true;
    }

    AZStd::string_view exePath;
    AZ::ComponentApplicationBus::BroadcastResult(exePath, &AZ::ComponentApplicationRequests::GetExecutableFolder);

    ////////////////////////////////////////////////////////////////////////////
    // ** DEVELOPMENT ONLY ** - creates the appropriate steam_appid.txt file needed to call SteamAPI_Init()
#if !defined(RELEASE)
    AZStd::string appidPath = AZStd::string::format("%.*s/steam_appid.txt", aznumeric_cast<int>(exePath.size()), exePath.data());
    azfopen(&pSteamAppID, appidPath.c_str(), "wt");
    fprintf(pSteamAppID, "%d", g_cvars.sys_steamAppId);
    fclose(pSteamAppID);
#endif // !defined(RELEASE)
       // ** END DEVELOPMENT ONLY **
       ////////////////////////////////////////////////////////////////////////////

    if (!SteamAPI_Init())
    {
        CryLog("[STEAM] SteamApi_Init failed");
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////
    // ** DEVELOPMENT ONLY ** - deletes the appropriate steam_appid.txt file as it's no longer needed
#if !defined(RELEASE)
    remove(appidPath.c_str());
#endif // !defined(RELEASE)
       // ** END DEVELOPMENT ONLY **
       ////////////////////////////////////////////////////////////////////////////

    m_bIsSteamInitialized = true;
    return true;
#else
    return false;
#endif
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

//////////////////////////////////////////////////////////////////////
void CSystem::OnLanguageAudioCVarChanged(ICVar* language)
{
    static Audio::SAudioRequest oLanguageRequest;
    static Audio::SAudioManagerRequestData<Audio::eAMRT_CHANGE_LANGUAGE> oLanguageRequestData;

    if (language && (language->GetType() == CVAR_STRING))
    {
        oLanguageRequest.pData = &oLanguageRequestData;
        oLanguageRequest.nFlags = Audio::eARF_PRIORITY_HIGH;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oLanguageRequest);
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
    static CTimeValue s_startTime = CTimeValue();
    if (systemGlobalState != m_systemGlobalState)
    {
        if (gEnv && gEnv->pTimer)
        {
            const CTimeValue endTime = gEnv->pTimer->GetAsyncTime();
            const float numSeconds = endTime.GetDifferenceInSeconds(s_startTime);
            CryLog("SetGlobalState %d->%d '%s'->'%s' %3.1f seconds",
                m_systemGlobalState, systemGlobalState,
                CSystem::GetSystemGlobalStateName(m_systemGlobalState), CSystem::GetSystemGlobalStateName(systemGlobalState),
                numSeconds);
            s_startTime = gEnv->pTimer->GetAsyncTime();
        }
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
void* CSystem::GetRootWindowMessageHandler()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION SYSTEM_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(System_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
    return reinterpret_cast<void*>(&WndProc);
#else
    CRY_ASSERT(false && "This platform does not support window message handlers");
    return NULL;
#endif
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
    bool bRemoved = stl::find_and_erase(m_windowMessageHandlers, pHandler);
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
        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
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
        UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
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

        const UINT bytesCopied = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);
        CRY_ASSERT(bytesCopied == rawInputSize);

        RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;
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

    return true;
}

#endif

std::shared_ptr<AZ::IO::FileIOBase> CSystem::CreateLocalFileIO()
{
    return std::make_shared<AZ::IO::LocalFileIO>();
}

IViewSystem* CSystem::GetIViewSystem()
{
    return m_pViewSystem;
}

ILevelSystem* CSystem::GetILevelSystem()
{
    return m_pLevelSystem;
}

#undef EXCLUDE_UPDATE_ON_CONSOLE

