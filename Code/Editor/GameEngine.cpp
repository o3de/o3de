/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "GameEngine.h"

// Qt
#include <QMessageBox>
#include <QThread>

// AzCore
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Console/IConsole.h>

// AzFramework
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>      // for TerrainDataRequests
#include <AzFramework/Archive/IArchive.h>

// Editor
#include "IEditorImpl.h"
#include "CryEditDoc.h"
#include "Settings.h"

// CryCommon
#include <CryCommon/INavigationSystem.h>
#include <CryCommon/MainThreadRenderRequestBus.h>

// Editor
#include "CryEdit.h"

#include "ViewManager.h"
#include "AnimationContext.h"
#include "UndoViewPosition.h"
#include "UndoViewRotation.h"
#include "MainWindow.h"
#include "Include/IObjectManager.h"
#include "ActionManager.h"

// Implementation of System Callback structure.
struct SSystemUserCallback
    : public ISystemUserCallback
{
    SSystemUserCallback(IInitializeUIInfo* logo) : m_threadErrorHandler(this) { m_pLogo = logo; };
    void OnSystemConnect(ISystem* pSystem) override
    {
        ModuleInitISystem(pSystem, "Editor");
    }

    bool OnError(const char* szErrorString) override
    {
        // since we show a message box, we have to use the GUI thread
        if (QThread::currentThread() != qApp->thread())
        {
            bool result = false;
            QMetaObject::invokeMethod(&m_threadErrorHandler, "OnError", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result), Q_ARG(const char*, szErrorString));
            return result;
        }

        if (szErrorString)
        {
            Log(szErrorString);
        }

        if (GetIEditor()->IsInTestMode())
        {
            exit(1);
        }

        char str[4096];

        if (szErrorString)
        {
            azsnprintf(str, 4096, "%s\r\nSave Level Before Exiting the Editor?", szErrorString);
        }
        else
        {
            azsnprintf(str, 4096, "Unknown Error\r\nSave Level Before Exiting the Editor?");
        }

        int res = IDNO;

        ICVar* pCVar = gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_no_crash_dialog") : nullptr;

        if (!pCVar || pCVar->GetIVal() == 0)
        {
            res = QMessageBox::critical(QApplication::activeWindow(), QObject::tr("Engine Error"), str, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        }

        if (res == QMessageBox::Yes || res == QMessageBox::No)
        {
            if (res == QMessageBox::Yes)
            {
                if (GetIEditor()->SaveDocument())
                {
                    QMessageBox::information(QApplication::activeWindow(), QObject::tr("Save"), QObject::tr("Level has been successfully saved!\r\nPress Ok to terminate Editor."));
                }
            }
        }

        return true;
    }

    bool OnSaveDocument() override
    {
        bool success = false;

        if (GetIEditor())
        {
            // Turn off save backup as we force a backup before reaching this point
            bool prevSaveBackup = gSettings.bBackupOnSave;
            gSettings.bBackupOnSave = false;

            success = GetIEditor()->SaveDocument();
            gSettings.bBackupOnSave = prevSaveBackup;
        }

        return success;
    }

    bool OnBackupDocument() override
    {
        CCryEditDoc* level = GetIEditor() ? GetIEditor()->GetDocument() : nullptr;
        if (level)
        {
            return level->BackupBeforeSave(true);
        }

        return false;
    }

    void OnProcessSwitch() override
    {
        if (GetIEditor()->IsInGameMode())
        {
            GetIEditor()->SetInGameMode(false);
        }
    }

    void OnInitProgress(const char* sProgressMsg) override
    {
        if (m_pLogo)
        {
            m_pLogo->SetInfoText(sProgressMsg);
        }
    }

    void ShowMessage(const char* text, const char* caption, unsigned int uType) override
    {
        if (CCryEditApp::instance()->IsInAutotestMode())
        {
            return;
        }

        const UINT kMessageBoxButtonMask = 0x000f;
        if (!GetIEditor()->IsInGameMode() && (uType == 0 || uType == MB_OK || !(uType & kMessageBoxButtonMask)))
        {
            static_cast<CEditorImpl*>(GetIEditor())->AddErrorMessage(text, caption);
            return;
        }
        CryMessageBox(text, caption, uType);
    }

    void OnSplashScreenDone()
    {
        m_pLogo = nullptr;
    }

private:
    IInitializeUIInfo* m_pLogo;
    ThreadedOnErrorHandler m_threadErrorHandler;
};

