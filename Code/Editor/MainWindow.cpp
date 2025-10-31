/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "MainWindow.h"

#include <algorithm>

// Qt
#ifdef Q_OS_WIN
#include <QAbstractEventDispatcher>
#endif
#include <QDebug>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>

// AzCore
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/Utils.h>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Viewport/CameraInput.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/Application/Ticker.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/EditorAnimationSystemRequestBus.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsWindow.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>
#include <AzToolsFramework/SourceControl/QtSourceControlNotificationHandler.h>
#include <AzToolsFramework/Viewport/LocalViewBookmarkLoader.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

// AzQtComponents
#include <AzQtComponents/Buses/ShortcutDispatch.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/InputDialog.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/DragAndDrop/MainWindowDragAndDrop.h>

// Editor
#include "Resource.h"
#include "LayoutWnd.h"
#include "AssetImporter/AssetImporterManager/AssetImporterManager.h"
#include "AssetImporter/AssetImporterManager/AssetImporterDragAndDropHandler.h"
#include "CryEdit.h"
#include "Controls/ConsoleSCB.h"
#include "ViewManager.h"
#include "CryEditDoc.h"
#include "ToolBox.h"
#include "LevelIndependentFileMan.h"
#include "GameEngine.h"
#include "MainStatusBar.h"
#include "Core/QtEditorApplication.h"
#include "UndoDropDown.h"
#include "EditorViewportSettings.h"

#include "QtViewPaneManager.h"
#include "ViewPane.h"
#include "Include/Command.h"
#include "Commands/CommandManager.h"
#include "SettingsManagerDialog.h"

#include "TrackView/TrackViewDialog.h"
#include "ErrorReportDialog.h"

#include "Dialogs/PythonScriptsDialog.h"

#include "AzAssetBrowser/AzAssetBrowserWindow.h"
#include "AssetEditor/AssetEditorWindow.h"

#include <ImGuiBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <LmbrCentral/Audio/AudioSystemComponentBus.h>
#include <Editor/EditorViewportCamera.h>

using namespace AZ;
using namespace AzQtComponents;
using namespace AzToolsFramework;

#define LAYOUTS_PATH "Editor\\Layouts\\"
#define LAYOUTS_EXTENSION ".layout"
#define LAYOUTS_WILDCARD "*.layout"
#define DUMMY_LAYOUT_NAME "Dummy_Layout"

class CEditorOpenViewCommand
    : public _i_reference_target_t
{
    QString m_className;
public:
    CEditorOpenViewCommand(IEditor* pEditor, const QString& className)
        : m_pEditor(pEditor)
        , m_className(className)
    {
        assert(m_pEditor);
    }
    void Execute()
    {
        // Create browse mode for this category.
        m_pEditor->OpenView(m_className);
    }

private:
    IEditor* m_pEditor;
};

namespace
{
    // The purpose of this vector is just holding shared pointers, so CEditorOpenViewCommand dtors are called at exit
    std::vector<_smart_ptr<CEditorOpenViewCommand> > s_openViewCmds;
}

class EngineConnectionListener
    : public AzFramework::EngineConnectionEvents::Bus::Handler
    , public AzFramework::AssetSystemInfoBus::Handler
{
public:
    using EConnectionState = AzFramework::SocketConnection::EConnectionState;

public:
    EngineConnectionListener()
        : m_state(EConnectionState::Disconnected)
    {
        AzFramework::EngineConnectionEvents::Bus::Handler::BusConnect();
        AzFramework::AssetSystemInfoBus::Handler::BusConnect();

        AzFramework::SocketConnection* engineConnection = AzFramework::SocketConnection::GetInstance();
        if (engineConnection)
        {
            m_state = engineConnection->GetConnectionState();
        }
    }

    ~EngineConnectionListener() override
    {
        AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();
        AzFramework::EngineConnectionEvents::Bus::Handler::BusDisconnect();
    }

public:
    void Connected([[maybe_unused]] AzFramework::SocketConnection* connection) override
    {
        m_state = EConnectionState::Connected;
    }
    void Connecting([[maybe_unused]] AzFramework::SocketConnection* connection) override
    {
        m_state = EConnectionState::Connecting;
    }
    void Listening([[maybe_unused]] AzFramework::SocketConnection* connection) override
    {
        m_state = EConnectionState::Listening;
    }
    void Disconnecting([[maybe_unused]] AzFramework::SocketConnection* connection) override
    {
        m_state = EConnectionState::Disconnecting;
    }
    void Disconnected([[maybe_unused]] AzFramework::SocketConnection* connection) override
    {
        m_state = EConnectionState::Disconnected;
    }

    void AssetCompilationSuccess(const AZStd::string& assetPath) override
    {
        m_lastAssetProcessorTask = assetPath;
    }

    void AssetCompilationFailed(const AZStd::string& assetPath) override
    {
        m_failedJobs.insert(assetPath);
    }

    void CountOfAssetsInQueue(const int& count) override
    {
        m_pendingJobsCount = count;
    }

    int GetJobsCount() const
    {
        return m_pendingJobsCount;
    }

    AZStd::set<AZStd::string> FailedJobsList() const
    {
        return m_failedJobs;
    }

    AZStd::string LastAssetProcessorTask() const
    {
        return m_lastAssetProcessorTask;
    }

public:
    EConnectionState GetState() const
    {
        return m_state;
    }

private:
    EConnectionState m_state;
    int m_pendingJobsCount = 0;
    AZStd::set<AZStd::string> m_failedJobs;
    AZStd::string m_lastAssetProcessorTask;
};

