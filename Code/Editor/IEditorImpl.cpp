/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CEditorImpl class implementation.

#include "EditorDefs.h"

#include "IEditorImpl.h"
#include <EditorCommonAPI.h>

// Qt
#include <QByteArray>

// AzCore
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#if defined(AZ_PLATFORM_MAC)
#include <AzCore/Utils/SystemUtilsApple_Platform.h>
#endif

// AzFramework
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

// AzToolsFramework
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

// Editor
#include "CryEdit.h"
#include "PluginManager.h"
#include "ViewManager.h"
#include "DisplaySettings.h"
#include "LevelIndependentFileMan.h"
#include "TrackView/TrackViewSequenceManager.h"
#include "AnimationContext.h"
#include "GameEngine.h"
#include "ToolBox.h"
#include "MainWindow.h"
#include "Settings.h"

#include "EditorFileMonitor.h"
#include "MainStatusBar.h"

#include "Util/FileUtil_impl.h"

#include "Editor/AssetDatabase/AssetDatabaseLocationListener.h"
#include "Editor/AzAssetBrowser/AzAssetBrowserRequestHandler.h"
#include "Editor/AssetEditor/AssetEditorRequestsHandler.h"

#include "Core/QtEditorApplication.h"                               // for Editor::EditorQtApplication

static CCryEditDoc * theDocument;
#include <QMimeData>
#include <QMessageBox>
#include <QProcess>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif
#ifndef VERIFY
#define VERIFY(EXPRESSION) { auto e = EXPRESSION; assert(e); }
#endif

const char* CEditorImpl::m_crashLogFileName = "SessionStatus/editor_statuses.json";