ThreadedOnErrorHandler::ThreadedOnErrorHandler(ISystemUserCallback* callback)
    : m_userCallback(callback)
{
    moveToThread(qApp->thread());
}

ThreadedOnErrorHandler::~ThreadedOnErrorHandler()
{
}

bool ThreadedOnErrorHandler::OnError(const char* error)
{
    return m_userCallback->OnError(error);
}

//! This class will be used by CSystem to find out whether the negotiation with the assetprocessor failed
class AssetProcessConnectionStatus
    : public AzFramework::AssetSystemConnectionNotificationsBus::Handler
{
public:
    AssetProcessConnectionStatus()
    {
        AzFramework::AssetSystemConnectionNotificationsBus::Handler::BusConnect();
    };
    ~AssetProcessConnectionStatus() override
    {
        AzFramework::AssetSystemConnectionNotificationsBus::Handler::BusDisconnect();
    }

    //! Notifies listeners that connection to the Asset Processor failed
    void ConnectionFailed() override
    {
        m_connectionFailed = true;
    }

    void NegotiationFailed() override
    {
        m_negotiationFailed = true;
    }

    bool CheckConnectionFailed()
    {
        return m_connectionFailed;
    }

    bool CheckNegotiationFailed()
    {
        return m_negotiationFailed;
    }
private:
    bool m_connectionFailed = false;
    bool m_negotiationFailed = false;
};

AZ_PUSH_DISABLE_WARNING(4273, "-Wunknown-warning-option")
CGameEngine::CGameEngine()
    : m_bIgnoreUpdates(false)
    , m_ePendingGameMode(ePGM_NotPending)
    , m_modalWindowDismisser(nullptr)
AZ_POP_DISABLE_WARNING
{
    m_pISystem = nullptr;
    m_bLevelLoaded = false;
    m_bInGameMode = false;
    m_bSimulationMode = false;
    m_bSyncPlayerPosition = true;
    m_hSystemHandle.reset(nullptr);
    m_bJustCreated = false;
    m_levelName = "Untitled";
    m_levelExtension = EditorUtils::LevelFile::GetDefaultFileExtension();
    m_playerViewTM.SetIdentity();
    GetIEditor()->RegisterNotifyListener(this);
}

AZ_PUSH_DISABLE_WARNING(4273, "-Wunknown-warning-option")
CGameEngine::~CGameEngine()
{
AZ_POP_DISABLE_WARNING
    GetIEditor()->UnregisterNotifyListener(this);
    m_pISystem->GetIMovieSystem()->SetCallback(nullptr);

    delete m_pISystem;
    m_pISystem = nullptr;

    m_hSystemHandle.reset(nullptr);

    delete m_pSystemUserCallback;
}

static int ed_killmemory_size;
static int ed_indexfiles;

void KillMemory(IConsoleCmdArgs* /* pArgs */)
{
    while (true)
    {
        const int kLimit = 10000000;
        int size;

        if (ed_killmemory_size > 0)
        {
            size = ed_killmemory_size;
        }
        else
        {
            size = rand() * rand();
            size = size > kLimit ? kLimit : size;
        }

        new uint8[size];
    }
}

static void CmdGotoEditor(IConsoleCmdArgs* pArgs)
{
    // feature is mostly useful for QA purposes, this works with the game "goto" command
    // this console command actually is used by the game command, the editor command shouldn't be used by the user
    int iArgCount = pArgs->GetArgCount();

    CViewManager* pViewManager = GetIEditor()->GetViewManager();
    CViewport* pRenderViewport = pViewManager->GetGameViewport();
    if (!pRenderViewport)
    {
        return;
    }

    float x, y, z, wx, wy, wz;

    if (iArgCount == 7
        && azsscanf(pArgs->GetArg(1), "%f", &x) == 1
        && azsscanf(pArgs->GetArg(2), "%f", &y) == 1
        && azsscanf(pArgs->GetArg(3), "%f", &z) == 1
        && azsscanf(pArgs->GetArg(4), "%f", &wx) == 1
        && azsscanf(pArgs->GetArg(5), "%f", &wy) == 1
        && azsscanf(pArgs->GetArg(6), "%f", &wz) == 1)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();

        tm.SetTranslation(Vec3(x, y, z));
        tm.SetRotation33(Matrix33::CreateRotationXYZ(DEG2RAD(Ang3(wx, wy, wz))));
        pRenderViewport->SetViewTM(tm);
    }
}