namespace
{
    void PyOpenViewPane(const char* viewClassName)
    {
        QtViewPaneManager::instance()->OpenPane(viewClassName);
    }

    void PyCloseViewPane(const char* viewClassName)
    {
        QtViewPaneManager::instance()->ClosePane(viewClassName);
    }

    bool PyIsViewPaneVisible(const char* viewClassName)
    {
        return QtViewPaneManager::instance()->IsVisible(viewClassName);
    }

    AZStd::vector<AZStd::string> PyGetViewPaneNames()
    {
        const QtViewPanes panes = QtViewPaneManager::instance()->GetRegisteredPanes();

        AZStd::vector<AZStd::string> names;
        names.reserve(panes.size());

        AZStd::transform(panes.begin(), panes.end(), AZStd::back_inserter(names), [](const QtViewPane& pane)
        {
            return AZStd::string(pane.m_name.toUtf8().constData());
        });

        return names;
    }

    void PyExit()
    {
        // Adding a single-shot QTimer to PyExit delays the QApplication::closeAllWindows call until
        // all the events in the event queue have been processed. Calling QApplication::closeAllWindows instead
        // of MainWindow::close ensures the Metal render window is cleaned up on macOS.
        QTimer::singleShot(0, qApp, &QApplication::closeAllWindows);
    }

    void PyExitNoPrompt()
    {
        // Set the level to "unmodified" so that it doesn't prompt to save on exit.
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        PyExit();
    }

    void PyTestOutput(const AZStd::string& output)
    {
        CCryEditApp::instance()->PrintAlways(output);
    }
}

