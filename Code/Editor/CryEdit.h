/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CRYEDIT_H
#define CRYINCLUDE_EDITOR_CRYEDIT_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include "CryEditDoc.h"
#include "ViewPane.h"

#include <QSettings>

#endif

class CCryDocManager;
class CCryEditDoc;
class CEditCommandLineInfo;
class CMainFrame;
class CConsoleDialog;
class QAction;
class MainWindow;
class QSharedMemory;

class SANDBOX_API RecentFileList
{
public:
    RecentFileList();

    void Remove(int index);
    void Add(const QString& filename);

    int GetSize();

    void GetDisplayName(QString& name, int index, const QString& curDir);

    QString& operator[](int index);

    void ReadList();
    void WriteList();

    static const int Max = 12;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QStringList m_arrNames;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QSettings m_settings;
};


/**
* Bus for controlling the application's idle processing (may include things like entity updates, ticks, viewport rendering, etc.).
* This is sometimes necessary in special event-processing loops to prevent long (or infinite) processing time because Idle Processing
* can perpetually generate more events.
*/
class EditorIdleProcessing : public AZ::EBusTraits
{
public:
    using Bus = AZ::EBus<EditorIdleProcessing>;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    /// Disable the Editor's idle processing. EnableIdleProcessing() must be called exactly once when special processing is complete.
    virtual void DisableIdleProcessing() {}

    /// Re-enables Idle Processing. Must be called exactly one time for every call to DisableIdleProcessing().
    virtual void EnableIdleProcessing() {}
};

using EditorIdleProcessingBus = AZ::EBus<EditorIdleProcessing>;

