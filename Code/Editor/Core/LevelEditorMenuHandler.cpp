/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

// Editor
#include "Resource.h"
#include "LevelEditorMenuHandler.h"
#include "CryEdit.h"
#include "MainWindow.h"
#include "QtViewPaneManager.h"
#include "ToolBox.h"
#include "Include/IObjectManager.h"
#include "Objects/SelectionGroup.h"
#include "ViewManager.h"

#include <AzCore/Interface/Interface.h>

// Qt
#include <QMenuBar>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QUrl>

// AzFramework
#include <AzFramework/API/ApplicationAPI.h>

// AzToolsFramework
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

// AzQtComponents
#include <AzQtComponents/Components/SearchLineEdit.h>

using namespace AZ;
using namespace AzToolsFramework;

static const char* const s_LUAEditorName = "Lua Editor";

// top level menu ids
static const char* const s_fileMenuId = "FileMenu";
static const char* const s_editMenuId = "EditMenu";
static const char* const s_gameMenuId = "GameMenu";
static const char* const s_toolMenuId = "ToolMenu";
static const char* const s_viewMenuId = "ViewMenu";
static const char* const s_helpMenuId = "HelpMenu";

static bool CompareLayoutNames(const QString& name1, const QString& name2)
{
    return name1.compare(name2, Qt::CaseInsensitive) < 0;
}

static void AddOpenViewPaneAction(ActionManager::MenuWrapper& menu, const char* viewPaneName, const char* menuActionText = nullptr)
{
    QAction* action = menu->addAction(QObject::tr(menuActionText ? menuActionText : viewPaneName));
    QObject::connect(action, &QAction::triggered, action, [viewPaneName]()
        {
            QtViewPaneManager::instance()->OpenPane(viewPaneName);
        });
}

namespace
{
    // This helper allows us to watch editor notifications to control action enable states
    class EditorListener
        : public QObject
        , public IEditorNotifyListener
    {
    public:
        explicit EditorListener(QObject* parent, AZStd::function<void(EEditorNotifyEvent)> trigger)
            : QObject(parent)
            , m_trigger(trigger)
        {}

        ~EditorListener() override
        {
            GetIEditor()->UnregisterNotifyListener(this);
        }

        void OnEditorNotifyEvent(EEditorNotifyEvent event) override
        {
            m_trigger(event);
        }

    private:
        AZStd::function<void(EEditorNotifyEvent)> m_trigger;
    };

    void DisableActionWhileLevelChanges(QAction* action, EEditorNotifyEvent e)
    {
        if (e == eNotify_OnBeginNewScene || e == eNotify_OnBeginLoad)
        {
            action->setDisabled(true);
        }
        else if (e == eNotify_OnEndNewScene || e == eNotify_OnEndLoad)
        {
            action->setDisabled(false);
        }
    }

    void HideActionWhileEntitiesDeselected(QAction* action, EEditorNotifyEvent editorNotifyEvent)
    {
        if (action == nullptr)
        {
            return;
        }

        switch (editorNotifyEvent)
        {
        case eNotify_OnEntitiesDeselected:
        {
            action->setVisible(false);
            break;
        }
        case eNotify_OnEntitiesSelected:
        {
            action->setVisible(true);
            break;
        }
        default:
            break;
        }
    }

    void DisableActionWhileInSimMode(QAction* action, EEditorNotifyEvent editorNotifyEvent)
    {
        if (action == nullptr)
        {
            return;
        }

        switch (editorNotifyEvent)
        {
        case eNotify_OnBeginSimulationMode:
        {
            action->setVisible(false);
            action->setDisabled(true);
            break;
        }
        case eNotify_OnEndSimulationMode:
        {
            action->setVisible(true);
            action->setDisabled(false);
            break;
        }
        default:
            break;
        }
    }
}

LevelEditorMenuHandler::LevelEditorMenuHandler(MainWindow* mainWindow, QtViewPaneManager* const viewPaneManager)
    : QObject(mainWindow)
    , m_mainWindow(mainWindow)
    , m_viewPaneManager(viewPaneManager)
    , m_actionManager(mainWindow->GetActionManager())
{
#if defined(AZ_PLATFORM_MAC)
    // Hide the non-native toolbar, then setNativeMenuBar to ensure it is always visible on macOS.
    m_mainWindow->menuBar()->hide();
    m_mainWindow->menuBar()->setNativeMenuBar(true);
#endif

    ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId());
    EditorMenuRequestBus::Handler::BusConnect();
}

LevelEditorMenuHandler::~LevelEditorMenuHandler()
{
    EditorMenuRequestBus::Handler::BusDisconnect();
    ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
}

void LevelEditorMenuHandler::Initialize()
{
    // make sure we can fix the view menus
    connect(
        m_viewPaneManager, &QtViewPaneManager::registeredPanesChanged,
        this, &LevelEditorMenuHandler::ResetToolsMenus);

    m_levelExtension = EditorUtils::LevelFile::GetDefaultFileExtension();

    m_topLevelMenus << CreateFileMenu();

    auto editMenu = CreateEditMenu();
    auto editMenuWrapper = ActionManager::MenuWrapper(editMenu, m_actionManager);
    PopulateEditMenu(editMenuWrapper);

    m_editmenu = editMenu;

    m_topLevelMenus << editMenu;
    m_topLevelMenus << CreateGameMenu();
    m_topLevelMenus << CreateToolsMenu();
    m_topLevelMenus << CreateViewMenu();
    m_topLevelMenus << CreateHelpMenu();

    // have to do this after creating the AWS Menu for the first time
    ResetToolsMenus();

    // Add our menus to the main window menu bar
    QMenuBar* menuBar = m_mainWindow->menuBar();
    menuBar->clear();
    for (QMenu* menu : m_topLevelMenus)
    {
        menuBar->addMenu(menu);
    }
}