/////////////////////////////////////////////////////////////////////////////
// MainWindow
/////////////////////////////////////////////////////////////////////////////
MainWindow* MainWindow::m_instance = nullptr;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_oldMainFrame(nullptr)
    , m_viewPaneManager(QtViewPaneManager::instance())
    , m_undoStateAdapter(new UndoStackStateAdapter(this))
    , m_activeView(nullptr)
    , m_settings("O3DE", "O3DE")
    , m_assetImporterManager(new AssetImporterManager(this))
    , m_sourceControlNotifHandler(new AzToolsFramework::QtSourceControlNotificationHandler(this))
    , m_viewPaneHost(nullptr)
    , m_autoSaveTimer(nullptr)
    , m_autoRemindTimer(nullptr)
    , m_backgroundUpdateTimer(nullptr)
    , m_connectionLostTimer(new QTimer(this))
{
    setObjectName("MainWindow"); // For IEditor::GetEditorMainWindow to work in plugins, where we can't link against MainWindow::instance()
    m_instance = this;

    //for new docking, create a DockMainWindow to host dock widgets so we can call QMainWindow::restoreState to restore docks without affecting our main toolbars.
    m_viewPaneHost = new AzQtComponents::DockMainWindow();

    m_viewPaneHost->setDockOptions(QMainWindow::GroupedDragging | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    m_connectionListener = AZStd::make_shared<EngineConnectionListener>();
    QObject::connect(m_connectionLostTimer, &QTimer::timeout, this, &MainWindow::ShowConnectionDisconnectedDialog);

    setStatusBar(new MainStatusBar(this));

    setAttribute(Qt::WA_DeleteOnClose, true);

    GetIEditor()->RegisterNotifyListener(this);

    AssetImporterDragAndDropHandler* assetImporterDragAndDropHandler = new AssetImporterDragAndDropHandler(this, m_assetImporterManager);
    connect(assetImporterDragAndDropHandler, &AssetImporterDragAndDropHandler::OpenAssetImporterManager, this, &MainWindow::OnOpenAssetImporterManager);
    connect(assetImporterDragAndDropHandler, &AssetImporterDragAndDropHandler::OpenAssetImporterManagerWithSuggestedPath, this, &MainWindow::OnOpenAssetImporterManagerAtPath);

    setFocusPolicy(Qt::StrongFocus);

    setAcceptDrops(true);

    // special handling for escape key (outside ActionManager)
    auto* escapeAction = new QAction(this);
    escapeAction->setShortcut(QKeySequence(Qt::Key_Escape));
    addAction(escapeAction);
    connect(escapeAction, &QAction::triggered, this, &MainWindow::OnEscapeAction);

    const QSize minSize(800, 600);
    if (size().height() < minSize.height() || size().width() < minSize.width())
    {
        resize(size().expandedTo(minSize));
    }
}

void MainWindow::SystemTick()
{
    AZ::ComponentApplication* componentApplication = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(componentApplication, &AZ::ComponentApplicationBus::Events::GetApplication);
    if (componentApplication)
    {
        componentApplication->TickSystem();
    }
}

#ifdef Q_OS_WIN
HWND MainWindow::GetNativeHandle()
{
    // if the parent widget is set, it's a window decoration wrapper
    // we use that instead, to ensure we're in lock step the code in CryEdit.cpp when it calls
    // InitGameSystem
    if (parentWidget() != nullptr)
    {
        assert(qobject_cast<AzQtComponents::WindowDecorationWrapper*>(parentWidget()));
        return QtUtil::getNativeHandle(parentWidget());
    }

    return QtUtil::getNativeHandle(this);
}
#endif // #ifdef Q_OS_WIN

void MainWindow::OnOpenAssetImporterManager(const QStringList& dragAndDropFileList)
{
    m_assetImporterManager->Exec(dragAndDropFileList);
}

void MainWindow::OnOpenAssetImporterManagerAtPath(const QStringList& dragAndDropFileList, const QString& path)
{
    m_assetImporterManager->Exec(dragAndDropFileList, path);
}

CLayoutWnd* MainWindow::GetLayout() const
{
    return m_pLayoutWnd;
}

CLayoutViewPane* MainWindow::GetActiveView() const
{
    return m_activeView;
}

QtViewport* MainWindow::GetActiveViewport() const
{
    return m_activeView ? qobject_cast<QtViewport*>(m_activeView->GetViewport()) : nullptr;
}

void MainWindow::SetActiveView(CLayoutViewPane* v)
{
    m_activeView = v;
}

MainWindow::~MainWindow()
{
    AzToolsFramework::SourceControlNotificationBus::Handler::BusDisconnect();

    m_connectionListener.reset();
    GetIEditor()->UnregisterNotifyListener(this);

    m_instance = nullptr;
}

void MainWindow::InitCentralWidget()
{
    m_pLayoutWnd = new CLayoutWnd(&m_settings);

    // Set the central widgets before calling CreateLayout to avoid reparenting everything later
    setCentralWidget(m_viewPaneHost);
    m_viewPaneHost->setCentralWidget(m_pLayoutWnd);

    if (MainWindow::instance()->IsPreview())
    {
        m_pLayoutWnd->CreateLayout(ET_Layout0, true, ET_ViewportModel);
    }
    else
    {
        if (!m_pLayoutWnd->LoadConfig())
        {
            m_pLayoutWnd->CreateLayout(ET_Layout0);
        }
    }

    // make sure the layout wnd knows to reset it's layout and settings
    connect(m_viewPaneManager, &QtViewPaneManager::layoutReset, m_pLayoutWnd, &CLayoutWnd::ResetLayout);

    AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::Bus::Events::NotifyCentralWidgetInitialized);
}

void MainWindow::Initialize()
{
    m_viewPaneManager->SetMainWindow(m_viewPaneHost, &m_settings, /*unused*/ QByteArray());

    RegisterStdViewClasses();
    InitCentralWidget();

    // load toolbars ("shelves") and macros
    GetIEditor()->GetToolBoxManager()->Load();

    m_editorActionsHandler.Initialize(this);

    InitStatusBar();

    AzToolsFramework::SourceControlNotificationBus::Handler::BusConnect();
    m_sourceControlNotifHandler->Init();

    if (!IsPreview())
    {
        RegisterOpenWndCommands();
    }

    ResetBackgroundUpdateTimer();

    ICVar* pBackgroundUpdatePeriod = gEnv->pConsole->GetCVar("ed_backgroundUpdatePeriod");
    if (pBackgroundUpdatePeriod)
    {
        pBackgroundUpdatePeriod->SetOnChangeCallback([](ICVar*) {
            MainWindow::instance()->ResetBackgroundUpdateTimer();
        });
    }

    AzToolsFramework::EditorEventsBus::Broadcast(&AzToolsFramework::EditorEvents::NotifyMainWindowInitialized, this);
}

void MainWindow::InitStatusBar()
{
    StatusBar()->Init();
    connect(qobject_cast<StatusBarItem*>(StatusBar()->GetItem("connection")), &StatusBarItem::clicked, this, &MainWindow::OnConnectionStatusClicked);
    connect(StatusBar(), &MainStatusBar::requestStatusUpdate, this, &MainWindow::OnUpdateConnectionStatus);
}