CEditorImpl::CEditorImpl()
    : m_pSystem(nullptr)
    , m_pFileUtil(nullptr)
    , m_pCommandManager(nullptr)
    , m_pPluginManager(nullptr)
    , m_pViewManager(nullptr)
    , m_pUndoManager(nullptr)
    , m_selectedAxis(AXIS_TERRAIN)
    , m_bUpdates(true)
    , m_bTerrainAxisIgnoreObjects(false)
    , m_pDisplaySettings(nullptr)
    , m_bSelectionLocked(true)
    , m_pGameEngine(nullptr)
    , m_pAnimationContext(nullptr)
    , m_pSequenceManager(nullptr)
    , m_pToolBoxManager(nullptr)
    , m_pMusicManager(nullptr)
    , m_pErrorReport(nullptr)
    , m_pLasLoadedLevelErrorReport(nullptr)
    , m_pSelectionTreeManager(nullptr)
    , m_pConsoleSync(nullptr)
    , m_pSettingsManager(nullptr)
    , m_pLevelIndependentFileMan(nullptr)
    , m_bShowStatusText(true)
    , m_bInitialized(false)
    , m_bExiting(false)
    , m_QtApplication(static_cast<Editor::EditorQtApplication*>(qApp))
{
    // note that this is a call into EditorCore.dll, which stores the g_pEditorPointer for all shared modules that share EditorCore.dll
    // this means that they don't need to do SetIEditor(...) themselves and its available immediately
    SetIEditor(this);

    m_pFileUtil = new CFileUtil_impl();
    m_pLevelIndependentFileMan = new CLevelIndependentFileMan;
    SetPrimaryCDFolder();
    gSettings.Load();

    m_pErrorReport = new CErrorReport;
    m_pCommandManager = new CEditorCommandManager;
    m_pEditorFileMonitor.reset(new CEditorFileMonitor());
    m_pDisplaySettings = new CDisplaySettings;
    m_pDisplaySettings->LoadRegistry();
    m_pPluginManager = new CPluginManager;

    m_pViewManager = new CViewManager;
    m_pUndoManager = new CUndoManager;
    m_pToolBoxManager = new CToolBoxManager;
    m_pSequenceManager = new CTrackViewSequenceManager;

    m_pAnimationContext = new CAnimationContext;

    DetectVersion();
    RegisterTools();

    m_pAssetDatabaseLocationListener = nullptr;
    m_pAssetBrowserRequestHandler = nullptr;
    m_assetEditorRequestsHandler = nullptr;

    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        if (AZ::IO::FixedMaxPath crashLogPath; settingsRegistry->Get(crashLogPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
        {
            crashLogPath /= m_crashLogFileName;
            AZ::IO::SystemFile::CreateDir(crashLogPath.ParentPath().FixedMaxPathString().c_str());
            QFile::setPermissions(crashLogPath.c_str(), QFileDevice::ReadOther | QFileDevice::WriteOther);
        }
    }
}

void CEditorImpl::Initialize()
{
#if defined(EXTERNAL_CRASH_REPORTING)
    CrashHandler::ToolsCrashHandler::InitCrashHandler("Editor", {});
#endif

    // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
    // on Windows 10
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Prevents (native) sibling widgets from causing problems with docked QOpenGLWidgets on Windows
    // The problem is due to native widgets ending up with pixel formats that are incompatible with the GL pixel format
    // (generally due to a lack of an alpha channel). This blocks the creation of a shared GL context.
    // And on macOS it prevents all kinds of bugs related to native widgets, specially regarding toolbars (duplicate toolbars, artifacts, crashes).
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    // Activate QT immediately so that its available as soon as CEditorImpl is (and thus GetIEditor())
    InitializeEditorCommon(GetIEditor());
}

//The only purpose of that function is to be called at the very begining of the shutdown sequence so that we can instrument and track
//how many crashes occur while shutting down
void CEditorImpl::OnBeginShutdownSequence()
{
}

void CEditorImpl::OnEarlyExitShutdownSequence()
{
}

void CEditorImpl::Uninitialize()
{
    if (m_pSystem)
    {
        UninitializeEditorCommonISystem(m_pSystem);
    }
    UninitializeEditorCommon();
}

void CEditorImpl::UnloadPlugins()
{
    AZStd::scoped_lock lock(m_pluginMutex);

    // Flush core buses. We're about to unload DLLs and need to ensure we don't have module-owned functions left behind.
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();

    // Send this message to ensure that any widgets queued for deletion will get deleted before their
    // plugin containing their vtable is unloaded. If not, access violations can occur
    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);

    GetPluginManager()->ReleaseAllPlugins();

    GetPluginManager()->UnloadAllPlugins();
}

void CEditorImpl::LoadPlugins()
{
    AZStd::scoped_lock lock(m_pluginMutex);

    constexpr const char* editorPluginFolder = "EditorPlugins";

    AZ::IO::FixedMaxPath pluginsPath;

#if defined(AZ_PLATFORM_MAC)
    char maxPathBuffer[AZ::IO::MaxPathLength];
    if (auto appBundlePathOutcome = AZ::SystemUtilsApple::GetPathToApplicationBundle(maxPathBuffer);
       appBundlePathOutcome)
    {
        AZ::IO::FixedMaxPath bundleRootDirectory = appBundlePathOutcome.GetValue();

        // the bundle directory includes Editor.app so we want the parent directory
        bundleRootDirectory = (bundleRootDirectory / "..").LexicallyNormal();
        pluginsPath = bundleRootDirectory / editorPluginFolder;
    }
#endif

    if (pluginsPath.empty())
    {
        // Use the executable directory as the starting point for the EditorPlugins path
        AZ::IO::FixedMaxPath executableDirectory = AZ::Utils::GetExecutableDirectory();
        pluginsPath = executableDirectory / editorPluginFolder;
    }

    // error handling for invalid paths is handled in LoadPlugins
    GetPluginManager()->LoadPlugins(pluginsPath.c_str());
}