AZ::Outcome<void, AZStd::string> CGameEngine::Init(
    bool bPreviewMode,
    bool bTestMode,
    const char* sInCmdLine,
    IInitializeUIInfo* logo,
    HWND hwndForInputSystem)
{
    m_pSystemUserCallback = new SSystemUserCallback(logo);

    constexpr const char* crySystemLibraryName = AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX  "CrySystem" AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;

    m_hSystemHandle = AZ::DynamicModuleHandle::Create(crySystemLibraryName);
    if (!m_hSystemHandle->Load(true))
    {
        auto errorMessage = AZStd::string::format("%s Loading Failed", crySystemLibraryName);
        Error(errorMessage.c_str());
        return AZ::Failure(errorMessage);
    }

    PFNCREATESYSTEMINTERFACE pfnCreateSystemInterface =
        m_hSystemHandle->GetFunction<PFNCREATESYSTEMINTERFACE>("CreateSystemInterface");

    SSystemInitParams sip;

    sip.bEditor = true;
    sip.bDedicatedServer = false;
    AZ::Interface<AZ::IConsole>::Get()->PerformCommand("sv_isDedicated false");
    sip.bPreview = bPreviewMode;
    sip.bTestMode = bTestMode;
    sip.hInstance = nullptr;

    sip.pSharedEnvironment = AZ::Environment::GetInstance();

#ifdef AZ_PLATFORM_MAC
    // Create a hidden QWidget. Would show a black window on macOS otherwise.
    auto window = new QWidget();
    QObject::connect(qApp, &QApplication::lastWindowClosed, window, &QWidget::deleteLater);
    sip.hWnd = (HWND)window->winId();
#else
    sip.hWnd = hwndForInputSystem;
#endif

    sip.pLogCallback = &m_logFile;
    sip.sLogFileName = "@log@/Editor.log";
    sip.pUserCallback = m_pSystemUserCallback;

    if (sInCmdLine)
    {
        azstrncpy(sip.szSystemCmdLine, AZ_COMMAND_LINE_LEN, sInCmdLine, AZ_COMMAND_LINE_LEN);
        if (strstr(sInCmdLine, "-export") || strstr(sInCmdLine, "/export") || strstr(sInCmdLine, "-autotest_mode"))
        {
            sip.bUnattendedMode = true;
        }
    }

    if (sip.bUnattendedMode)
    {
        m_modalWindowDismisser = AZStd::make_unique<ModalWindowDismisser>();
    }

    AssetProcessConnectionStatus apConnectionStatus;

    m_pISystem = pfnCreateSystemInterface(sip);

    if (!gEnv)
    {
        gEnv = m_pISystem->GetGlobalEnvironment();
    }

    if (!m_pISystem)
    {
        AZStd::string errorMessage = "Could not initialize CSystem.  View the logs for more details.";

        gEnv = nullptr;
        Error("CreateSystemInterface Failed");
        return AZ::Failure(errorMessage);
    }

    if (apConnectionStatus.CheckNegotiationFailed())
    {
        auto errorMessage = AZStd::string::format("Negotiation with Asset Processor failed.\n"
            "Please ensure the Asset Processor is running on the same branch and try again.");
        gEnv = nullptr;
        return AZ::Failure(errorMessage);
    }

    if (apConnectionStatus.CheckConnectionFailed())
    {
        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        auto errorMessage = AZStd::string::format("Unable to connect to the local Asset Processor.\n\n"
                                                  "The Asset Processor is either not running locally or not accepting connections on port %hu. "
                                                  "Check your remote_port settings in bootstrap.cfg or view the Asset Processor's \"Logs\" tab "
                                                  "for any errors.", connectionSettings.m_assetProcessorPort);
        gEnv = nullptr;
        return AZ::Failure(errorMessage);
    }

    SetEditorCoreEnvironment(gEnv);

    if (gEnv && gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->EnablePhysicsEvents(m_bSimulationMode);
    }

    CLogFile::AboutSystem();
    REGISTER_CVAR(ed_killmemory_size, -1, VF_DUMPTODISK, "Sets the testing allocation size. -1 for random");
    REGISTER_CVAR(ed_indexfiles, 1, VF_DUMPTODISK, "Index game resource files, 0 - inactive, 1 - active");
    REGISTER_COMMAND("ed_killmemory", KillMemory, VF_NULL, "");
    REGISTER_COMMAND("ed_goto", CmdGotoEditor, VF_CHEAT, "Internal command, used by the 'GOTO' console command\n");

    // The editor needs to handle the quit command differently
    gEnv->pConsole->RemoveCommand("quit");
    REGISTER_COMMAND("quit", CGameEngine::HandleQuitRequest, VF_RESTRICTEDMODE, "Quit/Shutdown the engine");

    EBUS_EVENT(CrySystemEventBus, OnCryEditorInitialized);
    
    return AZ::Success();
}