CMainFrame* MainWindow::GetOldMainFrame() const
{
    return m_oldMainFrame;
}

MainWindow* MainWindow::instance()
{
    return m_instance;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    gSettings.Save(true);

    AzFramework::SystemCursorState currentCursorState;
    bool isInGameMode = false;
    if (GetIEditor()->IsInGameMode())
    {
        isInGameMode = true;
        // Storecurrent state in case we need to restore Game Mode.
        AzFramework::InputSystemCursorRequestBus::EventResult(currentCursorState, AzFramework::InputDeviceMouse::Id,
            &AzFramework::InputSystemCursorRequests::GetSystemCursorState);
        // make sure the mouse is turned on before popping up any dialog boxes.
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
            AzFramework::SystemCursorState::UnconstrainedAndVisible);
    }
    if (GetIEditor()->GetDocument() && !GetIEditor()->GetDocument()->CanCloseFrame())
    {
        if (isInGameMode)
        {
            // make sure the mouse is turned back off if returning to the game.
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                currentCursorState);
        }
        event->ignore();
        return;
    }

    SaveConfig();

    // Some of the panes may ask for confirmation to save changes before closing.
    if (!QtViewPaneManager::instance()->ClosePanesWithRollback(QVector<QString>()) ||
        !GetIEditor() ||
        !GetIEditor()->GetLevelIndependentFileMan()->PromptChangedFiles())
    {
        if (isInGameMode)
        {
            // make sure the mouse is turned back off if returning to the game.
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                currentCursorState);

        }
        event->ignore();
        return;
    }

    Editor::EditorQtApplication::instance()->EnableOnIdle(false);

    if (GetIEditor()->GetDocument())
    {
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        GetIEditor()->GetDocument()->SetModifiedModules(eModifiedNothing);
    }

    // force clean up of all deferred deletes, so that we don't have any issues with windows from plugins not being deleted yet
    qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QMainWindow::closeEvent(event);
}

void MainWindow::SaveConfig()
{
    m_settings.setValue("mainWindowState", saveState());
    QtViewPaneManager::instance()->SaveLayout();
    if (m_pLayoutWnd)
    {
        m_pLayoutWnd->SaveConfig();
    }
}

void MainWindow::OnEscapeAction()
{
    if (!CCryEditApp::instance()->IsInAutotestMode())
    {
        if (GetIEditor()->IsInGameMode())
        {
            GetIEditor()->SetInGameMode(false);
        }
        else
        {
            AzToolsFramework::EditorEvents::Bus::Broadcast(
                &AzToolsFramework::EditorEvents::OnEscape);
        }
    }
}

QWidget* MainWindow::CreateSpacerRightWidget()
{
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setVisible(true);
    return spacer;
}

UndoRedoToolButton::UndoRedoToolButton(QWidget* parent)
    : QToolButton(parent)
{
}

void UndoRedoToolButton::Update(int count)
{
    setEnabled(count > 0);
}

bool MainWindow::IsPreview() const
{
    return GetIEditor()->IsInPreviewMode();
}

MainStatusBar* MainWindow::StatusBar() const
{
    assert(statusBar()->inherits("MainStatusBar"));
    return static_cast<MainStatusBar*>(statusBar());
}

void MainWindow::OpenViewPane(int paneId)
{
    OpenViewPane(QtViewPaneManager::instance()->GetPane(paneId));
}

void MainWindow::OpenViewPane(QtViewPane* pane)
{
    if (pane && pane->IsValid())
    {
        QtViewPaneManager::instance()->OpenPane(pane->m_name);
    }
    else
    {
        if (pane)
        {
            qWarning() << Q_FUNC_INFO << "Invalid pane" << pane->m_id << pane->m_category << pane->m_name;
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Invalid pane";
        }
    }
}

void MainWindow::AdjustToolBarIconSize(AzQtComponents::ToolBar::ToolBarIconSize size)
{
    const QList<QToolBar*> toolbars = findChildren<QToolBar*>();

    // make sure to set this back, so that the general settings page matches up with what the size is too
    if (gSettings.gui.nToolbarIconSize != static_cast<int>(size))
    {
        gSettings.gui.nToolbarIconSize = static_cast<int>(size);
    }

    for (auto toolbar : toolbars)
    {
        AzQtComponents::ToolBar::setToolBarIconSize(toolbar, size);
    }
}

