/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ISystem.h>
#include <IRenderer.h>
#include <IPhysics.h>
#include <IWindowMessageHandler.h>

#include "Timer.h"
#include <CryVersion.h>
#include "CmdLine.h"
#include "CryName.h"

#include <AzFramework/Archive/ArchiveVars.h>
#include "RenderBus.h"

#include <LoadScreenBus.h>

#include <AzCore/Module/DynamicModuleHandle.h>

namespace AzFramework
{
    class MissingAssetLogger;
}

struct IConsoleCmdArgs;
class CWatchdogThread;

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEM_H_SECTION_1 1
#define SYSTEM_H_SECTION_2 2
#define SYSTEM_H_SECTION_3 3
#define SYSTEM_H_SECTION_4 4
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_1
#include AZ_RESTRICTED_FILE(System_h)
#else
#if defined(WIN32) || defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_ALLOW_CREATE_BACKUP_LOG_FILE 1
#endif
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEFINE_DETECT_PROCESSOR 1
#endif

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) || defined(APPLE) || defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DO_PREASSERT 1
#endif
#if defined(MAC) || (defined(LINUX) && !defined(ANDROID))
#define AZ_LEGACY_CRYSYSTEM_TRAIT_ASM_VOLATILE_CPUID 1
#endif
#if (defined(WIN32) && !defined(WIN64)) || (defined(LINUX) && !defined(ANDROID) && !defined(LINUX64))
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HAS64BITEXT 1
#endif
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HTSUPPORTED 1
#endif
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID 1
#endif
#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HASAFFINITYMASK 1
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_CRYPAK_POSIX 1
#endif

#if defined(WIN64)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_BIT64 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_PACKED_PEHEADER 1
#endif
#if defined(WIN32) || defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_RENDERMEMORY_INFO 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_HANDLER_SYNC_AFFINITY 1
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_FORWARD_EXCEPTION_POINTERS 1
#endif

#if !defined(_WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_SINGLETON 1
#endif
#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_TRANSLATE 1
#endif
#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_APPEND_MODULENAME 1
#endif

#if !(defined(ANDROID) || defined(IOS) || defined(LINUX))
#define AZ_LEGACY_CRYSYSTEM_TRAIT_IMAGEHANDLER_TIFFIO 1
#endif

#if 1
#define AZ_LEGACY_CRYSYSTEM_TRAIT_JOBMANAGER_SIXWORKERTHREADS 0
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_MEMADDRESSRANGE_WINDOWS_STYLE 1
#endif

#if 1
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_EXCLUDEUPDATE_ON_CONSOLE 0
#endif
#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER 1
#endif
#if defined(WIN64) || defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_CAPTURESTACK 1
#endif

#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_SYSTEMCFG_MODULENAME 1
#endif

#if defined(WIN32) || defined(WIN64)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_THREADINFO_WINDOWS_STYLE 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_THREADTASK_EXCEPTIONS 1
#endif
//////////////////////////////////////////////////////////////////////////

#if defined(APPLE) || defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_FACTORY_REGISTRY_USE_PRINTF_FOR_FATAL 1
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_FTELL_NOT_FTELLI64 1
#endif

#endif

#if defined(USE_UNIXCONSOLE) || defined(USE_ANDROIDCONSOLE) || defined(USE_WINDOWSCONSOLE) || defined(USE_IOSCONSOLE) || defined(USE_NULLCONSOLE)
#define USE_DEDICATED_SERVER_CONSOLE
#endif

#if defined(LINUX)
    #include "CryLibrary.h"
#endif

#ifdef WIN32
typedef void* WIN_HMODULE;
#else
typedef void* WIN_HMODULE;
#endif

//forward declarations
namespace Audio
{
    struct IAudioSystem;
    struct IMusicSystem;
} // namespace Audio
struct IDataProbe;

#define PHSYICS_OBJECT_ENTITY 0

typedef void (__cdecl * VTuneFunction)(void);
extern VTuneFunction VTResume;
extern VTuneFunction VTPause;

#define MAX_STREAMING_POOL_INDEX 6
#define MAX_THREAD_POOL_INDEX 6