bool CGameEngine::InitGame(const char*)
{
    m_pISystem->ExecuteCommandLine();

    return true;
}

void CGameEngine::SetLevelPath(const QString& path)
{
    QByteArray levelPath;
    levelPath.reserve(AZ_MAX_PATH_LEN);
    levelPath = Path::ToUnixPath(Path::RemoveBackslash(path)).toUtf8();
    m_levelPath = levelPath;

    m_levelName = m_levelPath.mid(m_levelPath.lastIndexOf('/') + 1);

    const char* oldExtension = EditorUtils::LevelFile::GetOldCryFileExtension();
    const char* defaultExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

    // Store off if 
    if (QFileInfo(path + oldExtension).exists())
    {
        m_levelExtension = oldExtension;
    }
    else
    {
        m_levelExtension = defaultExtension;
    }
}

bool CGameEngine::LoadLevel(
    [[maybe_unused]] bool bDeleteAIGraph,
    bool bReleaseResources)
{
     m_bLevelLoaded = false;
    CLogFile::FormatLine("Loading map '%s' into engine...", m_levelPath.toUtf8().data());
    // Switch the current directory back to the Primary CD folder first.
    // The engine might have trouble to find some files when the current
    // directory is wrong
    QDir::setCurrent(GetIEditor()->GetPrimaryCDFolder());


    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    if (!usePrefabSystemForLevels)
    {
        QString pakFile = m_levelPath + "/level.pak";

        // Open Pak file for this level.
        if (!m_pISystem->GetIPak()->OpenPack(m_levelPath.toUtf8().data(), pakFile.toUtf8().data()))
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Level Pack File %s Not Found", pakFile.toUtf8().data());
        }
    }

    // Initialize physics grid.
    if (bReleaseResources)
    {
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
        int physicsEntityGridSize = static_cast<int>(terrainAabb.GetXExtent());

        //CryPhysics under performs if physicsEntityGridSize < nTerrainSize.
        if (physicsEntityGridSize <= 0)
        {
            ICVar* pCvar = m_pISystem->GetIConsole()->GetCVar("e_PhysEntityGridSizeDefault");
            physicsEntityGridSize = pCvar ? pCvar->GetIVal() : 4096;
        }

    }

    // Audio: notify audio of level loading start?
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_REFRESH);

    m_bLevelLoaded = true;

    return true;
}

bool CGameEngine::ReloadLevel()
{
    if (!LoadLevel(false, false))
    {
        return false;
    }

    return true;
}

void CGameEngine::SwitchToInGame()
{
    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
    if (streamer)
    {
        AZStd::binary_semaphore wait;
        AZ::IO::FileRequestPtr flush = streamer->FlushCaches();
        streamer->SetRequestCompleteCallback(flush, [&wait](AZ::IO::FileRequestHandle) { wait.release(); });
        streamer->QueueRequest(flush);
        wait.acquire();
    }

    GetIEditor()->Notify(eNotify_OnBeginGameMode);

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(true);
    m_bInGameMode = true;

    // Disable accelerators.
    GetIEditor()->EnableAcceleratos(false);
    //! Send event to switch into game.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_INGAME);

    m_pISystem->GetIMovieSystem()->Reset(true, false);

    // Transition to runtime entity context.
    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, StartPlayInEditor);

    if (!CCryEditApp::instance()->IsInAutotestMode())
    {
        // Constrain and hide the system cursor (important to do this last)
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    AzFramework::SystemCursorState::ConstrainedAndHidden);
    }

    Log("Entered game mode");
}