void MainWindow::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    switch (ev)
    {
    case eNotify_OnEndSceneOpen:
    case eNotify_OnEndSceneSave:
    {
        auto cryEdit = CCryEditApp::instance();
        if (cryEdit)
        {
            cryEdit->SetEditorWindowTitle(nullptr, AZ::Utils::GetProjectDisplayName().c_str(), GetIEditor()->GetGameEngine()->GetLevelName());
        }
    }
    break;
    case eNotify_OnCloseScene:
    {
        auto cryEdit = CCryEditApp::instance();
        if (cryEdit)
        {
            cryEdit->SetEditorWindowTitle(nullptr, AZ::Utils::GetProjectDisplayName().c_str(), nullptr);
        }
    }
    break;
    case eNotify_OnRefCoordSysChange:
        emit UpdateRefCoordSys();
        break;
    case eNotify_OnInvalidateControls:
        InvalidateControls();
        break;
    }

    switch (ev)
    {
    case eNotify_OnBeginSceneOpen:
    case eNotify_OnBeginNewScene:
    case eNotify_OnCloseScene:
        StopAutoSaveTimers();
        break;
    case eNotify_OnEndSceneOpen:
    case eNotify_OnEndNewScene:
        StartAutoSaveTimers();
        break;
    }
}

void MainWindow::InvalidateControls()
{
    emit UpdateRefCoordSys();
}

void MainWindow::RegisterStdViewClasses()
{
    AzAssetBrowserWindow::createListenerForShowAssetEditorEvent(this);

    CTrackViewDialog::RegisterViewClass();
    CErrorReportDialog::RegisterViewClass();
    CPythonScriptsDialog::RegisterViewClass();

    AzToolsFramework::CScriptTermDialog::RegisterViewClass();
    CConsoleSCB::RegisterViewClass();
    ConsoleVariableEditor::RegisterViewClass();
    CSettingsManagerDialog::RegisterViewClass();
    AzAssetBrowserWindow::RegisterViewClass();
    AssetEditorWindow::RegisterViewClass();
    AzToolsFramework::RegisterPaintBrushSettingsWindow();

    // Notify that views can now be registered
    AzToolsFramework::EditorEvents::Bus::Broadcast(
        &AzToolsFramework::EditorEvents::Bus::Events::NotifyRegisterViews);
}

void MainWindow::OnCustomizeToolbar()
{
    /* TODO_KDAB, rest of CMainFrm::OnCustomize() goes here*/
    SaveConfig();
}

void MainWindow::RefreshStyle()
{
    GetIEditor()->Notify(eNotify_OnStyleChanged);
}

void MainWindow::StopAutoSaveTimers()
{
    if (m_autoSaveTimer)
    {
        delete m_autoSaveTimer;
    }
    if (m_autoRemindTimer)
    {
        delete m_autoRemindTimer;
    }
    m_autoSaveTimer = nullptr;
    m_autoRemindTimer = nullptr;
}

void MainWindow::StartAutoSaveTimers()
{
    if (gSettings.autoBackupTime > 0 && gSettings.autoBackupEnabled)
    {
        m_autoSaveTimer = new QTimer(this);
        m_autoSaveTimer->start(gSettings.autoBackupTime * 1000 * 60);
        connect(
            m_autoSaveTimer,
            &QTimer::timeout,
            this,
            [&]()
            {
                if (gSettings.autoBackupEnabled)
                {
                    // Call autosave function of CryEditApp
                    GetIEditor()->GetDocument()->SaveAutoBackup();
                }
            });
    }
    if (gSettings.autoRemindTime > 0)
    {
        m_autoRemindTimer = new QTimer(this);
        m_autoRemindTimer->start(gSettings.autoRemindTime * 1000 * 60);
        connect(
            m_autoRemindTimer,
            &QTimer::timeout,
            this,
            [&]()
            {
                if (gSettings.autoRemindTime > 0)
                {
                    // Remind to save.
                    CCryEditApp::instance()->SaveAutoRemind();
                }
            });
    }
}

void MainWindow::ResetAutoSaveTimers()
{
    StopAutoSaveTimers();
    StartAutoSaveTimers();
}

void MainWindow::ResetBackgroundUpdateTimer()
{
    if (m_backgroundUpdateTimer)
    {
        delete m_backgroundUpdateTimer;
        m_backgroundUpdateTimer = nullptr;
    }

    if (gSettings.backgroundUpdatePeriod > 0)
    {
        m_backgroundUpdateTimer = new QTimer(this);
        m_backgroundUpdateTimer->start(gSettings.backgroundUpdatePeriod);
        connect(m_backgroundUpdateTimer, &QTimer::timeout, this, [&]() {
            // Make sure that visible editor window get low-fps updates while in the background

            CCryEditApp* pApp = CCryEditApp::instance();
            if (!isMinimized() && !pApp->IsWindowInForeground())
            {
                pApp->IdleProcessing(true);
            }
        });
    }
}

