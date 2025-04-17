/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>

#ifdef CRYSYSTEM_EXPORTS
#define CRYSYSTEM_API AZ_DLL_EXPORT
#else
#define CRYSYSTEM_API AZ_DLL_IMPORT
#endif
#include <AzCore/IO/SystemFile.h>

#include "CryAssert.h"
#include <CryCommon/IValidator.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define ISYSTEM_H_SECTION_1 1
#define ISYSTEM_H_SECTION_2 2
#define ISYSTEM_H_SECTION_3 3
#define ISYSTEM_H_SECTION_4 4
#define ISYSTEM_H_SECTION_5 5
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////
#include <IXml.h> // <> required for Interfuscator
#include <ILog.h> // <> required for Interfuscator
#include "CryVersion.h"
#include "smartptr.h"
#include <memory> // shared_ptr
#include <CrySystemBus.h>

struct ISystem;
struct ILog;
namespace AZ::IO
{
    struct IArchive;
}
struct IConsole;
struct IRemoteConsole;
struct IRenderer;
struct ICryFont;
struct IMovieSystem;
struct SFileVersion;
struct INameTable;
struct ILevelSystem;
class IXMLBinarySerializer;
struct IAVI_Reader;
struct ILocalizationManager;
struct IOutputPrintSink;
struct IWindowMessageHandler;

namespace AZ
{
    namespace IO
    {
        class FileIOBase;
    }
}

typedef void* WIN_HWND;

struct CLoadingTimeProfiler;

class ICmdLine;
class ILyShine;

enum EValidatorModule : int;
enum EValidatorSeverity : int;

enum ESystemUpdateFlags
{
    // Summary:
    //   Special update mode for editor.
    ESYSUPDATE_EDITOR = 0x0004
};

// Description:
//   Configuration platform. Autodetected at start, can be modified through the editor.
enum ESystemConfigPlatform
{
    CONFIG_INVALID_PLATFORM = 0,
    CONFIG_PC = 1,
    CONFIG_MAC = 2,
    CONFIG_OSX_METAL = 3,
    CONFIG_ANDROID = 4,
    CONFIG_IOS = 5,
    CONFIG_PROVO = 7,
    CONFIG_SALEM = 8,
    CONFIG_JASPER = 9,

    END_CONFIG_PLATFORM_ENUM, // MUST BE LAST VALUE. USED FOR ERROR CHECKING.
};

enum ESystemGlobalState
{
    ESYSTEM_GLOBAL_STATE_UNKNOWN,
    ESYSTEM_GLOBAL_STATE_INIT,
    ESYSTEM_GLOBAL_STATE_RUNNING,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PREPARE,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_MATERIALS,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_OBJECTS,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_STATIC_WORLD,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PRECACHE,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_TEXTURES,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_END,
    ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE
};

// Summary:
//   System wide events.
enum ESystemEvent
{
    // Description:
    // Seeds all random number generators to the same seed number, WParam will hold seed value.
    //##@{
    ESYSTEM_EVENT_RANDOM_SEED = 1,
    ESYSTEM_EVENT_RANDOM_ENABLE,
    ESYSTEM_EVENT_RANDOM_DISABLE,
    //##@}

    // Description:
    //   Changes to main window focus.
    //   wparam is not 0 is focused, 0 if not focused
    ESYSTEM_EVENT_CHANGE_FOCUS = 10,

    // Description:
    //   Moves of the main window.
    //   wparam=x, lparam=y
    ESYSTEM_EVENT_MOVE = 11,

    // Description:
    //   Resizes of the main window.
    //   wparam=width, lparam=height
    ESYSTEM_EVENT_RESIZE = 12,

    // Description:
    //   Activation of the main window.
    //   wparam=1/0, 1=active 0=inactive
    ESYSTEM_EVENT_ACTIVATE = 13,

    // Description:
    //   Main window position changed.
    ESYSTEM_EVENT_POS_CHANGED = 14,

    // Description:
    //   Main window style changed.
    ESYSTEM_EVENT_STYLE_CHANGED = 15,

    // Description:
    //   Sent before the loading movie is begun
    ESYSTEM_EVENT_LEVEL_LOAD_START_PRELOADINGSCREEN,

    // Description:
    //   Sent before the loading last save
    ESYSTEM_EVENT_LEVEL_LOAD_RESUME_GAME,

    // Description:
    //   Sent before starting level, before game rules initialization and before ESYSTEM_EVENT_LEVEL_LOAD_START event
    //   Used mostly for level loading profiling
    ESYSTEM_EVENT_LEVEL_LOAD_PREPARE,

    // Description:
    //   Sent to start the active loading screen rendering.
    ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN,

    // Description:
    //   Sent when loading screen is active
    ESYSTEM_EVENT_LEVEL_LOAD_LOADINGSCREEN_ACTIVE,

    // Description:
    //   Sent before starting loading a new level.
    //   Used for a more efficient resource management.
    ESYSTEM_EVENT_LEVEL_LOAD_START,

    // Description:
    //   Sent after loading a level finished.
    //   Used for a more efficient resource management.
    ESYSTEM_EVENT_LEVEL_LOAD_END,

    // Description:
    //   Sent after trying to load a level failed.
    //   Used for resetting the front end.
    ESYSTEM_EVENT_LEVEL_LOAD_ERROR,

    // Description:
    //   Sent in case the level was requested to load, but it's not ready
    //   Used in streaming install scenario for notifying the front end.
    ESYSTEM_EVENT_LEVEL_NOT_READY,

    // Description:
    //   Sent after precaching of the streaming system has been done
    ESYSTEM_EVENT_LEVEL_PRECACHE_START,

    // Description:
    //   Sent before object/texture precache stream requests are submitted
    ESYSTEM_EVENT_LEVEL_PRECACHE_FIRST_FRAME,

    // Description:
    //  Sent when level loading is completely finished with no more onscreen
    //  movie or info rendering, and when actual gameplay can start
    ESYSTEM_EVENT_LEVEL_GAMEPLAY_START,

    // Level is unloading.
    ESYSTEM_EVENT_LEVEL_UNLOAD,

    // Summary:
    //   Sent after level have been unloaded. For cleanup code.
    ESYSTEM_EVENT_LEVEL_POST_UNLOAD,

    // Summary:
    //   Called when the game framework has been initialized.
    ESYSTEM_EVENT_GAME_POST_INIT,

    // Summary:
    //   Called when the game framework has been initialized, not loading should happen in this event.
    ESYSTEM_EVENT_GAME_POST_INIT_DONE,

    // Summary:
    //   Sent when the system is doing a full shutdown.
    ESYSTEM_EVENT_FULL_SHUTDOWN,

    // Summary:
    //   Sent when the system is doing a fast shutdown.
    ESYSTEM_EVENT_FAST_SHUTDOWN,

    // Summary:
    //   When keyboard layout changed.
    ESYSTEM_EVENT_LANGUAGE_CHANGE,