bool LevelEditorMenuHandler::MRUEntryIsValid(const QString& entry, const QString& gameFolderPath)
{
    if (entry.isEmpty())
    {
        return false;
    }

    QFileInfo info(entry);
    if (!info.exists())
    {
        return false;
    }

    if (!entry.endsWith(m_levelExtension))
    {
        return false;
    }

    const QDir gameDir(gameFolderPath);
    QDir dir(entry); // actually pointing at file, first cdUp() gets us the parent dir
    while (dir.cdUp())
    {
        if (dir == gameDir)
        {
            return true;
        }
    }

    return false;
}

void LevelEditorMenuHandler::IncrementViewPaneVersion()
{
    m_viewPaneVersion++;
}

int LevelEditorMenuHandler::GetViewPaneVersion() const
{
    return m_viewPaneVersion;
}

void LevelEditorMenuHandler::UpdateViewLayoutsMenu(ActionManager::MenuWrapper& layoutsMenu)
{
    if (layoutsMenu.isNull())
    {
        return;
    }

    QStringList layoutNames = m_viewPaneManager->LayoutNames();
    std::sort(layoutNames.begin(), layoutNames.end(), CompareLayoutNames);
    layoutsMenu->clear();
    const int MAX_LAYOUTS = ID_VIEW_LAYOUT_LAST - ID_VIEW_LAYOUT_FIRST + 1;

    // Component entity layout is the default
    QString componentLayoutLabel = tr("Component Entity Layout");
    QAction* componentLayoutAction = layoutsMenu->addAction(componentLayoutLabel);
    connect(componentLayoutAction, &QAction::triggered, this, &LevelEditorMenuHandler::LoadComponentLayout);

    layoutsMenu.AddSeparator();

    for (int i = 0; i < layoutNames.size() && i <= MAX_LAYOUTS; i++)
    {
        const QString layoutName = layoutNames[i];
        QAction* action = layoutsMenu->addAction(layoutName);
        QMenu* subSubMenu = new QMenu();

        QAction* subSubAction = subSubMenu->addAction(tr("Load"));
        connect(subSubAction, &QAction::triggered, this, [layoutName, this]() { m_mainWindow->ViewLoadPaneLayout(layoutName); });

        subSubAction = subSubMenu->addAction(tr("Save"));
        connect(subSubAction, &QAction::triggered, this, [layoutName, this]() { m_mainWindow->ViewSavePaneLayout(layoutName); });

        subSubAction = subSubMenu->addAction(tr("Rename..."));
        connect(subSubAction, &QAction::triggered, this, [layoutName, this]() { m_mainWindow->ViewRenamePaneLayout(layoutName); });

        subSubAction = subSubMenu->addAction(tr("Delete"));
        connect(subSubAction, &QAction::triggered, this, [layoutName, this]() { m_mainWindow->ViewDeletePaneLayout(layoutName); });

        action->setMenu(subSubMenu);
    }

    layoutsMenu.AddAction(ID_VIEW_SAVELAYOUT);

    layoutsMenu.AddAction(ID_VIEW_LAYOUT_LOAD_DEFAULT);
}

void LevelEditorMenuHandler::ResetToolsMenus()
{
    if (!m_toolsMenu->isEmpty())
    {
        // Clear everything from the Tools menu
        m_toolsMenu->clear();
        AzToolsFramework::EditorMenuNotificationBus::Broadcast(&AzToolsFramework::EditorMenuNotificationBus::Handler::OnResetToolMenuItems);
    }

    QtViewPanes allRegisteredViewPanes = QtViewPaneManager::instance()->GetRegisteredPanes();

    QMap<QString, QList<QtViewPane*> > menuMap;

    CreateMenuMap(menuMap, allRegisteredViewPanes);

    CreateMenuOptions(&menuMap, m_toolsMenu, LyViewPane::CategoryTools);

    AzToolsFramework::EditorMenuNotificationBus::Broadcast(&AzToolsFramework::EditorMenuNotificationBus::Handler::OnPopulateToolMenuItems);

    m_toolsMenu.AddSeparator();

    // Other
    auto otherSubMenu = m_toolsMenu.AddMenu(QObject::tr("Other"));

    CreateMenuOptions(&menuMap, otherSubMenu, LyViewPane::CategoryOther);

    m_toolsMenu.AddSeparator();

    // Optional Sub Menus
    if (!menuMap.isEmpty())
    {
        QMap<QString, QList<QtViewPane*> >::iterator it = menuMap.begin();

        while (it != menuMap.end())
        {
            auto currentSubMenu = m_toolsMenu.AddMenu(it.key());
            CreateMenuOptions(&menuMap, currentSubMenu, it.key().toStdString().c_str());

            it = menuMap.begin();
        }
    }
}