CEditorImpl::~CEditorImpl()
{
    gSettings.Save();
    m_bExiting = true; // Can't save level after this point (while Crash)

    SAFE_DELETE(m_pViewManager)

    SAFE_DELETE(m_pPluginManager)
    SAFE_DELETE(m_pAnimationContext) // relies on undo manager
    SAFE_DELETE(m_pUndoManager)

    if (m_pDisplaySettings)
    {
        m_pDisplaySettings->SaveRegistry();
    }

    SAFE_DELETE(m_pDisplaySettings)
    SAFE_DELETE(m_pToolBoxManager)
    SAFE_DELETE(m_pCommandManager)
    SAFE_DELETE(m_pLasLoadedLevelErrorReport)

    SAFE_DELETE(m_pSettingsManager);

    SAFE_DELETE(m_pAssetDatabaseLocationListener);
    SAFE_DELETE(m_pAssetBrowserRequestHandler);
    SAFE_DELETE(m_assetEditorRequestsHandler);

    // Game engine should be among the last things to be destroyed.
    SAFE_DELETE(m_pLevelIndependentFileMan);
    SAFE_DELETE(m_pGameEngine);
    // The error report must be destroyed after the game, as the engine
    // refers to the error report and the game destroys the engine.
    SAFE_DELETE(m_pErrorReport);

    SAFE_DELETE(m_pFileUtil); // Vladimir@Conffx
}

void CEditorImpl::SetPrimaryCDFolder()
{
    QString szFolder = qApp->applicationDirPath();
    QDir::setCurrent(szFolder);
}

void CEditorImpl::SetGameEngine(CGameEngine* ge)
{
    m_pAssetDatabaseLocationListener = new AssetDatabase::AssetDatabaseLocationListener();
    m_pAssetBrowserRequestHandler = new AzAssetBrowserRequestHandler();
    m_assetEditorRequestsHandler = aznew AssetEditorRequestsHandler();

    m_pSystem = ge->GetSystem();
    m_pGameEngine = ge;

    InitializeEditorCommonISystem(m_pSystem);

    m_templateRegistry.LoadTemplates("Editor");

    m_pAnimationContext->Init();
}

void CEditorImpl::RegisterTools()
{
}

void CEditorImpl::ExecuteCommand(const char* sCommand, ...)
{
    va_list args;
    va_start(args, sCommand);
    ExecuteCommand(QString::asprintf(sCommand, args));
    va_end(args);
}

void CEditorImpl::ExecuteCommand(const QString& command)
{
    m_pCommandManager->Execute(command.toUtf8().data());
}

void CEditorImpl::Update()
{
    if (!m_bUpdates)
    {
        return;
    }

    // Make sure this is not called recursively
    m_bUpdates = false;

    if (IsInPreviewMode())
    {
        SetModifiedFlag(false);
        SetModifiedModule(eModifiedNothing);
    }

    m_bUpdates = true;
}

ISystem* CEditorImpl::GetSystem()
{
    return m_pSystem;
}


CCryEditDoc* CEditorImpl::GetDocument() const
{
    return theDocument;
}

bool CEditorImpl::IsLevelLoaded() const
{
    return GetDocument() && GetDocument()->IsDocumentReady();
}

void CEditorImpl::SetDocument(CCryEditDoc* pDoc)
{
    theDocument = pDoc;
}

void CEditorImpl::SetModifiedFlag(bool modified)
{
    if (GetDocument() && GetDocument()->IsDocumentReady())
    {
        GetDocument()->SetModifiedFlag(modified);

        if (modified)
        {
            GetDocument()->SetLevelExported(false);
        }
    }
}

void CEditorImpl::SetModifiedModule(EModifiedModule eModifiedModule, bool boSet)
{
    if (GetDocument())
    {
        GetDocument()->SetModifiedModules(eModifiedModule, boSet);
    }
}

bool CEditorImpl::IsLevelExported() const
{
    CCryEditDoc* pDoc = GetDocument();

    if (pDoc)
    {
        return pDoc->IsLevelExported();
    }

    return false;
}

bool CEditorImpl::SetLevelExported(bool boExported)
{
    if (GetDocument())
    {
        GetDocument()->SetLevelExported(boExported);
        return true;
    }
    return false;
}