    // Description:
    //   Toggled fullscreen.
    //   wparam is 1 means we switched to fullscreen, 0 if for windowed
    ESYSTEM_EVENT_TOGGLE_FULLSCREEN,
    ESYSTEM_EVENT_SHARE_SHADER_COMBINATIONS,

    // Summary:
    //   Start 3D post rendering
    ESYSTEM_EVENT_3D_POST_RENDERING_START,

    // Summary:
    //   End 3D post rendering
    ESYSTEM_EVENT_3D_POST_RENDERING_END,

    // Summary:
    //   Called before switching to level memory heap
    ESYSTEM_EVENT_SWITCHING_TO_LEVEL_HEAP_DEPRECATED,

    // Summary:
    //   Called after switching to level memory heap
    ESYSTEM_EVENT_SWITCHED_TO_LEVEL_HEAP_DEPRECATED,

    // Summary:
    //   Called before switching to global memory heap
    ESYSTEM_EVENT_SWITCHING_TO_GLOBAL_HEAP_DEPRECATED,

    // Summary:
    //   Called after switching to global memory heap
    ESYSTEM_EVENT_SWITCHED_TO_GLOBAL_HEAP_DEPRECATED,

    // Description:
    //   Sent after precaching of the streaming system has been done
    ESYSTEM_EVENT_LEVEL_PRECACHE_END,

    // Description:
    //      Sent when game mode switch begins
    ESYSTEM_EVENT_GAME_MODE_SWITCH_START,

    // Description:
    //      Sent when game mode switch ends
    ESYSTEM_EVENT_GAME_MODE_SWITCH_END,

    // Description:
    //   Video notifications
    //   wparam=[0/1/2/3] : [stop/play/pause/resume]
    ESYSTEM_EVENT_VIDEO,

    // Description:
    //   Sent if the game is paused
    ESYSTEM_EVENT_GAME_PAUSED,

    // Description:
    //   Sent if the game is resumed
    ESYSTEM_EVENT_GAME_RESUMED,

    // Description:
    //      Sent when time of day is set
    ESYSTEM_EVENT_TIME_OF_DAY_SET,

    // Description:
    //      Sent once the Editor finished initialization.
    ESYSTEM_EVENT_EDITOR_ON_INIT,

    // Description:
    //      Sent when frontend is initialised
    ESYSTEM_EVENT_FRONTEND_INITIALISED,

    // Description:
    //      Sent once the Editor switches between in-game and editing mode.
    ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED,

    // Description:
    //      Sent once the Editor switches simulation mode (AI/Physics).
    ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED,

    // Description:
    //      Sent when frontend is reloaded
    ESYSTEM_EVENT_FRONTEND_RELOADED,

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION ISYSTEM_H_SECTION_1
    #include AZ_RESTRICTED_FILE(ISystem_h)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION ISYSTEM_H_SECTION_2
    #include AZ_RESTRICTED_FILE(ISystem_h)
#endif
    ESYSTEM_EVENT_STREAMING_INSTALL_ERROR,

    // Description:
    //      Sent when the online services are initialized.
    ESYSTEM_EVENT_ONLINE_SERVICES_INITIALISED,

    // Description:
    //      Sent when a new audio implementation is loaded
    ESYSTEM_EVENT_AUDIO_IMPLEMENTATION_LOADED,

    // Description:
    //      Sent when simulation mode switch begins
    ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_SWITCH_START,

    // Description:
    //      Sent when simluation mode switch ends
    ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_SWITCH_END,

    ESYSTEM_EVENT_USER = 0x1000,

    ESYSTEM_BEAM_PLAYER_TO_CAMERA_POS
};

// Description:
//   User defined callback, which can be passed to ISystem.
struct ISystemUserCallback
{
    // <interfuscator:shuffle>
    virtual ~ISystemUserCallback() {}
    // Description:
    //   This method is called at the earliest point the ISystem pointer can be used
    //   the log might not be yet there.
    virtual void OnSystemConnect([[maybe_unused]] ISystem* pSystem) {}

    // Summary:
    //   Signals to User that engine error occurred.
    // Return Value:
    //      True to Halt execution or false to ignore this error
    virtual bool OnError(const char* szErrorString) = 0;

    // Notes:
    //   If working in Editor environment notify user that engine want to Save current document.
    //   This happens if critical error have occurred and engine gives a user way to save data and not lose it
    //   due to crash.
    virtual bool OnSaveDocument() = 0;

    // Notes:
    //  If working in Editor environment and a critical error occurs notify the user to backup
    //  the current document to prevent data loss due to crash.
    virtual bool OnBackupDocument() = 0;

    // Description:
    //   Notifies user that system wants to switch out of current process.
    // Example:
    //  Called when pressing ESC in game mode to go to Menu.
    virtual void OnProcessSwitch() = 0;

    // Description:
    //   Notifies user, usually editor, about initialization progress in system.
    virtual void OnInitProgress(const char* sProgressMsg) = 0;

    // Description:
    //   Initialization callback.  This is called early in CSystem::Init(), before
    //   any of the other callback methods is called.
    // See also:
    //   CSystem::Init()
    virtual void OnInit(ISystem*) { }

    // Summary:
    //   Shutdown callback.
    virtual void OnShutdown() { }

    // Summary:
    //   Quit callback.
    // See also:
    //   CSystem::Quit()
    virtual void OnQuit() { }

    // Description:
    //   Notify user of an update iteration.  Called in the update loop.
    virtual void OnUpdate() { }

    // Description:
    //   Show message by provider.
    virtual void ShowMessage(const char* text, const char* caption, unsigned int uType) { CryMessageBox(text, caption, uType); }

    // </interfuscator:shuffle>

    //   Post console load, for cvar setting
    virtual void OnConsoleCreated([[maybe_unused]] ::IConsole* pConsole) {}
};

// Description:
//   Interface used for getting notified when a system event occurs.
struct ISystemEventListener
{
    // <interfuscator:shuffle>
    virtual ~ISystemEventListener() {}
    virtual void OnSystemEventAnyThread([[maybe_unused]] ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam) {}
    virtual void OnSystemEvent([[maybe_unused]] ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam) { }
    // </interfuscator:shuffle>
};

// Description:
//   Structure used for getting notified when a system event occurs.
struct ISystemEventDispatcher
{
    // <interfuscator:shuffle>
    virtual ~ISystemEventDispatcher() {}
    virtual bool RegisterListener(ISystemEventListener* pListener) = 0;
    virtual bool RemoveListener(ISystemEventListener* pListener) = 0;

    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) = 0;
    virtual void Update() = 0;

    //virtual void OnLocaleChange() = 0;
    // </interfuscator:shuffle>
};

struct IErrorObserver
{
    // <interfuscator:shuffle>
    virtual ~IErrorObserver() {}
    virtual void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber) = 0;
    virtual void OnFatalError(const char* message) = 0;
    // </interfuscator:shuffle>
};

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION ISYSTEM_H_SECTION_3
    #include AZ_RESTRICTED_FILE(ISystem_h)