QMenu* LevelEditorMenuHandler::CreateFileMenu()
{
    auto fileMenu = m_actionManager->AddMenu(tr("&File"), s_fileMenuId);
    connect(fileMenu, &QMenu::aboutToShow, this, &LevelEditorMenuHandler::OnUpdateOpenRecent);

    // New level
    auto fileNew = fileMenu.AddAction(ID_FILE_NEW);
    GetIEditor()->RegisterNotifyListener(new EditorListener(fileNew, [fileNew](EEditorNotifyEvent e)
    {
        DisableActionWhileLevelChanges(fileNew, e);
    }));

    // Open level...
    auto fileOpenLevel = fileMenu.AddAction(ID_FILE_OPEN_LEVEL);
    GetIEditor()->RegisterNotifyListener(new EditorListener(fileOpenLevel, [fileOpenLevel](EEditorNotifyEvent e)
    {
        DisableActionWhileLevelChanges(fileOpenLevel, e);
    }));

#ifdef ENABLE_SLICE_EDITOR
    // New slice
    auto fileNewSlice = fileMenu.AddAction(ID_FILE_NEW_SLICE);
    GetIEditor()->RegisterNotifyListener(new EditorListener(fileNewSlice, [fileNewSlice](EEditorNotifyEvent e)
    {
        DisableActionWhileLevelChanges(fileNewSlice, e);
    }));

    // Open slice...
    auto fileOpenSlice = fileMenu.AddAction(ID_FILE_OPEN_SLICE);
    GetIEditor()->RegisterNotifyListener(new EditorListener(fileOpenSlice, [fileOpenSlice](EEditorNotifyEvent e)
    {
        DisableActionWhileLevelChanges(fileOpenSlice, e);
    }));
#endif

    // Save Selected Slice
    auto saveSelectedSlice = fileMenu.AddAction(ID_FILE_SAVE_SELECTED_SLICE);
    saveSelectedSlice->setVisible(false);
    GetIEditor()->RegisterNotifyListener(new EditorListener(saveSelectedSlice, [saveSelectedSlice](EEditorNotifyEvent e)
    {
        HideActionWhileEntitiesDeselected(saveSelectedSlice, e);
    }));

    // Save Slice to Root
    auto saveSliceToRoot = fileMenu.AddAction(ID_FILE_SAVE_SLICE_TO_ROOT);
    saveSliceToRoot->setVisible(false);
    GetIEditor()->RegisterNotifyListener(new EditorListener(saveSliceToRoot, [saveSliceToRoot](EEditorNotifyEvent e)
    {
        HideActionWhileEntitiesDeselected(saveSliceToRoot, e);
    }));

    // Open Recent
    m_mostRecentLevelsMenu = fileMenu.AddMenu(tr("Open Recent"));
    connect(m_mostRecentLevelsMenu, &QMenu::aboutToShow, this, &LevelEditorMenuHandler::UpdateMRUFiles);

    OnUpdateOpenRecent();

    // Import...
    auto assetImporterMenu = fileMenu.Get()->addAction(tr("Import..."));
    connect(assetImporterMenu, &QAction::triggered, this, &LevelEditorMenuHandler::ActivateAssetImporter);

    fileMenu.AddSeparator();

    // Save
    fileMenu.AddAction(ID_FILE_SAVE_LEVEL);

    // Save As...
    fileMenu.AddAction(ID_FILE_SAVE_AS);

    // Save Level Statistics
    fileMenu.AddAction(ID_TOOLS_LOGMEMORYUSAGE);
    fileMenu.AddSeparator();

    // Project Settings
    fileMenu.AddAction(ID_FILE_PROJECT_MANAGER_SETTINGS);

    // Platform Settings - Project Settings Tool
    // Shortcut must be set while adding the action otherwise it doesn't work
    fileMenu.Get()->addAction(
        tr(LyViewPane::ProjectSettingsTool),
        []() { QtViewPaneManager::instance()->OpenPane(LyViewPane::ProjectSettingsTool); },
        tr("Ctrl+Shift+P"));

    fileMenu.AddSeparator();
    fileMenu.AddAction(ID_FILE_PROJECT_MANAGER_NEW);
    fileMenu.AddAction(ID_FILE_PROJECT_MANAGER_OPEN);
    fileMenu.AddSeparator();

    // NEWMENUS: NEEDS IMPLEMENTATION
    // should have the equivalent of a Close here; it should be close the current slice, but the editor isn't slice based right now
    // so that won't work.
    // instead, it should be Close of the level, but we don't have that either. I'm leaving it here so that it's obvious where UX intended it
    // to go
    //fileMenu.AddAction(ID_FILE_CLOSE);

    // Show Log File
    fileMenu.AddAction(ID_FILE_EDITLOGFILE);

    fileMenu.AddSeparator();

    fileMenu.AddAction(ID_FILE_RESAVESLICES);

    fileMenu.AddSeparator();

    fileMenu.AddAction(ID_APP_EXIT);

    return fileMenu;
}