void MainWindow::OnStopAllSounds()
{
    LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalStopAllSounds);
}

void MainWindow::OnRefreshAudioSystem()
{
    AZStd::string levelName;
    AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequestBus::Events::GetLevelName);
    AZStd::to_lower(levelName.begin(), levelName.end());

    if (levelName == "untitled")
    {
        levelName.clear();
    }

    LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
        &LmbrCentral::AudioSystemComponentRequestBus::Events::GlobalRefreshAudio, AZStd::string_view{ levelName });
}

void MainWindow::SaveLayout()
{
    const int MAX_LAYOUTS = ID_VIEW_LAYOUT_LAST - ID_VIEW_LAYOUT_FIRST + 1;

    if (m_viewPaneManager->LayoutNames(true).count() >= MAX_LAYOUTS)
    {
        QMessageBox::critical(this, tr("Maximum number of layouts reached"), tr("Please delete a saved layout before creating another."));
        return;
    }

    QString layoutName = InputDialog::getText(this, tr("Layout Name"), QString(), QLineEdit::Normal, QString(), "[a-z]+[a-z0-9\\-\\_]*");
    if (layoutName.isEmpty())
    {
        return;
    }

    if (m_viewPaneManager->HasLayout(layoutName))
    {
        QMessageBox box(this); // Not static so we can remove help button
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        box.setText(tr("Overwrite Layout?"));
        box.setIcon(QMessageBox::Warning);
        box.setWindowFlags(box.windowFlags() & ~Qt::WindowContextHelpButtonHint);
        box.setInformativeText(tr("The chosen layout name already exists. Do you want to overwrite it?"));
        if (box.exec() != QMessageBox::Yes)
        {
            SaveLayout();
            return;
        }
    }

    m_viewPaneManager->SaveLayout(layoutName);
}