#endif

namespace AZ
{
    namespace Internal
    {
        class EnvironmentInterface;
    } // namespace Internal
} // namespace AZ

// Description:
//  Structure passed to Init method of ISystem interface.
struct SSystemInitParams
{
    void* hInstance;                                //
    void* hWnd;                                     //

    ILog* pLog;                                     // You can specify your own ILog to be used by System.
    ILogCallback* pLogCallback;                     // You can specify your own ILogCallback to be added on log creation (used by Editor).
    ISystemUserCallback* pUserCallback;
    const char* sLogFileName;                       // File name to use for log.
    bool autoBackupLogs;                            // if true, logs will be automatically backed up each startup
    IOutputPrintSink* pPrintSync;               // Print Sync which can be used to catch all output from engine
    char szSystemCmdLine[2048];                     // Command line.

    bool bEditor;                                   // When running in Editor mode.
    bool bPreview;                                  // When running in Preview mode (Minimal initialization).
    bool bTestMode;                                 // When running in Automated testing mode.
    bool bDedicatedServer;                          // When running a dedicated server.
    bool bSkipConsole;                                  // Don't create console
    bool bUnattendedMode;                           // When running as part of a build on build-machines: Prevent popping up of any dialog
    bool bSkipMovie;            // Don't load movie

    bool bToolMode;                                 // System is running inside a tool. Will not create USER directory or anything else that the game needs to do

    ISystem* pSystem;                                           // Pointer to existing ISystem interface, it will be reused if not NULL.

    // Summary:
    //  Initialization defaults.
    SSystemInitParams()
    {
        hInstance = NULL;
        hWnd = NULL;

        pLog = NULL;
        pLogCallback = NULL;
        pUserCallback = NULL;
        sLogFileName = NULL;
        autoBackupLogs = true;
        pPrintSync = NULL;
        memset(szSystemCmdLine, 0, sizeof(szSystemCmdLine));

        bEditor = false;
        bPreview = false;
        bTestMode = false;
        bDedicatedServer = false;
        bSkipConsole = false;
        bUnattendedMode = false;
        bSkipMovie = false;
        bToolMode = false;

        pSystem = NULL;
    }
};

// Notes:
//   Can be used for LoadConfiguration().
// See also:
//   LoadConfiguration()
struct ILoadConfigurationEntrySink
{
    // <interfuscator:shuffle>
    virtual ~ILoadConfigurationEntrySink() {}
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup) = 0;
    virtual void OnLoadConfigurationEntry_End() {}
    // </interfuscator:shuffle>
};

struct SPlatformInfo
{
    unsigned int numCoresAvailableToProcess;
    unsigned int numLogicalProcessors;

#if defined(WIN32) || defined(WIN64)
    enum EWinVersion
    {
        WinUndetected,
        Win2000,
        WinXP,
        WinSrv2003,
        WinVista,
        Win7,
        Win8,
        Win81,
        Win10
    };

    EWinVersion winVer;
    bool win64Bit;
    bool vistaKB940105Required;
#endif
};

// Description:
//  Holds info about system update stats over perior of time (cvar-tweakable)

struct SSystemUpdateStats
{
    SSystemUpdateStats()
        : avgUpdateTime(0.0f)
        , minUpdateTime(0.0f)
        , maxUpdateTime(0.0f) {}
    float avgUpdateTime;
    float minUpdateTime;
    float maxUpdateTime;
};

// Description:
//   Global environment.
//   Contains pointers to all global often needed interfaces.
//    This is a faster way to get interface pointer then calling ISystem interface to retrieve one.
// Notes:
//   Some pointers can be NULL, use with care.
// See also:
//   ISystem
struct SSystemGlobalEnvironment
{
    AZ::IO::IArchive*          pCryPak;
    AZ::IO::FileIOBase*        pFileIO;
    ICryFont*                  pCryFont;
    ::IConsole*                  pConsole;
    ISystem*                   pSystem = nullptr;
    ILog*                      pLog;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION ISYSTEM_H_SECTION_4
    #include AZ_RESTRICTED_FILE(ISystem_h)
#endif

    threadID                                 mMainThreadId;     //The main thread ID is used in multiple systems so should be stored globally

    //////////////////////////////////////////////////////////////////////////
    // Used by CRY_ASSERT
    bool                                            bIgnoreAllAsserts;
    bool                                            bNoAssertDialog;
    //////////////////////////////////////////////////////////////////////////

    bool                                            bToolMode;

    int                                             retCode = 0;

    ILINE const bool IsDedicated() const
    {
#if defined(CONSOLE)
        return false;
#else
        return bDedicated;
#endif
    }

#if !defined(CONSOLE)
    ILINE void SetIsEditor(bool isEditor)
    {
        bEditor = isEditor;
    }

    ILINE void SetIsEditorGameMode(bool isEditorGameMode)
    {
        bEditorGameMode = isEditorGameMode;
    }

    ILINE void SetIsEditorSimulationMode(bool isEditorSimulationMode)
    {
        bEditorSimulationMode = isEditorSimulationMode;
    }

    ILINE void SetIsDedicated(bool isDedicated)
    {
        bDedicated = isDedicated;
    }
#endif

    //this way the compiler can strip out code for consoles
    ILINE const bool IsEditor() const
    {
#if defined(CONSOLE)
        return false;
#else
        return bEditor;
#endif
    }

    ILINE const bool IsEditorGameMode() const
    {
#if defined(CONSOLE)
        return false;
#else
        return bEditorGameMode;
#endif
    }

    ILINE const bool IsEditorSimulationMode() const
    {
#if defined(CONSOLE)
        return false;
#else
        return bEditorSimulationMode;
#endif
    }

    ILINE const bool IsEditing() const
    {
#if defined(CONSOLE)
        return false;
#else
        return bEditor && !bEditorGameMode;
#endif
    }

    ILINE bool IsInToolMode() const
    {
        return bToolMode;
    }

    ILINE void SetToolMode(bool bNewToolMode)
    {
        bToolMode = bNewToolMode;
    }

#if !defined(CONSOLE)
private:
    bool bEditor;          // Engine is running under editor.
    bool bEditorGameMode;  // Engine is in editor game mode.
    bool bEditorSimulationMode;  // Engine is in editor simulation mode.
    bool bDedicated;             // Engine is in dedicated
#endif

public:
    SSystemGlobalEnvironment()
        : bToolMode(false)
    {
    };
};

// NOTE Nov 25, 2008: <pvl> the ISystem interface that follows has a member function
// called 'GetUserName'.  If we don't #undef'ine the same-named Win32 symbol here
// ISystem wouldn't even compile.
// TODO Nov 25, 2008: <pvl> there might be a better place for this?
#ifdef GetUserName
#undef GetUserName
#endif

////////////////////////////////////////////////////////////////////////////////////////////////