void LevelEditorMenuHandler::PopulateEditMenu(ActionManager::MenuWrapper& editMenu)
{
    // Undo
    editMenu.AddAction(ID_UNDO);

    // Redo
    editMenu.AddAction(ID_REDO);

    editMenu.AddSeparator();

    // NEWMENUS: NEEDS IMPLEMENTATION
    // Not quite ready for these yet. Have to register them with the ActionManager in MainWindow.cpp when we're ready
    // editMenu->addAction(ID_EDIT_CUT);
    // editMenu->addAction(ID_EDIT_COPY);
    // editMenu->addAction(ID_EDIT_PASTE);
    // editMenu.AddSeparator();

    // Duplicate
    editMenu.AddAction(AzToolsFramework::DuplicateSelect);

    // Delete
    editMenu.AddAction(AzToolsFramework::DeleteSelect);

    editMenu.AddSeparator();

    // Select All
    editMenu.AddAction(AzToolsFramework::SelectAll);

    // Invert Selection
    editMenu.AddAction(AzToolsFramework::InvertSelect);

    editMenu.AddSeparator();

    // New Viewport Interaction Model actions/shortcuts
    editMenu.AddAction(AzToolsFramework::EditPivot);
    editMenu.AddAction(AzToolsFramework::EditReset);
    editMenu.AddAction(AzToolsFramework::EditResetManipulator);

    // Hide Selection
    editMenu.AddAction(AzToolsFramework::HideSelection);

    // Show All
    editMenu.AddAction(AzToolsFramework::ShowAll);

    // Lock Selection
    editMenu.AddAction(AzToolsFramework::LockSelection);

    // UnLock All
    editMenu.AddAction(AzToolsFramework::UnlockAll);

    /*
     * The following block of code is part of the feature "Isolation Mode" and is temporarily
     * disabled for 1.10 release.
     * Jira: LY-49532
    // Isolate Selected
    QAction* isolateSelectedAction = editMenu->addAction(tr("Isolate Selected"));
    connect(isolateSelectedAction, &QAction::triggered, this, []() {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::EnterEditorIsolationMode);
    });
    // Exit Isolation
    QAction* exitIsolationAction = editMenu->addAction(tr("Exit Isolation"));
    connect(exitIsolationAction, &QAction::triggered, this, []() {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::ExitEditorIsolationMode);
    });
    connect(editMenu, &QMenu::aboutToShow, this, [isolateSelectedAction, exitIsolationAction]() {
        bool isInIsolationMode = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isInIsolationMode, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsEditorInIsolationMode);
        if (isInIsolationMode)
        {
            isolateSelectedAction->setDisabled(true);
            exitIsolationAction->setDisabled(false);
        }
        else
        {
            isolateSelectedAction->setDisabled(false);
            exitIsolationAction->setDisabled(true);
        }
    });
    */

    editMenu.AddSeparator();

    // Modify Menu
    auto modifyMenu = editMenu.AddMenu(tr("&Modify"));

    auto snapMenu = modifyMenu.AddMenu(tr("Snap"));

    snapMenu.AddAction(AzToolsFramework::SnapAngle);

    auto transformModeMenu = modifyMenu.AddMenu(tr("Transform Mode"));
    transformModeMenu.AddAction(AzToolsFramework::EditModeMove);
    transformModeMenu.AddAction(AzToolsFramework::EditModeRotate);
    transformModeMenu.AddAction(AzToolsFramework::EditModeScale);

    editMenu.AddSeparator();

    // Editor Settings
    auto editorSettingsMenu = editMenu.AddMenu(tr("Editor Settings"));

    // Global Preferences...
    editorSettingsMenu.AddAction(ID_TOOLS_PREFERENCES);

    // Editor Settings Manager
    AddOpenViewPaneAction(editorSettingsMenu, LyViewPane::EditorSettingsManager);

    // Keyboard Customization
    auto keyboardCustomizationMenu = editorSettingsMenu.AddMenu(tr("Keyboard Customization"));
    keyboardCustomizationMenu.AddAction(ID_TOOLS_CUSTOMIZEKEYBOARD);
    keyboardCustomizationMenu.AddAction(ID_TOOLS_EXPORT_SHORTCUTS);
    keyboardCustomizationMenu.AddAction(ID_TOOLS_IMPORT_SHORTCUTS);
}

QMenu* LevelEditorMenuHandler::CreateEditMenu()
{
    return m_actionManager->AddMenu(tr("&Edit"), s_editMenuId);
}

QMenu* LevelEditorMenuHandler::CreateGameMenu()
{
    auto gameMenu = m_actionManager->AddMenu(tr("&Game"), s_gameMenuId);

    // Play Game
    gameMenu.AddAction(ID_VIEW_SWITCHTOGAME);

    // Enable Physics/AI
    gameMenu.AddAction(ID_SWITCH_PHYSICS);
    gameMenu.AddSeparator();


    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
    if (!usePrefabSystemForLevels)
    {
        // Export to Engine
        gameMenu.AddAction(ID_FILE_EXPORTTOGAMENOSURFACETEXTURE);
    }

    // Export Selected Objects
    gameMenu.AddAction(ID_FILE_EXPORT_SELECTEDOBJECTS);

    // Export Occlusion Mesh
    gameMenu.AddAction(ID_FILE_EXPORTOCCLUSIONMESH);

    gameMenu.AddSeparator();

    // Synchronize Player with Camera
    gameMenu.AddAction(ID_GAME_SYNCPLAYER);

    gameMenu.AddSeparator();

    // Audio
    auto audioMenu = gameMenu.AddMenu(tr("Audio"));

    // Stop All Sounds
    audioMenu.AddAction(ID_SOUND_STOPALLSOUNDS);

    // Refresh Audio
    audioMenu.AddAction(ID_AUDIO_REFRESH_AUDIO_SYSTEM);

    gameMenu.AddSeparator();

    CreateDebuggingSubMenu(gameMenu);

    return gameMenu;
}

QMenu* LevelEditorMenuHandler::CreateToolsMenu()
{
    // Tools
    m_toolsMenu = m_actionManager->AddMenu(tr("&Tools"), s_toolMenuId);
    return m_toolsMenu;
}