void MainWindow::ViewDeletePaneLayout(const QString& layoutName)
{
    if (layoutName.isEmpty())
    {
        return;
    }

    QMessageBox box(this); // Not static so we can remove help button
    box.setText(tr("Delete Layout?"));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setIcon(QMessageBox::Warning);
    box.setWindowFlags(box.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    box.setInformativeText(tr("Are you sure you want to delete the layout '%1'?").arg(layoutName));
    if (box.exec() == QMessageBox::Yes)
    {
        m_viewPaneManager->RemoveLayout(layoutName);
    }
}

void MainWindow::ViewRenamePaneLayout(const QString& layoutName)
{
    if (layoutName.isEmpty())
    {
        return;
    }

    QString newLayoutName;
    bool validName = false;
    while (!validName)
    {
        newLayoutName = InputDialog::getText(this, tr("Layout Name"), QString(), QLineEdit::Normal, QString(), "[a-z]+[a-z0-9\\-\\_]*");

        if (m_viewPaneManager->HasLayout(newLayoutName))
        {
            QMessageBox box(this); // Not static so we can remove help button
            box.setText(tr("Layout name already exists"));
            box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            box.setIcon(QMessageBox::Warning);
            box.setWindowFlags(box.windowFlags() & ~Qt::WindowContextHelpButtonHint);
            box.setInformativeText(tr("The layout name '%1' already exists, please choose a different name").arg(newLayoutName));
            if (box.exec() == QMessageBox::No)
            {
                return;
            }
        }
        else
        {
            validName = true;
        }
    }

    m_viewPaneManager->RenameLayout(layoutName, newLayoutName);
}

void MainWindow::ViewLoadPaneLayout(const QString& layoutName)
{
    if (!layoutName.isEmpty())
    {
        m_viewPaneManager->RestoreLayout(layoutName);
    }
}

void MainWindow::ViewSavePaneLayout(const QString& layoutName)
{
    if (layoutName.isEmpty())
    {
        return;
    }

    QMessageBox box(this); // Not static so we can remove help button
    box.setText(tr("Overwrite Layout?"));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setIcon(QMessageBox::Warning);
    box.setWindowFlags(box.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    box.setInformativeText(tr("Do you want to overwrite the layout '%1' with the current one?").arg(layoutName));
    if (box.exec() == QMessageBox::Yes)
    {
        m_viewPaneManager->SaveLayout(layoutName);
    }
}

void MainWindow::OnUpdateConnectionStatus()
{
    auto* statusBar = StatusBar();

    if (!m_connectionListener)
    {
        statusBar->SetItem("connection", tr("Disconnected"), tr("Disconnected"), IDI_BALL_DISABLED);
        //TODO: disable clicking
    }
    else
    {
        using EConnectionState = EngineConnectionListener::EConnectionState;
        int icon = IDI_BALL_OFFLINE;
        QString tooltip, status;
        switch (m_connectionListener->GetState())
        {
        case EConnectionState::Connecting:
            // Checking whether we are not connected here instead of disconnect state because this function is called on a timer
            // and therefore we may not receive the disconnect state.
            if (m_connectedToAssetProcessor)
            {
                m_connectedToAssetProcessor = false;
                m_showAPDisconnectDialog = true;
            }
            tooltip = tr("Connecting to Asset Processor");
            icon = IDI_BALL_PENDING;
            break;
        case EConnectionState::Disconnecting:
            tooltip = tr("Disconnecting from Asset Processor");
            icon = IDI_BALL_PENDING;
            break;
        case EConnectionState::Listening:
            if (m_connectedToAssetProcessor)
            {
                m_connectedToAssetProcessor = false;
                m_showAPDisconnectDialog = true;
            }
            tooltip = tr("Listening for incoming connections");
            icon = IDI_BALL_PENDING;
            break;
        case EConnectionState::Connected:
            m_connectedToAssetProcessor = true;
            tooltip = tr("Connected to Asset Processor");
            icon = IDI_BALL_ONLINE;
            break;
        case EConnectionState::Disconnected:
            icon = IDI_BALL_OFFLINE;
            tooltip = tr("Disconnected from Asset Processor");
            break;
        }

        if (m_connectedToAssetProcessor)
        {
            m_connectionLostTimer->stop();
        }

        tooltip += "\n Last Asset Processor Task: ";
        tooltip += m_connectionListener->LastAssetProcessorTask().c_str();
        tooltip += "\n";
        AZStd::set<AZStd::string> failedJobs = m_connectionListener->FailedJobsList();
        int failureCount = static_cast<int>(failedJobs.size());
        if (failureCount)
        {
            tooltip += "\n Failed Jobs\n";
            for (auto failedJob : failedJobs)
            {
                tooltip += failedJob.c_str();
                tooltip += "\n";
            }
        }

        status = tr("Pending Jobs : %1  Failed Jobs : %2").arg(m_connectionListener->GetJobsCount()).arg(failureCount);

        statusBar->SetItem("connection", status, tooltip, icon);

        if (m_showAPDisconnectDialog && m_connectionListener->GetState() != EConnectionState::Connected)
        {
            m_showAPDisconnectDialog = false;// Just show the dialog only once if connection is lost
            m_connectionLostTimer->setSingleShot(true);
            m_connectionLostTimer->start(15000);
        }
    }
}

void MainWindow::ShowConnectionDisconnectedDialog()
{
    // when REMOTE_ASSET_PROCESSOR is undef'd it means behave as if there is no such thing as the remote asset processor.
#ifdef REMOTE_ASSET_PROCESSOR
    if (gEnv && gEnv->pSystem)
    {
        QMessageBox messageBox(this);
        messageBox.setWindowTitle(tr("Asset Processor has disconnected."));
        messageBox.setText(
            tr("Asset Processor is not connected. Please try (re)starting the Asset Processor or restarting the Editor.<br><br>"
                "Data may be lost while the Asset Processor is not running!<br>"
                "The status of the Asset Processor can be monitored from the editor in the bottom-right corner of the status bar.<br><br>"
                "Would you like to start the asset processor?<br>"));
        messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Ignore);
        messageBox.setDefaultButton(QMessageBox::Yes);
        messageBox.setIcon(QMessageBox::Critical);
        if (messageBox.exec() == QMessageBox::Yes)
        {
            AzFramework::AssetSystem::LaunchAssetProcessor();
        }
    }
    else
    {
        QMessageBox::critical(this, tr("Asset Processor has disconnected."),
            tr("Asset Processor is not connected. Please try (re)starting the asset processor or restarting the Editor.<br><br>"
                "Data may be lost while the asset processor is not running!<br>"
                "The status of the asset processor can be monitored from the editor in the bottom-right corner of the status bar."));
    }
#endif
}

void MainWindow::OnConnectionStatusClicked()
{
    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystemRequestBus::Events::ShowAssetProcessor);
}

static bool paneLessThan(const QtViewPane& v1, const QtViewPane& v2)
{
    return v1.m_name.compare(v2.m_name, Qt::CaseInsensitive) < 0;
}