struct SSystemCVars
{
    int sys_streaming_requests_grouping_time_period;
    int sys_streaming_sleep;
    int sys_streaming_memory_budget;
    int sys_streaming_max_finalize_per_frame;
    float sys_streaming_max_bandwidth;
    int sys_streaming_cpu;
    int sys_streaming_cpu_worker;
    int sys_streaming_debug;
    int sys_streaming_resetstats;
    int sys_streaming_debug_filter;
    float sys_streaming_debug_filter_min_time;
    int sys_streaming_use_optical_drive_thread;
    ICVar* sys_streaming_debug_filter_file_name;
    ICVar* sys_localization_folder;
    int sys_streaming_in_blocks;

    int sys_float_exceptions;
    int sys_no_crash_dialog;
    int sys_no_error_report_window;
    int sys_dump_aux_threads;
    int sys_WER;
    int sys_dump_type;
    int sys_ai;
    int sys_entitysystem;
    int sys_trackview;
    int sys_vtune;
    float sys_update_profile_time;
    int sys_limit_phys_thread_count;
    int sys_MaxFPS;
    float sys_maxTimeStepForMovieSystem;
    int sys_force_installtohdd_mode;
    int sys_report_files_not_found_in_paks = 0;

#ifdef USE_HTTP_WEBSOCKETS
    int sys_simple_http_base_port;
#endif

    int sys_asserts;
    int sys_error_debugbreak;

    int sys_FilesystemCaseSensitivity;

    AZ::IO::ArchiveVars archiveVars;

#if defined(WIN32)
    int sys_display_threads;
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_2
#include AZ_RESTRICTED_FILE(System_h)
#endif
};
extern SSystemCVars g_cvars;

class CSystem;

struct CProfilingSystem
    : public IProfilingSystem
{
    //////////////////////////////////////////////////////////////////////////
    // VTune Profiling interface.

    // Summary:
    //   Resumes vtune data collection.
    virtual void VTuneResume();
    // Summary:
    //   Pauses vtune data collection.
    virtual void VTunePause();
    //////////////////////////////////////////////////////////////////////////
};

class AssetSystem;

/*
===========================================
The System interface Class
===========================================
*/
class CXConsole;