void CGameEngine::SwitchToInEditor()
{
    // Transition to editor entity context.
    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, StopPlayInEditor);

    // Reset movie system
    for (int i = m_pISystem->GetIMovieSystem()->GetNumPlayingSequences(); --i >= 0;)
    {
        m_pISystem->GetIMovieSystem()->GetPlayingSequence(i)->Deactivate();
    }
    m_pISystem->GetIMovieSystem()->Reset(false, false);

    CViewport* pGameViewport = GetIEditor()->GetViewManager()->GetGameViewport();

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(m_bSimulationMode);

    // Enable accelerators.
    GetIEditor()->EnableAcceleratos(true);

    // [Anton] - order changed, see comments for CGameEngine::SetSimulationMode
    //! Send event to switch out of game.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_OUTOFGAME);

    m_bInGameMode = false;

    // Out of game in Editor mode.
    if (pGameViewport)
    {
        pGameViewport->SetViewTM(m_playerViewTM);
    }


    GetIEditor()->Notify(eNotify_OnEndGameMode);

    // Unconstrain the system cursor and make it visible (important to do this last)
    AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    AzFramework::SystemCursorState::UnconstrainedAndVisible);

    Log("Exited game mode");
}

void CGameEngine::HandleQuitRequest(IConsoleCmdArgs* /*args*/)
{
    if (GetIEditor()->GetGameEngine()->IsInGameMode())
    {
        GetIEditor()->GetGameEngine()->RequestSetGameMode(false);
        gEnv->pConsole->ShowConsole(false);
    }
    else
    {
        MainWindow::instance()->GetActionManager()->GetAction(ID_APP_EXIT)->trigger();
    }
}

void CGameEngine::RequestSetGameMode(bool inGame)
{
    m_ePendingGameMode = inGame ? ePGM_SwitchToInGame : ePGM_SwitchToInEditor;

    if (m_ePendingGameMode == ePGM_SwitchToInGame)
    {
        AzToolsFramework::EditorLegacyGameModeNotificationBus::Broadcast(
            &AzToolsFramework::EditorLegacyGameModeNotificationBus::Events::OnStartGameModeRequest);
    }
    else if (m_ePendingGameMode == ePGM_SwitchToInEditor)
    {
        AzToolsFramework::EditorLegacyGameModeNotificationBus::Broadcast(
            &AzToolsFramework::EditorLegacyGameModeNotificationBus::Events::OnStopGameModeRequest);
    }
}

void CGameEngine::SetGameMode(bool bInGame)
{
    if (m_bInGameMode == bInGame)
    {
        return;
    }

    if (!GetIEditor()->GetDocument())
    {
        return;
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_MODE_SWITCH_START, bInGame, 0);

    // Enables engine to know about that.
    gEnv->SetIsEditorGameMode(bInGame);

    // Ignore updates while changing in and out of game mode
    m_bIgnoreUpdates = true;

    // Switching modes will destroy the current AzFramework::EntityConext which may contain
    // data the queued events hold on to, so execute all queued events before switching.
    ExecuteQueuedEvents();

    if (bInGame)
    {
        SwitchToInGame();
    }
    else
    {
        SwitchToInEditor();
    }

    GetIEditor()->GetObjectManager()->SendEvent(EVENT_PHYSICS_APPLYSTATE);

    // Enables engine to know about that.
    if (MainWindow::instance())
    {
        AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
        MainWindow::instance()->setFocus();
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED, bInGame, 0);

    m_bIgnoreUpdates = false;

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_MODE_SWITCH_END, bInGame, 0);
}

void CGameEngine::SetSimulationMode(bool enabled, bool bOnlyPhysics)
{
    if (m_bSimulationMode == enabled)
    {
        return;
    }

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(enabled);

    if (enabled)
    {
        GetIEditor()->Notify(eNotify_OnBeginSimulationMode);
    }
    else
    {
        GetIEditor()->Notify(eNotify_OnEndSimulationMode);
    }

    m_bSimulationMode = enabled;

    // Enables engine to know about simulation mode.
    gEnv->SetIsEditorSimulationMode(enabled);

    if (m_bSimulationMode)
    {
        // [Anton] the order of the next 3 calls changed, since, EVENT_INGAME loads physics state (if any),
        // and Reset should be called before it
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_INGAME);
    }
    else
    {
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_OUTOFGAME);
    }

    GetIEditor()->GetObjectManager()->SendEvent(EVENT_PHYSICS_APPLYSTATE);

    // Execute all queued events before switching modes.
    ExecuteQueuedEvents();

    // Transition back to editor entity context.
    // Symmetry is not critical. It's okay to call this even if we never called StartPlayInEditor
    // (bOnlyPhysics was true when we entered simulation mode).
    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::StopPlayInEditor);

    if (m_bSimulationMode && !bOnlyPhysics)
    {
        // Transition to runtime entity context.
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::StartPlayInEditor);
    }

    AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
}

