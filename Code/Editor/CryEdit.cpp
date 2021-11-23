/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#ifdef WIN32
AZ_PUSH_DISABLE_WARNING(4458, "-Wunknown-warning-option")
#include <gdiplus.h>
AZ_POP_DISABLE_WARNING
#pragma comment (lib, "Gdiplus.lib")

#include <WinUser.h> // needed for MessageBoxW in the assert handler
#endif

#include <array>
#include <string>

#include "CryEdit.h"

// Qt
#include <QCommandLineParser>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QProcess>
#include <QScopedValueRollback>
#include <QClipboard>
#include <QMenuBar>
#include <QDialogButtonBox>

// Aws Native SDK
#include <aws/sts/STSClient.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/sts/model/GetFederationTokenRequest.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/json/JsonSerializer.h>

// AzCore
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Console/IConsole.h>

// AzFramework
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzFramework/ProjectManager/ProjectManager.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>

// AzToolsFramework
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Component/EditorLevelComponentAPIBus.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/PythonTerminal/ScriptHelpDialog.h>

// AzQtComponents
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/HandleDpiAwareness.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

// CryCommon
#include <CryCommon/ITimer.h>
#include <CryCommon/ILevelSystem.h>

// Editor
#include "Settings.h"

#include "GameExporter.h"
#include "GameResourcesExporter.h"

#include "ActionManager.h"
#include "MainWindow.h"

#include "Core/QtEditorApplication.h"
#include "StringDlg.h"
#include "NewLevelDialog.h"
#include "LayoutConfigDialog.h"
#include "ViewManager.h"
#include "FileTypeUtils.h"
#include "PluginManager.h"

#include "IEditorImpl.h"
#include "StartupLogoDialog.h"
#include "DisplaySettings.h"
#include "GameEngine.h"

#include "StartupTraceHandler.h"
#include "ToolsConfigPage.h"
#include "Objects/SelectionGroup.h"
#include "Include/IObjectManager.h"
#include "WaitProgress.h"

#include "ToolBox.h"
#include "LevelInfo.h"
#include "EditorPreferencesDialog.h"
#include "AnimationContext.h"

#include "GotoPositionDlg.h"

#include "ConsoleDialog.h"
#include "Controls/ConsoleSCB.h"

#include "ScopedVariableSetter.h"

#include "Util/3DConnexionDriver.h"

#include "DimensionsDialog.h"

#include "Util/AutoDirectoryRestoreFileDialog.h"
#include "Util/EditorAutoLevelLoadTest.h"
#include "AboutDialog.h"
#include <AzToolsFramework/PythonTerminal/ScriptHelpDialog.h>

#include "QuickAccessBar.h"

#include "Export/ExportManager.h"

#include "LevelFileDialog.h"
#include "LevelIndependentFileMan.h"
#include "WelcomeScreen/WelcomeScreenDialog.h"

#include "Controls/ReflectedPropertyControl/PropertyCtrl.h"
#include "Controls/ReflectedPropertyControl/ReflectedVar.h"

#include "EditorToolsApplication.h"

#include "Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h"

// AWSNativeSDK
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AWSNativeSDKInit/AWSNativeSDKInit.h>


#if defined(AZ_PLATFORM_WINDOWS)
#include <AzFramework/API/ApplicationAPI_Platform.h>
#endif

#if AZ_TRAIT_OS_PLATFORM_APPLE
#include "WindowObserver_mac.h"
#endif

#include <AzCore/RTTI/BehaviorContext.h>

#include <AzFramework/Render/Intersector.h>

#include <AzCore/std/smart_ptr/make_shared.h>

static const char O3DEEditorClassName[] = "O3DEEditorClass";
static const char O3DEApplicationName[] = "O3DEApplication";

static AZ::EnvironmentVariable<bool> inEditorBatchMode = nullptr;

RecentFileList::RecentFileList()
{
    m_settings.beginGroup(QStringLiteral("Application"));
    m_settings.beginGroup(QStringLiteral("Recent File List"));

    ReadList();
}

void RecentFileList::Remove(int index)
{
    m_arrNames.removeAt(index);
}

void RecentFileList::Add(const QString& f)
{
    QString filename = QDir::toNativeSeparators(f);
    m_arrNames.removeAll(filename);
    m_arrNames.push_front(filename);
    while (m_arrNames.count() > Max)
    {
        m_arrNames.removeAt(Max);
    }
}

int RecentFileList::GetSize()
{
    return m_arrNames.count();
}

void RecentFileList::GetDisplayName(QString& name, int index, const QString& curDir)
{
    name = m_arrNames[index];

    const QDir cur(curDir);
    QDir fileDir(name); // actually pointing at file, first cdUp() gets us the parent dir
    while (fileDir.cdUp())
    {
        if (fileDir == cur)
        {
            name = cur.relativeFilePath(name);
            break;
        }
    }

    name = QDir::toNativeSeparators(name);
}

QString& RecentFileList::operator[](int index)
{
    return m_arrNames[index];
}

void RecentFileList::ReadList()
{
    m_arrNames.clear();

    for (int i = 1; i <= Max; ++i)
    {
        QString f = m_settings.value(QStringLiteral("File%1").arg(i)).toString();
        if (!f.isEmpty())
        {
            m_arrNames.push_back(f);
        }
    }
}

void RecentFileList::WriteList()
{
    m_settings.remove(QString());

    int i = 1;
    for (auto f : m_arrNames)
    {
        m_settings.setValue(QStringLiteral("File%1").arg(i++), f);
    }
}



#define ERROR_LEN 256


CCryDocManager::CCryDocManager()
{
}

CCrySingleDocTemplate* CCryDocManager::SetDefaultTemplate(CCrySingleDocTemplate* pNew)
{
    CCrySingleDocTemplate* pOld = m_pDefTemplate;
    m_pDefTemplate = pNew;
    m_templateList.clear();
    m_templateList.push_back(m_pDefTemplate);
    return pOld;
}
// Copied from MFC to get rid of the silly ugly unoverridable doc-type pick dialog
void CCryDocManager::OnFileNew()
{
    assert(m_pDefTemplate != nullptr);

    m_pDefTemplate->OpenDocumentFile(nullptr);
    // if returns NULL, the user has already been alerted
}
bool CCryDocManager::DoPromptFileName(QString& fileName, [[maybe_unused]] UINT nIDSTitle,
    [[maybe_unused]] DWORD lFlags, bool bOpenFileDialog, [[maybe_unused]] CDocTemplate* pTemplate)
{
    CLevelFileDialog levelFileDialog(bOpenFileDialog);
    levelFileDialog.show();
    levelFileDialog.adjustSize();

    if (levelFileDialog.exec() == QDialog::Accepted)
    {
        fileName = levelFileDialog.GetFileName();
        return true;
    }

    return false;
}
CCryEditDoc* CCryDocManager::OpenDocumentFile(const char* filename, bool addToMostRecentFileList, COpenSameLevelOptions openSameLevelOptions)
{
    assert(filename != nullptr);

    const bool reopenIfSame = openSameLevelOptions == COpenSameLevelOptions::ReopenLevelIfSame;
    // find the highest confidence
    auto pos = m_templateList.begin();
    CCrySingleDocTemplate::Confidence bestMatch = CCrySingleDocTemplate::noAttempt;
    CCrySingleDocTemplate* pBestTemplate = nullptr;
    CCryEditDoc* pOpenDocument = nullptr;

    if (filename[0] == '\"')
    {
        ++filename;
    }
    QString szPath = QString::fromUtf8(filename);
    if (szPath.endsWith('"'))
    {
        szPath.remove(szPath.length() - 1, 1);
    }

    while (pos != m_templateList.end())
    {
        auto pTemplate = *(pos++);

        CCrySingleDocTemplate::Confidence match;
        assert(pOpenDocument == nullptr);
        match = pTemplate->MatchDocType(szPath.toUtf8().data(), pOpenDocument);
        if (match > bestMatch)
        {
            bestMatch = match;
            pBestTemplate = pTemplate;
        }
        if (match == CCrySingleDocTemplate::yesAlreadyOpen)
        {
            break;      // stop here
        }
    }

    if (!reopenIfSame && pOpenDocument != nullptr)
    {
        return pOpenDocument;
    }

    if (pBestTemplate == nullptr)
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Failed to open document."));
        return nullptr;
    }

    return pBestTemplate->OpenDocumentFile(szPath.toUtf8().data(), addToMostRecentFileList, false);
}

//////////////////////////////////////////////////////////////////////////////
// CCryEditApp

#undef ON_COMMAND
#define ON_COMMAND(id, method) \
    MainWindow::instance()->GetActionManager()->RegisterActionHandler(id, this, &CCryEditApp::method);

#undef ON_COMMAND_RANGE
#define ON_COMMAND_RANGE(idStart, idEnd, method) \
    for (int i = idStart; i <= idEnd; ++i) \
        ON_COMMAND(i, method);

AZ_CVAR_EXTERNED(bool, ed_previewGameInFullscreen_once);

void CCryEditApp::RegisterActionHandlers()
{
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_APP_SHOW_WELCOME, OnAppShowWelcomeScreen)
    ON_COMMAND(ID_DOCUMENTATION_TUTORIALS, OnDocumentationTutorials)
    ON_COMMAND(ID_DOCUMENTATION_O3DE, OnDocumentationO3DE)
    ON_COMMAND(ID_DOCUMENTATION_GAMELIFT, OnDocumentationGamelift)
    ON_COMMAND(ID_DOCUMENTATION_RELEASENOTES, OnDocumentationReleaseNotes)
    ON_COMMAND(ID_DOCUMENTATION_GAMEDEVBLOG, OnDocumentationGameDevBlog)
    ON_COMMAND(ID_DOCUMENTATION_FORUMS, OnDocumentationForums)
    ON_COMMAND(ID_DOCUMENTATION_AWSSUPPORT, OnDocumentationAWSSupport)
    ON_COMMAND(ID_FILE_EXPORT_SELECTEDOBJECTS, OnExportSelectedObjects)
    ON_COMMAND(ID_EDIT_HOLD, OnEditHold)
    ON_COMMAND(ID_EDIT_FETCH, OnEditFetch)
    ON_COMMAND(ID_FILE_EXPORTTOGAMENOSURFACETEXTURE, OnFileExportToGameNoSurfaceTexture)
    ON_COMMAND(ID_VIEW_SWITCHTOGAME, OnViewSwitchToGame)
    ON_COMMAND(ID_VIEW_SWITCHTOGAME_VIEWPORT, OnViewSwitchToGame)
    ON_COMMAND(ID_VIEW_SWITCHTOGAME_FULLSCREEN, OnViewSwitchToGameFullScreen)
    ON_COMMAND(ID_MOVE_OBJECT, OnMoveObject)
    ON_COMMAND(ID_RENAME_OBJ, OnRenameObj)
    ON_COMMAND(ID_UNDO, OnUndo)
    ON_COMMAND(ID_TOOLBAR_WIDGET_REDO, OnUndo)     // Can't use the same ID, because for the menu we can't have a QWidgetAction, while for the toolbar we want one
    ON_COMMAND(ID_IMPORT_ASSET, OnOpenAssetImporter)
    ON_COMMAND(ID_EDIT_LEVELDATA, OnEditLevelData)
    ON_COMMAND(ID_FILE_EDITLOGFILE, OnFileEditLogFile)
    ON_COMMAND(ID_FILE_RESAVESLICES, OnFileResaveSlices)
    ON_COMMAND(ID_FILE_EDITEDITORINI, OnFileEditEditorini)
    ON_COMMAND(ID_PREFERENCES, OnPreferences)
    ON_COMMAND(ID_REDO, OnRedo)
    ON_COMMAND(ID_TOOLBAR_WIDGET_REDO, OnRedo)
    ON_COMMAND(ID_FILE_OPEN_LEVEL, OnOpenLevel)
#ifdef ENABLE_SLICE_EDITOR
    ON_COMMAND(ID_FILE_NEW_SLICE, OnCreateSlice)
    ON_COMMAND(ID_FILE_OPEN_SLICE, OnOpenSlice)
#endif
    ON_COMMAND(ID_SWITCH_PHYSICS, OnSwitchPhysics)
    ON_COMMAND(ID_GAME_SYNCPLAYER, OnSyncPlayer)
    ON_COMMAND(ID_RESOURCES_REDUCEWORKINGSET, OnResourcesReduceworkingset)

    ON_COMMAND(ID_VIEW_CONFIGURELAYOUT, OnViewConfigureLayout)

    ON_COMMAND(IDC_SELECTION, OnDummyCommand)
    //////////////////////////////////////////////////////////////////////////
    ON_COMMAND(ID_TAG_LOC1, OnTagLocation1)
    ON_COMMAND(ID_TAG_LOC2, OnTagLocation2)
    ON_COMMAND(ID_TAG_LOC3, OnTagLocation3)
    ON_COMMAND(ID_TAG_LOC4, OnTagLocation4)
    ON_COMMAND(ID_TAG_LOC5, OnTagLocation5)
    ON_COMMAND(ID_TAG_LOC6, OnTagLocation6)
    ON_COMMAND(ID_TAG_LOC7, OnTagLocation7)
    ON_COMMAND(ID_TAG_LOC8, OnTagLocation8)
    ON_COMMAND(ID_TAG_LOC9, OnTagLocation9)
    ON_COMMAND(ID_TAG_LOC10, OnTagLocation10)
    ON_COMMAND(ID_TAG_LOC11, OnTagLocation11)
    ON_COMMAND(ID_TAG_LOC12, OnTagLocation12)
    //////////////////////////////////////////////////////////////////////////
    ON_COMMAND(ID_GOTO_LOC1, OnGotoLocation1)
    ON_COMMAND(ID_GOTO_LOC2, OnGotoLocation2)
    ON_COMMAND(ID_GOTO_LOC3, OnGotoLocation3)
    ON_COMMAND(ID_GOTO_LOC4, OnGotoLocation4)
    ON_COMMAND(ID_GOTO_LOC5, OnGotoLocation5)
    ON_COMMAND(ID_GOTO_LOC6, OnGotoLocation6)
    ON_COMMAND(ID_GOTO_LOC7, OnGotoLocation7)
    ON_COMMAND(ID_GOTO_LOC8, OnGotoLocation8)
    ON_COMMAND(ID_GOTO_LOC9, OnGotoLocation9)
    ON_COMMAND(ID_GOTO_LOC10, OnGotoLocation10)
    ON_COMMAND(ID_GOTO_LOC11, OnGotoLocation11)
    ON_COMMAND(ID_GOTO_LOC12, OnGotoLocation12)
    //////////////////////////////////////////////////////////////////////////

    ON_COMMAND(ID_TOOLS_LOGMEMORYUSAGE, OnToolsLogMemoryUsage)
    ON_COMMAND(ID_TOOLS_CUSTOMIZEKEYBOARD, OnCustomizeKeyboard)
    ON_COMMAND(ID_TOOLS_CONFIGURETOOLS, OnToolsConfiguretools)
    ON_COMMAND(ID_TOOLS_SCRIPTHELP, OnToolsScriptHelp)
#ifdef FEATURE_ORTHOGRAPHIC_VIEW
    ON_COMMAND(ID_VIEW_CYCLE2DVIEWPORT, OnViewCycle2dviewport)
#endif
    ON_COMMAND(ID_DISPLAY_GOTOPOSITION, OnDisplayGotoPosition)
    ON_COMMAND(ID_FILE_SAVELEVELRESOURCES, OnFileSavelevelresources)
    ON_COMMAND(ID_CLEAR_REGISTRY, OnClearRegistryData)
    ON_COMMAND(ID_VALIDATELEVEL, OnValidatelevel)
    ON_COMMAND(ID_TOOLS_PREFERENCES, OnToolsPreferences)
    ON_COMMAND(ID_SWITCHCAMERA_DEFAULTCAMERA, OnSwitchToDefaultCamera)
    ON_COMMAND(ID_SWITCHCAMERA_SEQUENCECAMERA, OnSwitchToSequenceCamera)
    ON_COMMAND(ID_SWITCHCAMERA_SELECTEDCAMERA, OnSwitchToSelectedcamera)
    ON_COMMAND(ID_SWITCHCAMERA_NEXT, OnSwitchcameraNext)
    ON_COMMAND(ID_OPEN_SUBSTANCE_EDITOR, OnOpenProceduralMaterialEditor)
    ON_COMMAND(ID_OPEN_ASSET_BROWSER, OnOpenAssetBrowserView)
    ON_COMMAND(ID_OPEN_AUDIO_CONTROLS_BROWSER, OnOpenAudioControlsEditor)

    ON_COMMAND(ID_DISPLAY_SHOWHELPERS, OnShowHelpers)
    ON_COMMAND(ID_OPEN_TRACKVIEW, OnOpenTrackView)
    ON_COMMAND(ID_OPEN_UICANVASEDITOR, OnOpenUICanvasEditor)

    ON_COMMAND(ID_OPEN_QUICK_ACCESS_BAR, OnOpenQuickAccessBar)

    ON_COMMAND(ID_FILE_SAVE_LEVEL, OnFileSave)
    ON_COMMAND(ID_FILE_EXPORTOCCLUSIONMESH, OnFileExportOcclusionMesh)

    // Project Manager
    ON_COMMAND(ID_FILE_PROJECT_MANAGER_SETTINGS, OnOpenProjectManagerSettings)
    ON_COMMAND(ID_FILE_PROJECT_MANAGER_NEW, OnOpenProjectManagerNew)
    ON_COMMAND(ID_FILE_PROJECT_MANAGER_OPEN, OnOpenProjectManager)
}