//////////////////////////////////////////////////////////////////////
//! ISystem implementation
class CSystem
    : public ISystem
    , public ILoadConfigurationEntrySink
    , public ISystemEventListener
    , public IWindowMessageHandler
    , public AZ::RenderNotificationsBus::Handler
    , public CrySystemRequestBus::Handler
{
public:
    CSystem(SharedEnvironmentInstance* pSharedEnvironment);
    ~CSystem();

    static void OnLanguageCVarChanged(ICVar* language);
    static void OnLanguageAudioCVarChanged(ICVar* language);
    static void OnLocalizationFolderCVarChanged(ICVar* const pLocalizationFolder);
    // adding CVAR to toggle assert verbosity level
    static void OnAssertLevelCvarChanged(ICVar* pArgs);
    static void SetAssertLevel(int _assertlevel);
    static void OnLogLevelCvarChanged(ICVar* pArgs);
    static void SetLogLevel(int _logLevel);

    // interface ILoadConfigurationEntrySink ----------------------------------

    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup);

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

    ///////////////////////////////////////////////////////////////////////////
    //! @name ISystem implementation
    //@{
    virtual bool Init(const SSystemInitParams& startupParams);
    virtual void Release();

    virtual SSystemGlobalEnvironment* GetGlobalEnvironment() { return &m_env; }

    virtual bool UpdatePreTickBus(int updateFlags = 0, int nPauseMode = 0);
    virtual bool UpdatePostTickBus(int updateFlags = 0, int nPauseMode = 0);
    virtual bool UpdateLoadtime();

    ////////////////////////////////////////////////////////////////////////
    // CrySystemRequestBus interface implementation
    ISystem* GetCrySystem() override;
    ////////////////////////////////////////////////////////////////////////

    void Relaunch(bool bRelaunch);
    bool IsRelaunch() const { return m_bRelaunch; };

    void SerializingFile(int mode) { m_iLoadingMode = mode; }
    int  IsSerializingFile() const { return m_iLoadingMode; }
    void Quit();
    bool IsQuitting() const;
    void ShutdownFileSystem(); // used to cleanup any file resources, such as cache handle.
    void SetAffinity();
    virtual const char* GetUserName();
    virtual int GetApplicationInstance();
    int GetApplicationLogInstance(const char* logFilePath) override;

    ITimer* GetITimer(){ return m_env.pTimer; }
    AZ::IO::IArchive* GetIPak() { return m_env.pCryPak; };
    IConsole* GetIConsole() { return m_env.pConsole; };
    IRemoteConsole* GetIRemoteConsole();
    IMovieSystem* GetIMovieSystem() { return m_env.pMovieSystem; };
    ICryFont* GetICryFont(){ return m_env.pCryFont; }
    ILog* GetILog(){ return m_env.pLog; }
    ICmdLine* GetICmdLine(){ return m_pCmdLine; }
    INameTable* GetINameTable() { return m_env.pNameTable; };
    IViewSystem* GetIViewSystem();
    ILevelSystem* GetILevelSystem();
    ISystemEventDispatcher* GetISystemEventDispatcher() { return m_pSystemEventDispatcher; }
    IProfilingSystem* GetIProfilingSystem() { return &m_ProfilingSystem; }
    //////////////////////////////////////////////////////////////////////////
    // retrieves the perlin noise singleton instance
    CPNoise3* GetNoiseGen();
    virtual uint64 GetUpdateCounter() { return m_nUpdateCounter; };

    void        DetectGameFolderAccessRights();

    virtual void ExecuteCommandLine(bool deferred=true);

    virtual void GetUpdateStats(SSystemUpdateStats& stats);

    //////////////////////////////////////////////////////////////////////////
    virtual XmlNodeRef CreateXmlNode(const char* sNodeName = "", bool bReuseStrings = false, bool bIsProcessingInstruction = false);
    virtual XmlNodeRef LoadXmlFromFile(const char* sFilename, bool bReuseStrings = false);
    virtual XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings = false, bool bSuppressWarnings = false);
    virtual IXmlUtils* GetXmlUtils();
    //////////////////////////////////////////////////////////////////////////

    void SetViewCamera(CCamera& Camera){ m_ViewCamera = Camera; }
    CCamera& GetViewCamera() { return m_ViewCamera; }

    void IgnoreUpdates(bool bIgnore) { m_bIgnoreUpdates = bIgnore; };

    void SetIProcess(IProcess* process);
    IProcess* GetIProcess(){ return m_pProcess; }

    bool IsTestMode() const { return m_bTestMode; }
    //@}

    void SleepIfNeeded();

    virtual void FatalError(const char* format, ...) PRINTF_PARAMS(2, 3);
    virtual void ReportBug(const char* format, ...) PRINTF_PARAMS(2, 3);
    // Validator Warning.
    void WarningV(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args);
    void Warning(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, ...);
    virtual int ShowMessage(const char* text, const char* caption, unsigned int uType);
    bool CheckLogVerbosity(int verbosity);

    //! Return pointer to user defined callback.
    ISystemUserCallback* GetUserCallback() const { return m_pUserCallback; };

    //////////////////////////////////////////////////////////////////////////
    virtual void SaveConfiguration();
    virtual void LoadConfiguration(const char* sFilename, ILoadConfigurationEntrySink* pSink = 0, bool warnIfMissing = true);
    virtual ESystemConfigSpec GetMaxConfigSpec() const;
    virtual ESystemConfigPlatform GetConfigPlatform() const;
    virtual void SetConfigPlatform(ESystemConfigPlatform platform);
    //////////////////////////////////////////////////////////////////////////

    virtual bool IsPaused() const { return m_bPaused; };

    virtual ILocalizationManager* GetLocalizationManager();
    virtual void debug_GetCallStack(const char** pFunctions, int& nCount);
    virtual void debug_LogCallStack(int nMaxFuncs = 32, int nFlags = 0);
    // Get the current callstack in raw address form (more lightweight than the above functions)
    // static as memReplay needs it before CSystem has been setup - expose a ISystem interface to this function if you need it outside CrySystem
    static  void debug_GetCallStackRaw(void** callstack, uint32& callstackLength);

public:
#if !defined(RELEASE)
    void SetVersionInfo(const char* const szVersion);
#endif

    void ShutdownModuleLibraries();