QMenu* LevelEditorMenuHandler::CreateViewMenu()
{
    auto viewMenu = m_actionManager->AddMenu(tr("&View"), s_viewMenuId);

    // NEWMENUS: NEEDS IMPLEMENTATION
    // minimize window - Ctrl+M
    // Zoom - Ctrl+Plus(+) -> Need the inverse too?

#ifdef FEATURE_ORTHOGRAPHIC_VIEW
    // Cycle Viewports
    viewMenu.AddAction(ID_VIEW_CYCLE2DVIEWPORT);
#endif

    // Center on Selection
    viewMenu.AddAction(ID_MODIFY_GOTO_SELECTION);

    // Show Quick Access Bar
    viewMenu.AddAction(ID_OPEN_QUICK_ACCESS_BAR);

    // Layouts
    if (CViewManager::IsMultiViewportEnabled()) // Only supports 1 viewport for now.
    {
        // Disable Layouts menu
        m_layoutsMenu = viewMenu.AddMenu(tr("Layouts"));
        connect(m_viewPaneManager, &QtViewPaneManager::savedLayoutsChanged, this, [this]()
            {
                UpdateViewLayoutsMenu(m_layoutsMenu);
            });

        UpdateViewLayoutsMenu(m_layoutsMenu);
    }

    // Viewport
    auto viewportViewsMenuWrapper = viewMenu.AddMenu(tr("Viewport"));

#ifdef FEATURE_ORTHOGRAPHIC_VIEW
    auto viewportTypesMenuWrapper = viewportViewsMenuWrapper.AddMenu(tr("Viewport Type"));

    m_viewportViewsMenu = viewportViewsMenuWrapper;
    connect(viewportTypesMenuWrapper, &QMenu::aboutToShow, this, &LevelEditorMenuHandler::UpdateOpenViewPaneMenu);

    InitializeViewPaneMenu(m_actionManager, viewportTypesMenuWrapper, [](const QtViewPane& view)
        {
            return view.IsViewportPane();
        });

    viewportViewsMenuWrapper.AddSeparator();

#endif

    if (CViewManager::IsMultiViewportEnabled())
    {
        viewportViewsMenuWrapper.AddAction(ID_VIEW_CONFIGURELAYOUT);
    }
    viewportViewsMenuWrapper.AddSeparator();

    viewportViewsMenuWrapper.AddAction(ID_DISPLAY_GOTOPOSITION);
    viewportViewsMenuWrapper.AddAction(ID_MODIFY_GOTO_SELECTION);

    auto gotoLocationMenu = viewportViewsMenuWrapper.AddMenu(tr("Go to Location"));
    gotoLocationMenu.AddAction(ID_GOTO_LOC1);
    gotoLocationMenu.AddAction(ID_GOTO_LOC2);
    gotoLocationMenu.AddAction(ID_GOTO_LOC3);
    gotoLocationMenu.AddAction(ID_GOTO_LOC4);
    gotoLocationMenu.AddAction(ID_GOTO_LOC5);
    gotoLocationMenu.AddAction(ID_GOTO_LOC6);
    gotoLocationMenu.AddAction(ID_GOTO_LOC7);
    gotoLocationMenu.AddAction(ID_GOTO_LOC8);
    gotoLocationMenu.AddAction(ID_GOTO_LOC9);
    gotoLocationMenu.AddAction(ID_GOTO_LOC10);
    gotoLocationMenu.AddAction(ID_GOTO_LOC11);
    gotoLocationMenu.AddAction(ID_GOTO_LOC12);

    auto rememberLocationMenu = viewportViewsMenuWrapper.AddMenu(tr("Remember Location"));
    rememberLocationMenu.AddAction(ID_TAG_LOC1);
    rememberLocationMenu.AddAction(ID_TAG_LOC2);
    rememberLocationMenu.AddAction(ID_TAG_LOC3);
    rememberLocationMenu.AddAction(ID_TAG_LOC4);
    rememberLocationMenu.AddAction(ID_TAG_LOC5);
    rememberLocationMenu.AddAction(ID_TAG_LOC6);
    rememberLocationMenu.AddAction(ID_TAG_LOC7);
    rememberLocationMenu.AddAction(ID_TAG_LOC8);
    rememberLocationMenu.AddAction(ID_TAG_LOC9);
    rememberLocationMenu.AddAction(ID_TAG_LOC10);
    rememberLocationMenu.AddAction(ID_TAG_LOC11);
    rememberLocationMenu.AddAction(ID_TAG_LOC12);

    viewportViewsMenuWrapper.AddSeparator();

    auto switchCameraMenu = viewportViewsMenuWrapper.AddMenu(tr("Switch Camera"));
    switchCameraMenu.AddAction(ID_SWITCHCAMERA_DEFAULTCAMERA);
    switchCameraMenu.AddAction(ID_SWITCHCAMERA_SEQUENCECAMERA);
    switchCameraMenu.AddAction(ID_SWITCHCAMERA_SELECTEDCAMERA);
    switchCameraMenu.AddAction(ID_SWITCHCAMERA_NEXT);

    // NEWMENUS:
    // MISSING AVIRECORDER

    viewportViewsMenuWrapper.AddSeparator();
    viewportViewsMenuWrapper.AddAction(ID_DISPLAY_SHOWHELPERS);

    // Refresh Style
    viewMenu.AddAction(ID_SKINS_REFRESH);

    return viewMenu;
}