// Description:
//   Main Engine Interface.
//   Initialize and dispatch all engine's subsystems.
struct ISystem
{
    // <interfuscator:shuffle>
    virtual ~ISystem() {}
    // Summary:
    //   Releases ISystem.
    virtual void Release() = 0;

                                                                                  // Summary:
                                                                                  //   Returns pointer to the global environment structure.
    virtual SSystemGlobalEnvironment* GetGlobalEnvironment() = 0;

    // Summary:
    //   Updates all subsystems (including the ScriptSink() )
    // Arguments:
    //   flags      - One or more flags from ESystemUpdateFlags structure.
    //   nPauseMode - 0=normal(no pause), 1=menu/pause, 2=cutscene
    virtual bool UpdatePreTickBus(int updateFlags = 0, int nPauseMode = 0) = 0;

    // Summary:
    //   Updates all subsystems (including the ScriptSink() )
    // Arguments:
    //   flags      - One or more flags from ESystemUpdateFlags structure.
    //   nPauseMode - 0=normal(no pause), 1=menu/pause, 2=cutscene
    virtual bool UpdatePostTickBus(int updateFlags = 0, int nPauseMode = 0) = 0;

    // Summary:
    //   Updates only require components during loading
    virtual bool UpdateLoadtime() = 0;

    // Summary:
    //   Retrieve the name of the user currently logged in to the computer.
    virtual const char* GetUserName() = 0;

    // Summary:
    //   Quits the application.
    virtual void    Quit() = 0;
    // Summary:
    //   Tells the system if it is relaunching or not.
    virtual void    Relaunch(bool bRelaunch) = 0;
    // Summary:
    //   Returns true if the application is in the shutdown phase.
    virtual bool    IsQuitting() const = 0;
    // Summary:
    //   Tells the system in which way we are using the serialization system.
    virtual void  SerializingFile(int mode) = 0;
    virtual int IsSerializingFile() const = 0;

    virtual bool IsRelaunch() const = 0;

    // Description:
    //   Displays error message.
    //   Logs it to console and file and error message box then terminates execution.
    virtual void FatalError(const char* sFormat, ...) PRINTF_PARAMS(2, 3) = 0;

    // Description:
    //   Reports a bug using the crash handler.
    //   Logs an error to the console and launches the crash handler, then continues execution.
    virtual void ReportBug(const char* sFormat, ...) PRINTF_PARAMS(2, 3) = 0;

    // Description:
    //   Report warning to current Validator object.
    //   Doesn't terminate the execution.
    //##@{
    virtual void WarningV(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args) = 0;
    virtual void Warning(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, ...) = 0;
    //##@}

    // Description:
    //   Report message by provider or by using CryMessageBox.
    //   Doesn't terminate the execution.
    virtual void ShowMessage(const char* text, const char* caption, unsigned int uType) = 0;

    // Summary:
    //   Compare specified verbosity level to the one currently set.
    virtual bool CheckLogVerbosity(int verbosity) = 0;

    // return the related subsystem interface
    virtual ILevelSystem* GetILevelSystem() = 0;
    virtual ICmdLine* GetICmdLine() = 0;
    virtual ILog* GetILog() = 0;
    virtual AZ::IO::IArchive* GetIPak() = 0;
    virtual ICryFont* GetICryFont() = 0;
    virtual IMovieSystem* GetIMovieSystem() = 0;
    virtual ::IConsole* GetIConsole() = 0;
    virtual IRemoteConsole* GetIRemoteConsole() = 0;
    virtual ISystemEventDispatcher* GetISystemEventDispatcher() = 0;

    virtual bool IsDevMode() const = 0;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IXmlNode interface.
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Creates new xml node.
    virtual XmlNodeRef CreateXmlNode(const char* sNodeName = "", bool bReuseStrings = false, bool bIsProcessingInstruction = false) = 0;
    // Summary:
    //   Loads xml from memory buffer, returns 0 if load failed.
    virtual XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings = false, bool bSuppressWarnings = false) = 0;
    // Summary:
    //   Loads xml file, returns 0 if load failed.
    virtual XmlNodeRef LoadXmlFromFile(const char* sFilename, bool bReuseStrings = false) = 0;
    // Summary:
    //   Retrieves access to XML utilities interface.
    virtual IXmlUtils* GetXmlUtils() = 0;

    // Description:
    //   When ignore update sets to true, system will ignore and updates and render calls.
    virtual void IgnoreUpdates(bool bIgnore) = 0;

    // Return Value:
    //   True if system running in Test mode.
    virtual bool IsTestMode() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // File version.
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Gets file version.
    virtual const SFileVersion& GetFileVersion() = 0;
    // Summary:
    //   Gets product version.
    virtual const SFileVersion& GetProductVersion() = 0;
    // Summary:
    //   Gets build version.
    virtual const SFileVersion& GetBuildVersion() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Configuration.
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Loads configurations from CVarGroup directory recursively
    //   If m_GraphicsSettingsMap is defined (in Graphics Settings Dialog box), fills in mapping based on sys_spec_Full
    // Arguments:
    //   sPath - e.g. "Game/Config/CVarGroups"
    virtual void AddCVarGroupDirectory(const AZStd::string& sPath) = 0;

    // Summary:
    //   Saves system configuration.
    virtual void SaveConfiguration() = 0;

    // Summary:
    //   Loads system configuration
    // Arguments:
    //   pCallback - 0 means normal LoadConfigVar behaviour is used
    virtual void LoadConfiguration(const char* sFilename, ILoadConfigurationEntrySink* pSink = 0, bool warnIfMissing = true) = 0;

    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Retrieves current configuration platform
    virtual ESystemConfigPlatform GetConfigPlatform() const = 0;

    // Summary:
    //   Changes current configuration platform.
    virtual void SetConfigPlatform(ESystemConfigPlatform platform) = 0;
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Query if system is now paused.
    //   Pause flag is set when calling system update with pause mode.
    virtual bool IsPaused() const = 0;

    // Summary:
    //   Retrieves localized strings manager interface.
    virtual ILocalizationManager* GetLocalizationManager() = 0;


    //////////////////////////////////////////////////////////////////////////
    // Error callback handling

    // Summary:
    //  Registers listeners to CryAssert and error messages. (may not be called if asserts are disabled)
    //  Each pointer can be registered only once. (stl::push_back_unique)
    //  It will return false if the pointer is already registered. Returns true, otherwise.
    virtual bool RegisterErrorObserver(IErrorObserver* errorObserver) = 0;

    // Summary:
    //  Unregisters listeners to CryAssert and error messages.
    //  It will return false if the pointer is not registered. Otherwise, returns true.
    virtual bool UnregisterErrorObserver(IErrorObserver* errorObserver) = 0;

    // Summary:
    //  Called after the processing of the assert message box on some platforms.
    //  It will be called even when asserts are disabled by the console variables.
    virtual void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber) = 0;

    // Summary:
    // Returns if the assert window from CryAssert is visible.
    // OBS1: needed by the editor, as in some cases it can freeze if during an assert engine it will handle
    // some events such as mouse movement in a CryPhysics assert.
    // OBS2: it will always return false, if asserts are disabled or ignored.
    virtual bool IsAssertDialogVisible() const = 0;

    // Summary:
    // Sets the AssertVisisble internal variable.
    // Typically it should only be called by CryAssert.
    virtual void SetAssertVisible(bool bAssertVisble) = 0;
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //   Get the index of the currently running O3DE application. (0 = first instance, 1 = second instance, etc)
    virtual int GetApplicationInstance() = 0;

    // Summary:
    //   Get log index of the currently running Open 3D Engine application. (0 = first instance, 1 = second instance, etc)
    virtual int GetApplicationLogInstance(const char* logFilePath) = 0;

    // Summary:
    //      Clear all currently logged and drawn on screen error messages
    virtual void ClearErrorMessages() = 0;
    //////////////////////////////////////////////////////////////////////////
    // For debugging use only!, query current C++ call stack.
    //////////////////////////////////////////////////////////////////////////

    // Notes:
    //   Pass nCount to indicate maximum number of functions to get.
    //   For debugging use only, query current C++ call stack.
    // Description:
    //   Fills array of function pointers, nCount return number of functions.
    virtual void debug_GetCallStack(const char** pFunctions, int& nCount) = 0;
    // Summary:
    //   Logs current callstack.
    // Notes:
    //   For debugging use only!, query current C++ call stack.
    virtual void debug_LogCallStack(int nMaxFuncs = 32, int nFlags = 0) = 0;

    // Description:
    //   Execute command line arguments.
    //   Should be after init game.
    // Example:
    //   +g_gametype ASSAULT +LoadLevel "testy"
    virtual void ExecuteCommandLine(bool deferred=true) = 0;

    // Description:
    //  GetSystemUpdate stats (all systems update without except console)
    //  very useful on dedicated server as we throttle it to fixed frequency
    //  returns zeroes if no updates happened yet
    virtual void GetUpdateStats(SSystemUpdateStats& stats) = 0;

    virtual ESystemGlobalState  GetSystemGlobalState(void) = 0;
    virtual void SetSystemGlobalState(ESystemGlobalState systemGlobalState) = 0;