bool CEditorImpl::IsModified()
{
    if (GetDocument())
    {
        return GetDocument()->IsModified();
    }
    return false;
}

bool CEditorImpl::SaveDocument()
{
    if (m_bExiting)
    {
        return false;
    }

    if (GetDocument())
    {
        return GetDocument()->Save();
    }
    else
    {
        return false;
    }
}

QString CEditorImpl::GetPrimaryCDFolder()
{
    return m_primaryCDFolder;
}

QString CEditorImpl::GetLevelFolder()
{
    return GetGameEngine()->GetLevelPath();
}

QString CEditorImpl::GetLevelName()
{
    m_levelNameBuffer = GetGameEngine()->GetLevelName();
    return m_levelNameBuffer;
}

QString CEditorImpl::GetLevelDataFolder()
{
    return Path::AddPathSlash(Path::AddPathSlash(GetGameEngine()->GetLevelPath()) + "LevelData");
}

QString CEditorImpl::GetSearchPath(EEditorPathName path)
{
    return gSettings.searchPaths[path][0];
}

QString CEditorImpl::GetResolvedUserFolder()
{
    m_userFolder = Path::GetResolvedUserSandboxFolder();
    return m_userFolder;
}

void CEditorImpl::SetDataModified()
{
    GetDocument()->SetModifiedFlag(true);
}

void CEditorImpl::SetStatusText(const QString& pszString)
{
    if (m_bShowStatusText && GetMainStatusBar())
    {
        GetMainStatusBar()->SetStatusText(pszString);
    }
}

IMainStatusBar* CEditorImpl::GetMainStatusBar()
{
    return MainWindow::instance()->StatusBar();
}

void CEditorImpl::SetAxisConstraints(AxisConstrains axisFlags)
{
    m_selectedAxis = axisFlags;
    m_pViewManager->SetAxisConstrain(axisFlags);
    SetTerrainAxisIgnoreObjects(false);

    // Update all views.
    UpdateViews(eUpdateObjects, nullptr);
}

AxisConstrains CEditorImpl::GetAxisConstrains()
{
    return m_selectedAxis;
}

void CEditorImpl::SetTerrainAxisIgnoreObjects(bool bIgnore)
{
    m_bTerrainAxisIgnoreObjects = bIgnore;
}

bool CEditorImpl::IsTerrainAxisIgnoreObjects()
{
    return m_bTerrainAxisIgnoreObjects;
}

CSettingsManager* CEditorImpl::GetSettingsManager()
{
    // Do not go any further before XML class is ready to use
    if (!gEnv)
    {
        return nullptr;
    }

    if (!GetISystem())
    {
        return nullptr;
    }

    if (!m_pSettingsManager)
    {
        m_pSettingsManager = new CSettingsManager(eSettingsManagerMemoryStorage);
    }

    return m_pSettingsManager;
}

CViewManager* CEditorImpl::GetViewManager()
{
    return m_pViewManager;
}

CViewport* CEditorImpl::GetActiveView()
{
    MainWindow* mainWindow = MainWindow::instance();
    if (mainWindow)
    {
        CLayoutViewPane* viewPane = mainWindow->GetActiveView();
        if (viewPane)
        {
            return qobject_cast<QtViewport*>(viewPane->GetViewport());
        }
    }
    return nullptr;
}

void CEditorImpl::SetActiveView(CViewport* viewport)
{
    m_pViewManager->SelectViewport(viewport);
}

void CEditorImpl::UpdateViews(int flags, const AABB* updateRegion)
{
    AABB prevRegion = m_pViewManager->GetUpdateRegion();
    if (updateRegion)
    {
        m_pViewManager->SetUpdateRegion(*updateRegion);
    }
    m_pViewManager->UpdateViews(flags);
    if (updateRegion)
    {
        m_pViewManager->SetUpdateRegion(prevRegion);
    }
}

void CEditorImpl::ReloadTrackView()
{
    Notify(eNotify_OnReloadTrackView);
}