#if defined(WIN32)
    friend LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
    virtual void* GetRootWindowMessageHandler();
    virtual void RegisterWindowMessageHandler(IWindowMessageHandler* pHandler);
    virtual void UnregisterWindowMessageHandler(IWindowMessageHandler* pHandler);

    // IWindowMessageHandler
#if defined(WIN32)
    virtual bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
#endif
    // ~IWindowMessageHandler

private:

    // Release all resources.
    void ShutDown();

    bool LoadEngineDLLs();

    //! @name Initialization routines
    //@{
    bool InitConsole();
    bool InitFileSystem();
    bool InitFileSystem_LoadEngineFolders(const SSystemInitParams& initParams);
    bool InitAudioSystem(const SSystemInitParams& initParams);

    //@}

    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    void CreateSystemVars();
    void CreateAudioVars();

    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadDLL(const char* dllName);

    void FreeLib(AZStd::unique_ptr<AZ::DynamicModuleHandle>& hLibModule);

    bool UnloadDLL(const char* dllName);
    void QueryVersionInfo();
    void LogVersion();
    void LogBuildInfo();
    void SetDevMode(bool bEnable);

#ifndef _RELEASE
    static void SystemVersionChanged(ICVar* pCVar);
#endif // #ifndef _RELEASE

    bool ReLaunchMediaCenter();
    void UpdateAudioSystems();

    void AddCVarGroupDirectory(const string& sPath);

    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadDynamiclibrary(const char* dllName) const;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_3
#include AZ_RESTRICTED_FILE(System_h)
#elif defined(WIN32)
    bool GetWinGameFolder(char* szMyDocumentsPath, int maxPathSize);
#endif

public:
    void EnableFloatExceptions(int type);

    // interface ISystem -------------------------------------------
    virtual IDataProbe* GetIDataProbe() { return m_pDataProbe; };
    virtual void SetForceNonDevMode(bool bValue);
    virtual bool GetForceNonDevMode() const;
    virtual bool WasInDevMode() const { return m_bWasInDevMode; };
    virtual bool IsDevMode() const { return m_bInDevMode && !GetForceNonDevMode(); }

    virtual void SetConsoleDrawEnabled(bool enabled) { m_bDrawConsole = enabled; }
    virtual void SetUIDrawEnabled(bool enabled) { m_bDrawUI = enabled; }

    // -------------------------------------------------------------

    //! attaches the given variable to the given container;
    //! recreates the variable if necessary
    ICVar* attachVariable (const char* szVarName, int* pContainer, const char* szComment, int dwFlags = 0);

    const CTimeValue& GetLastTickTime(void) const { return m_lastTickTime; }
    const ICVar* GetDedicatedMaxRate(void) const { return m_svDedicatedMaxRate; }

    std::shared_ptr<AZ::IO::FileIOBase> CreateLocalFileIO();

private: // ------------------------------------------------------

    // System environment.
    SSystemGlobalEnvironment m_env;

    CTimer                              m_Time;                             //!<
    CCamera                             m_ViewCamera;                   //!<
    bool                                    m_bInitializedSuccessfully;     //!< true if the system completed all initialization steps
    bool                                    m_bRelaunch;                    //!< relaunching the app or not (true beforerelaunch)
    int                                     m_iLoadingMode;             //!< Game is loading w/o changing context (0 not, 1 quickloading, 2 full loading)
    bool                                    m_bTestMode;                    //!< If running in testing mode.
    bool                                    m_bEditor;                      //!< If running in Editor.
    bool                                    m_bNoCrashDialog;
    bool                                    m_bNoErrorReportWindow;
    bool                  m_bPreviewMode;       //!< If running in Preview mode.
    bool                                    m_bDedicatedServer;     //!< If running as Dedicated server.
    bool                                    m_bIgnoreUpdates;           //!< When set to true will ignore Update and Render calls,
    bool                                    m_bForceNonDevMode;     //!< true when running on a cheat protected server or a client that is connected to it (not used in singlplayer)
    bool                                    m_bWasInDevMode;            //!< Set to true if was in dev mode.
    bool                                    m_bInDevMode;                   //!< Set to true if was in dev mode.
    bool                  m_bGameFolderWritable;//!< True when verified that current game folder have write access.
    int                                     m_ttMemStatSS;              //!< Time to memstat screenshot
    bool                                    m_bDrawConsole;              //!< Set to true if OK to draw the console.
    bool                                    m_bDrawUI;                   //!< Set to true if OK to draw UI.


    std::map<CCryNameCRC, AZStd::unique_ptr<AZ::DynamicModuleHandle> > m_moduleDLLHandles;

    //! current active process
    IProcess* m_pProcess;

    CCamera m_PhysRendererCamera;
    ICVar* m_p_draw_helpers_str;
    int m_iJumpToPhysProfileEnt;

    CTimeValue m_lastTickTime;

    //! system event dispatcher
    ISystemEventDispatcher* m_pSystemEventDispatcher;

    //! The default mono-spaced font for internal usage (profiling, debug info, etc.)
    IFFont* m_pIFont;

    //! The default font for end-user UI interfaces
    IFFont* m_pIFontUi;

    //! System to manage levels.
    ILevelSystem* m_pLevelSystem;

    //! System to manage views.
    IViewSystem* m_pViewSystem;

    // XML Utils interface.
    class CXmlUtils* m_pXMLUtils;

    int m_iApplicationInstance;

    //! to hold the values stored in system.cfg
    //! because editor uses it's own values,
    //! and then saves them to file, overwriting the user's resolution.
    int m_iHeight;
    int m_iWidth;
    int m_iColorBits;

    // System console variables.
    //////////////////////////////////////////////////////////////////////////

    // DLL names
    ICVar* m_sys_dll_response_system;