#if !defined(_RELEASE)
    virtual bool IsSavingResourceList() const = 0;
#endif

    // Summary:
    //      Register a IWindowMessageHandler that will be informed about window messages
    //      The delivered messages are platform-specific
    virtual void RegisterWindowMessageHandler(IWindowMessageHandler* pHandler) = 0;

    // Summary:
    //      Unregister an IWindowMessageHandler that was previously registered using RegisterWindowMessageHandler
    virtual void UnregisterWindowMessageHandler(IWindowMessageHandler* pHandler) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // EBus interface used to listen for cry system notifications
    class CrySystemNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~CrySystemNotifications() = default;

        // Override to be notified right before the call to ISystem::Render
        virtual void OnPreRender() {}

        // Override to be notified right after the call to ISystem::Render (but before RenderEnd)
        virtual void OnPostRender() {}
    };
    using CrySystemNotificationBus = AZ::EBus<CrySystemNotifications>;
};

//////////////////////////////////////////////////////////////////////////
// CrySystem DLL Exports.
//////////////////////////////////////////////////////////////////////////
typedef ISystem* (*PFNCREATESYSTEMINTERFACE)(SSystemInitParams& initParams);


//////////////////////////////////////////////////////////////////////////
// Global environment variable.
//////////////////////////////////////////////////////////////////////////
extern SSystemGlobalEnvironment* gEnv;


// Summary:
//   Gets the system interface.
inline ISystem* GetISystem()
{
    // Some unit tests temporarily install and then uninstall ISystem* mocks.
    // It is generally okay for runtime and tool systems which call this function to cache the returned pointer,
    // because their lifetime is usually shorter than the lifetime of the ISystem* implementation.
    // It is NOT safe for this function to cache it as a static itself, though, as the static it would cache
    // it inside may outlive the the actual instance implementing ISystem* when unit tests are torn down and then restarted.
    ISystem* systemInterface = gEnv ? gEnv->pSystem : nullptr;
    if (!systemInterface)
    {
        CrySystemRequestBus::BroadcastResult(systemInterface, &CrySystemRequests::GetCrySystem);
    }
    return systemInterface;
};
//////////////////////////////////////////////////////////////////////////

// Description:
//   This function must be called once by each module at the beginning, to setup global pointers.
void ModuleInitISystem(ISystem* pSystem, const char* moduleName);
void ModuleShutdownISystem(ISystem* pSystem);

void* GetModuleInitISystemSymbol();
void* GetModuleShutdownISystemSymbol();

#define PREVENT_MODULE_AND_ENVIRONMENT_SYMBOL_STRIPPING \
    AZ_UNUSED(GetModuleInitISystemSymbol()); \
    AZ_UNUSED(GetModuleShutdownISystemSymbol());


// Summary:
//   Interface of the DLL.
extern "C"
{
#if !defined(AZ_MONOLITHIC_BUILD)
    CRYSYSTEM_API
#endif
    ISystem* CreateSystemInterface(const SSystemInitParams& initParams);
}

// Description:
//   Displays error message.
//   Logs it to console and file and error message box.
//   Then terminates execution.
void CryFatalError(const char*, ...) PRINTF_PARAMS(1, 2);
inline void CryFatalError(const char* format, ...)
{
    if (!gEnv || !gEnv->pSystem)
    {
        return;
    }

    va_list ArgList;
    char szBuffer[MAX_WARNING_LENGTH];
    va_start(ArgList, format);
    int count = azvsnprintf(szBuffer, sizeof(szBuffer), format, ArgList);
    if (count == -1 || count >= sizeof(szBuffer))
    {
        szBuffer[sizeof(szBuffer) - 1] = '\0';
    }
    va_end(ArgList);

    gEnv->pSystem->FatalError("%s", szBuffer);
}

//////////////////////////////////////////////////////////////////////////

// Description:
//   Displays warning message.
//   Logs it to console and file and display a warning message box.
//   Doesn't terminate execution.
void CryWarning(EValidatorModule, EValidatorSeverity, const char*, ...) PRINTF_PARAMS(3, 4);
inline void CryWarning(EValidatorModule module, EValidatorSeverity severity, const char* format, ...)
{
    if (!gEnv || !gEnv->pSystem || !format)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    GetISystem()->WarningV(module, severity, 0, 0, format, args);
    va_end(args);
}

#ifdef EXCLUDE_CVARHELP
#define CVARHELP(_comment)  0
#else
#define CVARHELP(_comment)  _comment
#endif

//Provide macros for fixing cvars for release mode on consoles to enums to allow for code stripping
//Do not enable for PC, apply VF_CHEAT there if required
#if defined(CONSOLE)
#define CONST_CVAR_FLAGS (VF_CHEAT)
#else
#define CONST_CVAR_FLAGS (VF_NULL)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION ISYSTEM_H_SECTION_5
    #include AZ_RESTRICTED_FILE(ISystem_h)
