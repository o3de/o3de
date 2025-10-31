/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ISystem.h>
#include <IRenderer.h>
#include <IWindowMessageHandler.h>

#include <CryVersion.h>
#include "CmdLine.h"

#include <AzFramework/Archive/ArchiveVars.h>
#include <CryCommon/LoadScreenBus.h>

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Math/Crc.h>

#include <CryCommon/TimeValue.h>

#include <list>
#include <map>

namespace AzFramework
{
    class MissingAssetLogger;
}

struct IConsoleCmdArgs;
struct ICVar;
struct IFFont;
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

#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_EXCLUDEUPDATE_ON_CONSOLE 0
#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER 1
#endif

//////////////////////////////////////////////////////////////////////////

#endif

#ifdef WIN32
using WIN_HMODULE = void*;
#else
typedef void* WIN_HMODULE;
#endif


#define PHSYICS_OBJECT_ENTITY 0

struct SSystemCVars
{
    ICVar* sys_localization_folder;

    int sys_float_exceptions;
    int sys_no_crash_dialog;
    int sys_no_error_report_window;
    int sys_dump_aux_threads;
    int sys_WER;
    int sys_dump_type;
    int sys_trackview;
    float sys_update_profile_time;
    int sys_MaxFPS;
    float sys_maxTimeStepForMovieSystem;
    
    int sys_asserts;
    int sys_error_debugbreak;

    AZ::IO::ArchiveVars archiveVars;
};
extern SSystemCVars g_cvars;

class CSystem;

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
    , public CrySystemRequestBus::Handler
{
public:
    CSystem();
    ~CSystem();

    static void OnLanguageCVarChanged(ICVar* language);
    static void OnLocalizationFolderCVarChanged(ICVar* const pLocalizationFolder);
    // adding CVAR to toggle assert verbosity level
    static void OnAssertLevelCvarChanged(ICVar* pArgs);
    static void SetAssertLevel(int _assertlevel);
    static void OnLogLevelCvarChanged(ICVar* pArgs);
    static void SetLogLevel(int _logLevel);

    // interface ILoadConfigurationEntrySink ----------------------------------

    void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup) override;

    // ISystemEventListener
    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    ///////////////////////////////////////////////////////////////////////////
    //! @name ISystem implementation
    //@{
    virtual bool Init(const SSystemInitParams& startupParams);
    void Release() override;

    SSystemGlobalEnvironment* GetGlobalEnvironment() override { return &m_env; }

    bool UpdatePreTickBus(int updateFlags = 0, int nPauseMode = 0) override;
    bool UpdatePostTickBus(int updateFlags = 0, int nPauseMode = 0) override;
    bool UpdateLoadtime() override;

    ////////////////////////////////////////////////////////////////////////
    // CrySystemRequestBus interface implementation
    ISystem* GetCrySystem() override;
    ////////////////////////////////////////////////////////////////////////

    void Relaunch(bool bRelaunch) override;
    bool IsRelaunch() const override { return m_bRelaunch; };

    void SerializingFile(int mode) override { m_iLoadingMode = mode; }
    int  IsSerializingFile() const override { return m_iLoadingMode; }
    void Quit() override;
    bool IsQuitting() const override;
    void ShutdownFileSystem(); // used to cleanup any file resources, such as cache handle.
    const char* GetUserName() override;
    int GetApplicationInstance() override;
    int GetApplicationLogInstance(const char* logFilePath) override;

    AZ::IO::IArchive* GetIPak() override { return m_env.pCryPak; };
    IConsole* GetIConsole() override { return m_env.pConsole; };
    IRemoteConsole* GetIRemoteConsole() override;
    IMovieSystem* GetIMovieSystem() override { return m_movieSystem; };
    ICryFont* GetICryFont() override{ return m_env.pCryFont; }
    ILog* GetILog() override{ return m_env.pLog; }
    ICmdLine* GetICmdLine() override{ return m_pCmdLine; }
    ILevelSystem* GetILevelSystem() override;
    ISystemEventDispatcher* GetISystemEventDispatcher() override { return m_pSystemEventDispatcher; }

    void DetectGameFolderAccessRights();

    void ExecuteCommandLine(bool deferred=true) override;

    void GetUpdateStats(SSystemUpdateStats& stats) override;

    //////////////////////////////////////////////////////////////////////////
    XmlNodeRef CreateXmlNode(const char* sNodeName = "", bool bReuseStrings = false, bool bIsProcessingInstruction = false) override;
    XmlNodeRef LoadXmlFromFile(const char* sFilename, bool bReuseStrings = false) override;
    XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings = false, bool bSuppressWarnings = false) override;
    IXmlUtils* GetXmlUtils() override;
    //////////////////////////////////////////////////////////////////////////

    void IgnoreUpdates(bool bIgnore) override { m_bIgnoreUpdates = bIgnore; };

    bool IsTestMode() const override { return m_bTestMode; }
    //@}

    void SleepIfNeeded();

    void FatalError(const char* format, ...) override PRINTF_PARAMS(2, 3);
    void ReportBug(const char* format, ...) override PRINTF_PARAMS(2, 3);
    // Validator Warning.
    void WarningV(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args) override;
    void Warning(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, ...) override;
    void ShowMessage(const char* text, const char* caption, unsigned int uType) override;
    bool CheckLogVerbosity(int verbosity) override;

    //! Return pointer to user defined callback.
    ISystemUserCallback* GetUserCallback() const { return m_pUserCallback; };

    //////////////////////////////////////////////////////////////////////////
    void SaveConfiguration() override;
    void LoadConfiguration(const char* sFilename, ILoadConfigurationEntrySink* pSink = nullptr, bool warnIfMissing = true) override;
    ESystemConfigPlatform GetConfigPlatform() const override;
    void SetConfigPlatform(ESystemConfigPlatform platform) override;
    //////////////////////////////////////////////////////////////////////////

    bool IsPaused() const override { return m_bPaused; };

    ILocalizationManager* GetLocalizationManager() override;
    void debug_GetCallStack(const char** pFunctions, int& nCount) override;
    void debug_LogCallStack(int nMaxFuncs = 32, int nFlags = 0) override;