#if !defined(_RELEASE)
    ICVar* m_sys_resource_cache_folder;
#endif

#if AZ_LOADSCREENCOMPONENT_ENABLED
    ICVar* m_game_load_screen_uicanvas_path;
    ICVar* m_level_load_screen_uicanvas_path;
    ICVar* m_game_load_screen_sequence_to_auto_play;
    ICVar* m_level_load_screen_sequence_to_auto_play;
    ICVar* m_game_load_screen_sequence_fixed_fps;
    ICVar* m_level_load_screen_sequence_fixed_fps;
    ICVar* m_game_load_screen_max_fps;
    ICVar* m_level_load_screen_max_fps;
    ICVar* m_game_load_screen_minimum_time{};
    ICVar* m_level_load_screen_minimum_time{};
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    ICVar* m_sys_initpreloadpacks;
    ICVar* m_sys_menupreloadpacks;

    ICVar* m_cvAIUpdate;
    ICVar* m_rWidth;
    ICVar* m_rHeight;
    ICVar* m_rWidthAndHeightAsFractionOfScreenSize;
    ICVar* m_rTabletWidthAndHeightAsFractionOfScreenSize;
    ICVar* m_rHDRDolby;
    ICVar* m_rMaxWidth;
    ICVar* m_rMaxHeight;
    ICVar* m_rColorBits;
    ICVar* m_rDepthBits;
    ICVar* m_rStencilBits;
    ICVar* m_rFullscreen;
    ICVar* m_rFullscreenWindow;
    ICVar* m_rFullscreenNativeRes;
    ICVar* m_rDisplayInfo;
    ICVar* m_rOverscanBordersDrawDebugView;
    ICVar* m_sysNoUpdate;
    ICVar* m_cvEntitySuppressionLevel;
    ICVar* m_pCVarQuit;
    ICVar* m_cvMemStats;
    ICVar* m_cvMemStatsThreshold;
    ICVar* m_cvMemStatsMaxDepth;
    ICVar* m_sysKeyboard;
    ICVar* m_sysWarnings;                                       //!< might be 0, "sys_warnings" - Treat warning as errors.
    ICVar* m_cvSSInfo;                                          //!< might be 0, "sys_SSInfo" 0/1 - get file sourcesafe info
    ICVar* m_svDedicatedMaxRate;
    ICVar* m_sys_firstlaunch;
    ICVar* m_sys_asset_processor;
    ICVar* m_sys_load_files_to_memory;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_4