void CEditorImpl::ResetViews()
{
    m_pViewManager->ResetViews();

    m_pDisplaySettings->SetRenderFlags(m_pDisplaySettings->GetRenderFlags());
}

IEditorFileMonitor* CEditorImpl::GetFileMonitor()
{
    return m_pEditorFileMonitor.get();
}

float CEditorImpl::GetTerrainElevation(float x, float y)
{
    float terrainElevation = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainElevation
        , &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y,
        AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR, nullptr);
    return terrainElevation;
}

const QColor& CEditorImpl::GetColorByName(const QString& name)
{
    return m_QtApplication->GetColorByName(name);
}

const QtViewPane* CEditorImpl::OpenView(QString sViewClassName, bool reuseOpened)
{
    auto openMode = reuseOpened ? QtViewPane::OpenMode::None : QtViewPane::OpenMode::MultiplePanes;
    return QtViewPaneManager::instance()->OpenPane(sViewClassName, openMode);
}

QWidget* CEditorImpl::FindView(QString viewClassName)
{
    return QtViewPaneManager::instance()->GetView(viewClassName);
}

// Intended to give a window focus only if it is currently open
bool CEditorImpl::SetViewFocus(const char* sViewClassName)
{
    QWidget* findWindow = FindView(sViewClassName);
    if (findWindow)
    {
        findWindow->setFocus(Qt::OtherFocusReason);
        return true;
    }
    return false;
}

bool CEditorImpl::SelectColor(QColor& color, QWidget* parent)
{
    const AZ::Color c = AzQtComponents::fromQColor(color);
    AzQtComponents::ColorPicker dlg(AzQtComponents::ColorPicker::Configuration::RGB, tr("Select Color"), parent);
    dlg.setCurrentColor(c);
    dlg.setSelectedColor(c);
    if (dlg.exec() == QDialog::Accepted)
    {
        color = AzQtComponents::toQColor(dlg.currentColor());
        return true;
    }
    return false;
}

void CEditorImpl::SetInGameMode(bool inGame)
{
    if (IsInSimulationMode())
    {
        return;
    }

    if (m_pGameEngine)
    {
        m_pGameEngine->RequestSetGameMode(inGame);
    }
}

bool CEditorImpl::IsInGameMode()
{
    if (m_pGameEngine)
    {
        return m_pGameEngine->IsInGameMode();
    }
    return false;
}

bool CEditorImpl::IsInSimulationMode()
{
    if (m_pGameEngine)
    {
        return m_pGameEngine->GetSimulationMode();
    }
    return false;
}

bool CEditorImpl::IsInTestMode()
{
    return CCryEditApp::instance()->IsInTestMode();
}

bool CEditorImpl::IsInConsolewMode()
{
    return CCryEditApp::instance()->IsInConsoleMode();
}

bool CEditorImpl::IsInLevelLoadTestMode()
{
    return CCryEditApp::instance()->IsInLevelLoadTestMode();
}

bool CEditorImpl::IsInPreviewMode()
{
    return CCryEditApp::instance()->IsInPreviewMode();
}

static AZStd::string SafeGetStringFromDocument(rapidjson::Document& projectCfg, const char* memberName)
{
    if (projectCfg.HasMember(memberName) && projectCfg[memberName].IsString())
    {
        return projectCfg[memberName].GetString();
    }

    return "";
}