QMenu* LevelEditorMenuHandler::CreateHelpMenu()
{
    // Help
    auto helpMenu = m_actionManager->AddMenu(tr("&Help"), s_helpMenuId);

    auto lineEditSearchAction = new QWidgetAction(m_mainWindow);
    auto containerWidget = new QWidget(m_mainWindow);
    auto lineEdit = new AzQtComponents::SearchLineEdit(m_mainWindow);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(lineEdit);
    containerWidget->setLayout(layout);
    containerWidget->setContentsMargins(2, 0, 2, 0);
    lineEdit->setPlaceholderText(tr("Search documentation"));
    lineEditSearchAction->setDefaultWidget(containerWidget);

    auto searchAction = [lineEdit]()
        {
            auto text = lineEdit->text();
            if (text.isEmpty())
            {
                QDesktopServices::openUrl(QUrl("https://o3de.org/docs/"));
            }
            else
            {
                static const int versionStringSize = 128;
                char productVersionString[versionStringSize];
                const SFileVersion& productVersion = gEnv->pSystem->GetProductVersion();
                productVersion.ToString(productVersionString, versionStringSize);

                QUrl docSearchUrl("https://o3de.org/docs/");
                QUrlQuery docSearchQuery;
                docSearchQuery.addQueryItem("query", text);
                docSearchUrl.setQuery(docSearchQuery);
                QDesktopServices::openUrl(docSearchUrl);
            }
            lineEdit->clear();
        };
    connect(lineEdit, &QLineEdit::returnPressed, this, searchAction);
    connect(helpMenu.Get(), &QMenu::aboutToHide, lineEdit, &QLineEdit::clear);
    connect(helpMenu.Get(), &QMenu::aboutToShow, lineEdit, &QLineEdit::clearFocus);
    helpMenu->addAction(lineEditSearchAction);

    // Tutorials
    helpMenu.AddAction(ID_DOCUMENTATION_TUTORIALS);

    // Documentation
    auto documentationMenu = helpMenu.AddMenu(tr("Documentation"));

    // Open 3D Engine Documentation
    documentationMenu.AddAction(ID_DOCUMENTATION_O3DE);

    // GameLift Documentation
    documentationMenu.AddAction(ID_DOCUMENTATION_GAMELIFT);

    // Release Notes
    documentationMenu.AddAction(ID_DOCUMENTATION_RELEASENOTES);

    // GameDev Resources
    auto gameDevResourceMenu = helpMenu.AddMenu(tr("GameDev Resources"));

    // Game Dev Blog
    gameDevResourceMenu.AddAction(ID_DOCUMENTATION_GAMEDEVBLOG);

    // Forums
    gameDevResourceMenu.AddAction(ID_DOCUMENTATION_FORUMS);

    // AWS Support
    gameDevResourceMenu.AddAction(ID_DOCUMENTATION_AWSSUPPORT);

    helpMenu.AddSeparator();

    // About Open 3D Engine
    helpMenu.AddAction(ID_APP_ABOUT);

    // Welcome dialog
    auto helpWelcome = helpMenu.AddAction(ID_APP_SHOW_WELCOME);
    GetIEditor()->RegisterNotifyListener(new EditorListener(helpWelcome, [helpWelcome](EEditorNotifyEvent e)
        {
            DisableActionWhileLevelChanges(helpWelcome, e);
        }));

    return helpMenu;
}

QAction* LevelEditorMenuHandler::CreateViewPaneAction(const QtViewPane* view)
{
    QAction* action = m_actionManager->HasAction(view->m_id) ? m_actionManager->GetAction(view->m_id) : nullptr;

    if (!action)
    {
        const QString menuText = view->m_options.optionalMenuText.length() > 0
            ? view->m_options.optionalMenuText
            : view->m_name;

        action = new QAction(menuText, this);
        action->setObjectName(view->m_name);
        action->setCheckable(true);

        if (view->m_options.showOnToolsToolbar)
        {
            action->setIcon(QIcon(view->m_options.toolbarIcon.c_str()));
        }

        m_actionManager->AddAction(view->m_id, action);

        if (!view->m_options.shortcut.isEmpty())
        {
            action->setShortcut(view->m_options.shortcut);
        }

        QObject::connect(
            action, &QAction::triggered, this, &LevelEditorMenuHandler::checkOrOpenView, Qt::UniqueConnection);
    }

    return action;
}

// Function used to show menu options without its icon and be able to toggle shortcut visibility in the new menu layout
// This is a work around for the fact that setting the shortcut on the original action isn't working for some reasons
// and we need to investigate it further in the future.
QAction* LevelEditorMenuHandler::CreateViewPaneMenuItem(
    [[maybe_unused]] ActionManager* actionManager, ActionManager::MenuWrapper& menu, const QtViewPane* view)
{
    QAction* action = CreateViewPaneAction(view);

    if (action && view->m_options.isDisabledInSimMode)
    {
        AddDisableActionInSimModeListener(action);
    }

    menu->addAction(action);

    if (view->m_options.showOnToolsToolbar)
    {
        m_mainWindow->GetToolbarManager()->AddButtonToEditToolbar(action);
    }

    return action;
}

void LevelEditorMenuHandler::InitializeViewPaneMenu(
    ActionManager* actionManager, ActionManager::MenuWrapper& menu,
    AZStd::function <bool(const QtViewPane& view)> functor)
{
    QtViewPanes views = QtViewPaneManager::instance()->GetRegisteredPanes();

    for (auto it = views.cbegin(), end = views.cend(); it != end; ++it)
    {
        const QtViewPane& view = *it;
        if (!functor(view))
        {
            continue;
        }

        CreateViewPaneMenuItem(actionManager, menu, it);
    }
}

void LevelEditorMenuHandler::LoadComponentLayout()
{
    m_viewPaneManager->RestoreDefaultLayout();
}

QMap<QString, QList<QtViewPane*>> LevelEditorMenuHandler::CreateMenuMap(
    QMap<QString, QList<QtViewPane*> >& menuMap, QtViewPanes& allRegisteredViewPanes)
{
    // set up view panes to each category
    for (QtViewPane& viewpane : allRegisteredViewPanes)
    {
        // only store the view panes that should be shown in the menu
        if (!viewpane.IsViewportPane())
        {
            menuMap[viewpane.m_category].push_back(&viewpane);
        }
    }

    return menuMap;
}