#include AZ_RESTRICTED_FILE(System_h)
#endif

    ICVar* m_sys_audio_disable;

    ICVar* m_sys_min_step;
    ICVar* m_sys_max_step;
    ICVar* m_sys_enable_budgetmonitoring;
    ICVar* m_sys_memory_debug;
    ICVar* m_sys_preload;

    //  ICVar *m_sys_filecache;
    ICVar* m_gpu_particle_physics;

    string  m_sSavedRDriver;                                //!< to restore the driver when quitting the dedicated server

    //////////////////////////////////////////////////////////////////////////
    //! User define callback for system events.
    ISystemUserCallback* m_pUserCallback;

    SFileVersion m_fileVersion;
    SFileVersion m_productVersion;
    SFileVersion m_buildVersion;
    IDataProbe* m_pDataProbe;

    class CLocalizedStringsManager* m_pLocalizationManager;

    // Name table.
    CNameTable m_nameTable;

    ESystemConfigSpec m_nServerConfigSpec;
    ESystemConfigSpec m_nMaxConfigSpec;
    ESystemConfigPlatform m_ConfigPlatform;

    CProfilingSystem m_ProfilingSystem;

    // Pause mode.
    bool m_bPaused;
    bool m_bNoUpdate;

    uint64 m_nUpdateCounter;

    bool m_executedCommandLine = false;

    AZStd::unique_ptr<AzFramework::MissingAssetLogger> m_missingAssetLogger;

public:
    ICVar* m_sys_main_CPU;
    ICVar* m_sys_streaming_CPU;
    ICVar* m_sys_TaskThread_CPU[MAX_THREAD_POOL_INDEX];

    //////////////////////////////////////////////////////////////////////////
    // File version.
    //////////////////////////////////////////////////////////////////////////
    virtual const SFileVersion& GetFileVersion();
    virtual const SFileVersion& GetProductVersion();
    virtual const SFileVersion& GetBuildVersion();

    bool InitVTuneProfiler();

    void OpenBasicPaks();
    void OpenLanguagePak(const char* sLanguage);
    void OpenLanguageAudioPak(const char* sLanguage);
    void GetLocalizedPath(const char* sLanguage, string& sLocalizedPath);
    void GetLocalizedAudioPath(const char* sLanguage, string& sLocalizedPath);
    void CloseLanguagePak(const char* sLanguage);
    void CloseLanguageAudioPak(const char* sLanguage);
    void UpdateMovieSystem(const int updateFlags, const float fFrameTime, const bool bPreUpdate);

    //////////////////////////////////////////////////////////////////////////
    // CryAssert and error related.
    virtual bool RegisterErrorObserver(IErrorObserver* errorObserver);
    bool UnregisterErrorObserver(IErrorObserver* errorObserver);
    virtual void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber);
    void OnFatalError(const char* message);

    bool IsAssertDialogVisible() const;
    void SetAssertVisible(bool bAssertVisble);
    //////////////////////////////////////////////////////////////////////////

    virtual void ClearErrorMessages()
    {
        m_ErrorMessages.clear();
    }

    bool IsLoading()
    {
        return m_eRuntimeState == ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN;
    }

    virtual ESystemGlobalState  GetSystemGlobalState(void);
    virtual void SetSystemGlobalState(ESystemGlobalState systemGlobalState);

#if !defined(_RELEASE)
    virtual bool IsSavingResourceList() const { return (g_cvars.archiveVars.nSaveLevelResourceList != 0); }
#endif

private:
    std::vector<IErrorObserver*> m_errorObservers;
    ESystemGlobalState m_systemGlobalState;
    static const char* GetSystemGlobalStateName(const ESystemGlobalState systemGlobalState);

public:
    void InitLocalization();

protected: // -------------------------------------------------------------

    CCmdLine*                                      m_pCmdLine;

    string  m_currentLanguageAudio;
    string  m_systemConfigName; // computed from system_(hardwareplatform)_(assetsPlatform) - eg, system_android_android.cfg or system_windows_pc.cfg

    std::vector< std::pair<CTimeValue, float> > m_updateTimes;

    struct SErrorMessage
    {
        string m_Message;
        float m_fTimeToShow;
        float m_Color[4];
        bool m_HardFailure;
    };
    typedef std::list<SErrorMessage> TErrorMessages;
    TErrorMessages m_ErrorMessages;
    bool m_bHasRenderedErrorMessage;

    ESystemEvent m_eRuntimeState;
    bool m_bIsAsserting;

    std::vector<IWindowMessageHandler*> m_windowMessageHandlers;
    bool m_initedOSAllocator = false;
    bool m_initedSysAllocator = false;
};