AZStd::string CEditorImpl::LoadProjectIdFromProjectData()
{
    const char* MissingProjectId = "";

    // get the full path of the project.json
    AZStd::string fullPath;
    AZStd::string relPath("project.json");
    bool fullPathFound = false;

    using namespace AzToolsFramework;
    AssetSystemRequestBus::BroadcastResult(fullPathFound, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relPath, fullPath);

    if (!fullPathFound)
    {
        return MissingProjectId;
    }

    QFile file(fullPath.c_str());
    if (!file.open(QIODevice::ReadOnly))
    {
        return MissingProjectId;
    }

    // Read the project.json file using its full path
    QByteArray fileContents = file.readAll();
    file.close();

    rapidjson::Document projectCfg;
    projectCfg.Parse(fileContents);

    if (!projectCfg.IsObject())
    {
        return MissingProjectId;
    }

    AZStd::string projectId = SafeGetStringFromDocument(projectCfg, "project_id");

    // if we don't have a valid projectId by now, it's not happening
    if (projectId.empty() || projectId[0] == '\0')
    {
        return MissingProjectId;
    }

    // get the project Id and project name from the project.json file
    QString projectName(SafeGetStringFromDocument(projectCfg, "project_name").data());

    QFileInfo fileInfo(fullPath.c_str());
    QDir folderDirectory = fileInfo.dir();

    // get the project name from the folder directory
    QString editorProjectName = folderDirectory.dirName();

    // if the project name in the file doesn't match the directory name, it probably means that this is
    // a copied project, and not safe to put any plain text into the projectId string
    if (editorProjectName.compare(projectName, Qt::CaseInsensitive) != 0)
    {
        return projectId;
    }

    // get the project Id generated by using the project name from the folder directory
    QByteArray editorProjectNameUtf8 = editorProjectName.toUtf8();
    AZ::Uuid id = AZ::Uuid::CreateName(editorProjectNameUtf8.constData());

    // The projects that Open 3D Engine ships with had their project IDs hand-generated based on the name of the level.
    // Therefore, if the UUID from the project name is the same as the UUID in the file, it's one of our projects
    // and we can therefore send the name back, making it easier for Metrics to determine which level it was.
    // We are checking to see if this is a project we ship with Open 3D Engine, and therefore we can unobfuscate non-customer information.
    if (id != AZ::Uuid(projectId.data()))
    {
        return projectId;
    }

    QByteArray projectNameUtf8 = projectName.toUtf8();

    projectId += " [";
    projectId += projectNameUtf8.constData();
    projectId += "]";

    return projectId;
}

void CEditorImpl::DetectVersion()
{
#if defined(AZ_PLATFORM_WINDOWS)
    char exe[_MAX_PATH];
    DWORD dwHandle;
    UINT len;

    wchar_t ver[1024 * 8];

    AZ::Utils::GetExecutablePath(exe, _MAX_PATH);
    AZStd::wstring exeW;
    AZStd::to_wstring(exeW, exe);

    int verSize = GetFileVersionInfoSizeW(exeW.c_str(), &dwHandle);
    if (verSize > 0)
    {
        GetFileVersionInfoW(exeW.c_str(), dwHandle, 1024 * 8, ver);
        VS_FIXEDFILEINFO* vinfo;
        VerQueryValueW(ver, L"\\", (void**)&vinfo, &len);

        m_fileVersion.v[0] = vinfo->dwFileVersionLS & 0xFFFF;
        m_fileVersion.v[1] = vinfo->dwFileVersionLS >> 16;
        m_fileVersion.v[2] = vinfo->dwFileVersionMS & 0xFFFF;
        m_fileVersion.v[3] = vinfo->dwFileVersionMS >> 16;

        m_productVersion.v[0] = vinfo->dwProductVersionLS & 0xFFFF;
        m_productVersion.v[1] = vinfo->dwProductVersionLS >> 16;
        m_productVersion.v[2] = vinfo->dwProductVersionMS & 0xFFFF;
        m_productVersion.v[3] = vinfo->dwProductVersionMS >> 16;
    }
#else
    // This requires the application version to be set using QCoreApplication::setApplicationVersion, which isn't done yet.
    const QString version = qApp->applicationVersion();
    if (!version.isEmpty())
    {
        QByteArray versionBytes = version.toUtf8();
        m_fileVersion.Set(versionBytes.data());
        m_productVersion.Set(versionBytes.data());
    }
#endif
}

XmlNodeRef CEditorImpl::FindTemplate(const QString& templateName)
{
    return m_templateRegistry.FindTemplate(templateName);
}

