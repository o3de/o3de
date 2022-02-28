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

    // Currently (December 13, 2021), this function is only used by slice editor code.
    // When the slice editor is not enabled, there are no references to the
    // HideActionWhileEntitiesDeselected function, causing a compiler warning and
    // subsequently a build error.
#ifdef ENABLE_SLICE_EDITOR
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
#endif

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

    m_editmenu = editMenu;

    m_topLevelMenus << editMenu;
    m_topLevelMenus << CreateGameMenu();
    m_topLevelMenus << CreateToolsMenu();
    m_topLevelMenus << CreateViewMenu();
    m_topLevelMenus << CreateHelpMenu();

    // have to do this after creating the AWS Menu for the first time
    ResetToolsMenus();

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
}

QMenu* LevelEditorMenuHandler::CreateFileMenu()
{
    return nullptr;
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
    snapMenu.AddAction(AzToolsFramework::SnapToGrid);

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
    return nullptr;
}

QMenu* LevelEditorMenuHandler::CreateGameMenu()
{
    return nullptr;
}

QMenu* LevelEditorMenuHandler::CreateToolsMenu()
{
    return nullptr;
}

QMenu* LevelEditorMenuHandler::CreateViewMenu()
{
    return nullptr;
}

QMenu* LevelEditorMenuHandler::CreateHelpMenu()
{
    return nullptr;
}

QAction* LevelEditorMenuHandler::CreateViewPaneAction([[maybe_unused]] const QtViewPane* view)
{
    return nullptr;
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
    [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
    [[maybe_unused]] AzToolsFramework::ViewportEditorMode mode)
{
}

void LevelEditorMenuHandler::OnEditorModeDeactivated(
    [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
{
    if (mode == ViewportEditorMode::Component)
    {
        RestoreEditMenuToDefault();
    }
}

void LevelEditorMenuHandler::AddEditMenuAction([[maybe_unused]] QAction* action)
{
}

void LevelEditorMenuHandler::AddMenuAction(
    [[maybe_unused]] AZStd::string_view categoryId, [[maybe_unused]] QAction* action, [[maybe_unused]]  bool addToToolsToolbar)
{
}

void LevelEditorMenuHandler::RestoreEditMenuToDefault()
{
}

#include <Core/moc_LevelEditorMenuHandler.cpp>