#endif
#if defined(_RELEASE) && defined(IS_CONSOLE_PLATFORM)
#ifndef LOG_CONST_CVAR_ACCESS
#error LOG_CONST_CVAR_ACCESS should be defined in ProjectDefines.h
#endif

#include "IConsole.h"
namespace Detail
{
    template<typename T>
    struct SQueryTypeEnum;
    template<>
    struct SQueryTypeEnum<int>
    {
        static const int type = CVAR_INT;
        static int ParseString(const char* s) { return atoi(s); }
    };
    template<>
    struct SQueryTypeEnum<float>
    {
        static const int type = CVAR_FLOAT;
        static float ParseString(const char* s) { return (float)atof(s); }
    };

    template<typename T>
    struct SDummyCVar
        : ICVar
    {
        const T value;
#if LOG_CONST_CVAR_ACCESS
        mutable bool bWasRead;
        mutable bool bWasChanged;
        SDummyCVar(T val)
            : value(val)
            , bWasChanged(false)
            , bWasRead(false) {}
#else
        SDummyCVar(T val)
            : value(val) {}
#endif

        void WarnUse() const
        {
#if LOG_CONST_CVAR_ACCESS
            if (!bWasRead)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[CVAR] Read from const CVar '%s' via name look-up, this is non-optimal", GetName());
                bWasRead = true;
            }
#endif
        }

        void InvalidAccess() const
        {
#if LOG_CONST_CVAR_ACCESS
            if (!bWasChanged)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[CVAR] Write to const CVar '%s' with wrong value '%f' was ignored. This indicates a bug in code or a config file", GetName(), GetFVal());
                bWasChanged = true;
            }
#endif
        }

        void Release() {}
        int GetIVal() const { WarnUse(); return static_cast<int>(value); }
        int64 GetI64Val() const { WarnUse(); return static_cast<int64>(value); }
        float GetFVal() const { WarnUse(); return static_cast<float>(value); }
        const char* GetString() const { return ""; }
        const char* GetDataProbeString() const { return ""; }
        void Set(const char* s)
        {
            if (SQueryTypeEnum<T>::ParseString(s) != value)
            {
                InvalidAccess();
            }
        }
        void ForceSet(const char* s) { Set(s); }
        void Set(const float f)
        {
            if (static_cast<T>(f) != value)
            {
                InvalidAccess();
            }
        }
        void Set(const int i)
        {
            if (static_cast<T>(i) != value)
            {
                InvalidAccess();
            }
        }
        void ClearFlags([[maybe_unused]] int flags) {}
        int GetFlags() const { return VF_CONST_CVAR | VF_READONLY; }
        int SetFlags([[maybe_unused]] int flags) { return 0; }
        int GetType() { return SQueryTypeEnum<T>::type; }
        const char* GetHelp() { return NULL; }
        bool IsConstCVar() const { return true; }
        void SetOnChangeCallback(ConsoleVarFunc pChangeFunc) { (void)pChangeFunc; }
        bool AddOnChangeFunctor(AZ::Name, const AZStd::function<void()>&)
        {
            return false;
        }
        ConsoleVarFunc GetOnChangeCallback() const { InvalidAccess(); return NULL; }
        int GetRealIVal() const { return GetIVal(); }
        void SetLimits([[maybe_unused]] float min, [[maybe_unused]] float max) { return; }
        void GetLimits([[maybe_unused]] float& min, [[maybe_unused]] float& max) { return; }
        bool HasCustomLimits() { return false; }
        void SetDataProbeString([[maybe_unused]] const char* pDataProbeString) { InvalidAccess(); }
    };
}

#define REGISTER_DUMMY_CVAR(type, name, value)                                           \
    do {                                                                                 \
        static struct DummyCVar                                                          \
            : Detail::SDummyCVar<type>                                                   \
        {                                                                                \
            DummyCVar()                                                                  \
                : Detail::SDummyCVar<type>(value) {}                                     \
            const char* GetName() const { return name; }                                 \
        } DummyStaticInstance;                                                           \
        if (!(gEnv->pConsole != 0 ? gEnv->pConsole->Register(&DummyStaticInstance) : 0)) \
        {                                                                                \
            AZ::Debug::Trace::Instance().Break();                                        \
            CryFatalError("Can not register dummy CVar");                                \
        }                                                                                \
    } while (0)

# define CONSOLE_CONST_CVAR_MODE
# define DeclareConstIntCVar(name, defaultValue) enum : int { name = (defaultValue) }
# define DeclareStaticConstIntCVar(name, defaultValue) enum : int { name = (defaultValue) }

# define DefineConstIntCVarName(strname, name, defaultValue, flags, help) { static_assert((int)(defaultValue) == (int)(name)); REGISTER_DUMMY_CVAR(int, strname, defaultValue); }
# define DefineConstIntCVar(name, defaultValue, flags, help) { static_assert((int)(defaultValue) == (int)(name)); REGISTER_DUMMY_CVAR(int, (#name), defaultValue); }
// DefineConstIntCVar2 is deprecated, any such instance can be converted to the 3 variant by removing the quotes around the first parameter
# define DefineConstIntCVar3(name, _var_, defaultValue, flags, help) { static_assert((int)(defaultValue) == (int)(_var_)); REGISTER_DUMMY_CVAR(int, name, defaultValue); }
# define AllocateConstIntCVar(scope, name)

# define DefineConstFloatCVar(name, flags, help) { REGISTER_DUMMY_CVAR(float, (#name), name ## Default); }
# define DeclareConstFloatCVar(name)
# define DeclareStaticConstFloatCVar(name)
# define AllocateConstFloatCVar(scope, name)

# define IsCVarConstAccess(expr) expr

#else

# define DeclareConstIntCVar(name, defaultValue) int name { defaultValue }
# define DeclareStaticConstIntCVar(name, defaultValue) static int name
# define DefineConstIntCVarName(strname, name, defaultValue, flags, help) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register(strname, &name, defaultValue, flags | CONST_CVAR_FLAGS, CVARHELP(help)))
# define DefineConstIntCVar(name, defaultValue, flags, help) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register((#name), &name, defaultValue, flags | CONST_CVAR_FLAGS, CVARHELP(help), 0, false))
// DefineConstIntCVar2 is deprecated, any such instance can be converted to the 3 variant by removing the quotes around the first parameter
# define DefineConstIntCVar3(_name, _var, _def_val, _flags, help) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register(_name, &(_var), (_def_val), (_flags) | CONST_CVAR_FLAGS, CVARHELP(help), 0, false))
# define AllocateConstIntCVar(scope, name) int scope:: name

# define DefineConstFloatCVar(name, flags, help) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register((#name), &name, name ## Default, flags | CONST_CVAR_FLAGS, CVARHELP(help), 0, false))
# define DeclareConstFloatCVar(name) float name
# define DeclareStaticConstFloatCVar(name) static float name
# define AllocateConstFloatCVar(scope, name) float scope:: name