void CEditorImpl::AddTemplate(const QString& templateName, XmlNodeRef& tmpl)
{
    m_templateRegistry.AddTemplate(templateName, tmpl);
}

bool CEditorImpl::ExecuteConsoleApp(const QString& CommandLine, QString& OutputText, [[maybe_unused]] bool bNoTimeOut, bool bShowWindow)
{
    CLogFile::FormatLine("Executing console application '%s'", CommandLine.toUtf8().data());

    QProcess process;
    if (bShowWindow)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        process.start("cmd.exe", { QString("/C %1").arg(CommandLine) });
#elif defined(AZ_PLATFORM_LINUX)
       //KDAB_TODO
#elif defined(AZ_PLATFORM_MAC)
        process.start("/usr/bin/osascript", { QString("-e 'tell application \"Terminal\" to do script \"%1\"'").arg(QString(CommandLine).replace("\"", "\\\"")) });
#else
        process.start("/usr/bin/csh", { QString("-c \"%1\"'").arg(QString(CommandLine).replace("\"", "\\\"")) } );
#endif
    }
    else
    {
        process.start(CommandLine, QStringList());
    }

    if (!process.waitForStarted())
    {
        return false;
    }

    // Wait for the process to finish
    process.waitForFinished();

    OutputText += process.readAllStandardOutput();
    OutputText += process.readAllStandardError();

    return true;
}

void CEditorImpl::BeginUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Begin();
    }
}

void CEditorImpl::RestoreUndo(bool undo)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Restore(undo);
    }
}

void CEditorImpl::AcceptUndo(const QString& name)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Accept(name);
    }
}

void CEditorImpl::CancelUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Cancel();
    }
}

void CEditorImpl::SuperBeginUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->SuperBegin();
    }
}

void CEditorImpl::SuperAcceptUndo(const QString& name)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->SuperAccept(name);
    }
}

void CEditorImpl::SuperCancelUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->SuperCancel();
    }
}

void CEditorImpl::SuspendUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Suspend();
    }
}

void CEditorImpl::ResumeUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Resume();
    }
}

void CEditorImpl::Undo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Undo();
    }
}

void CEditorImpl::Redo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Redo();
    }
}

bool CEditorImpl::IsUndoRecording()
{
    if (m_pUndoManager)
    {
        return m_pUndoManager->IsUndoRecording();
    }
    return false;
}

bool CEditorImpl::IsUndoSuspended()
{
    if (m_pUndoManager)
    {
        return m_pUndoManager->IsUndoSuspended();
    }
    return false;
}

void CEditorImpl::RecordUndo(IUndoObject* obj)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->RecordUndo(obj);
    }
}

bool CEditorImpl::FlushUndo(bool isShowMessage)
{
    if (isShowMessage && m_pUndoManager && m_pUndoManager->IsHaveUndo() && QMessageBox::question(AzToolsFramework::GetActiveWindow(), QObject::tr("Flush Undo"), QObject::tr("After this operation undo will not be available! Are you sure you want to continue?")) != QMessageBox::Yes)
    {
        return false;
    }

    if (m_pUndoManager)
    {
        m_pUndoManager->Flush();
    }
    return true;
}

bool CEditorImpl::ClearLastUndoSteps(int steps)
{
    if (!m_pUndoManager || !m_pUndoManager->IsHaveUndo())
    {
        return false;
    }

    m_pUndoManager->ClearUndoStack(steps);
    return true;
}

bool CEditorImpl::ClearRedoStack()
{
    if (!m_pUndoManager || !m_pUndoManager->IsHaveRedo())
    {
        return false;
    }

    m_pUndoManager->ClearRedoStack();
    return true;
}

void CEditorImpl::SetConsoleVar(const char* var, float value)
{
    ICVar* ivar = GetSystem()->GetIConsole()->GetCVar(var);
    if (ivar)
    {
        ivar->Set(value);
    }
}

float CEditorImpl::GetConsoleVar(const char* var)
{
    ICVar* ivar = GetSystem()->GetIConsole()->GetCVar(var);
    if (ivar)
    {
        return ivar->GetFVal();
    }
    return 0;
}