// sort the menu option name in alphabetically order
struct CaseInsensitiveStringCompare
{
    bool operator() (const QString& left, const QString& right) const
    {
        return left.toLower() < right.toLower();
    }
};

static void LaunchLuaEditor()
{
    AzToolsFramework::EditorRequestBus::Broadcast(
        &AzToolsFramework::EditorRequests::LaunchLuaEditor, nullptr);
}

void LevelEditorMenuHandler::CreateMenuOptions(
    QMap<QString, QList<QtViewPane*> >* menuMap, ActionManager::MenuWrapper& menu, const char* category)
{
    // list in the menu and remove this menu category from the menuMap
    QList<QtViewPane*> menuList = menuMap->take(category);

    std::map<QString, std::function<void()>, CaseInsensitiveStringCompare> sortMenuMap;

    // store menu options into the map
    // name as a key, functionality as a value
    for (QtViewPane* viewpane : menuList)
    {
        if (viewpane->m_options.builtInActionId != LyViewPane::NO_BUILTIN_ACTION)
        {
            sortMenuMap[viewpane->m_name] = [this, &menu, viewpane]()
            {
                // Handle shortcuts for actions with a built-in ID since they
                // bypass our CreateViewPaneMenuItem method
                QAction* action = menu.AddAction(viewpane->m_options.builtInActionId);
                if (action)
                {
                    if (viewpane->m_options.isDisabledInSimMode)
                    {
                        AddDisableActionInSimModeListener(action);
                    }
                    if (!viewpane->m_options.shortcut.isEmpty())
                    {
                        action->setShortcut(viewpane->m_options.shortcut);
                    }
                }
            };
        }
        else
        {
            const QString menuText = viewpane->m_options.optionalMenuText.length() > 0
                ? viewpane->m_options.optionalMenuText
                : viewpane->m_name;

            sortMenuMap[menuText] = [this, viewpane, &menu]()
            {
                CreateViewPaneMenuItem(m_actionManager, menu, viewpane);
            };
        }
    }

    if (category == LyViewPane::CategoryTools)
    {
        // Add LUA Editor into the Tools map
        sortMenuMap[s_LUAEditorName] = [this, &menu]()
        {
            auto luaEditormenu = menu.AddAction(ID_TOOLS_LUA_EDITOR);
            connect(luaEditormenu, &QAction::triggered, this, &LaunchLuaEditor, Qt::UniqueConnection);
        };
    }

    // add each menu option into the menu
    std::map<QString, std::function<void()>, CaseInsensitiveStringCompare>::iterator iter;
    for (iter = sortMenuMap.begin(); iter != sortMenuMap.end(); ++iter)
    {
        iter->second();
    }
}

void LevelEditorMenuHandler::CreateDebuggingSubMenu(ActionManager::MenuWrapper gameMenu)
{
    // DebuggingSubMenu
    auto debuggingSubMenu = gameMenu.AddMenu(QObject::tr("Debugging"));

    // Error Report
    AddOpenViewPaneAction(debuggingSubMenu, LyViewPane::ErrorReport);

    debuggingSubMenu.AddSeparator();

    // Configure Toolbox Macros
    debuggingSubMenu.AddAction(ID_TOOLS_CONFIGURETOOLS);

    // Toolbox Macros
    m_macrosMenu = debuggingSubMenu.AddMenu(tr("ToolBox Macros"));
    connect(m_macrosMenu, &QMenu::aboutToShow, this, &LevelEditorMenuHandler::UpdateMacrosMenu, Qt::UniqueConnection);

    UpdateMacrosMenu();
}

void LevelEditorMenuHandler::UpdateMRUFiles()
{
    auto cryEdit = CCryEditApp::instance();
    RecentFileList* mruList = cryEdit->GetRecentFileList();
    const int numMru = mruList->GetSize();

    if (!m_mostRecentLevelsMenu)
    {
        return;
    }

    // Remove most recent items
    m_mostRecentLevelsMenu->clear();

    // Insert mrus
    QString sCurDir = QString(Path::GetEditingGameDataFolder().c_str()) + QDir::separator();

    QFileInfo gameDir(sCurDir); // Pass it through QFileInfo so it comes out normalized
    const QString gameDirPath = gameDir.absolutePath();

    for (int i = 0; i < numMru; ++i)
    {
        if (!MRUEntryIsValid((*mruList)[i], gameDirPath))
        {
            continue;
        }

        QString displayName;
        mruList->GetDisplayName(displayName, i, sCurDir);

        QString entry = QString("%1 %2").arg(i + 1).arg(displayName);
        QAction* action = m_actionManager->GetAction(ID_FILE_MRU_FILE1 + i);
        action->setText(entry);

        m_actionManager->RegisterActionHandler(ID_FILE_MRU_FILE1 + i, [i]()
            {
                auto cryEdit = CCryEditApp::instance();
                RecentFileList* mruList = cryEdit->GetRecentFileList();
                // Check file is still available
                if (mruList->GetSize() > i)
                {
                    cryEdit->OpenDocumentFile((*mruList)[i].toUtf8().data());
                }
            });
        m_actionManager->RegisterUpdateCallback(ID_FILE_MRU_FILE1 + i, cryEdit, &CCryEditApp::OnUpdateFileOpen);

        GetIEditor()->RegisterNotifyListener(new EditorListener(action, [action](EEditorNotifyEvent e)
            {
                DisableActionWhileLevelChanges(action, e);
            }));

        m_mostRecentLevelsMenu->addAction(action);
    }

    // Used when disabling the "Open Recent" menu options
    OnUpdateOpenRecent();

    m_mostRecentLevelsMenu->addSeparator();

    // Clear All
    auto clearAllMenu = m_mostRecentLevelsMenu->addAction(tr("Clear All"));
    connect(clearAllMenu, &QAction::triggered, this, &LevelEditorMenuHandler::ClearAll);
}