# define IsCVarConstAccess(expr)

#endif

// the following macros allow the help text to be easily stripped out

// Summary:
//   Preferred way to register a CVar
#define REGISTER_CVAR(_var, _def_val, _flags, _comment) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register((#_var), &(_var), (_def_val), (_flags), CVARHELP(_comment)))

// Summary:
//   Preferred way to register a CVar with a callback
#define REGISTER_CVAR_CB(_var, _def_val, _flags, _comment, _onchangefunction) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register((#_var), &(_var), (_def_val), (_flags), CVARHELP(_comment), _onchangefunction))

// Summary:
//   Preferred way to register a string CVar
#define REGISTER_STRING(_name, _def_val, _flags, _comment) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->RegisterString(_name, (_def_val), (_flags), CVARHELP(_comment)))

// Summary:
//   Preferred way to register a string CVar with a callback
#define REGISTER_STRING_CB(_name, _def_val, _flags, _comment, _onchangefunction) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->RegisterString(_name, (_def_val), (_flags), CVARHELP(_comment), _onchangefunction))

// Summary:
//   Preferred way to register an int CVar
#define REGISTER_INT(_name, _def_val, _flags, _comment) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->RegisterInt(_name, (_def_val), (_flags), CVARHELP(_comment)))

// Summary:
//   Preferred way to register an int CVar with a callback
#define REGISTER_INT_CB(_name, _def_val, _flags, _comment, _onchangefunction) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->RegisterInt(_name, (_def_val), (_flags), CVARHELP(_comment), _onchangefunction))

// Summary:
//   Preferred way to register a float CVar
#define REGISTER_FLOAT(_name, _def_val, _flags, _comment) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->RegisterFloat(_name, (_def_val), (_flags), CVARHELP(_comment)))

// Summary:
//   Offers more flexibility but more code is required
#define REGISTER_CVAR2(_name, _var, _def_val, _flags, _comment) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register(_name, _var, (_def_val), (_flags), CVARHELP(_comment)))

// Summary:
//   Offers more flexibility but more code is required
#define REGISTER_CVAR2_CB(_name, _var, _def_val, _flags, _comment, _onchangefunction) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register(_name, _var, (_def_val), (_flags), CVARHELP(_comment), _onchangefunction))

// Summary:
//   Offers more flexibility but more code is required, explicit address taking of destination variable
#define REGISTER_CVAR3(_name, _var, _def_val, _flags, _comment) \
    (gEnv->pConsole == 0 ? 0 : gEnv->pConsole->Register(_name, &(_var), (_def_val), (_flags), CVARHELP(_comment)))

// Summary:
//   Preferred way to register a console command
#define REGISTER_COMMAND(_name, _func, _flags, _comment) \
    (gEnv->pConsole == 0 ? false : gEnv->pConsole->AddCommand(_name, _func, (_flags), CVARHELP(_comment)))

// Summary:
//   Preferred way to unregister a CVar
#define UNREGISTER_CVAR(_name) \
    (gEnv->pConsole == 0 ? (void)0 : gEnv->pConsole->UnregisterVariable(_name))

// Summary:
//   Preferred way to unregister a console command
#define UNREGISTER_COMMAND(_name) \
    (gEnv->pConsole == 0 ? (void)0 : gEnv->pConsole->RemoveCommand(_name))

////////////////////////////////////////////////////////////////////////////////
//
// Development only cvars
//
// N.B:
// (1) Registered as real cvars *in non release builds only*.
// (2) Can still be manipulated in release by the mapped variable, so not the same as const cvars.
// (3) Any 'OnChanged' callback will need guarding against in release builds since the cvar won't exist
// (4) Any code that tries to get ICVar* will need guarding against in release builds since the cvar won't exist
//
// ILLEGAL_DEV_FLAGS is a mask of all those flags which make no sense in a _DEV_ONLY or _DEDI_ONLY cvar since the
// cvar potentially won't exist in a release build.
//
#define ILLEGAL_DEV_FLAGS (VF_NET_SYNCED | VF_CHEAT | VF_CHEAT_ALWAYS_CHECK | VF_CHEAT_NOCHECK | VF_READONLY | VF_CONST_CVAR)

#if defined(_RELEASE)
#define REGISTER_CVAR_DEV_ONLY(_var, _def_val, _flags, _comment)                                                               NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0); _var = _def_val
#define REGISTER_CVAR_CB_DEV_ONLY(_var, _def_val, _flags, _comment, _onchangefunction)                  NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0); _var = _def_val /* _onchangefunction consumed; callback not available */
#define REGISTER_STRING_DEV_ONLY(_name, _def_val, _flags, _comment)                                                        NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)                                  /* consumed; pure cvar not available */
#define REGISTER_STRING_CB_DEV_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)               NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)                                  /* consumed; pure cvar not available */
#define REGISTER_INT_DEV_ONLY(_name, _def_val, _flags, _comment)                                                               NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)                                  /* consumed; pure cvar not available */
#define REGISTER_INT_CB_DEV_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)                  NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)                                  /* consumed; pure cvar not available */
#define REGISTER_FLOAT_DEV_ONLY(_name, _def_val, _flags, _comment)                                                         NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)                                  /* consumed; pure cvar not available */
#define REGISTER_CVAR2_DEV_ONLY(_name, _var, _def_val, _flags, _comment)                                                NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0); *(_var) = _def_val
#define REGISTER_CVAR2_CB_DEV_ONLY(_name, _var, _def_val, _flags, _comment, _onchangefunction)       NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0); *(_var) = _def_val
#define REGISTER_CVAR3_DEV_ONLY(_name, _var, _def_val, _flags, _comment)                                                NULL; static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0); _var = _def_val
#define REGISTER_COMMAND_DEV_ONLY(_name, _func, _flags, _comment)                                                  /* consumed; command not available */
#else
#define REGISTER_CVAR_DEV_ONLY(_var, _def_val, _flags, _comment)                                                               REGISTER_CVAR(_var, _def_val, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR_CB_DEV_ONLY(_var, _def_val, _flags, _comment, _onchangefunction)                  REGISTER_CVAR_CB(_var, _def_val, ((_flags) | VF_DEV_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_STRING_DEV_ONLY(_name, _def_val, _flags, _comment)                                                        REGISTER_STRING(_name, _def_val, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_STRING_CB_DEV_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)               REGISTER_STRING_CB(_name, _def_val, ((_flags) | VF_DEV_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_INT_DEV_ONLY(_name, _def_val, _flags, _comment)                                                               REGISTER_INT(_name, _def_val, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_INT_CB_DEV_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)                  REGISTER_INT_CB(_name, _def_val, ((_flags) | VF_DEV_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_FLOAT_DEV_ONLY(_name, _def_val, _flags, _comment)                                                         REGISTER_FLOAT(_name, _def_val, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR2_DEV_ONLY(_name, _var, _def_val, _flags, _comment)                                                REGISTER_CVAR2(_name, _var, _def_val, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR2_CB_DEV_ONLY(_name, _var, _def_val, _flags, _comment, _onchangefunction)       REGISTER_CVAR2_CB(_name, _var, _def_val, ((_flags) | VF_DEV_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR3_DEV_ONLY(_name, _var, _def_val, _flags, _comment)                                                REGISTER_CVAR3(_name, _var, _def_val, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_COMMAND_DEV_ONLY(_name, _func, _flags, _comment)                                                          REGISTER_COMMAND(_name, _func, ((_flags) | VF_DEV_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#endif // defined(_RELEASE)
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Dedicated server only cvars
//
// N.B:
// (1) Registered as real cvars in all non release builds
// (2) Registered as real cvars in release on dedi servers only, otherwise treated as DEV_ONLY type cvars (see above)
//