CAnimationContext* CEditorImpl::GetAnimation()
{
    return m_pAnimationContext;
}

CTrackViewSequenceManager* CEditorImpl::GetSequenceManager()
{
    return m_pSequenceManager;
}

ITrackViewSequenceManager* CEditorImpl::GetSequenceManagerInterface()
{
    return GetSequenceManager();
}

void CEditorImpl::StartLevelErrorReportRecording()
{
    IErrorReport* errorReport = GetErrorReport();
    if (errorReport)
    {
        errorReport->Clear();
        errorReport->SetImmediateMode(false);
        errorReport->SetShowErrors(true);
    }
}

// Confetti Start: Leroy Sikkes
void CEditorImpl::Notify(EEditorNotifyEvent event)
{
    NotifyExcept(event, nullptr);
}

void CEditorImpl::NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener)
{
    if (m_bExiting)
    {
        return;
    }

    std::list<IEditorNotifyListener*>::iterator it = m_listeners.begin();
    while (it != m_listeners.end())
    {
        if (*it == listener)
        {
            it++;
            continue; // skip "except" listener
        }

        (*it++)->OnEditorNotifyEvent(event);
    }

    if (event == eNotify_OnInit)
    {
        REGISTER_COMMAND("py", CmdPy, 0, "Execute a Python code snippet.");
    }

    GetPluginManager()->NotifyPlugins(event);
}
// Confetti end: Leroy Sikkes

void CEditorImpl::RegisterNotifyListener(IEditorNotifyListener* listener)
{
    listener->m_bIsRegistered = true;
    stl::push_back_unique(m_listeners, listener);
}

void CEditorImpl::UnregisterNotifyListener(IEditorNotifyListener* listener)
{
    m_listeners.remove(listener);
    listener->m_bIsRegistered = false;
}


void CEditorImpl::ShowStatusText(bool bEnable)
{
    m_bShowStatusText = bEnable;
}

void CEditorImpl::ReduceMemory()
{
    GetIEditor()->GetUndoManager()->ClearRedoStack();
    GetIEditor()->GetUndoManager()->ClearUndoStack();

#if defined(AZ_PLATFORM_WINDOWS)
    HANDLE hHeap = GetProcessHeap();

    if (hHeap)
    {
        uint64 maxsize = (uint64)HeapCompact(hHeap, 0);
        CryLogAlways("Max Free Memory Block = %I64d Kb", maxsize / 1024);
    }
#endif
}

ESystemConfigPlatform CEditorImpl::GetEditorConfigPlatform() const
{
    return m_pSystem->GetConfigPlatform();
}

void CEditorImpl::InitFinished()
{
    if (!m_bInitialized)
    {
        m_bInitialized = true;
        Notify(eNotify_OnInit);

        // Let system wide listeners know about this as well.
        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_ON_INIT, 0, 0);
    }
}

void CEditorImpl::ReloadTemplates()
{
    m_templateRegistry.LoadTemplates("Editor");
}

void CEditorImpl::CmdPy(IConsoleCmdArgs* pArgs)
{
    if (AzToolsFramework::EditorPythonRunnerRequestBus::HasHandlers())
    {
        // Execute the given script command.
        QString scriptCmd = pArgs->GetCommandLine();

        scriptCmd = scriptCmd.right(scriptCmd.length() - 2); // The part of the text after the 'py'
        scriptCmd = scriptCmd.trimmed();
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, scriptCmd.toUtf8().data(), false);
    }
    else
    {
        AZ_Warning("python", false, "EditorPythonRunnerRequestBus has no handlers");
    }
}

// Vladimir@Conffx
SSystemGlobalEnvironment* CEditorImpl::GetEnv()
{
    assert(gEnv);
    return gEnv;
}

// Leroy@Conffx
SEditorSettings* CEditorImpl::GetEditorSettings()
{
    return &gSettings;
}