void LevelEditorMenuHandler::ClearAll()
{
    RecentFileList* mruList = CCryEditApp::instance()->GetRecentFileList();

    // remove everything from the mru list
    for (int i = mruList->GetSize(); i > 0; i--)
    {
        mruList->Remove(i - 1);
    }

    // save the settings immediately to the registry
    mruList->WriteList();

    // re-update the menus
    UpdateMRUFiles();
}

// Used for disabling "Open Recent" menu option
void LevelEditorMenuHandler::OnUpdateOpenRecent()
{
    RecentFileList* mruList = CCryEditApp::instance()->GetRecentFileList();
    const int numMru = mruList->GetSize();
    QString currentMru = numMru > 0 ? (*mruList)[0] : QString();

    if (!currentMru.isEmpty())
    {
        m_mostRecentLevelsMenu->setEnabled(true);
    }
    else
    {
        m_mostRecentLevelsMenu->setEnabled(false);
    }
}

void LevelEditorMenuHandler::OnUpdateMacrosMenu()
{
    auto tools = GetIEditor()->GetToolBoxManager();
    const int macroCount = tools->GetMacroCount(true);

    if (macroCount <= 0)
    {
        m_macrosMenu->setEnabled(false);
    }
    else
    {
        m_macrosMenu->setEnabled(true);
    }
}

void LevelEditorMenuHandler::UpdateMacrosMenu()
{
    m_macrosMenu->clear();

    auto tools = GetIEditor()->GetToolBoxManager();
    const int macroCount = tools->GetMacroCount(true);

    for (int i = 0; i < macroCount; ++i)
    {
        auto macro = tools->GetMacro(i, true);
        const int toolbarId = macro->GetToolbarId();
        if (toolbarId == -1 || toolbarId == ID_TOOLS_TOOL1)
        {
            m_macrosMenu->addAction(macro->action());
        }
    }
}

void LevelEditorMenuHandler::UpdateOpenViewPaneMenu()
{
    // This function goes through all the viewport menu actions (top, left, perspective...)
    // and adds a check mark on the viewport that has focus

    QtViewport* viewport = m_mainWindow->GetActiveViewport();
    QString activeViewportName = viewport ? viewport->GetName() : QString();

    for (QAction* action : m_viewportViewsMenu->actions())
    {
        action->setChecked(action->objectName() == activeViewportName);
    }
}

void LevelEditorMenuHandler::checkOrOpenView()
{
    QAction* action = qobject_cast<QAction*>(sender());
    const QString viewPaneName = action->objectName();
    // If this action is checkable and was just unchecked, then we
    // should close the view pane
    if (action->isCheckable() && !action->isChecked())
    {
        QtViewPaneManager::instance()->ClosePane(viewPaneName);
    }
    // Otherwise, this action should open the view pane
    else
    {
        const QtViewPane* pane = QtViewPaneManager::instance()->OpenPane(viewPaneName);

        QObject::connect(pane->Widget(), &QWidget::destroyed, action, [action]
        {
            action->setChecked(false);
        });
    }
}

void LevelEditorMenuHandler::AddDisableActionInSimModeListener(QAction* action)
{
    GetIEditor()->RegisterNotifyListener(new EditorListener(action, [action](EEditorNotifyEvent e)
    {
        DisableActionWhileInSimMode(action, e);
    }));
}

void LevelEditorMenuHandler::OnEditorModeActivated(
    [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
{
    if (mode == ViewportEditorMode::Component)
    {
        if (auto menuWrapper = m_actionManager->FindMenu(s_editMenuId);
            !menuWrapper.isNull())
        {
            // copy of menu actions
            auto actions = menuWrapper.Get()->actions();
            // remove all non-reserved edit menu options
            actions.erase(
                std::remove_if(actions.begin(), actions.end(), [](QAction* action)
                {
                    return !action->property("Reserved").toBool();
                }),
                actions.end());

            // clear and update the menu with new actions
            menuWrapper.Get()->clear();
            menuWrapper.Get()->addActions(actions);
        }
    }
}

void LevelEditorMenuHandler::OnEditorModeDeactivated(
    [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
{
    if (mode == ViewportEditorMode::Component)
    {
        RestoreEditMenuToDefault();
    }
}

void LevelEditorMenuHandler::AddEditMenuAction(QAction* action)
{
    if (auto menuWrapper = m_actionManager->FindMenu(s_editMenuId);
        !menuWrapper.isNull())
    {
        menuWrapper.Get()->addAction(action);
    }
}

void LevelEditorMenuHandler::AddMenuAction(AZStd::string_view categoryId, QAction* action, bool addToToolsToolbar)
{
    auto menuWrapper = m_actionManager->FindMenu(categoryId.data());
    if (menuWrapper.isNull())
    {
        AZ_Assert(false, "No %s category exists in Editor menu.");
        return;
    }
    menuWrapper.Get()->addAction(action);

    if (addToToolsToolbar)
    {
        m_mainWindow->GetToolbarManager()->AddButtonToEditToolbar(action);
    }
}

void LevelEditorMenuHandler::RestoreEditMenuToDefault()
{
    if (auto menuWrapper = m_actionManager->FindMenu(s_editMenuId);
        !menuWrapper.isNull())
    {
        menuWrapper.Get()->clear();
        PopulateEditMenu(menuWrapper);
    }
}

#include <Core/moc_LevelEditorMenuHandler.cpp>