// TODO Registering all cvars for Dedicated server as well. Currently CrySystems have no concept of Dedicated server with cmake.
// If we introduce server specific targets for CrySystems, we can add DEDICATED_SERVER flags to those and add the flag back in here.
#if defined(_RELEASE)
#define REGISTER_CVAR_DEDI_ONLY(_var, _def_val, _flags, _comment)                                                          REGISTER_CVAR(_var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR_CB_DEDI_ONLY(_var, _def_val, _flags, _comment, _onchangefunction)                 REGISTER_CVAR_CB(_var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_STRING_DEDI_ONLY(_name, _def_val, _flags, _comment)                                                       REGISTER_STRING(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_STRING_CB_DEDI_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)          REGISTER_STRING_CB(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_INT_DEDI_ONLY(_name, _def_val, _flags, _comment)                                                          REGISTER_INT(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_INT_CB_DEDI_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)                 REGISTER_INT_CB(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_FLOAT_DEDI_ONLY(_name, _def_val, _flags, _comment)                                                        REGISTER_FLOAT(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR2_DEDI_ONLY(_name, _var, _def_val, _flags, _comment)                                               REGISTER_CVAR2(_name, _var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_CVAR2_CB_DEDI_ONLY(_name, _var, _def_val, _flags, _comment, _onchangefunction)  REGISTER_CVAR2_CB(_name, _var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#define REGISTER_COMMAND_DEDI_ONLY(_name, _func, _flags, _comment)                                                         REGISTER_COMMAND(_name, _func, ((_flags) | VF_DEDI_ONLY), _comment); static_assert(((_flags) & ILLEGAL_DEV_FLAGS) == 0)
#else
#define REGISTER_CVAR_DEDI_ONLY(_var, _def_val, _flags, _comment)                                                          REGISTER_CVAR_DEV_ONLY(_var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment)
#define REGISTER_CVAR_CB_DEDI_ONLY(_var, _def_val, _flags, _comment, _onchangefunction)                 REGISTER_CVAR_CB_DEV_ONLY(_var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction)
#define REGISTER_STRING_DEDI_ONLY(_name, _def_val, _flags, _comment)                                                       REGISTER_STRING_DEV_ONLY(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment)
#define REGISTER_STRING_CB_DEDI_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)          REGISTER_STRING_CB_DEV_ONLY(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction)
#define REGISTER_INT_DEDI_ONLY(_name, _def_val, _flags, _comment)                                                          REGISTER_INT_DEV_ONLY(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment)
#define REGISTER_INT_CB_DEDI_ONLY(_name, _def_val, _flags, _comment, _onchangefunction)                 REGISTER_INT_CB_DEV_ONLY(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction)
#define REGISTER_FLOAT_DEDI_ONLY(_name, _def_val, _flags, _comment)                                                        REGISTER_FLOAT_DEV_ONLY(_name, _def_val, ((_flags) | VF_DEDI_ONLY), _comment)
#define REGISTER_CVAR2_DEDI_ONLY(_name, _var, _def_val, _flags, _comment)                                               REGISTER_CVAR2_DEV_ONLY(_name, _var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment)
#define REGISTER_CVAR2_CB_DEDI_ONLY(_name, _var, _def_val, _flags, _comment, _onchangefunction)  REGISTER_CVAR2_CB_DEV_ONLY(_name, _var, _def_val, ((_flags) | VF_DEDI_ONLY), _comment, _onchangefunction)
#define REGISTER_COMMAND_DEDI_ONLY(_name, _func, _flags, _comment)                                                         REGISTER_COMMAND_DEV_ONLY(_name, _func, ((_flags) | VF_DEDI_ONLY), _comment)
#endif // defined(_RELEASE)
//
////////////////////////////////////////////////////////////////////////////////

#ifdef EXCLUDE_NORMAL_LOG               // setting this removes a lot of logging to reduced code size (useful for consoles)

#define CryLog(...) ((void)0)
#define CryComment(...) ((void)0)
#define CryLogAlways(...) ((void)0)
#define CryOutputToCallback(...) ((void)0)

#else // EXCLUDE_NORMAL_LOG

// Summary:
//   Simple logs of data with low verbosity.
void CryLog(const char*, ...) PRINTF_PARAMS(1, 2);
inline void CryLog(const char* format, ...)
{
    // Fran: we need these guards for the testing framework to work
    if (gEnv && gEnv->pSystem && gEnv->pLog)
    {
        va_list args;
        va_start(args, format);
        gEnv->pLog->LogV(ILog::eMessage, format, args);
        va_end(args);
    }
}
// Notes:
//   Very rarely used log comment.
void CryComment(const char*, ...) PRINTF_PARAMS(1, 2);
inline void CryComment(const char* format, ...)
{
    // Fran: we need these guards for the testing framework to work
    if (gEnv && gEnv->pSystem && gEnv->pLog)
    {
        va_list args;
        va_start(args, format);
        gEnv->pLog->LogV(ILog::eComment, format, args);
        va_end(args);
    }
}
// Summary:
//   Logs important data that must be printed regardless verbosity.
void CryLogAlways(const char*, ...) PRINTF_PARAMS(1, 2);
inline void CryLogAlways(const char* format, ...)
{
    // log should not be used before system is ready
    // error before system init should be handled explicitly

    // Fran: we need these guards for the testing framework to work

    if (gEnv && gEnv->pSystem && gEnv->pLog)
    {
        //      assert(gEnv);
        //      assert(gEnv->pSystem);

        va_list args;
        va_start(args, format);
        gEnv->pLog->LogV(ILog::eAlways, format, args);
        va_end(args);
    }
}

//! Writes to CLog via a callback function
//! Any formatting is the responsiblity of the callback function
//! The callback function should write to the supplied stream argument
inline void CryOutputToCallback(ILog::ELogType logType, const ILog::LogWriteCallback& messageCallback)
{
    // writes directly to the log without formatting
    // This is able to bypase the format limits of 4096 + 32 characters for output
    if (gEnv && gEnv->pLog)
    {
        gEnv->pLog->LogWithCallback(logType, messageCallback);
    }
}


#endif // EXCLUDE_NORMAL_LOG