void MainWindow::RegisterOpenWndCommands()
{
    s_openViewCmds.clear();

    auto panes = m_viewPaneManager->GetRegisteredPanes(/* viewPaneMenuOnly=*/ false);
    std::sort(panes.begin(), panes.end(), paneLessThan);

    for (auto viewPane : panes)
    {
        if (viewPane.m_category.isEmpty())
        {
            continue;
        }

        const QString className = viewPane.m_name;

        // Make a open-view command for the class.
        QString classNameLowered = viewPane.m_name.toLower();
        classNameLowered.replace(' ', '_');
        QString openCommandName = "open_";
        openCommandName += classNameLowered;

        CEditorOpenViewCommand* pCmd = new CEditorOpenViewCommand(GetIEditor(), viewPane.m_name);
        s_openViewCmds.push_back(pCmd);

        CCommand0::SUIInfo cmdUI;
        cmdUI.caption = className.toUtf8().data();
        cmdUI.tooltip = (QString("Open ") + className).toUtf8().data();
        cmdUI.iconFilename = className.toUtf8().data();
        GetIEditor()->GetCommandManager()->RegisterUICommand("editor", openCommandName.toUtf8().data(),
            "", "", [pCmd] { pCmd->Execute(); }, cmdUI);
        GetIEditor()->GetCommandManager()->GetUIInfo("editor", openCommandName.toUtf8().data(), cmdUI);
    }
}

bool MainWindow::event(QEvent* event)
{
#ifdef Q_OS_MAC
    if (event->type() == QEvent::HoverMove)
    {
        // this fixes a problem on macOS where the mouse cursor was not
        // set when hovering over the splitter handles between dock widgets
        // it might be fixed in future Qt versions
        auto mouse = static_cast<QHoverEvent*>(event);
        bool result = QMainWindow::event(event);
        void setCocoaMouseCursor(QWidget*);
        setCocoaMouseCursor(childAt(mouse->pos()));
        return result;
    }
#endif
    return QMainWindow::event(event);
}

void MainWindow::ToggleConsole()
{
    m_viewPaneManager->TogglePane(LyViewPane::Console);

    QtViewPane* pane = m_viewPaneManager->GetPane(LyViewPane::Console);
    if (!pane)
    {
        return;
    }

    // If we toggled the console on, we want to focus its input text field
    if (pane->IsVisible())
    {
        CConsoleSCB* console = qobject_cast<CConsoleSCB*>(pane->Widget());
        if (!console)
        {
            return;
        }

        console->SetInputFocus();
    }
}

void MainWindow::ConnectivityStateChanged(const AzToolsFramework::SourceControlState state)
{

    gSettings.enableSourceControl = (state == AzToolsFramework::SourceControlState::Active || state == AzToolsFramework::SourceControlState::ConfigurationInvalid);
    gSettings.SaveEnableSourceControlFlag(false);
    gSettings.SaveSettingsRegistryFile();
}

void MainWindow::OnGotoSelected()
{
    AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::GoToSelectedEntitiesInViewports);
}

// don't want to eat escape as if it were a shortcut, as it would eat it for other windows that also care about escape
// and are reading it as an event rather.
void MainWindow::keyPressEvent(QKeyEvent* e)
{
    // We shouldn't need to do this, as there's already an escape key shortcut set on an action
    // attached to the MainWindow. We need to explicitly trap the escape key here because when in
    // Game Mode, all of the actions attached to the MainWindow are disabled.
    if (e->key() == Qt::Key_Escape)
    {
       MainWindow::OnEscapeAction();
       return;
    }
    return QMainWindow::keyPressEvent(e);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    using namespace AzQtComponents;
    DragAndDropContextBase context;
    DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragEnter, event, context);
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
    using namespace AzQtComponents;
    DragAndDropContextBase context;
    DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragMove, event, context);
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    using namespace AzQtComponents;
    DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragLeave, event);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    using namespace AzQtComponents;
    DragAndDropContextBase context;
    DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::Drop, event, context);
}

bool MainWindow::focusNextPrevChild(bool next)
{
    // Don't change the focus when we're in game mode or else the viewport could
    // stop receiving input events
    if (GetIEditor()->IsInGameMode())
    {
        return false;
    }

    return QMainWindow::focusNextPrevChild(next);
}

namespace AzToolsFramework
{
    void MainWindowEditorFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("open_pane", PyOpenViewPane, nullptr, "Opens a view pane specified by the pane class name."));
            addLegacyGeneral(behaviorContext->Method("close_pane", PyCloseViewPane, nullptr, "Closes a view pane specified by the pane class name."));
            addLegacyGeneral(behaviorContext->Method("is_pane_visible", PyIsViewPaneVisible, nullptr, "Returns true if pane specified by the pane class name is visible."));
            addLegacyGeneral(behaviorContext->Method("get_pane_class_names", PyGetViewPaneNames, nullptr, "Get all available class names for use with open_pane & close_pane."));
            addLegacyGeneral(behaviorContext->Method("exit", PyExit, nullptr, "Exits the editor."));
            addLegacyGeneral(behaviorContext->Method("exit_no_prompt", PyExitNoPrompt, nullptr, "Exits the editor without prompting to save first."));
            addLegacyGeneral(behaviorContext->Method("test_output", PyTestOutput, nullptr, "Report test information."));
        }
    }
}

#include <moc_MainWindow.cpp>