void CGameEngine::SetPlayerViewMatrix(const Matrix34& tm, [[maybe_unused]] bool bEyePos)
{
    m_playerViewTM = tm;
}

void CGameEngine::SyncPlayerPosition(bool bEnable)
{
    m_bSyncPlayerPosition = bEnable;

    if (m_bSyncPlayerPosition)
    {
        SetPlayerViewMatrix(m_playerViewTM);
    }
}

void CGameEngine::SetCurrentMOD(const char* sMod)
{
    m_MOD = sMod;
}

QString CGameEngine::GetCurrentMOD() const
{
    return m_MOD;
}

void CGameEngine::Update()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    switch (m_ePendingGameMode)
    {
    case ePGM_SwitchToInGame:
    {
        SetGameMode(true);
        m_ePendingGameMode = ePGM_NotPending;
        break;
    }

    case ePGM_SwitchToInEditor:
    {
        bool wasInSimulationMode = GetIEditor()->GetGameEngine()->GetSimulationMode();
        if (wasInSimulationMode)
        {
            GetIEditor()->GetGameEngine()->SetSimulationMode(false);
        }
        SetGameMode(false);
        if (wasInSimulationMode)
        {
            GetIEditor()->GetGameEngine()->SetSimulationMode(true);
        }
        m_ePendingGameMode = ePGM_NotPending;
        break;
    }
    }

    AZ::ComponentApplication* componentApplication = nullptr;
    EBUS_EVENT_RESULT(componentApplication, AZ::ComponentApplicationBus, GetApplication);

    if (m_bInGameMode)
    {
        if (gEnv->pSystem)
        {
            gEnv->pSystem->UpdatePreTickBus();
            componentApplication->Tick();
            gEnv->pSystem->UpdatePostTickBus();
        }

        if (CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport())
        {
            pRenderViewport->Update();
        }
    }
    else
    {
        // [marco] check current sound and vis areas for music etc.
        // but if in game mode, 'cos is already done in the above call to game->update()
        unsigned int updateFlags = ESYSUPDATE_EDITOR;
        GetIEditor()->GetAnimation()->Update();
        GetIEditor()->GetSystem()->UpdatePreTickBus(updateFlags);
        componentApplication->Tick();
        GetIEditor()->GetSystem()->UpdatePostTickBus(updateFlags);
    }
}

void CGameEngine::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnSplashScreenDestroyed:
    {
        if (m_pSystemUserCallback != nullptr)
        {
            m_pSystemUserCallback->OnSplashScreenDone();
        }
    }
    break;
    }
}

void CGameEngine::OnTerrainModified(const Vec2& modPosition, float modAreaRadius, bool fullTerrain)
{
    INavigationSystem* pNavigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)

    if (pNavigationSystem)
    {
        // Only report local modifications, not a change in the full terrain (probably happening during initialization)
        if (fullTerrain == false)
        {
            const Vec2 offset(modAreaRadius * 1.5f, modAreaRadius * 1.5f);
            AABB updateBox;
            updateBox.min = modPosition - offset;
            updateBox.max = modPosition + offset;
            AzFramework::Terrain::TerrainDataRequests* terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
            AZ_Assert(terrain != nullptr, "Expecting a valid terrain handler when the terrain is modified");
            const float terrainHeight1 = terrain->GetHeightFromFloats(updateBox.min.x, updateBox.min.y);
            const float terrainHeight2 = terrain->GetHeightFromFloats(updateBox.max.x, updateBox.max.y);
            const float terrainHeight3 = terrain->GetHeightFromFloats(modPosition.x, modPosition.y);

            updateBox.min.z = min(terrainHeight1, min(terrainHeight2, terrainHeight3)) - (modAreaRadius * 2.0f);
            updateBox.max.z = max(terrainHeight1, max(terrainHeight2, terrainHeight3)) + (modAreaRadius * 2.0f);
            pNavigationSystem->WorldChanged(updateBox);
        }
    }
}

void CGameEngine::OnAreaModified(const AABB& modifiedArea)
{
    INavigationSystem* pNavigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
    if (pNavigationSystem)
    {
        pNavigationSystem->WorldChanged(modifiedArea);
    }
}

void CGameEngine::ExecuteQueuedEvents()
{
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();
    AZ::MainThreadRenderRequestBus::ExecuteQueuedEvents();
}

#include <moc_GameEngine.cpp>