enum class COpenSameLevelOptions
{
    ReopenLevelIfSame,
    NotReopenIfSame
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
class SANDBOX_API CCryEditApp
    : public QObject
    , protected AzFramework::AssetSystemInfoBus::Handler
    , protected EditorIdleProcessingBus::Handler
    , protected AzFramework::AssetSystemStatusBus::Handler
{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    friend MainWindow;
    Q_OBJECT
public:
    enum ECreateLevelResult
    {
        ECLR_OK = 0,
        ECLR_ALREADY_EXISTS,
        ECLR_DIR_CREATION_FAILED,
        ECLR_MAX_PATH_EXCEEDED
    };


    CCryEditApp();
    ~CCryEditApp();

    static CCryEditApp* instance();

    bool CreateLevel(bool& wasCreateLevelOperationCancelled);
    void LoadFile(QString fileName);
    void ForceNextIdleProcessing() { m_bForceProcessIdle = true; }
    void KeepEditorActive(bool bActive) { m_bKeepEditorActive = bActive; };
    bool IsInTestMode() const { return m_bTestMode; };
    bool IsInPreviewMode() const { return m_bPreviewMode; };
    bool IsInExportMode() const { return m_bExportMode; };
    bool IsExportingLegacyData() const { return m_bIsExportingLegacyData; }
    bool IsInConsoleMode() const { return m_bConsoleMode; };
    bool IsInAutotestMode() const { return m_bAutotestMode; };
    bool IsInLevelLoadTestMode() const { return m_bLevelLoadTestMode; }
    bool IsInRegularEditorMode();
    bool IsExiting() const { return m_bExiting; }
    void EnableAccelerator(bool bEnable);
    void SaveAutoBackup();
    void SaveAutoRemind();
    void ExportToGame(bool bNoMsgBox = true);
    //! \param sTitleStr overwrites the default title of the Editor
    void SetEditorWindowTitle(QString sTitleStr = QString(), QString sPreTitleStr = QString(), QString sPostTitleStr = QString());
    RecentFileList* GetRecentFileList();
    virtual void AddToRecentFileList(const QString& lpszPathName);
    ECreateLevelResult CreateLevel(const QString& templateName, const QString& levelName, QString& fullyQualifiedLevelName);
    bool FirstInstance(bool bForceNewInstance = false);
    void InitFromCommandLine(CEditCommandLineInfo& cmdInfo);
    bool CheckIfAlreadyRunning();
    //! @return successful outcome if initialization succeeded. or failed outcome with error message.
    AZ::Outcome<void, AZStd::string> InitGameSystem(HWND hwndForInputSystem);
    void CreateSplashScreen();
    void InitPlugins();
    bool InitGame();

    bool InitConsole();
    int IdleProcessing(bool bBackground);
    bool IsWindowInForeground();
    void RunInitPythonScript(CEditCommandLineInfo& cmdInfo);

    void DisableIdleProcessing() override;
    void EnableIdleProcessing() override;

    // Print to stdout even if there out has been redirected
    void PrintAlways(const AZStd::string& output);

    //! Launches the Lua Editor/Debugger
    //! \param files A space separated list of aliased paths
    void OpenLUAEditor(const char* files);

    QString GetRootEnginePath() const;
    void RedirectStdoutToNull();

    // Overrides
public:
    virtual bool InitInstance();
    virtual int ExitInstance(int exitCode = 0);
    virtual bool OnIdle(LONG lCount);
    virtual CCryEditDoc* OpenDocumentFile(const char* filename,
        bool addToMostRecentFileList=true,
        COpenSameLevelOptions openSameLevelOptions = COpenSameLevelOptions::NotReopenIfSame);

    CCryDocManager* GetDocManager() { return m_pDocManager; }

    // Implementation
    void OnCreateLevel();
    void OnOpenLevel();
    void OnAppAbout();
    void OnAppShowWelcomeScreen();
    void OnUpdateShowWelcomeScreen(QAction* action);
    void OnDocumentationTutorials();
    void OnDocumentationGlossary();
    void OnDocumentationO3DE();
    void OnDocumentationReleaseNotes();
    void OnDocumentationGameDevBlog();
    void OnDocumentationForums();
    void OnEditHold();
    void OnEditFetch();
    void OnFileExportToGameNoSurfaceTexture();
    void OnViewSwitchToGame();
    void OnViewSwitchToGameFullScreen();
    void OnMoveObject();
    void OnRenameObj();
    void OnUndo();
    void OnEditLevelData();
    void OnFileEditLogFile();
    void OnFileEditEditorini();
    void OnPreferences();
    void OnOpenProjectManagerSettings();
    void OnOpenProjectManagerNew();
    void OnOpenProjectManager();
    void OnRedo();
    void OnUpdateRedo(QAction* action);
    void OnUpdateUndo(QAction* action);
    void OnSwitchPhysics();
    void OnSwitchPhysicsUpdate(QAction* action);
    void OnSyncPlayer();
    void OnSyncPlayerUpdate(QAction* action);
    void OnResourcesReduceworkingset();
    void OnDummyCommand() {};
    void OnFileSave();
    void OnUpdateDocumentReady(QAction* action);
    void OnUpdateFileOpen(QAction* action);
    void OnUpdateNonGameMode(QAction* action);
    void OnUpdateNewLevel(QAction* action);
    void OnUpdatePlayGame(QAction* action);
    void OnToolsLogMemoryUsage();
    void OnToolsPreferences();

protected:
    // ------- AzFramework::AssetSystemInfoBus::Handler ------
    void OnError(AzFramework::AssetSystem::AssetSystemErrors error) override;
    // -------------------------------------------

    // ------- AzFramework::AssetSystemStatusBus::Handler ------
    void AssetSystemWaiting() override;
    // -------------------------------------------

private:
    friend class EditorActionsHandler;

    void InitLevel(const CEditCommandLineInfo& cmdInfo);

    bool ConnectToAssetProcessor() const;
    void CompileCriticalAssets() const;

    CMainFrame* GetMainFrame() const;
    void WriteConfig();
    bool UserExportToGame(bool bNoMsgBox = true);
    static void ShowSplashScreen(CCryEditApp* app);
    static void CloseSplashScreen();
    static void OutputStartupMessage(QString str);
    bool ShowEnableDisableGemDialog(const QString& title, const QString& message);
    QString ShowWelcomeDialog();

    bool FixDanglingSharedMemory(const QString& sharedMemName) const;

    //! Displays level load errors after a certain number of idle frames have been processed.
    //! Due to the asyncrhonous nature of loading assets any errors that are reported by components
    //! can happen after the level is loaded. This method will wait for a few idle updates and then
    //! display the load errors to ensure all errors are displayed properly.
    void DisplayLevelLoadErrors();

    class CEditorImpl* m_pEditor = nullptr;
    static CCryEditApp* s_currentInstance;
    //! True if editor is in test mode.
    //! Test mode is a special mode enabled when Editor ran with /test command line.
    //! In this mode editor starts up, but exit immediately after all initialization.
    bool m_bTestMode = false;
    //! In this mode editor will load specified cry file, export t, and then close.
    bool m_bExportMode = false;
    QString m_exportFile;
    //! This flag is set to true every time any of the "Export" commands is being executed.
    //! Once exporting is finished the flag is set back to false.
    //! UI events like "New Level" or "Open Level", should not be allowed while m_bIsExportingLegacyData==true.
    //! Otherwise it could trigger crashes trying to export while exporting.
    bool m_bIsExportingLegacyData = false;
    //! If application exiting.
    bool m_bExiting = false;
    //! True if editor is in preview mode.
    //! In this mode only very limited functionality is available and only for fast preview of models.
    bool m_bPreviewMode = false;
    // Only console window is created.
    bool m_bConsoleMode = false;
    // Skip showing the WelcomeScreenDialog
    bool m_bSkipWelcomeScreenDialog = false;
    // Level load test mode
    bool m_bLevelLoadTestMode = false;
    //! Current file in preview mode.
    char m_sPreviewFile[_MAX_PATH];
    //! True if "/runpythontest" was passed as a flag.
    bool m_bRunPythonTestScript = false;
    //! True if "/runpython" was passed as a flag.
    bool m_bRunPythonScript = false;
    //! File to run on startup
    QString m_execFile;
    //! Command to run on startup
    QString m_execLineCmd;
    //! Autotest mode: Special mode meant for automated testing, things like blocking dialogs or error report windows won't appear
    bool m_bAutotestMode = false;

    CConsoleDialog* m_pConsoleDialog = nullptr;

    float m_fastRotateAngle = 45.0f;
    float m_moveSpeedStep = 0.1f;

    ULONG_PTR m_gdiplusToken;
    QSharedMemory* m_mutexApplication = nullptr;
    //! was the editor active in the previous frame ... needed to detect if the game lost focus and
    //! dispatch proper SystemEvent (needed to release input keys)
    bool m_bPrevActive = false;
    // If this flag is set, the next OnIdle() will update, even if the app is in the background, and then
    // this flag will be reset.
    bool m_bForceProcessIdle = false;
    // This is set while IdleProcessing is running to prevent re-entrancy
    bool m_idleProcessingRunning = false;
    // Keep the editor alive, even if no focus is set
    bool m_bKeepEditorActive = false;
    // Currently creating a new level
    bool m_creatingNewLevel = false;
    bool m_openingLevel = false;
    bool m_savingLevel = false;
    // Flag indicating if the errors for the currently loaded level have been displayed
    bool m_levelErrorsHaveBeenDisplayed = false;
    // Number of idle frames that have passed before displaying level errors
    int m_numBeforeDisplayErrorFrames = 0;

    QString m_lastOpenLevelPath;
    QString m_rootEnginePath;

    int m_disableIdleProcessingCounter = 0; //!< Counts requests to disable idle processing. When non-zero, idle processing will be disabled.

    CCryDocManager* m_pDocManager = nullptr;

// Disable warning for dll export since this member won't be used outside this class
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZ::IO::FileDescriptorRedirector m_stdoutRedirection = AZ::IO::FileDescriptorRedirector(1); // < 1 for STDOUT
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

private:
    // Optional Uri to start an external lua debugger. If not specified,
    // then the Editor will open LuaIDE.exe.
    // For example, if using The Visual Studio Debugger Extension provided by lumbermixalot
    // The value will be: "vscode://lumbermixalot.o3de-lua-debug/debug?"
    // The following parameters will be added to the URI at runtime:
    // "projectPath". Absolute path of the game projec root.
    // "enginePath". Absolute path of the engine root. if not specified, it will be assume to be one directory above the game project root.
    // "files[]". A list of files, 
    // Full example using the Uri shown below:
    // "vscode://lumbermixalot.o3de-lua-debug/debug?projectPath=D:\mydir\myproject&enginePath=C:\GIT\o3de&files[]=D:\mydir\myproject\scripts\something.lua&files[]=D:\mydir\myproject\scripts\utils\something2.lua"
    // or
    // "vscode://lumbermixalot.o3de-lua-debug/debug?projectPath=D:\GIT\o3de\AutomatedTesting&files[]=D:\GIT\o3de\AutomatedTesting\Assets\Scripts\something.lua"
    static constexpr AZStd::string_view LuaDebuggerUriRegistryKey = "/O3DE/Lua/Debugger/Uri";

    struct PythonOutputHandler;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZStd::shared_ptr<PythonOutputHandler> m_pythonOutputHandler;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    friend struct PythonTestOutputHandler;

    void OpenProjectManager(const AZStd::string& screen);
    void OnUpdateWireframe(QAction* action);
    void OnViewConfigureLayout();

    void OnCustomizeKeyboard();
    void OnToolsScriptHelp();
    void OnViewCycle2dviewport();
    void OnDisplayGotoPosition();
    void OnFileSavelevelresources();
    void OnClearRegistryData();
    void OnSwitchToSequenceCamera();
    void OnUpdateSwitchToSequenceCamera(QAction* action);
    void OnSwitchToSelectedcamera();
    void OnUpdateSwitchToSelectedCamera(QAction* action);
    void OnSwitchcameraNext();
    void OnOpenProceduralMaterialEditor();
    void OnOpenAssetBrowserView();
    void OnOpenTrackView();
    void OnOpenAudioControlsEditor();
    void OnOpenUICanvasEditor();

    // @param files: A list of file paths, separated by '|';
    void OpenExternalLuaDebugger(AZStd::string_view luaDebuggerUri, AZStd::string_view enginePath, AZStd::string_view projectPath, const char * files);

public:
    void ExportLevel(bool bExportToGame, bool bExportTexture, bool bAutoExport);
    static bool Command_ExportToEngine();
};

//////////////////////////////////////////////////////////////////////////
class CCrySingleDocTemplate
    : public QObject
{
    Q_OBJECT
private:
    explicit CCrySingleDocTemplate(const QMetaObject* pDocClass)
        : QObject()
        , m_documentClass(pDocClass)
    {
    }
public:
    enum Confidence
    {
        noAttempt,
        maybeAttemptForeign,
        maybeAttemptNative,
        yesAttemptForeign,
        yesAttemptNative,
        yesAlreadyOpen
    };

    template<typename DOCUMENT>
    static CCrySingleDocTemplate* create()
    {
        return new CCrySingleDocTemplate(&DOCUMENT::staticMetaObject);
    }
    ~CCrySingleDocTemplate() {};
    // avoid creating another CMainFrame
    // close other type docs before opening any things
    virtual CCryEditDoc* OpenDocumentFile(const char* lpszPathName, bool addToMostRecentFileList, bool bMakeVisible);
    virtual CCryEditDoc* OpenDocumentFile(const char* lpszPathName, bool bMakeVisible = TRUE);
    virtual Confidence MatchDocType(const char* lpszPathName, CCryEditDoc*& rpDocMatch);

private:
    const QMetaObject* m_documentClass = nullptr;
};

class CDocTemplate;
class CCryDocManager
{
    CCrySingleDocTemplate* m_pDefTemplate = nullptr;
public:
    CCryDocManager();
    virtual ~CCryDocManager() = default;
    CCrySingleDocTemplate* SetDefaultTemplate(CCrySingleDocTemplate* pNew);
    // Copied from MFC to get rid of the silly ugly unoverridable doc-type pick dialog
    virtual void OnFileNew();
    virtual bool DoPromptFileName(QString& fileName, UINT nIDSTitle,
        DWORD lFlags, bool bOpenFileDialog, CDocTemplate* pTemplate);
    virtual CCryEditDoc* OpenDocumentFile(const char* filename, bool addToMostRecentFileList, COpenSameLevelOptions openSameLevelOptions = COpenSameLevelOptions::NotReopenIfSame);

    QVector<CCrySingleDocTemplate*> m_templateList;
};

#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class CryEditPythonHandler final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(CryEditPythonHandler, "{D4B19973-54D9-44BD-9E70-6069462A0CDC}")
        virtual ~CryEditPythonHandler() = default;

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}

        class CryEditHandler
        {
        public:
            AZ_RTTI(CryEditHandler, "{6C1FD05A-2F39-4094-80D4-CA526676F13E}")
            virtual ~CryEditHandler() = default;
        };

        class CryEditCheckoutHandler
        {
        public:
            AZ_RTTI(CryEditCheckoutHandler, "{C65EF439-6754-4ACD-AEA2-196F2DBA0AF3}")
            virtual ~CryEditCheckoutHandler() = default;
        };
    };

} // namespace AzToolsFramework

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule();
extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule();

#endif // CRYINCLUDE_EDITOR_CRYEDIT_H