CCryEditApp* CCryEditApp::s_currentInstance = nullptr;
/////////////////////////////////////////////////////////////////////////////
// CCryEditApp construction
CCryEditApp::CCryEditApp()
{
    s_currentInstance = this;

    m_sPreviewFile[0] = 0;

    // Place all significant initialization in InitInstance
    ZeroStruct(m_tagLocations);
    ZeroStruct(m_tagAngles);

    AzFramework::AssetSystemInfoBus::Handler::BusConnect();

    m_disableIdleProcessingCounter = 0;
    EditorIdleProcessingBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CCryEditApp::~CCryEditApp()
{
    EditorIdleProcessingBus::Handler::BusDisconnect();
    AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();
    s_currentInstance = nullptr;
}

CCryEditApp* CCryEditApp::instance()
{
    return s_currentInstance;
}

class CEditCommandLineInfo
{
public:
    bool m_bTest = false;
    bool m_bAutoLoadLevel = false;
    bool m_bExport = false;
    bool m_bExportTexture = false;

    bool m_bMatEditMode = false;

    bool m_bConsoleMode = false;
    bool m_bNullRenderer = false;
    bool m_bDeveloperMode = false;
    bool m_bRunPythonScript = false;
    bool m_bRunPythonTestScript = false;
    bool m_bShowVersionInfo = false;
    QString m_exportFile;
    QString m_strFileName;
    QString m_appRoot;
    QString m_logFile;
    QString m_pythonArgs;
    QString m_pythonTestCase;
    QString m_execFile;
    QString m_execLineCmd;

    bool m_bSkipWelcomeScreenDialog = false;
    bool m_bAutotestMode = false;

    struct CommandLineStringOption
    {
        QString name;
        QString description;
        QString valueName;
    };

    CEditCommandLineInfo()
    {
        bool dummy;
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        parser.setApplicationDescription(QObject::tr("O3DE Editor"));
        // nsDocumentRevisionDebugMode is an argument that the macOS system passed into an App bundle that is being debugged.
        // Need to include it here so that Qt argument parser does not error out.
        bool nsDocumentRevisionsDebugMode = false;
        const std::vector<std::pair<QString, bool&> > options = {
            { "export", m_bExport },
            { "exportTexture", m_bExportTexture },
            { "test", m_bTest },
            { "auto_level_load", m_bAutoLoadLevel },
            { "MatEdit", m_bMatEditMode },
            { "BatchMode", m_bConsoleMode },
            { "NullRenderer", m_bNullRenderer },
            { "devmode", m_bDeveloperMode },
            { "VTUNE", dummy },
            { "runpython", m_bRunPythonScript },
            { "runpythontest", m_bRunPythonTestScript },
            { "version", m_bShowVersionInfo },
            { "NSDocumentRevisionsDebugMode", nsDocumentRevisionsDebugMode},
            { "skipWelcomeScreenDialog", m_bSkipWelcomeScreenDialog},
            { "autotest_mode", m_bAutotestMode},
            { "regdumpall", dummy },
            { "attach-debugger", dummy }, // Attaches a debugger for the current application
            { "wait-for-debugger", dummy }, // Waits until a debugger is attached to the current application
        };

        QString dummyString;
        const std::vector<std::pair<CommandLineStringOption, QString&> > stringOptions = {
            {{"logfile", "File name of the log file to write out to.", "logfile"}, m_logFile},
            {{"runpythonargs", "Command-line argument string to pass to the python script if --runpython or --runpythontest was used.", "runpythonargs"}, m_pythonArgs},
            {{"pythontestcase", "Test case name of python test script if --runpythontest was used.", "pythontestcase"}, m_pythonTestCase},
            {{"exec", "cfg file to run on startup, used for systems like automation", "exec"}, m_execFile},
            {{"rhi", "Command-line argument to force which rhi to use", "dummyString"}, dummyString },
            {{"rhi-device-validation", "Command-line argument to configure rhi validation", "dummyString"}, dummyString },
            {{"exec_line", "command to run on startup, used for systems like automation", "exec_line"}, m_execLineCmd},
            {{"regset", "Command-line argument to override settings registry values", "regset"}, dummyString},
            {{"regremove", "Deletes a value within the global settings registry at the JSON pointer path @key", "regremove"}, dummyString},
            {{"regdump", "Sets a value within the global settings registry at the JSON pointer path @key with value of @value", "regdump"}, dummyString},
            {{"project-path", "Supplies the path to the project that the Editor should use", "project-path"}, dummyString},
            {{"engine-path", "Supplies the path to the engine", "engine-path"}, dummyString},
            {{"project-cache-path", "Path to the project cache", "project-cache-path"}, dummyString},
            {{"project-user-path", "Path to the project user path", "project-user-path"}, dummyString},
            {{"project-log-path", "Path to the project log path", "project-log-path"}, dummyString}
            // add dummy entries here to prevent QCommandLineParser error-ing out on cmd line args that will be parsed later
        };


        parser.addPositionalArgument("file", QCoreApplication::translate("main", "file to open"));
        for (const auto& option : options)
        {
            parser.addOption(QCommandLineOption(option.first));
        }

        for (const auto& option : stringOptions)
        {
            parser.addOption(QCommandLineOption(option.first.name, option.first.description, option.first.valueName));
        }

        QStringList args = qApp->arguments();

#ifdef Q_OS_WIN32
        for (QString& arg : args)
        {
            if (!arg.isEmpty() && arg[0] == '/')
            {
                arg[0] = '-'; // QCommandLineParser only supports - and -- prefixes
            }
        }
#endif

        if (!parser.parse(args))
        {
            AZ_TracePrintf("QT CommandLine Parser", "QT command line parsing warned with message %s."
                " Has the QCommandLineParser had these options added to it", parser.errorText().toUtf8().constData());
        }

        // Get boolean options
        const int numOptions = static_cast<int>(options.size());
        for (int i = 0; i < numOptions; ++i)
        {
            options[i].second = parser.isSet(options[i].first);
        }

        // Get string options
        for (auto& option : stringOptions)
        {
            option.second = parser.value(option.first.valueName);
        }

        m_bExport = m_bExport || m_bExportTexture;

        const QStringList positionalArgs = parser.positionalArguments();

        if (!positionalArgs.isEmpty())
        {
            m_strFileName = positionalArgs.first();

            if (positionalArgs.first().at(0) != '[')
            {
                m_exportFile = positionalArgs.first();
            }
        }
    }
};

struct SharedData
{
    bool raise = false;
    char text[_MAX_PATH];
};
/////////////////////////////////////////////////////////////////////////////
// CTheApp::FirstInstance
//      FirstInstance checks for an existing instance of the application.
//      If one is found, it is activated.
//
//      This function uses a technique similar to that described in KB
//      article Q141752 to locate the previous instance of the application. .
bool CCryEditApp::FirstInstance(bool bForceNewInstance)
{
    QSystemSemaphore sem(QString(O3DEApplicationName) + "_sem", 1);
    sem.acquire();
    {
        FixDanglingSharedMemory(O3DEEditorClassName);
    }
    sem.release();
    m_mutexApplication = new QSharedMemory(O3DEEditorClassName);
    if (!m_mutexApplication->create(sizeof(SharedData)) && !bForceNewInstance)
    {
        m_mutexApplication->attach();
        // another instance is already running - activate it
        sem.acquire();
        SharedData* data = reinterpret_cast<SharedData*>(m_mutexApplication->data());
        data->raise = true;

        if (m_bPreviewMode)
        {
            // IF in preview mode send this window copy data message to load new preview file.
            azstrcpy(data->text, MAX_PATH, m_sPreviewFile);
        }
        return false;
    }
    else
    {
        m_mutexApplication->attach();
        // this is the first instance
        sem.acquire();
        ::memset(m_mutexApplication->data(), 0, m_mutexApplication->size());
        sem.release();
        QTimer* t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [this]() {
            QSystemSemaphore sem(QString(O3DEApplicationName) + "_sem", 1);
            sem.acquire();
            SharedData* data = reinterpret_cast<SharedData*>(m_mutexApplication->data());
            QString preview = QString::fromLatin1(data->text);
            if (data->raise)
            {
                QWidget* w = MainWindow::instance();
                w->setWindowState((w->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
                w->raise();
                w->activateWindow();
                data->raise = false;
            }
            if (!preview.isEmpty())
            {
                // Load this file
                LoadFile(preview);
                data->text[0] = 0;
            }
            sem.release();
        });
        t->start(1000);

        return true;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnFileSave()
{
    if (m_savingLevel)
    {
        return;
    }

    const QScopedValueRollback<bool> rollback(m_savingLevel, true);
    
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    if (!usePrefabSystemForLevels)
    {
        GetIEditor()->GetDocument()->DoFileSave();
    }
    else
    {
        auto* prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        auto* prefabIntegrationInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabIntegrationInterface>::Get();
        AZ_Assert(prefabEditorEntityOwnershipInterface != nullptr, "PrefabEditorEntityOwnershipInterface is not found.");
        AZ_Assert(prefabIntegrationInterface != nullptr, "PrefabIntegrationInterface is not found.");
        AzToolsFramework::Prefab::TemplateId rootPrefabTemplateId = prefabEditorEntityOwnershipInterface->GetRootPrefabTemplateId();
        prefabIntegrationInterface->ExecuteSavePrefabDialog(rootPrefabTemplateId, true);
    }
}


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateDocumentReady(QAction* action)
{
    action->setEnabled(GetIEditor()
        && GetIEditor()->GetDocument()
        && GetIEditor()->GetDocument()->IsDocumentReady()
        && !m_bIsExportingLegacyData
        && !m_creatingNewLevel
        && !m_openingLevel
        && !m_savingLevel);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateFileOpen(QAction* action)
{
    action->setEnabled(!m_bIsExportingLegacyData && !m_creatingNewLevel && !m_openingLevel && !m_savingLevel);
}

bool CCryEditApp::ShowEnableDisableGemDialog(const QString& title, const QString& message)
{
    const QString informativeMessage = QObject::tr("Please follow the instructions <a href=\"http://docs.aws.amazon.com/lumberyard/latest/userguide/gems-system-gems.html\">here</a>, after which the Editor will be re-launched automatically.");

    QMessageBox box(AzToolsFramework::GetActiveWindow());
    box.addButton(QObject::tr("Continue"), QMessageBox::AcceptRole);
    box.addButton(QObject::tr("Back"), QMessageBox::RejectRole);
    box.setWindowTitle(title);
    box.setText(message);
    box.setInformativeText(informativeMessage);
    box.setWindowFlags(box.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (box.exec() == QMessageBox::AcceptRole)
    {
        // Called from a modal dialog with the main window as its parent. Best not to close the main window while the dialog is still active.
        QTimer::singleShot(0, MainWindow::instance(), &MainWindow::close);
        return true;
    }

    return false;
}

QString CCryEditApp::ShowWelcomeDialog()
{
    WelcomeScreenDialog wsDlg(MainWindow::instance());
    wsDlg.SetRecentFileList(GetRecentFileList());
    wsDlg.exec();
    QString levelName = wsDlg.GetLevelPath();
    return levelName;
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::InitDirectory()
{
    //////////////////////////////////////////////////////////////////////////
    // Initializes Root folder of the game.
    //////////////////////////////////////////////////////////////////////////
    QString szExeFileName = qApp->applicationDirPath();
    const static char* s_engineMarkerFile = "engine.json";

    while (!QFile::exists(QString("%1/%2").arg(szExeFileName, s_engineMarkerFile)))
    {
        QDir currentdir(szExeFileName);
        if (!currentdir.cdUp())
        {
            break;
        }
        szExeFileName = currentdir.absolutePath();
    }
    QDir::setCurrent(szExeFileName);
}


//////////////////////////////////////////////////////////////////////////
// Needed to work with custom memory manager.
//////////////////////////////////////////////////////////////////////////

CCryEditDoc* CCrySingleDocTemplate::OpenDocumentFile(const char* lpszPathName, bool bMakeVisible /*= true*/)
{
    return OpenDocumentFile(lpszPathName, true, bMakeVisible);
}

CCryEditDoc* CCrySingleDocTemplate::OpenDocumentFile(const char* lpszPathName, bool addToMostRecentFileList, [[maybe_unused]] bool bMakeVisible)
{
    CCryEditDoc* pCurDoc = GetIEditor()->GetDocument();

    if (pCurDoc)
    {
        if (!pCurDoc->SaveModified())
        {
            return nullptr;
        }
    }

    if (!pCurDoc)
    {
        pCurDoc = qobject_cast<CCryEditDoc*>(m_documentClass->newInstance());
        if (pCurDoc == nullptr)
            return nullptr;
        pCurDoc->setParent(this);
    }

    pCurDoc->SetModifiedFlag(false);
    if (lpszPathName == nullptr)
    {
        pCurDoc->SetTitle(tr("Untitled"));
        pCurDoc->OnNewDocument();
    }
    else
    {
        pCurDoc->OnOpenDocument(lpszPathName);
        pCurDoc->SetPathName(lpszPathName);
        if (addToMostRecentFileList)
        {
            CCryEditApp::instance()->AddToRecentFileList(lpszPathName);
        }
    }

    return pCurDoc;
}

CCrySingleDocTemplate::Confidence CCrySingleDocTemplate::MatchDocType(const char* lpszPathName, CCryEditDoc*& rpDocMatch)
{
    assert(lpszPathName != nullptr);
    rpDocMatch = nullptr;

    // go through all documents
    CCryEditDoc* pDoc = GetIEditor()->GetDocument();
    if (pDoc)
    {
        QString prevPathName = pDoc->GetLevelPathName();
        // all we need to know here is whether it is the same file as before.
        if (!prevPathName.isEmpty())
        {
            // QFileInfo is guaranteed to return true iff the two paths refer to the same path.
            if (QFileInfo(prevPathName) == QFileInfo(QString::fromUtf8(lpszPathName)))
            {
                // already open
                rpDocMatch = pDoc;
                return yesAlreadyOpen;
            }
        }
    }

    // see if it matches our default suffix

    const QString strFilterExt = EditorUtils::LevelFile::GetDefaultFileExtension();
    const QString strOldFilterExt = EditorUtils::LevelFile::GetOldCryFileExtension();
    const QString strSliceFilterExt = AzToolsFramework::SliceUtilities::GetSliceFileExtension().c_str();

    // see if extension matches
    assert(strFilterExt[0] == '.');
    QString strDot = "." + Path::GetExt(lpszPathName);
    if (!strDot.isEmpty())
    {
        if(strDot == strFilterExt || strDot == strOldFilterExt || strDot == strSliceFilterExt)
        {
            return yesAttemptNative; // extension matches, looks like ours
        }
    }
    // otherwise we will guess it may work
    return yesAttemptForeign;
}

/////////////////////////////////////////////////////////////////////////////
namespace
{
    AZStd::mutex g_splashScreenStateLock;
    enum ESplashScreenState
    {
        eSplashScreenState_Init, eSplashScreenState_Started, eSplashScreenState_Destroy
    };
    ESplashScreenState g_splashScreenState = eSplashScreenState_Init;
    IInitializeUIInfo* g_pInitializeUIInfo = nullptr;
    QWidget* g_splashScreen = nullptr;
}

QString FormatVersion([[maybe_unused]] const SFileVersion& v)
{
    return QObject::tr("Version %1").arg(LY_VERSION_BUILD_NUMBER);
}

QString FormatRichTextCopyrightNotice()
{
    // copyright symbol is HTML Entity = &#xA9;
    QString copyrightHtmlSymbol = "&#xA9;";
    QString copyrightString = QObject::tr("Copyright %1 Contributors to the Open 3D Engine Project");

    return copyrightString.arg(copyrightHtmlSymbol);
}

/////////////////////////////////////////////////////////////////////////////
void CCryEditApp::ShowSplashScreen(CCryEditApp* app)
{
    g_splashScreenStateLock.lock();

    CStartupLogoDialog* splashScreen = new CStartupLogoDialog(FormatVersion(app->m_pEditor->GetFileVersion()), FormatRichTextCopyrightNotice());

    g_pInitializeUIInfo = splashScreen;
    g_splashScreen = splashScreen;
    g_splashScreenState = eSplashScreenState_Started;

    g_splashScreenStateLock.unlock();

    splashScreen->show();
    // Make sure the initial paint of the splash screen occurs so we dont get stuck with a blank window
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    QObject::connect(splashScreen, &QObject::destroyed, splashScreen, [=]
    {
        AZStd::scoped_lock lock(g_splashScreenStateLock);
        g_pInitializeUIInfo = nullptr;
        g_splashScreen = nullptr;
    });
}

/////////////////////////////////////////////////////////////////////////////
void CCryEditApp::CreateSplashScreen()
{
    if (!m_bConsoleMode && !IsInAutotestMode())
    {
        // Create startup output splash
        ShowSplashScreen(this);

        GetIEditor()->Notify(eNotify_OnSplashScreenCreated);
    }
    else
    {
        // Create console
        m_pConsoleDialog = new CConsoleDialog();
        m_pConsoleDialog->show();

        g_pInitializeUIInfo = m_pConsoleDialog;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CCryEditApp::CloseSplashScreen()
{
    if (CStartupLogoDialog::instance())
    {
        delete CStartupLogoDialog::instance();
        g_splashScreenStateLock.lock();
        g_splashScreenState = eSplashScreenState_Destroy;
        g_splashScreenStateLock.unlock();
    }

    GetIEditor()->Notify(eNotify_OnSplashScreenDestroyed);
}

/////////////////////////////////////////////////////////////////////////////
void CCryEditApp::OutputStartupMessage(QString str)
{
    g_splashScreenStateLock.lock();
    if (g_pInitializeUIInfo)
    {
        g_pInitializeUIInfo->SetInfoText(str.toUtf8().data());
    }
    g_splashScreenStateLock.unlock();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::InitFromCommandLine(CEditCommandLineInfo& cmdInfo)
{
    m_bConsoleMode |= cmdInfo.m_bConsoleMode;
    inEditorBatchMode = AZ::Environment::CreateVariable<bool>("InEditorBatchMode", m_bConsoleMode);

    m_bTestMode |= cmdInfo.m_bTest;

    m_bSkipWelcomeScreenDialog = cmdInfo.m_bSkipWelcomeScreenDialog || !cmdInfo.m_execFile.isEmpty() || !cmdInfo.m_execLineCmd.isEmpty() || cmdInfo.m_bAutotestMode;
    m_bExportMode = cmdInfo.m_bExport;
    m_bRunPythonTestScript = cmdInfo.m_bRunPythonTestScript;
    m_bRunPythonScript = cmdInfo.m_bRunPythonScript || cmdInfo.m_bRunPythonTestScript;
    m_execFile = cmdInfo.m_execFile;
    m_execLineCmd = cmdInfo.m_execLineCmd;
    m_bAutotestMode = cmdInfo.m_bAutotestMode || cmdInfo.m_bConsoleMode;

    m_pEditor->SetMatEditMode(cmdInfo.m_bMatEditMode);

    if (m_bExportMode)
    {
        m_exportFile = cmdInfo.m_exportFile;
    }

    // Do we have a passed filename ?
    if (!cmdInfo.m_strFileName.isEmpty())
    {
        if (!m_bRunPythonScript && IsPreviewableFileType(cmdInfo.m_strFileName.toUtf8().constData()))
        {
            m_bPreviewMode = true;
            azstrncpy(m_sPreviewFile, _MAX_PATH, cmdInfo.m_strFileName.toUtf8().constData(), _MAX_PATH);
        }
    }

    if (cmdInfo.m_bAutoLoadLevel)
    {
        m_bLevelLoadTestMode = true;
        gEnv->bNoAssertDialog = true;
        CEditorAutoLevelLoadTest::Instance();
    }
}

/////////////////////////////////////////////////////////////////////////////
AZ::Outcome<void, AZStd::string> CCryEditApp::InitGameSystem(HWND hwndForInputSystem)
{
    CGameEngine* pGameEngine = new CGameEngine;

    AZ::Outcome<void, AZStd::string> initOutcome = pGameEngine->Init(m_bPreviewMode, m_bTestMode, qApp->arguments().join(" ").toUtf8().data(), g_pInitializeUIInfo, hwndForInputSystem);
    if (!initOutcome.IsSuccess())
    {
        return initOutcome;
    }

    AZ_Assert(pGameEngine, "Game engine initialization failed, but initOutcome returned success.");

    m_pEditor->SetGameEngine(pGameEngine);

    // needs to be called after CrySystem has been loaded.
    gSettings.LoadDefaultGamePaths();

    return AZ::Success();
}

/////////////////////////////////////////////////////////////////////////////
bool CCryEditApp::CheckIfAlreadyRunning()
{
    bool bForceNewInstance = false;

    if (!m_bPreviewMode)
    {
        FixDanglingSharedMemory(O3DEApplicationName);
        m_mutexApplication = new QSharedMemory(O3DEApplicationName);
        if (!m_mutexApplication->create(16))
        {
            // Don't prompt the user in non-interactive export mode.  Instead, default to allowing multiple instances to
            // run simultaneously, so that multiple level exports can be run in parallel on the same machine.
            // NOTE:  If you choose to do this, be sure to export *different* levels, since nothing prevents multiple runs
            // from trying to write to the same level at the same time.
            // If we're running interactively, let's ask and make sure the user actually intended to do this.
            if (!m_bExportMode && QMessageBox::question(AzToolsFramework::GetActiveWindow(), QObject::tr("Too many apps"), QObject::tr("There is already an Open 3D Engine application running\nDo you want to start another one?")) != QMessageBox::Yes)
            {
                return false;
            }

            bForceNewInstance = true;
        }
    }

    if (!FirstInstance(bForceNewInstance))
    {
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool CCryEditApp::InitGame()
{
    if (!m_bPreviewMode && !GetIEditor()->IsInMatEditMode())
    {
        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        Log((QString("project_path = %1").arg(!projectPath.empty() ? projectPath.c_str() : "<not set>")).toUtf8().data());

        ICVar* pVar = gEnv->pConsole->GetCVar("sys_localization_folder");
        const char* sLocalizationFolder = pVar ? pVar->GetString() : nullptr;
        Log((QString("sys_localization_folder = ") + (sLocalizationFolder && sLocalizationFolder[0] ? sLocalizationFolder : "<not set>")).toUtf8().data());

        OutputStartupMessage("Starting Game...");

        if (!GetIEditor()->GetGameEngine()->InitGame(nullptr))
        {
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Apply settings post engine initialization.
    GetIEditor()->GetDisplaySettings()->PostInitApply();
    gSettings.PostInitApply();
    return true;
}

/////////////////////////////////////////////////////////////////////////////
void CCryEditApp::InitPlugins()
{
    OutputStartupMessage("Loading Plugins...");
    // Load the plugins
    {
        GetIEditor()->LoadPlugins();

#if defined(AZ_PLATFORM_WINDOWS)
        C3DConnexionDriver* p3DConnexionDriver = new C3DConnexionDriver;
        GetIEditor()->GetPluginManager()->RegisterPlugin(0, p3DConnexionDriver);
#endif
    }
}

////////////////////////////////////////////////////////////////////////////
// Be careful when calling this function: it should be called after
// everything else has finished initializing, otherwise, certain things
// aren't set up yet. If in doubt, wrap it in a QTimer::singleShot(0ms);
void CCryEditApp::InitLevel(const CEditCommandLineInfo& cmdInfo)
{
    const char* defaultExtension = EditorUtils::LevelFile::GetDefaultFileExtension();
    const char* oldExtension = EditorUtils::LevelFile::GetOldCryFileExtension();

    if (m_bPreviewMode)
    {
        GetIEditor()->EnableAcceleratos(false);

        // Load geometry object.
        if (!cmdInfo.m_strFileName.isEmpty())
        {
            LoadFile(cmdInfo.m_strFileName);
        }
    }
    else if (m_bExportMode && !m_exportFile.isEmpty())
    {
        GetIEditor()->SetModifiedFlag(false);
        GetIEditor()->SetModifiedModule(eModifiedNothing);
        auto pDocument = OpenDocumentFile(m_exportFile.toUtf8().constData());
        if (pDocument)
        {
            GetIEditor()->SetModifiedFlag(false);
            GetIEditor()->SetModifiedModule(eModifiedNothing);
            ExportLevel(cmdInfo.m_bExport, cmdInfo.m_bExportTexture, true);
            // Terminate process.
            CLogFile::WriteLine("Editor: Terminate Process after export");
        }
        // the call to quit() must be posted to the event queue because the app is currently not yet running.
        // if we were to call quit() right now directly, the app would ignore it.
        QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        return;
    }
    else if ((cmdInfo.m_strFileName.endsWith(defaultExtension, Qt::CaseInsensitive))
            || (cmdInfo.m_strFileName.endsWith(oldExtension, Qt::CaseInsensitive)))
    {
        auto pDocument = OpenDocumentFile(cmdInfo.m_strFileName.toUtf8().constData());
        if (pDocument)
        {
            GetIEditor()->SetModifiedFlag(false);
            GetIEditor()->SetModifiedModule(eModifiedNothing);
        }
    }
    else
    {
        //////////////////////////////////////////////////////////////////////////
        //It can happen that if you are switching between projects and you have auto load set that
        //you could inadvertently load the wrong project and not know it, you would think you are editing
        //one level when in fact you are editing the old one. This can happen if both projects have the same
        //relative path... which is often the case when branching.
        // Ex. D:\cryengine\dev\ gets branched to D:\cryengine\branch\dev
        // Now you have gamesdk in both roots and therefore GameSDK\Levels\Singleplayer\Forest in both
        // If you execute the branch the m_pRecentFileList will be an absolute path to the old gamesdk,
        // then if auto load is set simply takes the old level and loads it in the new branch...
        //I would question ever trying to load a level not in our gamesdk, what happens when there are things that
        //an not exist in the level when built in a different gamesdk.. does it erase them, most likely, then you
        //just screwed up the level for everyone in the other gamesdk...
        //So if we are auto loading a level outside our current gamesdk we should act as though the flag
        //was unset and pop the dialog which should be in the correct location. This is not fool proof, but at
        //least this its a compromise that doesn't automatically do something you probably shouldn't.
        bool autoloadLastLevel = gSettings.bAutoloadLastLevelAtStartup;
        if (autoloadLastLevel
            && GetRecentFileList()
            && GetRecentFileList()->GetSize())
        {
            QString gamePath = Path::GetEditingGameDataFolder().c_str();
            Path::ConvertSlashToBackSlash(gamePath);
            gamePath = Path::ToUnixPath(gamePath.toLower());
            gamePath = Path::AddSlash(gamePath);

            QString fullPath = GetRecentFileList()->m_arrNames[0];
            Path::ConvertSlashToBackSlash(fullPath);
            fullPath = Path::ToUnixPath(fullPath.toLower());
            fullPath = Path::AddSlash(fullPath);

            if (fullPath.indexOf(gamePath, 0) != 0)
            {
                autoloadLastLevel = false;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        QString levelName;
        bool isLevelNameValid = false;
        bool doLevelNeedLoading = true;
        const bool runningPythonScript = cmdInfo.m_bRunPythonScript || cmdInfo.m_bRunPythonTestScript;

        AZ::EBusLogicalResult<bool, AZStd::logical_or<bool> > skipStartupUIProcess(false);
        EBUS_EVENT_RESULT(skipStartupUIProcess, AzToolsFramework::EditorEvents::Bus, SkipEditorStartupUI);

        if (!skipStartupUIProcess.value)
        {
            do
            {
                isLevelNameValid = false;
                doLevelNeedLoading = true;
                if (gSettings.bShowDashboardAtStartup
                    && !runningPythonScript
                    && !GetIEditor()->IsInMatEditMode()
                    && !m_bConsoleMode
                    && !m_bSkipWelcomeScreenDialog
                    && !m_bPreviewMode
                    && !autoloadLastLevel)
                {
                    levelName = ShowWelcomeDialog();
                }
                else if (autoloadLastLevel
                         && GetRecentFileList()
                         && GetRecentFileList()->GetSize())
                {
                    levelName = GetRecentFileList()->m_arrNames[0];
                }

                if (levelName.isEmpty())
                {
                    break;
                }
                if (levelName == "new")
                {
                    //implies that the user has clicked the create new level option
                    bool wasCreateLevelOperationCancelled = false;
                    bool isNewLevelCreationSuccess = false;
                    // This will show the new level dialog until a valid input has been entered by the user or until the user click cancel
                    while (!isNewLevelCreationSuccess && !wasCreateLevelOperationCancelled)
                    {
                        isNewLevelCreationSuccess = CreateLevel(wasCreateLevelOperationCancelled);
                        if (isNewLevelCreationSuccess == true)
                        {
                            doLevelNeedLoading = false;
                            isLevelNameValid = true;
                        }
                    }
                    ;
                }
                else if (levelName == "new slice")
                {
                    QMessageBox::warning(AzToolsFramework::GetActiveWindow(), "Not implemented", "New Slice is not yet implemented.");
                }
                else
                {
                    //implies that the user wants to open an existing level
                    doLevelNeedLoading = true;
                    isLevelNameValid = true;
                }
            } while (!isLevelNameValid);// if we reach here and levelName is not valid ,it implies that the user has clicked cancel on the create new level dialog

            // load level
            if (doLevelNeedLoading && !levelName.isEmpty())
            {
                if (!CFileUtil::Exists(levelName, false))
                {
                    QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Missing level"), QObject::tr("Failed to auto-load last opened level. Level file does not exist:\n\n%1").arg(levelName));
                    return;
                }

                QString str;
                str = tr("Loading level %1 ...").arg(levelName);
                OutputStartupMessage(str);

                OpenDocumentFile(levelName.toUtf8().data());
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
bool CCryEditApp::InitConsole()
{
    // Execute command from cmdline -exec_line if applicable
    if (!m_execLineCmd.isEmpty())
    {
        gEnv->pConsole->ExecuteString(QString("%1").arg(m_execLineCmd).toLocal8Bit());
    }

    // Execute cfg from cmdline -exec if applicable
    if (!m_execFile.isEmpty())
    {
        gEnv->pConsole->ExecuteString(QString("exec %1").arg(m_execFile).toLocal8Bit());
    }

    // Execute special configs.
    gEnv->pConsole->ExecuteString("exec editor_autoexec.cfg");
    gEnv->pConsole->ExecuteString("exec editor.cfg");
    gEnv->pConsole->ExecuteString("exec user.cfg");

    GetISystem()->ExecuteCommandLine();

    return true;
}

/////////////////////////////////////////////////////////////////////////////

void CCryEditApp::CompileCriticalAssets() const
{
    // regardless of what is set in the bootstrap wait for AP to be ready
    // wait a maximum of 100 milliseconds and pump the system event loop until empty
    struct AssetsInQueueNotification
        : public AzFramework::AssetSystemInfoBus::Handler
    {
        void CountOfAssetsInQueue(const int& count) override
        {
            CCryEditApp::OutputStartupMessage(QString("Asset Processor working... %1 jobs remaining.").arg(count));
        }
    };
    AssetsInQueueNotification assetsInQueueNotifcation;
    assetsInQueueNotifcation.BusConnect();
    bool ready{};
    while (!ready)
    {
        AzFramework::AssetSystemRequestBus::BroadcastResult(ready, &AzFramework::AssetSystemRequestBus::Events::WaitUntilAssetProcessorReady, AZStd::chrono::milliseconds(100));
        if (!ready)
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
        }
    }
    assetsInQueueNotifcation.BusDisconnect();
    CCryEditApp::OutputStartupMessage(QString("Asset Processor is now ready."));

}

bool CCryEditApp::ConnectToAssetProcessor() const
{
    bool connectedToAssetProcessor = false;

    // When the AssetProcessor is already launched it should take less than a second to perform a connection
    // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
    // and able to negotiate a connection when running a debug build
    // and to negotiate a connection

    // Setting the connectTimeout to 3 seconds if not set within the settings registry
    AZStd::chrono::seconds connectTimeout(3);
    // Initialize the launchAssetProcessorTimeout to 15 seconds by default and check the settings registry for an override
    AZStd::chrono::seconds launchAssetProcessorTimeout(15);
    AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
    if (settingsRegistry)
    {
        AZ::s64 timeoutValue{};
        if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, timeoutValue,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "connect_ap_timeout"))
        {
            connectTimeout = AZStd::chrono::seconds(timeoutValue);
        }

        // Reset timeout integer
        timeoutValue = {};
        if (AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, timeoutValue,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "launch_ap_timeout"))
        {
            launchAssetProcessorTimeout = AZStd::chrono::seconds(timeoutValue);
        }
    }

    CCryEditApp::OutputStartupMessage(QString("Connecting to Asset Processor... "));

    AzFramework::AssetSystem::ConnectionSettings connectionSettings;
    AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);

    connectionSettings.m_launchAssetProcessorOnFailedConnection = true;
    connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
    connectionSettings.m_connectionIdentifier = AzFramework::AssetSystem::ConnectionIdentifiers::Editor;
    connectionSettings.m_loggingCallback = [](AZStd::string_view logData)
    {
        CCryEditApp::OutputStartupMessage(QString::fromUtf8(logData.data(), aznumeric_cast<int>(logData.size())));
    };

    AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

    if (connectedToAssetProcessor)
    {
        CCryEditApp::OutputStartupMessage(QString("Connected to Asset Processor"));
        CompileCriticalAssets();
        return true;
    }

    CCryEditApp::OutputStartupMessage(QString("Failed to connect to Asset Processor"));
    return false;
}

//! This handles the normal logging of Python output in the Editor by outputting
//! the data to both the Editor Console and the Editor.log file
struct CCryEditApp::PythonOutputHandler
    : public AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
{
    PythonOutputHandler()
    {
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();
    }

    ~PythonOutputHandler() override
    {
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    int GetOrder() override
    {
        return 0;
    }

    void OnTraceMessage([[maybe_unused]] AZStd::string_view message) override
    {
        AZ_TracePrintf("python_test", "%.*s", static_cast<int>(message.size()), message.data());
    }

    void OnErrorMessage([[maybe_unused]] AZStd::string_view message) override
    {
        AZ_Error("python_test", false, "%.*s", static_cast<int>(message.size()), message.data());
    }

    void OnExceptionMessage([[maybe_unused]] AZStd::string_view message) override
    {
        AZ_Error("python_test", false, "EXCEPTION: %.*s", static_cast<int>(message.size()), message.data());
    }
};

//! Outputs Python test script print() to stdout
//! If an exception happens in a Python test script, the process terminates
struct PythonTestOutputHandler final
    : public CCryEditApp::PythonOutputHandler
{
    PythonTestOutputHandler() = default;
    ~PythonTestOutputHandler() override = default;

    void OnTraceMessage(AZStd::string_view message) override
    {
        PythonOutputHandler::OnTraceMessage(message);
        printf("%.*s\n", static_cast<int>(message.size()), message.data());
    }

    void OnErrorMessage(AZStd::string_view message) override
    {
        PythonOutputHandler::OnErrorMessage(message);
        printf("ERROR: %.*s\n", static_cast<int>(message.size()), message.data());
    }

    void OnExceptionMessage(AZStd::string_view message) override
    {
        PythonOutputHandler::OnExceptionMessage(message);
        printf("EXCEPTION: %.*s\n", static_cast<int>(message.size()), message.data());
    }
};

void CCryEditApp::RunInitPythonScript(CEditCommandLineInfo& cmdInfo)
{
    if (cmdInfo.m_bRunPythonTestScript)
    {
        m_pythonOutputHandler = AZStd::make_shared<PythonTestOutputHandler>();
    }
    else
    {
        m_pythonOutputHandler = AZStd::make_shared<PythonOutputHandler>();
    }

    using namespace AzToolsFramework;
    if (cmdInfo.m_bRunPythonScript || cmdInfo.m_bRunPythonTestScript)
    {
        // cmdInfo data is only available on startup, copy it
        QByteArray fileStr = cmdInfo.m_strFileName.toUtf8();

        // We support specifying multiple files in the cmdline by separating them with ';'
        AZStd::vector<AZStd::string_view> fileList;
        AzFramework::StringFunc::TokenizeVisitor(
            fileStr.constData(),
            [&fileList](AZStd::string_view elem)
            {
                fileList.push_back(elem);
            }, ';', false /* keepEmptyStrings */
        );

        if (cmdInfo.m_pythonArgs.length() > 0 || cmdInfo.m_bRunPythonTestScript)
        {
            QByteArray pythonArgsStr = cmdInfo.m_pythonArgs.toUtf8();
            AZStd::vector<AZStd::string_view> pythonArgs;
            AzFramework::StringFunc::TokenizeVisitor(pythonArgsStr.constData(),
                [&pythonArgs](AZStd::string_view elem)
                {
                    pythonArgs.push_back(elem);
                }, ' '
            );

            if (cmdInfo.m_bRunPythonTestScript)
            {
                // Multiple testcases can be specified them with ';', these should match the files to run
                AZStd::vector<AZStd::string_view> testcaseList;
                QByteArray pythonTestCase = cmdInfo.m_pythonTestCase.toUtf8();
                testcaseList.resize(fileList.size());
                {
                    int i = 0;
                    AzFramework::StringFunc::TokenizeVisitor(
                        pythonTestCase.constData(),
                        [&i, &testcaseList](AZStd::string_view elem)
                        {
                            testcaseList[i++] = (elem);
                        }, ';', false /* keepEmptyStrings */
                    );
                }

                bool success = true;
                auto ExecuteByFilenamesTests = [&pythonArgs, &fileList, &testcaseList, &success](EditorPythonRunnerRequests* pythonRunnerRequests)
                {
                    for (int i = 0; i < fileList.size(); ++i)
                    {
                        bool cur_success = pythonRunnerRequests->ExecuteByFilenameAsTest(fileList[i], testcaseList[i], pythonArgs);
                        success = success && cur_success;
                    }
                };
                EditorPythonRunnerRequestBus::Broadcast(ExecuteByFilenamesTests);

                if (success)
                {
                    // Close the editor gracefully as the test has completed
                    GetIEditor()->GetDocument()->SetModifiedFlag(false);
                    QTimer::singleShot(0, qApp, &QApplication::closeAllWindows);
                }
                else
                {
                    // Close down the application with 0xF exit code indicating failure of the test
                    AZ::Debug::Trace::Terminate(0xF);
                }
            }
            else
            {
                auto ExecuteByFilenamesWithArgs = [&pythonArgs, &fileList](EditorPythonRunnerRequests* pythonRunnerRequests)
                {
                    for (AZStd::string_view filename : fileList)
                    {
                        pythonRunnerRequests->ExecuteByFilenameWithArgs(filename, pythonArgs);
                    }
                };
                EditorPythonRunnerRequestBus::Broadcast(ExecuteByFilenamesWithArgs);
            }
        }
        else
        {
            auto ExecuteByFilenames = [&fileList](EditorPythonRunnerRequests* pythonRunnerRequests)
            {
                for (AZStd::string_view filename : fileList)
                {
                    pythonRunnerRequests->ExecuteByFilename(filename);
                }
            };
            EditorPythonRunnerRequestBus::Broadcast(ExecuteByFilenames);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CCryEditApp initialization
bool CCryEditApp::InitInstance()
{
    QElapsedTimer startupTimer;
    startupTimer.start();
    InitDirectory();

    // create / attach to the environment:
    AttachEditorCoreAZEnvironment(AZ::Environment::GetInstance());
    m_pEditor = new CEditorImpl();

    // parameters must be parsed early to capture arguments for test bootstrap
    CEditCommandLineInfo cmdInfo;

    InitFromCommandLine(cmdInfo);

    InitDirectory();

    qobject_cast<Editor::EditorQtApplication*>(qApp)->Initialize(); // Must be done after CEditorImpl() is created
    m_pEditor->Initialize();

    // let anything listening know that they can use the IEditor now
    AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyIEditorAvailable, m_pEditor);

    if (cmdInfo.m_bShowVersionInfo)
    {
        CAboutDialog aboutDlg(FormatVersion(m_pEditor->GetFileVersion()), FormatRichTextCopyrightNotice());
        aboutDlg.exec();
        return false;
    }

    // Reflect property control classes to the serialize context...
    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
    AZ_Assert(serializeContext, "Serialization context not available");
    ReflectedVarInit::setupReflection(serializeContext);
    RegisterReflectedVarHandlers();

    CreateSplashScreen();

    // Register the application's document templates. Document templates
    // serve as the connection between documents, frame windows and views
    CCrySingleDocTemplate* pDocTemplate = CCrySingleDocTemplate::create<CCryEditDoc>();

    m_pDocManager = new CCryDocManager;
    ((CCryDocManager*)m_pDocManager)->SetDefaultTemplate(pDocTemplate);

    auto mainWindow = new MainWindow();
#ifdef Q_OS_MACOS
    auto mainWindowWrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionDisabled);
#else
    // No need for mainwindow wrapper for MatEdit mode
    auto mainWindowWrapper = new AzQtComponents::WindowDecorationWrapper(cmdInfo.m_bMatEditMode ? AzQtComponents::WindowDecorationWrapper::OptionDisabled
        : AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
#endif
    mainWindowWrapper->setGuest(mainWindow);
    HWND mainWindowWrapperHwnd = (HWND)mainWindowWrapper->winId();

    AZ::IO::FixedMaxPath engineRootPath;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
    }
    QDir engineRoot = QString::fromUtf8(engineRootPath.c_str(), aznumeric_cast<int>(engineRootPath.Native().size()));
    AzQtComponents::StyleManager::addSearchPaths(
        QStringLiteral("style"),
        engineRoot.filePath(QStringLiteral("Code/Editor/Style")),
        QStringLiteral(":/Assets/Editor/Style"),
        engineRootPath);
    AzQtComponents::StyleManager::setStyleSheet(mainWindow, QStringLiteral("style:Editor.qss"));

    // Note: we should use getNativeHandle to get the HWND from the widget, but
    // it returns an invalid handle unless the widget has been shown and polished and even then
    // it sometimes returns an invalid handle.
    // So instead, we use winId(), which does consistently work
    //mainWindowWrapperHwnd = QtUtil::getNativeHandle(mainWindowWrapper);

    // Connect to the AssetProcessor at this point
    // It will be launched if not running
    ConnectToAssetProcessor();

    auto initGameSystemOutcome = InitGameSystem(mainWindowWrapperHwnd);
    if (!initGameSystemOutcome.IsSuccess())
    {
        return false;
    }

    if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get())
    {
        AZ::ComponentApplicationLifecycle::SignalEvent(*settingsRegistry, "LegacySystemInterfaceCreated", R"({})");
    }

    // Process some queued events come from system init
    // Such as asset catalog loaded notification.
    // There are some systems need to load configurations from assets for post initialization but before loading level
    AZ::TickBus::ExecuteQueuedEvents();

    qobject_cast<Editor::EditorQtApplication*>(qApp)->LoadSettings();

    // Create Sandbox user folder if necessary
    AZ::IO::FileIOBase::GetDirectInstance()->CreatePath(Path::GetUserSandboxFolder().toUtf8().data());

    if (!InitGame())
    {
        if (gEnv && gEnv->pLog)
        {
            gEnv->pLog->LogError("Game can not be initialized, InitGame() failed.");
        }
        if (!cmdInfo.m_bExport)
        {
            QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Game can not be initialized, please refer to the editor log file"));
        }
        return false;
    }

    // Meant to be called before MainWindow::Initialize
    InitPlugins();

    mainWindow->Initialize();

    GetIEditor()->GetCommandManager()->RegisterAutoCommands();
    GetIEditor()->AddUIEnums();

    mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "O3DE", "mainWindowGeometry");
    m_pDocManager->OnFileNew();

    if (IsInRegularEditorMode())
    {
        // QuickAccessBar creation should be before m_pMainWnd->SetFocus(),
        // since it receives the focus at creation time. It brakes MainFrame key accelerators.
        m_pQuickAccessBar = new CQuickAccessBar;
        m_pQuickAccessBar->setVisible(false);
    }

    if (MainWindow::instance())
    {
        if (m_bConsoleMode || IsInAutotestMode())
        {
            AZ::Environment::FindVariable<int>("assertVerbosityLevel").Set(1);
            m_pConsoleDialog->raise();
        }
        else if (!GetIEditor()->IsInMatEditMode())
        {
            MainWindow::instance()->show();
            MainWindow::instance()->raise();
            MainWindow::instance()->update();
            MainWindow::instance()->setFocus();

#if AZ_TRAIT_OS_PLATFORM_APPLE
            QWindow* window = mainWindowWrapper->windowHandle();
            if (window)
            {
                Editor::WindowObserver* observer = new Editor::WindowObserver(window, this);
                connect(observer, &Editor::WindowObserver::windowIsMovingOrResizingChanged, Editor::EditorQtApplication::instance(), &Editor::EditorQtApplication::setIsMovingOrResizing);
            }
#endif
        }
    }

    if (m_bAutotestMode)
    {
        ICVar* const noErrorReportWindowCVar = gEnv && gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_no_error_report_window") : nullptr;
        if (noErrorReportWindowCVar)
        {
            noErrorReportWindowCVar->Set(true);
        }
        ICVar* const showErrorDialogOnLoadCVar = gEnv && gEnv->pConsole ? gEnv->pConsole->GetCVar("ed_showErrorDialogOnLoad") : nullptr;
        if (showErrorDialogOnLoadCVar)
        {
            showErrorDialogOnLoadCVar->Set(false);
        }
    }

    SetEditorWindowTitle(nullptr, AZ::Utils::GetProjectName().c_str(), nullptr);
    if (!GetIEditor()->IsInMatEditMode())
    {
        m_pEditor->InitFinished();
    }

    // Make sure Python is started before we attempt to restore the Editor layout, since the user
    // might have custom view panes in the saved layout that will need to be registered.
    auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
    if (editorPythonEventsInterface)
    {
        editorPythonEventsInterface->StartPython();
    }

    if (!GetIEditor()->IsInMatEditMode() && !GetIEditor()->IsInConsolewMode())
    {
        bool restoreDefaults = !mainWindowWrapper->restoreGeometryFromSettings();
        QtViewPaneManager::instance()->RestoreLayout(restoreDefaults);
    }

    CloseSplashScreen();

    // DON'T CHANGE ME!
    // Test scripts listen for this line, so please don't touch this without updating them.
    // We consider ourselves "initialized enough" at this stage because all further initialization may be blocked by the modal welcome screen.
    CLogFile::WriteLine(QString("Engine initialized, took %1s.").arg(startupTimer.elapsed() / 1000.0, 0, 'f', 2));

    // Init the level after everything else is finished initializing, otherwise, certain things aren't set up yet
    QTimer::singleShot(0, this, [this, cmdInfo] {
        InitLevel(cmdInfo);
    });

#ifdef USE_WIP_FEATURES_MANAGER
    // load the WIP features file
    CWipFeatureManager::Instance()->EnableManager(!cmdInfo.m_bDeveloperMode);
    CWipFeatureManager::Init();
#endif

    if (!m_bConsoleMode && !m_bPreviewMode)
    {
        GetIEditor()->UpdateViews();
        if (MainWindow::instance())
        {
            MainWindow::instance()->setFocus();
        }
    }

    if (!InitConsole())
    {
        return true;
    }

    if (IsInRegularEditorMode())
    {
        int startUpMacroIndex = GetIEditor()->GetToolBoxManager()->GetMacroIndex("startup", true);
        if (startUpMacroIndex >= 0)
        {
            CryLogAlways("Executing the startup macro");
            GetIEditor()->GetToolBoxManager()->ExecuteMacro(startUpMacroIndex, true);
        }
    }

    if (GetIEditor()->GetCommandManager()->IsRegistered("editor.open_lnm_editor"))
    {
        CCommand0::SUIInfo uiInfo;
#if !defined(NDEBUG)
        bool ok =
#endif
            GetIEditor()->GetCommandManager()->GetUIInfo("editor.open_lnm_editor", uiInfo);
        assert(ok);
    }

    RunInitPythonScript(cmdInfo);

    return true;
}

void CCryEditApp::RegisterEventLoopHook(IEventLoopHook* pHook)
{
    pHook->pNextHook = m_pEventLoopHook;
    m_pEventLoopHook = pHook;
}

void CCryEditApp::UnregisterEventLoopHook(IEventLoopHook* pHookToRemove)
{
    IEventLoopHook* pPrevious = nullptr;
    for (IEventLoopHook* pHook = m_pEventLoopHook; pHook != nullptr; pHook = pHook->pNextHook)
    {
        if (pHook == pHookToRemove)
        {
            if (pPrevious)
            {
                pPrevious->pNextHook = pHookToRemove->pNextHook;
            }
            else
            {
                m_pEventLoopHook = pHookToRemove->pNextHook;
            }

            pHookToRemove->pNextHook = nullptr;
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::LoadFile(QString fileName)
{
    if (GetIEditor()->GetViewManager()->GetViewCount() == 0)
    {
        return;
    }

    LoadTagLocations();

    if (MainWindow::instance() || m_pConsoleDialog)
    {
        SetEditorWindowTitle(nullptr, AZ::Utils::GetProjectName().c_str(), GetIEditor()->GetGameEngine()->GetLevelName());
    }

    GetIEditor()->SetModifiedFlag(false);
    GetIEditor()->SetModifiedModule(eModifiedNothing);
}

//////////////////////////////////////////////////////////////////////////
inline void ExtractMenuName(QString& str)
{
    // eliminate &
    int pos = str.indexOf('&');
    if (pos >= 0)
    {
        str = str.left(pos) + str.right(str.length() - pos - 1);
    }
    // cut the string
    for (int i = 0; i < str.length(); i++)
    {
        if (str[i] == 9)
        {
            str = str.left(i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::EnableAccelerator([[maybe_unused]] bool bEnable)
{
    /*
    if (bEnable)
    {
        //LoadAccelTable( MAKEINTRESOURCE(IDR_MAINFRAME) );
        m_AccelManager.UpdateWndTable();
        CLogFile::WriteLine( "Enable Accelerators" );
    }
    else
    {
        CMainFrame *mainFrame = (CMainFrame*)m_pMainWnd;
        if (mainFrame->m_hAccelTable)
            DestroyAcceleratorTable( mainFrame->m_hAccelTable );
        mainFrame->m_hAccelTable = nullptr;
        mainFrame->LoadAccelTable( MAKEINTRESOURCE(IDR_GAMEACCELERATOR) );
        CLogFile::WriteLine( "Disable Accelerators" );
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::SaveAutoRemind()
{
    // Added a static variable here to avoid multiple messageboxes to
    // remind the user of saving the file. Many message boxes would appear as this
    // is triggered by a timer even which does not stop when the message box is called.
    // Used a static variable instead of a member variable because this value is not
    // Needed anywhere else.
    static bool boIsShowingWarning(false);

    // Ingore in game mode, or if no level created, or level not modified
    if (GetIEditor()->IsInGameMode() || !GetIEditor()->GetGameEngine()->IsLevelLoaded() || !GetIEditor()->GetDocument()->IsModified())
    {
        return;
    }

    if (boIsShowingWarning)
    {
        return;
    }

    boIsShowingWarning = true;
    if (QMessageBox::question(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Auto Reminder: You did not save level for at least %1 minute(s)\r\nDo you want to save it now?").arg(gSettings.autoRemindTime), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // Save now.
        GetIEditor()->SaveDocument();
    }
    boIsShowingWarning = false;
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::WriteConfig()
{
    IEditor* pEditor = GetIEditor();
    if (pEditor && pEditor->GetDisplaySettings())
    {
        pEditor->GetDisplaySettings()->SaveRegistry();
    }
}

// App command to run the dialog
void CCryEditApp::OnAppAbout()
{
    CAboutDialog aboutDlg(FormatVersion(m_pEditor->GetFileVersion()), FormatRichTextCopyrightNotice());
    aboutDlg.exec();
}

// App command to run the Welcome to Open 3D Engine dialog
void CCryEditApp::OnAppShowWelcomeScreen()
{
    // This logic is a simplified version of the startup
    // flow that also shows the Welcome dialog

    if (m_bIsExportingLegacyData
        || m_creatingNewLevel
        || m_openingLevel
        || m_savingLevel)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QString(), "The Welcome screen cannot be displayed because a level load/save is in progress.");
        return;
    }

    QString levelName;
    bool showWelcomeDialog = true;
    while (showWelcomeDialog)
    {
        // Keep showing the Welcome dialog as long as the user cancels
        // a level creation/load triggered from the Welcome dialog
        levelName = ShowWelcomeDialog();

        if (levelName == "new")
        {
            // The user has clicked on the create new level option
            bool wasCreateLevelOperationCancelled = false;
            bool isNewLevelCreationSuccess = false;
            // This will show the new level dialog until a valid input has been entered by the user or until the user click cancel
            while (!isNewLevelCreationSuccess && !wasCreateLevelOperationCancelled)
            {
                isNewLevelCreationSuccess = CreateLevel(wasCreateLevelOperationCancelled);
            }

            if (isNewLevelCreationSuccess)
            {
                showWelcomeDialog = false;
                levelName.clear();
            }
        }
        else if (levelName == "new slice")
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), "Not implemented", "New Slice is not yet implemented.");
        }
        else
        {
            // The user has selected an existing level to open
            showWelcomeDialog = false;
        }
    }

    if (!levelName.isEmpty())
    {
        // load level
        if (!CFileUtil::Exists(levelName, false))
        {
            QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Missing level"), QObject::tr("Failed to auto-load last opened level. Level file does not exist:\n\n%1").arg(levelName));
        }
        else
        {
            OpenDocumentFile(levelName.toUtf8().data());
        }
    }
}

void CCryEditApp::OnUpdateShowWelcomeScreen(QAction* action)
{
    action->setEnabled(!m_bIsExportingLegacyData
        && !m_creatingNewLevel
        && !m_openingLevel
        && !m_savingLevel);
}

void CCryEditApp::OnDocumentationTutorials()
{
    QString webLink = tr("https://o3deorg.netlify.app/docs/learning-guide/");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationGlossary()
{
    QString webLink = tr("https://docs.o3de.org/docs/user-guide/appendix/glossary/");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationO3DE()
{
    QString webLink = tr("https://o3deorg.netlify.app/docs/");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationGamelift()
{
    QString webLink = tr("https://docs.aws.amazon.com/gamelift/");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationReleaseNotes()
{
    QString webLink = tr("https://o3deorg.netlify.app/docs/release-notes/");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationGameDevBlog()
{
    QString webLink = tr("https://aws.amazon.com/blogs/gamedev");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationForums()
{
    QString webLink = tr("https://o3deorg.netlify.app/community/");
    QDesktopServices::openUrl(QUrl(webLink));
}

void CCryEditApp::OnDocumentationAWSSupport()
{
    QString webLink = tr("https://aws.amazon.com/contact-us");
    QDesktopServices::openUrl(QUrl(webLink));
}

bool CCryEditApp::FixDanglingSharedMemory(const QString& sharedMemName) const
{
    QSystemSemaphore sem(sharedMemName + "_sem", 1);
    sem.acquire();
    {
        QSharedMemory fix(sharedMemName);
        if (!fix.attach())
        {
            if (fix.error() != QSharedMemory::NotFound)
            {
                sem.release();
                return false;
            }
        }
        // fix.detach() when destructed, taking out any dangling shared memory
        // on unix
    }
    sem.release();
    return true;
}

/////////////////////////////////////////////////////////////////////////////
// CCryEditApp message handlers


int CCryEditApp::ExitInstance(int exitCode)
{
    AZ_TracePrintf("Exit", "Called ExitInstance() with exit code: 0x%x", exitCode);

    if (m_pEditor)
    {
        m_pEditor->OnBeginShutdownSequence();
    }
    qobject_cast<Editor::EditorQtApplication*>(qApp)->UnloadSettings();

    #ifdef USE_WIP_FEATURES_MANAGER
    //
    // close wip features manager
    //
    CWipFeatureManager::Shutdown();
    #endif

    if (IsInRegularEditorMode())
    {
        if (GetIEditor())
        {
            int shutDownMacroIndex = GetIEditor()->GetToolBoxManager()->GetMacroIndex("shutdown", true);
            if (shutDownMacroIndex >= 0)
            {
                CryLogAlways("Executing the shutdown macro");
                GetIEditor()->GetToolBoxManager()->ExecuteMacro(shutDownMacroIndex, true);
            }
        }
    }

    if (GetIEditor() && !GetIEditor()->IsInMatEditMode())
    {
        //Nobody seems to know in what case that kind of exit can happen so instrumented to see if it happens at all
        if (m_pEditor)
        {
            m_pEditor->OnEarlyExitShutdownSequence();
        }

        gEnv->pLog->FlushAndClose();

        // note: the intention here is to quit immediately without processing anything further
        // on linux and mac, _exit has that effect
        // however, on windows, _exit() still invokes CRT functions, unloads, and destructors
        // so on windows, we need to use TerminateProcess
#if defined(AZ_PLATFORM_WINDOWS)
       TerminateProcess(GetCurrentProcess(), exitCode);
#else

        _exit(exitCode);
#endif
    }

    SAFE_DELETE(m_pConsoleDialog);
    SAFE_DELETE(m_pQuickAccessBar);

    if (GetIEditor())
    {
        GetIEditor()->Notify(eNotify_OnQuit);
    }

    // if we're aborting due to an unexpected shutdown then don't call into objects that don't exist yet.
    if ((gEnv) && (gEnv->pSystem) && (gEnv->pSystem->GetILevelSystem()))
    {
        gEnv->pSystem->GetILevelSystem()->UnloadLevel();
    }

    if (GetIEditor())
    {
        GetIEditor()->GetDocument()->DeleteTemporaryLevel();
    }

    m_bExiting = true;

    HEAP_CHECK
    ////////////////////////////////////////////////////////////////////////
    // Executed directly before termination of the editor, just write a
    // quick note to the log so that we can later see that the editor
    // terminated flawlessly. Also delete temporary files.
    ////////////////////////////////////////////////////////////////////////
        WriteConfig();

    if (m_pEditor)
    {
        // Ensure component entities are wiped prior to unloading plugins,
        // since components may be implemented in those plugins.
        EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, ResetEditorContext);

        // vital, so that the Qt integration can unhook itself!
        m_pEditor->UnloadPlugins();
        m_pEditor->Uninitialize();
    }

    //////////////////////////////////////////////////////////////////////////
    // Quick end for editor.
    if (gEnv && gEnv->pSystem)
    {
        gEnv->pSystem->Quit();
        SAFE_RELEASE(gEnv->pSystem);
    }
    //////////////////////////////////////////////////////////////////////////

    if (m_pEditor)
    {
        m_pEditor->DeleteThis();
        m_pEditor = nullptr;
    }

    // save accelerator manager configuration.
    //m_AccelManager.SaveOnExit();

#ifdef WIN32
    Gdiplus::GdiplusShutdown(m_gdiplusToken);
#endif

    if (m_mutexApplication)
    {
        delete m_mutexApplication;
    }

    DetachEditorCoreAZEnvironment();
    return 0;
}

bool CCryEditApp::IsWindowInForeground()
{
    return Editor::EditorQtApplication::instance()->IsActive();
}

void CCryEditApp::DisableIdleProcessing()
{
    m_disableIdleProcessingCounter++;
}

void CCryEditApp::EnableIdleProcessing()
{
    m_disableIdleProcessingCounter--;
    AZ_Assert(m_disableIdleProcessingCounter >= 0, "m_disableIdleProcessingCounter must be nonnegative");
}

bool CCryEditApp::OnIdle([[maybe_unused]] LONG lCount)
{
    if (0 == m_disableIdleProcessingCounter)
    {
        return IdleProcessing(false);
    }
    else
    {
        return false;
    }
}

int CCryEditApp::IdleProcessing(bool bBackgroundUpdate)
{
    AZ_Assert(m_disableIdleProcessingCounter == 0, "We should not be in IdleProcessing()");

    //HEAP_CHECK
    if (!MainWindow::instance())
    {
        return 0;
    }

    if (!GetIEditor()->GetSystem())
    {
        return 0;
    }

    // Ensure we don't get called re-entrantly
    // This can occur when a nested Qt event loop fires (e.g. by way of a modal dialog calling exec)
    if (m_idleProcessingRunning)
    {
        return 0;
    }
    QScopedValueRollback<bool> guard(m_idleProcessingRunning, true);

    ////////////////////////////////////////////////////////////////////////
    // Call the update function of the engine
    ////////////////////////////////////////////////////////////////////////
    if (m_bTestMode && !bBackgroundUpdate)
    {
        // Terminate process.
        CLogFile::WriteLine("Editor: Terminate Process");
        exit(0);
    }

    bool bIsAppWindow = IsWindowInForeground();
    bool bActive = false;
    int res = 0;
    if (bIsAppWindow || m_bForceProcessIdle || m_bKeepEditorActive
        // Automated tests must always keep the editor active, or they can get stuck
        || m_bAutotestMode || m_bRunPythonTestScript)
    {
        res = 1;
        bActive = true;
    }

    if (m_bForceProcessIdle && bIsAppWindow)
    {
        m_bForceProcessIdle = false;
    }

    // focus changed
    if (m_bPrevActive != bActive)
    {
        GetIEditor()->GetSystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, bActive, 0);
    #if defined(AZ_PLATFORM_WINDOWS)
        // This is required for the audio system to be notified of focus changes in the editor.  After discussing it
        // with the macOS team, they are working on unifying the system events between the editor and standalone
        // launcher so this is only needed on windows.
        if (bActive)
        {
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnSetFocus);
        }
        else
        {
            EBUS_EVENT(AzFramework::WindowsLifecycleEvents::Bus, OnKillFocus);
        }
    #endif
    }

    m_bPrevActive = bActive;

    // Don't tick application if we're doing idle processing during an assert.
    const bool isErrorWindowVisible = (gEnv && gEnv->pSystem->IsAssertDialogVisible());
    if (isErrorWindowVisible)
    {
        if (m_pEditor)
        {
            m_pEditor->Update();
        }
    }
    else if (bActive || (bBackgroundUpdate && !bIsAppWindow))
    {
        // Update Game
        GetIEditor()->GetGameEngine()->Update();

        if (!GetIEditor()->IsInGameMode())
        {
            if (m_pEditor)
            {
                m_pEditor->Update();
            }

            GetIEditor()->Notify(eNotify_OnIdleUpdate);
        }

        AZ::ComponentApplication* componentApplication = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(componentApplication, &AZ::ComponentApplicationRequests::GetApplication);
        if (componentApplication)
        {
            componentApplication->TickSystem();
        }
    }
    else if (GetIEditor()->GetSystem() && GetIEditor()->GetSystem()->GetILog())
    {
        GetIEditor()->GetSystem()->GetILog()->Update(); // print messages from other threads
    }

    DisplayLevelLoadErrors();

    if (CConsoleSCB::GetCreatedInstance())
    {
        CConsoleSCB::GetCreatedInstance()->FlushText();
    }

    return res;
}

void CCryEditApp::DisplayLevelLoadErrors()
{
    CCryEditDoc* currentLevel = GetIEditor()->GetDocument();
    if (currentLevel && currentLevel->IsDocumentReady() && !m_levelErrorsHaveBeenDisplayed)
    {
        // Generally it takes a few idle updates for meshes to load and be processed by their components. This value
        // was picked based on examining when mesh components are updated and their materials are checked for
        // errors (2 updates) plus one more for good luck.
        const int IDLE_FRAMES_TO_WAIT = 3;
        ++m_numBeforeDisplayErrorFrames;
        if (m_numBeforeDisplayErrorFrames > IDLE_FRAMES_TO_WAIT)
        {
            GetIEditor()->CommitLevelErrorReport();
            GetIEditor()->GetErrorReport()->Display();
            m_numBeforeDisplayErrorFrames = 0;
            m_levelErrorsHaveBeenDisplayed = true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::ExportLevel(bool bExportToGame, bool bExportTexture, bool bAutoExport)
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
    if (usePrefabSystemForLevels)
    {
        AZ_Assert(false, "Prefab system doesn't require level exports.");
        return;
    }

    if (bExportTexture)
    {
        CGameExporter gameExporter;
        gameExporter.SetAutoExportMode(bAutoExport);
        gameExporter.Export(eExp_SurfaceTexture);
    }
    else if (bExportToGame)
    {
        CGameExporter gameExporter;
        gameExporter.SetAutoExportMode(bAutoExport);
        gameExporter.Export();
    }
}


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnEditHold()
{
    GetIEditor()->GetDocument()->Hold(HOLD_FETCH_FILE);
}


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnEditFetch()
{
    GetIEditor()->GetDocument()->Fetch(HOLD_FETCH_FILE);
}


//////////////////////////////////////////////////////////////////////////
bool CCryEditApp::UserExportToGame(bool bNoMsgBox)
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
    if (usePrefabSystemForLevels)
    {
        AZ_Assert(false, "Export Level should no longer exist.");
        return false;
    }

    if (!GetIEditor()->GetGameEngine()->IsLevelLoaded())
    {
        if (bNoMsgBox == false)
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Please load a level before attempting to export."));
        }
        return false;
    }
    else
    {
        EditorUtils::AzWarningAbsorber absorb("Source Control");

        // Record errors and display a dialog with them at the end.
        CErrorsRecorder errRecorder(GetIEditor());

        // Temporarily disable auto backup.
        CScopedVariableSetter<bool> autoBackupEnabledChange(gSettings.autoBackupEnabled, false);
        CScopedVariableSetter<int> autoRemindTimeChange(gSettings.autoRemindTime, 0);

        m_bIsExportingLegacyData = true;
        CGameExporter gameExporter;

        unsigned int flags = eExp_CoverSurfaces;

        // Change the cursor to show that we're busy.
        QWaitCursor wait;

        if (gameExporter.Export(flags, eLittleEndian, "."))
        {
            m_bIsExportingLegacyData = false;
            return true;
        }
        m_bIsExportingLegacyData = false;
        return false;
    }
}

void CCryEditApp::ExportToGame(bool bNoMsgBox)
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
    if (usePrefabSystemForLevels)
    {
        AZ_Assert(false, "Prefab system no longer exports levels.");
        return;
    }

    CGameEngine* pGameEngine = GetIEditor()->GetGameEngine();
    if (!pGameEngine->IsLevelLoaded())
    {
        if (pGameEngine->GetLevelPath().isEmpty())
        {
            QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), ("Open or create a level first."));
            return;
        }

        CErrorsRecorder errRecorder(GetIEditor());
        // If level not loaded first fast export terrain.
        m_bIsExportingLegacyData = true;
        CGameExporter gameExporter;
        gameExporter.Export();
        m_bIsExportingLegacyData = false;
    }

    {
        UserExportToGame(bNoMsgBox);
    }
}

void CCryEditApp::OnFileExportToGameNoSurfaceTexture()
{
    UserExportToGame(false);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::DeleteSelectedEntities([[maybe_unused]] bool includeDescendants)
{
    GetIEditor()->BeginUndo();
    CUndo undo("Delete Selected Object");
    GetIEditor()->GetObjectManager()->DeleteSelection();
    GetIEditor()->AcceptUndo("Delete Selection");
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedBrushes);
}

void CCryEditApp::OnMoveObject()
{
    ////////////////////////////////////////////////////////////////////////
    // Move the selected object to the marker position
    ////////////////////////////////////////////////////////////////////////
}

void CCryEditApp::OnRenameObj()
{
}

void CCryEditApp::OnViewSwitchToGame()
{
    if (IsInPreviewMode())
    {
        return;
    }
    // close all open menus
    auto activePopup = qApp->activePopupWidget();
    if (qobject_cast<QMenu*>(activePopup))
    {
        activePopup->hide();
    }
    // TODO: Add your command handler code here
    bool inGame = !GetIEditor()->IsInGameMode();
    GetIEditor()->SetInGameMode(inGame);
}

void CCryEditApp::OnViewSwitchToGameFullScreen()
{
    ed_previewGameInFullscreen_once = true;
    OnViewSwitchToGame();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnExportSelectedObjects()
{
    CExportManager* pExportManager = static_cast<CExportManager*> (GetIEditor()->GetExportManager());
    QString filename = "untitled";
    CBaseObject* pObj = GetIEditor()->GetSelectedObject();
    if (pObj)
    {
        filename = pObj->GetName();
    }
    else
    {
        QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();
        if (!levelName.isEmpty())
        {
            filename = levelName;
        }
    }
    QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
    pExportManager->Export(filename.toUtf8().data(), "obj", levelPath.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnFileExportOcclusionMesh()
{
    CExportManager* pExportManager = static_cast<CExportManager*> (GetIEditor()->GetExportManager());
    QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();
    QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
    pExportManager->Export(levelName.toUtf8().data(), "ocm", levelPath.toUtf8().data(), false, false, true);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenAssetImporter()
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::SceneSettings);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateSelected(QAction* action)
{
    action->setEnabled(!GetIEditor()->GetSelection()->IsEmpty());
}

void CCryEditApp::OnShowHelpers()
{
    GetIEditor()->GetDisplaySettings()->DisplayHelpers(!GetIEditor()->GetDisplaySettings()->IsDisplayHelpers());
    GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnEditLevelData()
{
    auto dir = QFileInfo(GetIEditor()->GetDocument()->GetLevelPathName()).dir();
    CFileUtil::EditTextFile(dir.absoluteFilePath("leveldata.xml").toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnFileEditLogFile()
{
    CFileUtil::EditTextFile(CLogFile::GetLogFileName(), 0, IFileUtil::FILE_TYPE_SCRIPT);
}

void CCryEditApp::OnFileResaveSlices()
{
    AZStd::vector<AZ::Data::AssetInfo> sliceAssetInfos;
    sliceAssetInfos.reserve(5000);
    AZ::Data::AssetCatalogRequests::AssetEnumerationCB sliceCountCb = [&sliceAssetInfos]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
    {
        // Only add slices and nothing that has been temporarily added to the catalog with a macro in it (ie @engroot@)
        if (info.m_assetType == azrtti_typeid<AZ::SliceAsset>() && info.m_relativePath[0] != '@')
        {
            sliceAssetInfos.push_back(info);
        }
    };
    AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, sliceCountCb, nullptr);

    QString warningMessage = QString("Resaving all slices can be *extremely* slow depending on source control and on the number of slices in your project!\n\nYou can speed this up dramatically by checking out all your slices before starting this!\n\n Your project has %1 slices.\n\nDo you want to continue?").arg(sliceAssetInfos.size());

    if (QMessageBox::Cancel == QMessageBox::warning(MainWindow::instance(), tr("!!!WARNING!!!"), warningMessage, QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
    {
        return;
    }

    AZ::SerializeContext* serialize = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serialize, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

    if (!serialize)
    {
        AZ_TracePrintf("Resave Slices", "Couldn't get the serialize context.  Something is very wrong.  Aborting!!!");
        return;
    }

    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (!fileIO)
    {
        AZ_Error("Resave Slices", false, "File IO is not initialized.");
        return;
    }

    int numFailures = 0;

    // Create a lambda for load & save logic to make the lambda below easier to read
    auto LoadAndSaveSlice = [serialize, &numFailures](const AZStd::string& filePath)
    {
        AZ::Entity* newRootEntity = nullptr;

        // Read in the slice file first
        {
            AZ::IO::FileIOStream readStream(filePath.c_str(), AZ::IO::OpenMode::ModeRead);
            newRootEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(readStream, serialize, AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading));
        }

        // If we successfully loaded the file
        if (newRootEntity)
        {
            if (!AZ::Utils::SaveObjectToFile(filePath, AZ::DataStream::ST_XML, newRootEntity))
            {
                AZ_TracePrintf("Resave Slices", "Unable to serialize the slice (%s) out to a file.  Unable to resave this slice\n", filePath.c_str());
                numFailures++;
            }
        }
        else
        {
            AZ_TracePrintf("Resave Slices", "Unable to read a slice (%s) file from disk.  Unable to resave this slice.\n", filePath.c_str());
            numFailures++;
        }
    };

    const size_t numSlices = sliceAssetInfos.size();
    int slicesProcessed = 0;
    int slicesRequestedForProcessing = 0;

    if (numSlices > 0)
    {
        AzToolsFramework::ProgressShield::LegacyShowAndWait(MainWindow::instance(), tr("Checking out and resaving slices..."),
            [numSlices, &slicesProcessed, &sliceAssetInfos, &LoadAndSaveSlice, &slicesRequestedForProcessing, &numFailures](int& current, int& max)
            {
                const static int numToProcessPerCall = 5;

                if (slicesRequestedForProcessing < numSlices)
                {
                    for (int index = 0; index < numToProcessPerCall; index++)
                    {
                        if (slicesRequestedForProcessing < numSlices)
                        {
                            AZStd::string sourceFile;
                            AzToolsFramework::AssetSystemRequestBus::Broadcast(&AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, sliceAssetInfos[slicesRequestedForProcessing].m_relativePath, sourceFile);

                            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditForFile, sourceFile.c_str(), [&slicesProcessed, sourceFile, &LoadAndSaveSlice, &numFailures](bool success)
                                {
                                    slicesProcessed++;

                                    if (success)
                                    {
                                        LoadAndSaveSlice(sourceFile);
                                    }
                                    else
                                    {
                                        AZ_TracePrintf("Resave Slices", "Unable to check a slice (%s) out of source control.  Unable to resave this slice\n", sourceFile.c_str());
                                        numFailures++;
                                    }
                                }
                            );
                            slicesRequestedForProcessing++;
                        }
                    }
                }

                current = slicesProcessed;
                max = static_cast<int>(numSlices);
                return slicesProcessed == numSlices;
            }
        );

        QString completeMessage;
        if (numFailures > 0)
        {
            completeMessage = QString("All slices processed.  There were %1 slices that could not be resaved.  Please check the console for details.").arg(numFailures);
        }
        else
        {
            completeMessage = QString("All slices successfully process and re-saved!");
        }

        QMessageBox::information(MainWindow::instance(), tr("Re-saving complete"), completeMessage, QMessageBox::Ok);
    }
    else
    {
        QMessageBox::information(MainWindow::instance(), tr("No slices found"), tr("There were no slices found to resave."), QMessageBox::Ok);
    }

}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnFileEditEditorini()
{
    CFileUtil::EditTextFile(EDITOR_CFG_FILE);
}

void CCryEditApp::OnPreferences()
{
    /*
    //////////////////////////////////////////////////////////////////////////////
    // Accels edit by CPropertyPage
    CAcceleratorManager tmpAccelManager;
    tmpAccelManager = m_AccelManager;
    CAccelMapPage page(&tmpAccelManager);
    CPropertySheet sheet;
    sheet.SetTitle( _T("Preferences") );
    sheet.AddPage(&page);
    if (sheet.DoModal() == IDOK) {
        m_AccelManager = tmpAccelManager;
        m_AccelManager.UpdateWndTable();
    }
    */
}

void CCryEditApp::OnOpenProjectManagerSettings()
{
    OpenProjectManager("UpdateProject");
}

void CCryEditApp::OnOpenProjectManagerNew()
{
    OpenProjectManager("CreateProject");
}

void CCryEditApp::OnOpenProjectManager()
{
    OpenProjectManager("Projects");
}

void CCryEditApp::OpenProjectManager(const AZStd::string& screen)
{
    // provide the current project path for in case we want to update the project
    AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
#if !AZ_TRAIT_OS_PLATFORM_APPLE && !AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    const char* argumentQuoteString = R"(")";
#else
    const char* argumentQuoteString = R"(\")";
#endif
    const AZStd::string commandLineOptions = AZStd::string::format(R"( --screen %s --project-path %s%s%s)",
        screen.c_str(),
        argumentQuoteString, projectPath.c_str(), argumentQuoteString);
    bool launchSuccess = AzFramework::ProjectManager::LaunchProjectManager(commandLineOptions);
    if (!launchSuccess)
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Failed to launch O3DE Project Manager"), QObject::tr("Failed to find or start the O3dE Project Manager"));
    }
}


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUndo()
{
    //GetIEditor()->GetObjectManager()->UndoLastOp();
    GetIEditor()->Undo();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnRedo()
{
    GetIEditor()->Redo();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateRedo(QAction* action)
{
    if (GetIEditor()->GetUndoManager()->IsHaveRedo())
    {
        action->setEnabled(true);
    }
    else
    {
        action->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateUndo(QAction* action)
{
    if (GetIEditor()->GetUndoManager()->IsHaveUndo())
    {
        action->setEnabled(true);
    }
    else
    {
        action->setEnabled(false);
    }
}

/// Undo command to track entering and leaving Simulation Mode.
class SimulationModeCommand
    : public AzToolsFramework::UndoSystem::URSequencePoint
{
public:
    AZ_CLASS_ALLOCATOR(SimulationModeCommand, AZ::SystemAllocator, 0);
    AZ_RTTI(SimulationModeCommand, "{FB9FB958-5C56-47F6-B168-B5F564F70E69}");

    SimulationModeCommand(const AZStd::string& friendlyName);

    void Undo() override;
    void Redo() override;

    bool Changed() const override { return true; } // State will always have changed.

private:
    void UndoRedo()
    {
        if (ActionManager* actionManager = MainWindow::instance()->GetActionManager())
        {
            if (auto* action = actionManager->GetAction(ID_SWITCH_PHYSICS))
            {
                action->trigger();
            }
        }
    }
};

SimulationModeCommand::SimulationModeCommand(const AZStd::string& friendlyName)
    : URSequencePoint(friendlyName)
{
}

void SimulationModeCommand::Undo()
{
    UndoRedo();
}

void SimulationModeCommand::Redo()
{
    UndoRedo();
}

namespace UndoRedo
{
    static bool IsHappening()
    {
        bool undoRedo = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            undoRedo, &AzToolsFramework::ToolsApplicationRequests::IsDuringUndoRedo);

        return undoRedo;
    }
} // namespace UndoRedo

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSwitchPhysics()
{
    if (GetIEditor()->GetGameEngine() && !GetIEditor()->GetGameEngine()->GetSimulationMode() && !GetIEditor()->GetGameEngine()->IsLevelLoaded())
    {
        // Don't allow physics to be toggled on if we haven't loaded a level yet
        return;
    }

    QWaitCursor wait;

    AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> undoBatch;
    if (!UndoRedo::IsHappening())
    {
        undoBatch = AZStd::make_unique<AzToolsFramework::ScopedUndoBatch>("Switching Physics Simulation");

        auto simulationModeCommand = AZStd::make_unique<SimulationModeCommand>(AZStd::string("Switch Physics"));
        // simulationModeCommand managed by undoBatch
        simulationModeCommand->SetParent(undoBatch->GetUndoBatch());
        simulationModeCommand.release();
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_SWITCH_START, 0, 0);

    uint32 flags = GetIEditor()->GetDisplaySettings()->GetSettings();
    if (flags & SETTINGS_PHYSICS)
    {
        flags &= ~SETTINGS_PHYSICS;
    }
    else
    {
        flags |= SETTINGS_PHYSICS;
    }

    GetIEditor()->GetDisplaySettings()->SetSettings(flags);

    if ((flags & SETTINGS_PHYSICS) == 0)
    {
        GetIEditor()->GetGameEngine()->SetSimulationMode(false);
        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED, 0, 0);
    }
    else
    {
        GetIEditor()->GetGameEngine()->SetSimulationMode(true);
        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED, 1, 0);
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_SWITCH_END, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSwitchPhysicsUpdate(QAction* action)
{
    Q_ASSERT(action->isCheckable());
    action->setChecked(!m_bIsExportingLegacyData && GetIEditor()->GetGameEngine()->GetSimulationMode());
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSyncPlayer()
{
    GetIEditor()->GetGameEngine()->SyncPlayerPosition(!GetIEditor()->GetGameEngine()->IsSyncPlayerPosition());
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSyncPlayerUpdate(QAction* action)
{
    Q_ASSERT(action->isCheckable());
    action->setChecked(!GetIEditor()->GetGameEngine()->IsSyncPlayerPosition());
}

void CCryEditApp::OnUpdateNonGameMode(QAction* action)
{
    action->setEnabled(!GetIEditor()->IsInGameMode());
}

void CCryEditApp::OnUpdateNewLevel(QAction* action)
{
    action->setEnabled(!m_bIsExportingLegacyData);
}

void CCryEditApp::OnUpdatePlayGame(QAction* action)
{
    action->setEnabled(!m_bIsExportingLegacyData && GetIEditor()->IsLevelLoaded());
}

//////////////////////////////////////////////////////////////////////////
CCryEditApp::ECreateLevelResult CCryEditApp::CreateLevel(const QString& levelName, QString& fullyQualifiedLevelName /* ={} */)
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    // If we are creating a new level and we're in simulate mode, then switch it off before we do anything else
    if (GetIEditor()->GetGameEngine() && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        // Preserve the modified flag, we don't want this switch of physics to change that flag
        bool bIsDocModified = GetIEditor()->GetDocument()->IsModified();
        OnSwitchPhysics();
        GetIEditor()->GetDocument()->SetModifiedFlag(bIsDocModified);

        if (usePrefabSystemForLevels)
        {
            auto* rootSpawnableInterface = AzFramework::RootSpawnableInterface::Get();
            if (rootSpawnableInterface)
            {
                rootSpawnableInterface->ProcessSpawnableQueue();
            }
        }
    }

    const QScopedValueRollback<bool> rollback(m_creatingNewLevel);
    m_creatingNewLevel = true;
    GetIEditor()->Notify(eNotify_OnBeginCreate);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorBeginCreate);

    QString currentLevel = GetIEditor()->GetLevelFolder();
    if (!currentLevel.isEmpty())
    {
        GetIEditor()->GetSystem()->GetIPak()->ClosePacks(currentLevel.toUtf8().data());
    }

    QString cryFileName = levelName.mid(levelName.lastIndexOf('/') + 1, levelName.length() - levelName.lastIndexOf('/') + 1);
    QString levelPath = QStringLiteral("%1/Levels/%2/").arg(Path::GetEditingGameDataFolder().c_str(), levelName);
    fullyQualifiedLevelName = levelPath + cryFileName + EditorUtils::LevelFile::GetDefaultFileExtension();

    //_MAX_PATH includes null terminator, so we actually want to cap at _MAX_PATH-1
    if (fullyQualifiedLevelName.length() >= _MAX_PATH-1)
    {
        GetIEditor()->Notify(eNotify_OnEndCreate);
        return ECLR_MAX_PATH_EXCEEDED;
    }

    // Does the directory already exist ?
    if (QFileInfo(levelPath).exists())
    {
        GetIEditor()->Notify(eNotify_OnEndCreate);
        return ECLR_ALREADY_EXISTS;
    }

    // Create the directory
    CLogFile::WriteLine("Creating level directory");
    if (!CFileUtil::CreatePath(levelPath))
    {
        GetIEditor()->Notify(eNotify_OnEndCreate);
        return ECLR_DIR_CREATION_FAILED;
    }

    if (GetIEditor()->GetDocument()->IsDocumentReady())
    {
        m_pDocManager->OnFileNew();
    }

    ICVar* sv_map = gEnv->pConsole->GetCVar("sv_map");
    if (sv_map)
    {
        sv_map->Set(levelName.toUtf8().data());
    }


    GetIEditor()->GetDocument()->InitEmptyLevel(128, 1);

    GetIEditor()->SetStatusText("Creating Level...");

    // Save the document to this folder
    GetIEditor()->GetDocument()->SetPathName(fullyQualifiedLevelName);
    GetIEditor()->GetGameEngine()->SetLevelPath(levelPath);

    if (usePrefabSystemForLevels)
    {
        auto* service = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        if (service)
        {
            service->CreateNewLevelPrefab(fullyQualifiedLevelName.toUtf8().constData(), DefaultLevelTemplateName);
        }
    }

    if (GetIEditor()->GetDocument()->Save())
    {
        if (!usePrefabSystemForLevels)
        {
            m_bIsExportingLegacyData = true;
            CGameExporter gameExporter;
            gameExporter.Export();
            m_bIsExportingLegacyData = false;
        }

        GetIEditor()->GetGameEngine()->LoadLevel(true, true);
        GetIEditor()->GetSystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_START, 0, 0);

        GetIEditor()->GetSystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_END, 0, 0);
    }

    if (!usePrefabSystemForLevels)
    {
        // No terrain, but still need to export default octree and visarea data.
        CGameExporter gameExporter;
        gameExporter.Export(eExp_CoverSurfaces | eExp_SurfaceTexture, eLittleEndian, ".");
    }

    GetIEditor()->GetDocument()->CreateDefaultLevelAssets(128, 1);
    GetIEditor()->GetDocument()->SetDocumentReady(true);
    GetIEditor()->SetStatusText("Ready");

    // At the end of the creating level process, add this level to the MRU list
    CCryEditApp::instance()->AddToRecentFileList(fullyQualifiedLevelName);

    GetIEditor()->Notify(eNotify_OnEndCreate);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorEndCreate);
    return ECLR_OK;
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnCreateLevel()
{
    if (m_creatingNewLevel)
    {
        return;
    }
    bool wasCreateLevelOperationCancelled = false;
    bool isNewLevelCreationSuccess = false;
    // This will show the new level dialog until a valid input has been entered by the user or until the user click cancel
    while (!isNewLevelCreationSuccess && !wasCreateLevelOperationCancelled)
    {
        wasCreateLevelOperationCancelled = false;
        isNewLevelCreationSuccess = CreateLevel(wasCreateLevelOperationCancelled);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CCryEditApp::CreateLevel(bool& wasCreateLevelOperationCancelled)
{
    bool bIsDocModified = GetIEditor()->GetDocument()->IsModified();
    if (GetIEditor()->GetDocument()->IsDocumentReady() && bIsDocModified)
    {
        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
        if (!usePrefabSystemForLevels)
        {
            QString str = QObject::tr("Level %1 has been changed. Save Level?").arg(GetIEditor()->GetGameEngine()->GetLevelName());
            int result = QMessageBox::question(
                AzToolsFramework::GetActiveWindow(), QObject::tr("Save Level"), str,
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (QMessageBox::Yes == result)
            {
                if (!GetIEditor()->GetDocument()->DoFileSave())
                {
                    // if the file save operation failed, assume that the user was informed of why
                    // already and treat it as a cancel
                    wasCreateLevelOperationCancelled = true;
                    return false;
                }

                bIsDocModified = false;
            }
            else if (QMessageBox::No == result)
            {
                // Set Modified flag to false to prevent show Save unchanged dialog again
                GetIEditor()->GetDocument()->SetModifiedFlag(false);
            }
            else if (QMessageBox::Cancel == result)
            {
                wasCreateLevelOperationCancelled = true;
                return false;
            }
        }
        else
        {
            auto* prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            auto* prefabIntegrationInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabIntegrationInterface>::Get();
            AZ_Assert(prefabEditorEntityOwnershipInterface != nullptr, "PrefabEditorEntityOwnershipInterface is not found.");
            AZ_Assert(prefabIntegrationInterface != nullptr, "PrefabIntegrationInterface is not found.");

            if (prefabEditorEntityOwnershipInterface == nullptr || prefabIntegrationInterface == nullptr)
            {
                return false;
            }

            AzToolsFramework::Prefab::TemplateId rootPrefabTemplateId = prefabEditorEntityOwnershipInterface->GetRootPrefabTemplateId();
            int prefabSaveSelection = prefabIntegrationInterface->ExecuteClosePrefabDialog(rootPrefabTemplateId);

            // In order to get the accept and reject codes of QDialog and QDialogButtonBox aligned, we do (1-prefabSaveSelection) here.
            // For example, QDialog::Rejected(0) is emitted when dialog is closed. But the int value corresponds to
            // QDialogButtonBox::AcceptRole(0).
            switch (1 - prefabSaveSelection)
            {
            case QDialogButtonBox::AcceptRole:
                bIsDocModified = false;
                break;
            case QDialogButtonBox::RejectRole:
                wasCreateLevelOperationCancelled = true;
                return false;
            case QDialogButtonBox::InvalidRole:
                // Set Modified flag to false to prevent show Save unchanged dialog again
                GetIEditor()->GetDocument()->SetModifiedFlag(false);
                break;
            }
        }
    }

    const char* temporaryLevelName = GetIEditor()->GetDocument()->GetTemporaryLevelName();

    CNewLevelDialog dlg;
    dlg.m_level = "";

    if (dlg.exec() != QDialog::Accepted)
    {
        wasCreateLevelOperationCancelled = true;
        GetIEditor()->GetDocument()->SetModifiedFlag(bIsDocModified);
        return false;
    }

    if (!GetIEditor()->GetLevelIndependentFileMan()->PromptChangedFiles())
    {
        return false;
    }

    QString levelNameWithPath = dlg.GetLevel();
    QString levelName = levelNameWithPath.mid(levelNameWithPath.lastIndexOf('/') + 1);

    if (levelName == temporaryLevelName && GetIEditor()->GetLevelName() != temporaryLevelName)
    {
        GetIEditor()->GetDocument()->DeleteTemporaryLevel();
    }

    if (levelName.length() == 0 || !AZ::StringFunc::Path::IsValid(levelName.toUtf8().data()))
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Level name is invalid, please choose another name."));
        return false;
    }

    //Verify that we are not using the temporary level name
    if (QString::compare(levelName, temporaryLevelName) == 0)
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Please enter a level name that is different from the temporary name."));
        return false;
    }

    // We're about to start creating a level, so start recording errors to display at the end.
    GetIEditor()->StartLevelErrorReportRecording();

    QString fullyQualifiedLevelName;
    ECreateLevelResult result = CreateLevel(levelNameWithPath, fullyQualifiedLevelName);

    if (result == ECLR_ALREADY_EXISTS)
    {
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Level with this name already exists, please choose another name."));
        return false;
    }
    else if (result == ECLR_DIR_CREATION_FAILED)
    {
        QString szLevelRoot = QStringLiteral("%1\\Levels\\%2").arg(Path::GetEditingGameDataFolder().c_str(), levelName);

        QByteArray windowsErrorMessage(ERROR_LEN, 0);
        QByteArray cwd(ERROR_LEN, 0);
        DWORD dw = GetLastError();

#ifdef WIN32
        wchar_t windowsErrorMessageW[ERROR_LEN];
        windowsErrorMessageW[0] = L'\0';
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            windowsErrorMessageW,
            ERROR_LEN, nullptr);
        _getcwd(cwd.data(), cwd.length());
        AZStd::to_string(windowsErrorMessage.data(), ERROR_LEN, windowsErrorMessageW);
#else
        windowsErrorMessage = strerror(dw);
        cwd = QDir::currentPath().toUtf8();
#endif

        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Failed to create level directory: %1\n Error: %2\nCurrent Path: %3").arg(szLevelRoot, windowsErrorMessage, cwd));
        return false;
    }
    else if (result == ECLR_MAX_PATH_EXCEEDED)
    {
        QFileInfo info(fullyQualifiedLevelName);
        const AZStd::string rawProjectDirectory = Path::GetEditingGameDataFolder();
        const QString projectDirectory = QDir::toNativeSeparators(QString::fromUtf8(rawProjectDirectory.data(), static_cast<int>(rawProjectDirectory.size())));
        const QString elidedLevelName = QStringLiteral("%1...%2").arg(levelName.left(10)).arg(levelName.right(10));
        const QString elidedLevelFileName = QStringLiteral("%1...%2").arg(info.fileName().left(10)).arg(info.fileName().right(10));
        const QString message = QObject::tr(
            "The fully-qualified path for the new level exceeds the maximum supported path length of %1 characters (it's %2 characters long). Please choose a smaller name.\n\n"
            "The fully-qualified path is made up of the project folder (\"%3\", %4 characters), the \"Levels\" sub-folder, a folder named for the level (\"%5\", %6 characters) and the level file (\"%7\", %8 characters), plus necessary separators.\n\n"
            "Please also note that on most platforms, individual components of the path (folder/file names can't exceed  approximately 255 characters)\n\n"
            "Click \"Copy to Clipboard\" to copy the fully-qualified name and close this message.")
            .arg(_MAX_PATH - 1).arg(fullyQualifiedLevelName.size())
            .arg(projectDirectory).arg(projectDirectory.size())
            .arg(elidedLevelName).arg(levelName.size())
            .arg(elidedLevelFileName).arg(info.fileName().size());
        QMessageBox messageBox(QMessageBox::Critical, QString(), message, QMessageBox::Ok, AzToolsFramework::GetActiveWindow());
        QPushButton* copyButton = messageBox.addButton(QObject::tr("Copy to Clipboard"), QMessageBox::ActionRole);
        QObject::connect(copyButton, &QPushButton::pressed, this, [fullyQualifiedLevelName]() { QGuiApplication::clipboard()->setText(fullyQualifiedLevelName); });
        messageBox.exec();
        return false;
    }

    // force the level being rendered at least once
    m_bForceProcessIdle = true;

    m_levelErrorsHaveBeenDisplayed = false;

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnCreateSlice()
{
    QMessageBox::warning(AzToolsFramework::GetActiveWindow(), "Not implemented", "New Slice is not yet implemented.");
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenLevel()
{
    CLevelFileDialog levelFileDialog(true);
    levelFileDialog.show();
    levelFileDialog.adjustSize();

    if (levelFileDialog.exec() == QDialog::Accepted)
    {
        OpenDocumentFile(levelFileDialog.GetFileName().toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenSlice()
{
    QString fileName = QFileDialog::getOpenFileName(MainWindow::instance(),
        tr("Open Slice"),
        Path::GetEditingGameDataFolder().c_str(),
        tr("Slice (*.slice)"));

    if (!fileName.isEmpty())
    {
        OpenDocumentFile(fileName.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
CCryEditDoc* CCryEditApp::OpenDocumentFile(const char* filename, bool addToMostRecentFileList, COpenSameLevelOptions openSameLevelOptions)
{
    if (m_openingLevel)
    {
        return GetIEditor()->GetDocument();
    }

    // If we are loading and we're in simulate mode, then switch it off before we do anything else
    if (GetIEditor()->GetGameEngine() && GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        // Preserve the modified flag, we don't want this switch of physics to change that flag
        bool bIsDocModified = GetIEditor()->GetDocument()->IsModified();
        OnSwitchPhysics();
        GetIEditor()->GetDocument()->SetModifiedFlag(bIsDocModified);
    }

    // We're about to start loading a level, so start recording errors to display at the end.
    GetIEditor()->StartLevelErrorReportRecording();

    const QScopedValueRollback<bool> rollback(m_openingLevel, true);

    MainWindow::instance()->menuBar()->setEnabled(false);

    CCryEditDoc* doc = nullptr;
    bool bVisible = false;
    bool bTriggerConsole = false;

    doc = GetIEditor()->GetDocument();
    bVisible = GetIEditor()->ShowConsole(true);
    bTriggerConsole = true;

    if (GetIEditor()->GetLevelIndependentFileMan()->PromptChangedFiles())
    {
        SandboxEditor::StartupTraceHandler openDocTraceHandler;
        openDocTraceHandler.StartCollection();
        if (m_bAutotestMode)
        {
            openDocTraceHandler.SetShowWindow(false);
        }

        // in this case, we set addToMostRecentFileList to always be true because adding files to the MRU list
        // automatically culls duplicate and normalizes paths anyway
        m_pDocManager->OpenDocumentFile(filename, addToMostRecentFileList, openSameLevelOptions);

        if (openDocTraceHandler.HasAnyErrors())
        {
            doc->SetHasErrors();
        }
    }

    if (bTriggerConsole)
    {
        GetIEditor()->ShowConsole(bVisible);
    }
    LoadTagLocations();

    MainWindow::instance()->menuBar()->setEnabled(true);

    if (doc->GetEditMode() == CCryEditDoc::DocumentEditingMode::SliceEdit)
    {
        // center camera on entities in slice
        if (ActionManager* actionManager = MainWindow::instance()->GetActionManager())
        {
            GetIEditor()->GetUndoManager()->Suspend();
            actionManager->GetAction(AzToolsFramework::SelectAll)->trigger();
            actionManager->GetAction(ID_GOTO_SELECTED)->trigger();
            GetIEditor()->GetUndoManager()->Resume();
        }
    }

    m_levelErrorsHaveBeenDisplayed = false;

    return doc; // the API wants a CDocument* to be returned. It seems not to be used, though, in our current state.
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnResourcesReduceworkingset()
{
#ifdef WIN32 // no such thing on macOS
    SetProcessWorkingSetSize(GetCurrentProcess(), std::numeric_limits<SIZE_T>::max(), std::numeric_limits<SIZE_T>::max());
#endif
}

void CCryEditApp::OnUpdateWireframe(QAction* action)
{
    Q_ASSERT(action->isCheckable());
    int             nWireframe(R_SOLID_MODE);
    ICVar*      r_wireframe(gEnv->pConsole->GetCVar("r_wireframe"));

    if (r_wireframe)
    {
        nWireframe = r_wireframe->GetIVal();
    }

    action->setChecked(nWireframe == R_WIREFRAME_MODE);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnViewConfigureLayout()
{
    if (GetIEditor()->IsInGameMode())
    {
        // you may not change your viewports while game mode is running.
        CryLog("You may not change viewport configuration while in game mode.");
        return;
    }
    CLayoutWnd* layout = GetIEditor()->GetViewManager()->GetLayout();
    if (layout)
    {
        CLayoutConfigDialog dlg;
        dlg.SetLayout(layout->GetLayout());
        if (dlg.exec() == QDialog::Accepted)
        {
            // Will kill this Pane. so must be last line in this function.
            layout->CreateLayout(dlg.GetLayout());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::TagLocation(int index)
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (!pRenderViewport)
    {
        return;
    }

    Vec3 vPosVec = pRenderViewport->GetViewTM().GetTranslation();

    m_tagLocations[index - 1] = vPosVec;
    m_tagAngles[index - 1] = Ang3::GetAnglesXYZ(Matrix33(pRenderViewport->GetViewTM()));

    QString sTagConsoleText("");
    sTagConsoleText = tr("Camera Tag Point %1 set to the position: x=%2, y=%3, z=%4 ").arg(index).arg(vPosVec.x, 0, 'f', 2).arg(vPosVec.y, 0, 'f', 2).arg(vPosVec.z, 0, 'f', 2);

    GetIEditor()->WriteToConsole(sTagConsoleText.toUtf8().data());

    if (gSettings.bAutoSaveTagPoints)
    {
        SaveTagLocations();
    }
}

void CCryEditApp::SaveTagLocations()
{
    // Save to file.
    QString filename = QFileInfo(GetIEditor()->GetDocument()->GetLevelPathName()).dir().absoluteFilePath("tags.txt");
    QFile f(filename);
    if (f.open(QFile::WriteOnly))
    {
        QTextStream stream(&f);
        for (int i = 0; i < 12; i++)
        {
            stream <<
                m_tagLocations[i].x << "," << m_tagLocations[i].y << "," <<  m_tagLocations[i].z << "," <<
                m_tagAngles[i].x << "," << m_tagAngles[i].y << "," << m_tagAngles[i].z << Qt::endl;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::GotoTagLocation(int index)
{
    QString sTagConsoleText("");
    Vec3 pos = m_tagLocations[index - 1];

    if (!IsVectorsEqual(m_tagLocations[index - 1], Vec3(0, 0, 0)))
    {
        // Change render viewport view TM to the stored one.
        CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
        if (pRenderViewport)
        {
            Matrix34 tm = Matrix34::CreateRotationXYZ(m_tagAngles[index - 1]);
            tm.SetTranslation(pos);
            pRenderViewport->SetViewTM(tm);
            Vec3 vPosVec(tm.GetTranslation());

            GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_BEAM_PLAYER_TO_CAMERA_POS, (UINT_PTR)&tm, 0);

            sTagConsoleText = tr("Moved Camera To Tag Point %1 (x=%2, y=%3, z=%4)").arg(index).arg(vPosVec.x, 0, 'f', 2).arg(vPosVec.y, 0, 'f', 2).arg(vPosVec.z, 0, 'f', 2);
        }
    }
    else
    {
        sTagConsoleText = tr("Camera Tag Point %1 not set").arg(index);
    }

    if (!sTagConsoleText.isEmpty())
    {
        GetIEditor()->WriteToConsole(sTagConsoleText.toUtf8().data());
    }
}


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::LoadTagLocations()
{
    QString filename = QFileInfo(GetIEditor()->GetDocument()->GetLevelPathName()).dir().absoluteFilePath("tags.txt");
    // Load tag locations from file.

    ZeroStruct(m_tagLocations);

    QFile f(filename);
    if (f.open(QFile::ReadOnly))
    {
        QTextStream stream(&f);
        for (int i = 0; i < 12; i++)
        {
            QStringList line = stream.readLine().split(",");
            float x = 0, y = 0, z = 0, ax = 0, ay = 0, az = 0;
            if (line.count() == 6)
            {
                x = line[0].toFloat();
                y = line[1].toFloat();
                z = line[2].toFloat();
                ax = line[3].toFloat();
                ay = line[4].toFloat();
                az = line[5].toFloat();
            }

            m_tagLocations[i] = Vec3(x, y, z);
            m_tagAngles[i] = Ang3(ax, ay, az);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnToolsLogMemoryUsage()
{
    gEnv->pConsole->ExecuteString("SaveLevelStats");
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnTagLocation1() { TagLocation(1); }
void CCryEditApp::OnTagLocation2() { TagLocation(2); }
void CCryEditApp::OnTagLocation3() { TagLocation(3); }
void CCryEditApp::OnTagLocation4() { TagLocation(4); }
void CCryEditApp::OnTagLocation5() { TagLocation(5); }
void CCryEditApp::OnTagLocation6() { TagLocation(6); }
void CCryEditApp::OnTagLocation7() { TagLocation(7); }
void CCryEditApp::OnTagLocation8() { TagLocation(8); }
void CCryEditApp::OnTagLocation9() { TagLocation(9); }
void CCryEditApp::OnTagLocation10() { TagLocation(10); }
void CCryEditApp::OnTagLocation11() { TagLocation(11); }
void CCryEditApp::OnTagLocation12() { TagLocation(12); }


//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnGotoLocation1() { GotoTagLocation(1); }
void CCryEditApp::OnGotoLocation2() { GotoTagLocation(2); }
void CCryEditApp::OnGotoLocation3() { GotoTagLocation(3); }
void CCryEditApp::OnGotoLocation4() { GotoTagLocation(4); }
void CCryEditApp::OnGotoLocation5() { GotoTagLocation(5); }
void CCryEditApp::OnGotoLocation6() { GotoTagLocation(6); }
void CCryEditApp::OnGotoLocation7() { GotoTagLocation(7); }
void CCryEditApp::OnGotoLocation8() { GotoTagLocation(8); }
void CCryEditApp::OnGotoLocation9() { GotoTagLocation(9); }
void CCryEditApp::OnGotoLocation10() { GotoTagLocation(10); }
void CCryEditApp::OnGotoLocation11() { GotoTagLocation(11); }
void CCryEditApp::OnGotoLocation12() { GotoTagLocation(12); }

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnCustomizeKeyboard()
{
    MainWindow::instance()->OnCustomizeToolbar();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnToolsConfiguretools()
{
    ToolsConfigDialog dlg;
    if (dlg.exec() == QDialog::Accepted)
    {
        MainWindow::instance()->UpdateToolsMenu();
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnToolsScriptHelp()
{
    AzToolsFramework::CScriptHelpDialog::GetInstance()->show();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnViewCycle2dviewport()
{
    GetIEditor()->GetViewManager()->Cycle2DViewport();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnDisplayGotoPosition()
{
    GotoPositionDialog dialog;
    dialog.exec();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnFileSavelevelresources()
{
    CGameResourcesExporter saver;
    saver.GatherAllLoadedResources();
    saver.ChooseDirectoryAndSave();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnClearRegistryData()
{
    if (QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QString(), QObject::tr("Clear all sandbox registry data ?"),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        QSettings settings;
        settings.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnValidatelevel()
{
    // TODO: Add your command handler code here
    CLevelInfo levelInfo;
    levelInfo.Validate();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnToolsPreferences()
{
    EditorPreferencesDialog dlg(MainWindow::instance());
    dlg.exec();
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSwitchToDefaultCamera()
{
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateSwitchToDefaultCamera(QAction* action)
{
    Q_ASSERT(action->isCheckable());
    {
        action->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSwitchToSequenceCamera()
{
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateSwitchToSequenceCamera(QAction* action)
{
    Q_ASSERT(action->isCheckable());
    {
        action->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSwitchToSelectedcamera()
{
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnUpdateSwitchToSelectedCamera(QAction* action)
{
    Q_ASSERT(action->isCheckable());
    {
        action->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnSwitchcameraNext()
{

}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenAssetBrowserView()
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::AssetBrowser);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenTrackView()
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::TrackView);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenAudioControlsEditor()
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::AudioControlsEditor);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::OnOpenUICanvasEditor()
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::UiEditor);
}

//////////////////////////////////////////////////////////////////////////
RecentFileList* CCryEditApp::GetRecentFileList()
{
    static RecentFileList list;
    return &list;
};

//////////////////////////////////////////////////////////////////////////
void CCryEditApp::AddToRecentFileList(const QString& lpszPathName)
{
    // In later MFC implementations (WINVER >= 0x0601) files must exist before they can be added to the recent files list.
    // Here we override the new CWinApp::AddToRecentFileList code with the old implementation to remove this requirement.

    if (IsInAutotestMode())
    {
        // Never add to the recent file list when in auto test mode
        // This would cause issues for devs running tests locally impacting their normal workflows/setups
        return;
    }


    if (GetRecentFileList())
    {
        GetRecentFileList()->Add(lpszPathName);
    }

    // write the list immediately so it will be remembered even after a crash
    if (GetRecentFileList())
    {
        GetRecentFileList()->WriteList();
    }
    else
    {
        CLogFile::WriteLine("ERROR: Recent File List is NULL!");
    }
}

//////////////////////////////////////////////////////////////////////////
bool CCryEditApp::IsInRegularEditorMode()
{
    return !IsInTestMode() && !IsInPreviewMode()
           && !IsInExportMode() && !IsInConsoleMode() && !IsInLevelLoadTestMode() && !GetIEditor()->IsInMatEditMode();
}

void CCryEditApp::OnOpenQuickAccessBar()
{
    if (m_pQuickAccessBar == nullptr)
    {
        return;
    }

    QRect geo = m_pQuickAccessBar->geometry();
    geo.moveCenter(MainWindow::instance()->geometry().center());
    m_pQuickAccessBar->setGeometry(geo);
    m_pQuickAccessBar->setVisible(true);
    m_pQuickAccessBar->setFocus();
}

void CCryEditApp::SetEditorWindowTitle(QString sTitleStr, QString sPreTitleStr, QString sPostTitleStr)
{
    if (MainWindow::instance() || m_pConsoleDialog)
    {
        if (sTitleStr.isEmpty())
        {
            sTitleStr = QObject::tr("O3DE Editor [Stable 21.11]");
        }

        if (!sPreTitleStr.isEmpty())
        {
            sTitleStr.insert(sTitleStr.length(), QStringLiteral(" - %1").arg(sPreTitleStr));
        }

        if (!sPostTitleStr.isEmpty())
        {
            sTitleStr.insert(sTitleStr.length(), QStringLiteral(" - %1").arg(sPostTitleStr));
        }

        MainWindow::instance()->setWindowTitle(sTitleStr);
        if (m_pConsoleDialog)
        {
            m_pConsoleDialog->setWindowTitle(sTitleStr);
        }
    }
}

bool CCryEditApp::Command_ExportToEngine()
{
    return CCryEditApp::instance()->UserExportToGame(true);
}

CMainFrame * CCryEditApp::GetMainFrame() const
{
    return MainWindow::instance()->GetOldMainFrame();
}

void CCryEditApp::StartProcessDetached(const char* process, const char* args)
{
    // Build the arguments as a QStringList
    AZStd::vector<AZStd::string> tokens;

    // separate the string based on spaces for paths like "-launch", "lua", "-files";
    // also separate the string and keep spaces inside the folder path;
    // Ex: C:\dev\Foundation\dev\Cache\AutomatedTesting\pc\automatedtesting\scripts\components\a a\empty.lua;
    // Ex: C:\dev\Foundation\dev\Cache\AutomatedTesting\pc\automatedtesting\scripts\components\a a\'empty'.lua;
    AZStd::string currentStr(args);
    AZStd::size_t firstQuotePos = AZStd::string::npos;
    AZStd::size_t secondQuotePos = 0;
    AZStd::size_t pos = 0;

    while (!currentStr.empty())
    {
        firstQuotePos = currentStr.find_first_of('\"');
        pos = currentStr.find_first_of(" ");

        if ((firstQuotePos != AZStd::string::npos) && (firstQuotePos < pos || pos == AZStd::string::npos))
        {
            secondQuotePos = currentStr.find_first_of('\"', firstQuotePos + 1);
            if (secondQuotePos == AZStd::string::npos)
            {
                AZ_Warning("StartProcessDetached", false, "String tokenize failed, no matching \" found.");
                return;
            }

            AZStd::string newElement(AZStd::string(currentStr.data() + (firstQuotePos + 1), (secondQuotePos - 1)));
            tokens.push_back(newElement);

            currentStr = currentStr.substr(secondQuotePos + 1);

            firstQuotePos = AZStd::string::npos;
            secondQuotePos = 0;
            continue;
        }
        else
        {
            if (pos != AZStd::string::npos)
            {
                AZStd::string newElement(AZStd::string(currentStr.data() + 0, pos));
                tokens.push_back(newElement);
                currentStr = currentStr.substr(pos + 1);
            }
            else
            {
                tokens.push_back(AZStd::string(currentStr));
                break;
            }
        }
    }

    QStringList argsList;
    for (const auto& arg : tokens)
    {
        argsList.push_back(QString(arg.c_str()));
    }

    // Launch the process
    [[maybe_unused]] bool startDetachedReturn = QProcess::startDetached(
        process,
        argsList,
        QCoreApplication::applicationDirPath()
    );
    AZ_Warning("StartProcessDetached", startDetachedReturn, "Failed to start process:%s args:%s", process, args);
}

void CCryEditApp::OpenLUAEditor(const char* files)
{
    AZStd::string args = "-launch lua";
    if (files && strlen(files) > 0)
    {
        AZStd::vector<AZStd::string> resolvedPaths;

        AZStd::vector<AZStd::string> tokens;

        AzFramework::StringFunc::Tokenize(files, tokens, '|');

        for (const auto& file : tokens)
        {
            char resolved[AZ_MAX_PATH_LEN];

            AZStd::string fullPath = Path::GamePathToFullPath(file.c_str()).toUtf8().data();
            azstrncpy(resolved, AZ_MAX_PATH_LEN, fullPath.c_str(), fullPath.size());

            if (AZ::IO::FileIOBase::GetInstance()->Exists(resolved))
            {
                AZStd::string current = '\"' + AZStd::string(resolved) + '\"';
                AZStd::replace(current.begin(), current.end(), '\\', '/');
                resolvedPaths.push_back(current);
            }
        }

        if (!resolvedPaths.empty())
        {
            for (const auto& resolvedPath : resolvedPaths)
            {
                args.append(AZStd::string::format(" -files %s", resolvedPath.c_str()));
            }
        }
    }

    AZ::IO::FixedMaxPathString engineRoot = AZ::Utils::GetEnginePath();
    AZ_Assert(!engineRoot.empty(), "Unable to query Engine Path");

    AZStd::string_view exePath;
    AZ::ComponentApplicationBus::BroadcastResult(exePath, &AZ::ComponentApplicationRequests::GetExecutableFolder);

#if defined(AZ_PLATFORM_LINUX)
    // On Linux platforms, launching a process is not done through a shell and its arguments are passed in
    // separately. There is no need to wrap the process path in case of spaces in the path
    constexpr const char* argumentQuoteString = "";
#else
    constexpr const char* argumentQuoteString = "\"";
#endif    

    AZStd::string process = AZStd::string::format("%s%.*s" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "LuaIDE"
#if defined(AZ_PLATFORM_WINDOWS)
        ".exe"
#endif
        "%s", argumentQuoteString, aznumeric_cast<int>(exePath.size()), exePath.data(), argumentQuoteString);

    AZStd::string processArgs = AZStd::string::format("%s -engine-path \"%s\"", args.c_str(), engineRoot.c_str());
    StartProcessDetached(process.c_str(), processArgs.c_str());
}

void CCryEditApp::PrintAlways(const AZStd::string& output)
{
    m_stdoutRedirection.WriteBypassingRedirect(output.c_str(), static_cast<unsigned int>(output.size()));
}

QString CCryEditApp::GetRootEnginePath() const
{
    return m_rootEnginePath;
}

void CCryEditApp::RedirectStdoutToNull()
{
    m_stdoutRedirection.RedirectTo(AZ::IO::SystemFile::GetNullFilename());
}

void CCryEditApp::OnError(AzFramework::AssetSystem::AssetSystemErrors error)
{
    AZStd::string errorMessage = "";

    switch (error)
    {
    case AzFramework::AssetSystem::ASSETSYSTEM_FAILED_TO_LAUNCH_ASSETPROCESSOR:
        errorMessage = AZStd::string::format("Failed to start the Asset Processor.\r\nPlease make sure that AssetProcessor is available in the same folder the Editor is in.\r\n");
        break;
    case AzFramework::AssetSystem::ASSETSYSTEM_FAILED_TO_CONNECT_TO_ASSETPROCESSOR:
        errorMessage = AZStd::string::format("Failed to connect to the Asset Processor.\r\nPlease make sure that AssetProcessor is available in the same folder the Editor is in and another copy is not already running somewhere else.\r\n");
        break;
    }

    CryMessageBox(errorMessage.c_str(), "Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

void CCryEditApp::OnOpenProceduralMaterialEditor()
{
    QtViewPaneManager::instance()->OpenPane(LyViewPane::SubstanceEditor);
}

namespace Editor
{
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }
}

#if defined(AZ_PLATFORM_WINDOWS)
//Due to some laptops not autoswitching to the discrete gpu correctly we are adding these
//dllspecs as defined in the amd and nvidia white papers to 'force on' the use of the
//discrete chips.  This will be overriden by users setting application profiles
//and may not work on older drivers or bios. In theory this should be enough to always force on
//the discrete chips.

//http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
//https://community.amd.com/thread/169965

// It is unclear if this is also needed for linux or osx at this time(22/02/2017)
extern "C"
{
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

#ifdef Q_OS_WIN
#pragma comment(lib, "Shell32.lib")
#endif

struct CryAllocatorsRAII
{
    CryAllocatorsRAII()
    {
        AZ_Assert(!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady(), "Expected allocator to not be initialized, hunt down the static that is initializing it");

        AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
    }

    ~CryAllocatorsRAII()
    {
        AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
    }
};


extern "C" int AZ_DLL_EXPORT CryEditMain(int argc, char* argv[])
{
    CryAllocatorsRAII cryAllocatorsRAII;

    // Debugging utilities
    for (int i = 1; i < argc; ++i)
    {
        if (azstricmp(argv[i], "--attach-debugger") == 0)
        {
            AZ::Debug::Trace::AttachDebugger();
        }
        else if (azstricmp(argv[i], "--wait-for-debugger") == 0)
        {
            AZ::Debug::Trace::WaitForDebugger();
        }
    }

    // ensure the EditorEventsBus context gets created inside EditorLib
    [[maybe_unused]] const auto& editorEventsContext = AzToolsFramework::EditorEvents::Bus::GetOrCreateContext();

    // connect relevant buses to global settings
    gSettings.Connect();

    auto theApp = AZStd::make_unique<CCryEditApp>();
    // this does some magic to set the current directory...
    {
        QCoreApplication app(argc, argv);
        CCryEditApp::InitDirectory();
    }

    // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
    // on Windows 10
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // QtOpenGL attributes and surface format setup.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapInterval(0);
#ifdef AZ_DEBUG_BUILD
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    QSurfaceFormat::setDefaultFormat(format);

    Editor::EditorQtApplication::InstallQtLogHandler();

    AzQtComponents::Utilities::HandleDpiAwareness(AzQtComponents::Utilities::SystemDpiAware);
    Editor::EditorQtApplication* app = Editor::EditorQtApplication::newInstance(argc, argv);

    if (app->arguments().contains("-autotest_mode"))
    {
        // Nullroute all stdout to null for automated tests, this way we make sure
        // that the test result output is not polluted with unrelated output data.
        theApp->RedirectStdoutToNull();
    }

    // Hook the trace bus to catch errors, boot the AZ app after the QApplication is up
    int ret = 0;

    // open a scope to contain the AZToolsApp instance;
    {
        EditorInternal::EditorToolsApplication AZToolsApp(&argc, &argv);

        {
            CEditCommandLineInfo cmdInfo;
            if (!cmdInfo.m_bAutotestMode && !cmdInfo.m_bConsoleMode && !cmdInfo.m_bExport && !cmdInfo.m_bExportTexture &&
                !cmdInfo.m_bNullRenderer && !cmdInfo.m_bMatEditMode && !cmdInfo.m_bTest)
            {
                if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
                {
                    nativeUI->SetMode(AZ::NativeUI::Mode::ENABLED);
                }
            }
        }

        // The settings registry has been created by the AZ::ComponentApplication constructor at this point
        AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            registry, Editor::GetBuildTargetName());

        if (!AZToolsApp.Start())
        {
            return -1;
        }

        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyQtApplicationAvailable, app);

        int exitCode = 0;

        bool didCryEditStart = CCryEditApp::instance()->InitInstance();
        AZ_Error("Editor", didCryEditStart, "O3DE Editor did not initialize correctly, and will close."
            "\nThis could be because of incorrectly configured components, or missing required gems."
            "\nSee other errors for more details.");

        AzToolsFramework::EditorEventsBus::Broadcast(&AzToolsFramework::EditorEvents::NotifyEditorInitialized);

        if (didCryEditStart)
        {
            app->EnableOnIdle();

            ret = app->exec();
        }
        else
        {
            exitCode = 1;
        }

        CCryEditApp::instance()->ExitInstance(exitCode);

    }

    delete app;

    gSettings.Disconnect();

    return ret;
}

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    AZ::Environment::Detach();
}

#include <moc_CryEdit.cpp>