public:
#if !defined(RELEASE)
    void SetVersionInfo(const char* const szVersion);
#endif

#if defined(WIN32)
    friend LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
    void RegisterWindowMessageHandler(IWindowMessageHandler* pHandler) override;
    void UnregisterWindowMessageHandler(IWindowMessageHandler* pHandler) override;

    // IWindowMessageHandler
#if defined(WIN32)
    bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
#endif
    // ~IWindowMessageHandler

private:

    // Release all resources.
    void ShutDown();

    //! @name Initialization routines
    //@{
    bool InitConsole();
    bool InitFileSystem();
    bool InitFileSystem_LoadEngineFolders(const SSystemInitParams& initParams);
    bool InitAudioSystem();

    //@}

    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    void CreateSystemVars();

    void QueryVersionInfo();
    void LogVersion();
    void LogBuildInfo();
    void SetDevMode(bool bEnable);

#ifndef _RELEASE
    static void SystemVersionChanged(ICVar* pCVar);
#endif // #ifndef _RELEASE

    void UpdateAudioSystems();

    void AddCVarGroupDirectory(const AZStd::string& sPath) override;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_3
#include AZ_RESTRICTED_FILE(System_h)
#elif defined(WIN32)
    bool GetWinGameFolder(char* szMyDocumentsPath, int maxPathSize);
#endif

public:
    void EnableFloatExceptions(int type);

    // interface ISystem -------------------------------------------
    bool IsDevMode() const override { return m_bInDevMode; }

    // -------------------------------------------------------------

    //! attaches the given variable to the given container;
    //! recreates the variable if necessary
    ICVar* attachVariable (const char* szVarName, int* pContainer, const char* szComment, int dwFlags = 0);

    const CTimeValue& GetLastTickTime() const { return m_lastTickTime; }

private: // ------------------------------------------------------

    // System environment.
    SSystemGlobalEnvironment m_env;

    bool m_bInitializedSuccessfully; //!< true if the system completed all initialization steps
    bool m_bRelaunch; //!< relaunching the app or not (true beforerelaunch)
    int m_iLoadingMode; //!< Game is loading w/o changing context (0 not, 1 quickloading, 2 full loading)
    bool m_bTestMode; //!< If running in testing mode.
    bool m_bEditor; //!< If running in Editor.
    bool m_bNoCrashDialog;
    bool m_bNoErrorReportWindow;
    bool m_bPreviewMode; //!< If running in Preview mode.
    bool m_bDedicatedServer; //!< If running as Dedicated server.
    bool m_bIgnoreUpdates; //!< When set to true will ignore Update and Render calls,
    bool m_bInDevMode; //!< Set to true if was in dev mode.
    bool m_bGameFolderWritable; //!< True when verified that current game folder have write access.

    CTimeValue m_lastTickTime;

    //! system event dispatcher
    ISystemEventDispatcher* m_pSystemEventDispatcher;

    //! System to manage levels.
    ILevelSystem* m_pLevelSystem;

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

    ICVar* m_sysNoUpdate;
    ICVar* m_svDedicatedMaxRate;
    ICVar* m_sys_firstlaunch;
    ICVar* m_sys_load_files_to_memory;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_4
#include AZ_RESTRICTED_FILE(System_h)
#endif

    ICVar* m_gpu_particle_physics;

    AZStd::string  m_sSavedRDriver;                                //!< to restore the driver when quitting the dedicated server

    //////////////////////////////////////////////////////////////////////////
    //! User define callback for system events.
    ISystemUserCallback* m_pUserCallback;

    SFileVersion m_fileVersion;
    SFileVersion m_productVersion;
    SFileVersion m_buildVersion;

    class CLocalizedStringsManager* m_pLocalizationManager;

    ESystemConfigPlatform m_ConfigPlatform;

    // Pause mode.
    bool m_bPaused;
    bool m_bNoUpdate;

    bool m_executedCommandLine = false;

    AZStd::unique_ptr<AzFramework::MissingAssetLogger> m_missingAssetLogger;

    IMovieSystem* m_movieSystem;

public:

    //////////////////////////////////////////////////////////////////////////
    // File version.
    //////////////////////////////////////////////////////////////////////////
    const SFileVersion& GetFileVersion() override;
    const SFileVersion& GetProductVersion() override;
    const SFileVersion& GetBuildVersion() override;

    void OpenPlatformPaks();
    void OpenLanguagePak(const char* sLanguage);
    void OpenLanguageAudioPak(const char* sLanguage);
    void GetLocalizedPath(const char* sLanguage, AZStd::string& sLocalizedPath);
    void GetLocalizedAudioPath(const char* sLanguage, AZStd::string& sLocalizedPath);
    void CloseLanguagePak(const char* sLanguage);
    void CloseLanguageAudioPak(const char* sLanguage);
    void UpdateMovieSystem(const int updateFlags, const float fFrameTime, const bool bPreUpdate);

    //////////////////////////////////////////////////////////////////////////
    // CryAssert and error related.
    bool RegisterErrorObserver(IErrorObserver* errorObserver) override;
    bool UnregisterErrorObserver(IErrorObserver* errorObserver) override;
    void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber) override;
    void OnFatalError(const char* message);

    bool IsAssertDialogVisible() const override;
    void SetAssertVisible(bool bAssertVisble) override;
    //////////////////////////////////////////////////////////////////////////

    void ClearErrorMessages() override
    {
        m_ErrorMessages.clear();
    }

    bool IsLoading()
    {
        return m_eRuntimeState == ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN;
    }

    ESystemGlobalState  GetSystemGlobalState() override;
    void SetSystemGlobalState(ESystemGlobalState systemGlobalState) override;

#if !defined(_RELEASE)
    bool IsSavingResourceList() const override { return (g_cvars.archiveVars.nSaveLevelResourceList != 0); }
#endif

private:
    std::vector<IErrorObserver*> m_errorObservers;
    ESystemGlobalState m_systemGlobalState;
    static const char* GetSystemGlobalStateName(const ESystemGlobalState systemGlobalState);

public:
    void InitLocalization();

protected: // -------------------------------------------------------------

    CCmdLine*                                      m_pCmdLine;

    AZStd::string  m_currentLanguageAudio;

    std::vector< std::pair<CTimeValue, float> > m_updateTimes;

    struct SErrorMessage
    {
        AZStd::string m_Message;
        float m_fTimeToShow;
        float m_Color[4];
        bool m_HardFailure;
    };
    using TErrorMessages = std::list<SErrorMessage>;
    TErrorMessages m_ErrorMessages;
    bool m_bHasRenderedErrorMessage;

    ESystemEvent m_eRuntimeState;
    bool m_bIsAsserting;

    std::vector<IWindowMessageHandler*> m_windowMessageHandlers;
};
