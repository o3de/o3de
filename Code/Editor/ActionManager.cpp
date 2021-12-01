/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ActionManager.h"

// Qt
#include <QSignalMapper>
#include <QDebug>
#include <QMenuBar>
#include <QScopedValueRollback>

// Editor
#include "MainWindow.h"
#include "QtViewPaneManager.h"
#include "ShortcutDispatcher.h"
#include "ToolbarManager.h"


static const char* const s_reserved = "Reserved"; ///< "Reserved" property used for actions that cannot be overridden.
                                                  ///< (e.g. KeySequences such as Ctrl+S and Ctrl+Z for Save/Undo etc.)

static const char* const s_menuIdProperty = "MenuId"; ///< "MenuId" property used when adding top level menus to the
                                                      ///<  menu bar so they can be uniquely identified with FindMenu()

static const int s_invalidGuardActionId = -1;

ActionManagerExecutionGuard::ActionManagerExecutionGuard(ActionManager* actionManager, QAction* action)
    : m_actionManager(actionManager)
    , m_actionId(s_invalidGuardActionId)
    , m_canExecute(true)
{
    // both actionManager and action can be nullptr, and this is totally valid.
    // the lambdas using this might be out of scope and their QPointers may have been cleared
    if (actionManager && action)
    {
        m_actionId = action->data().toInt();
        m_canExecute = actionManager->InsertActionExecuting(m_actionId);
    }
}

ActionManagerExecutionGuard::ActionManagerExecutionGuard(ActionManager* actionManager, int actionId)
    : m_actionManager(actionManager)
    , m_actionId(actionId)
    , m_canExecute(true)
{
    // both actionManager and action can be nullptr, and this is totally valid. The lambdas using this might be out of scope and
    // their QPointers may have been cleared
    if (actionManager)
    {
        m_canExecute = actionManager->InsertActionExecuting(m_actionId);
    }
}

ActionManagerExecutionGuard::~ActionManagerExecutionGuard()
{
    // Only bother removing the action if it successfully inserted, indicated by m_canExecute.
    // If during the insert, it was found that the action was already present, then there
    // is no need to remove it, and doing so might cause problems because any it implies
    // that something has already executed the action higher up the callstack and we don't
    // want to remove the id so that future action triggers within the same callstack are prevented
    // from executing.
    if (m_canExecute && m_actionManager && (m_actionId != s_invalidGuardActionId))
    {
        m_actionManager->RemoveActionExecuting(m_actionId);
    }
}

PatchedAction::PatchedAction(const QString& name, QObject* parent)
    : QAction(name, parent)
{
}

bool PatchedAction::event(QEvent* ev)
{
    // *Really* honor Qt::WindowShortcut. Floating dock widgets are a separate window (Qt::Window flag is set) even though they have a parent.

    if (ev->type() == QEvent::Shortcut && shortcutContext() == Qt::WindowShortcut)
    {
        // This prevents shortcuts from firing while we're in a long running operation
        // started by a shortcut
        static bool reentranceLock = false;
        if (reentranceLock)
        {
            return true;
        }

        QScopedValueRollback<bool> reset(reentranceLock, true);

        QWidget* focusWidget = ShortcutDispatcher::focusWidget();
        if (!focusWidget)
        {
            return QAction::event(ev);
        }

        for (QWidget* associatedWidget : associatedWidgets())
        {
            QWidget* associatedWindow = associatedWidget->window();
            QWidget* focusWindow = focusWidget->window();
            if (associatedWindow == focusWindow)
            {
                // Fair enough, we accept it.
                return QAction::event(ev);
            }
            else if (associatedWindow && focusWindow)
            {
                /**
                 * But do allow if the focused window is actually a floating dock widget.
                 * For example, If Entity Outliner is floating, the gizmos (key 1, 2, 3 4) should still work.
                 *
                 * FIXME: But then why are those main toolbar actions using Qt::WindowShortcut instead of Qt::ApplicationShortcut ?
                 * This block goes against what the original PatchedAction fixed.
                 * Consider either removing it and using regular QActions, or using Qt::ApplicationShortcut
                 * See also LY-35177
                 */
                QString focusWindowName = focusWindow->objectName();
                if (focusWindowName.isEmpty())
                {
                    continue;
                }

                QWidget* child = associatedWindow->findChild<QWidget*>(focusWindowName);
                if (child)
                {
                    // Also accept if the focus window is a child of the associated window
                    return QAction::event(ev);
                }
            }
        }

        // Bug detected: Qt is propagating a shortcut with context Qt::WindowShortcut outside of window boundaries
        // Consume the event instead of processing it.
        qDebug() << "Discarding buggy shortcut";
        return true;
    }

    return QAction::event(ev);
}

/////////////////////////////////////////////////////////////////////////////
// ActionWrapper
/////////////////////////////////////////////////////////////////////////////
ActionManager::ActionWrapper& ActionManager::ActionWrapper::SetMenu(DynamicMenu* menu)
{
    menu->SetAction(m_action, m_actionManager);
    return *this;
}

ActionManager::ActionWrapper& ActionManager::ActionWrapper::SetReserved()
{
    m_action->setProperty("Reserved", true);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// DynamicMenu
/////////////////////////////////////////////////////////////////////////////
DynamicMenu::DynamicMenu(QObject* parent)
    : QObject(parent)
    , m_action(nullptr)
    , m_menu(nullptr)
{
    m_actionMapper = new QSignalMapper(this);
    connect(m_actionMapper, SIGNAL(mapped(int)), this, SLOT(TriggerAction(int)));
}

void DynamicMenu::SetAction(QAction* action, ActionManager* am)
{
    Q_ASSERT(action);
    Q_ASSERT(am);
    m_action = action;
    m_menu = new QMenu();
    m_action->setMenu(m_menu);
    connect(m_menu, &QMenu::aboutToShow, this, &DynamicMenu::ShowMenu);
    m_actionManager = am;
    setParent(m_action);
}

void DynamicMenu::SetParentMenu(QMenu* menu, ActionManager* am)
{
    Q_ASSERT(menu && !m_menu);
    Q_ASSERT(!m_action);
    m_menu = menu;
    connect(m_menu, &QMenu::aboutToShow, this, &DynamicMenu::ShowMenu);
    m_actionManager = am;
}

void DynamicMenu::AddAction(int id, QAction* action)
{
    Q_ASSERT(!m_actions.contains(id));
    action->setData(id);
    m_actions[id] = action;
    m_menu->addAction(action);

    connect(action, SIGNAL(triggered()), m_actionMapper, SLOT(map()));
    m_actionMapper->setMapping(action, id);
}

void DynamicMenu::AddSeparator()
{
    Q_ASSERT(m_menu);
    m_menu->addSeparator();
}

ActionManager::ActionWrapper DynamicMenu::AddAction(int id, const QString& name)
{
    QAction* action = new PatchedAction(name, this);
    AddAction(id, action);
    return ActionManager::ActionWrapper(action, m_actionManager);
}

void DynamicMenu::UpdateAllActions()
{
    for (auto action : m_menu->actions())
    {
        int id = action->data().toInt();
        OnMenuUpdate(id, action);
    }
}

void DynamicMenu::ShowMenu()
{
    if (m_actions.isEmpty())
    {
        CreateMenu();
    }
    UpdateAllActions();
}

void DynamicMenu::TriggerAction(int id)
{
    OnMenuChange(id, m_actions.value(id));
    UpdateAllActions();
}

#include <QTimer>

/////////////////////////////////////////////////////////////////////////////
// ActionManager
/////////////////////////////////////////////////////////////////////////////
ActionManager::ActionManager(
    MainWindow* parent, QtViewPaneManager* const qtViewPaneManager, ShortcutDispatcher* shortcutDispatcher)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_qtViewPaneManager(qtViewPaneManager)
    , m_shortcutDispatcher(shortcutDispatcher)
{
    m_actionMapper = new QSignalMapper(this);
    connect(m_actionMapper, SIGNAL(mapped(int)), this, SLOT(ActionTriggered(int)));

    connect(m_qtViewPaneManager, &QtViewPaneManager::registeredPanesChanged, this, &ActionManager::RebuildRegisteredViewPaneIds);

    // KDAB_TODO: This will be used later, particularly for the toolbars
    //connect(QCoreApplication::eventDispatcher(), SIGNAL(aboutToBlock()),
    //    this, SLOT(UpdateActions()));
    // so long use a simple timer to make it work
    QTimer* timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &ActionManager::UpdateActions);
    timer->start();

    // connect to the Action Request Bus and notify other listeners this has happened
    AzToolsFramework::EditorActionRequestBus::Handler::BusConnect();
}

ActionManager::~ActionManager()
{
    AzToolsFramework::EditorActionRequestBus::Handler::BusDisconnect();
}

void ActionManager::AddMenu(QMenu* menu)
{
    m_menus.push_back(menu);
    connect(menu, &QMenu::aboutToShow, this, &ActionManager::UpdateMenu);
}

ActionManager::MenuWrapper ActionManager::AddMenu(const QString& title, const QString& menuId)
{
    const auto menu = new QMenu(title);

    // set a unique identifier for this menu item so it
    // can be looked up later using FindMenu()
    if (!menuId.isEmpty())
    {
        menu->setProperty(s_menuIdProperty, menuId);
    }

    AddMenu(menu);

    return MenuWrapper(menu, this);
}

ActionManager::MenuWrapper ActionManager::FindMenu(const QString& menuId)
{
    // attempt to find menu by menuId
    auto menuIt = AZStd::find_if(m_menus.begin(), m_menus.end(), [&menuId](const QMenu* menu)
    {
        if (!menu->property(s_menuIdProperty).isNull())
        {
            return menu->property(s_menuIdProperty).toString() == menuId;
        }

        return false;
    });

    // return the menu with the matching name, if not found return nullptr
    QMenu* menu = [this, menuIt, &menuId]() -> QMenu*
    {
        if (menuIt != m_menus.end())
        {
            return *menuIt;
        }

        AZ_UNUSED(menuId); // Prevent unused warning in release builds
        AZ_Warning("ActionManager", false, "Did not find menu with menuId %s", menuId.toUtf8().data());
        return nullptr;
    }();

    return MenuWrapper(menu, this);
}

void ActionManager::AddToolBar(QToolBar* toolBar)
{
    m_toolBars.push_back(toolBar);
}

ActionManager::ToolBarWrapper ActionManager::AddToolBar(int id)
{
    AmazonToolbar t = m_mainWindow->GetToolbarManager()->GetToolbar(id);
    Q_ASSERT(t.IsInstantiated());

    AddToolBar(t.Toolbar());
    return ToolBarWrapper(t.Toolbar(), this);
}

bool ActionManager::eventFilter([[maybe_unused]] QObject* watched, QEvent* event)
{
    // if events are shortcut events, we don't want to filter out
    if (event->type() == QEvent::Shortcut)
    {
        m_isShortcutEvent = true;
    }

    return false;
}

bool ActionManager::InsertActionExecuting(int id)
{
    // If the action handler puts up a modal dialog, the event queue will be pumped
    // and double clicks on menu items will go through, in some cases.
    // This is to guard against that.
    if (m_executingIds.find(id) != m_executingIds.end())
    {
        return false;
    }

    m_executingIds.insert(id);
    return true;
}

bool ActionManager::RemoveActionExecuting(int id)
{
    bool idWasInList = m_executingIds.remove(id);
    Q_ASSERT(idWasInList);
    return idWasInList;
}


void ActionManager::AddAction(int id, QAction* action)
{
    action->setData(id);
    AddAction(action);
}

void ActionManager::AddAction(AZ::Crc32 id, QAction* action)
{
    action->setData(aznumeric_cast<AZ::u32>(id));
    AddAction(action);
}


void ActionManager::AddAction(QAction* action)
{
    const int id = action->data().toInt();

    if (m_actions.contains(id))
    {
        qWarning() << "ActionManager already contains action with id=" << id;
        Q_ASSERT(false);
    }

    m_actions[id] = action;
    connect(action, SIGNAL(triggered()), m_actionMapper, SLOT(map()));
    m_actionMapper->setMapping(action, id);

    action->installEventFilter(this);

    // Add the action if the parent is a widget
    auto widget = qobject_cast<QWidget*>(parent());
    if (widget)
    {
        widget->addAction(action);
    }
}

void ActionManager::RemoveAction(QAction* action)
{
    auto storedAction = m_actions.find(action->data().toInt());
    if (storedAction != m_actions.end())
    {
        m_actions.remove(action->data().toInt());
    }

    action->removeEventFilter(this);
    m_actionMapper->removeMappings(action);

    if (auto widget = qobject_cast<QWidget*>(parent()))
    {
        widget->removeAction(action);
    }
}

ActionManager::ActionWrapper ActionManager::AddAction(int id, const QString& name)
{
    QAction* action = ActionIsWidget(id) ? new WidgetAction(id, m_mainWindow, name, this)
        : static_cast<QAction*>(new PatchedAction(name, this)); // static cast to base so ternary compiles
    AddAction(id, action);
    return ActionWrapper(action, this);
}

ActionManager::ActionWrapper ActionManager::AddAction(AZ::Crc32 id, const QString& name)
{
    QAction* action = ActionIsWidget(aznumeric_cast<AZ::u32>(id))
        ? new WidgetAction(aznumeric_cast<AZ::u32>(id), m_mainWindow, name, this)
        : static_cast<QAction*>(new PatchedAction(name, this)); // static cast to base so ternary compile
    AddAction(id, action);
    return ActionWrapper(action, this);
}

bool ActionManager::HasAction(QAction* action) const
{
    return action && HasAction(action->data().toInt());
}

bool ActionManager::HasAction(int id) const
{
    return m_actions.contains(id);
}

QAction* ActionManager::GetAction(int id) const
{
    auto it = m_actions.find(id);

    if (it == m_actions.cend())
    {
        qWarning() << Q_FUNC_INFO << "Couldn't get action " << id;
        Q_ASSERT(false);
        return nullptr;
    }
    else
    {
        return *it;
    }
}

void ActionManager::ActionTriggered(int id)
{
    if (m_mainWindow->menuBar()->isEnabled())
    {
        if (m_actionHandlers.contains(id))
        {
            ActionManagerExecutionGuard guard(this, id);

            if (guard.CanExecute())
            {
                m_actionHandlers[id]();
            }
        }
    }
}

void ActionManager::RebuildRegisteredViewPaneIds()
{
    QtViewPanes views = QtViewPaneManager::instance()->GetRegisteredPanes();

    for (auto& view : views)
    {
        m_registeredViewPaneIds.insert(view.m_id);
    }
}

// is this action suspended (allowed to respond or not)
static bool ActionSuspended(const bool defaultActionsSuspended, const QAction* const action)
{
    // if default actions are suspended and this is not a reserved action, do not update
    return defaultActionsSuspended && !action->property(s_reserved).toBool();
}

// for the menu that is about to be opened, visit each action (menu item) and
// update callbacks for unsuspended actions.
// recurse one level deep if the action is itself a menu, if all actions
// in that menu are suspended, gray out the menu so it cannot be selected.
static void UpdateMenus(
    QMenu* menu, const bool defaultActionsSuspended,
    QHash<int, std::function<void()>>& updateCallbacks,
    QList<QAction*> topLevelMenuActions, int depth)
{
    const auto actions = menu->actions();
    const int actionCount = actions.size();
    int suspendedActionCounter =  0;

    for (auto action : actions)
    {
        // if an action is itself a menu, we want to check its
        // own menu actions (children), but only one level down
        if (action->menu() && depth == 0)
        {
            UpdateMenus(
                action->menu(), defaultActionsSuspended,
                updateCallbacks, topLevelMenuActions, depth + 1);
        }

        if (ActionSuspended(defaultActionsSuspended, action))
        {
            suspendedActionCounter++;
            continue;
        }

        // call all update callbacks for the given menu
        // only do this at the level of the menu we clicked/hovered
        const auto id = action->data().toInt();
        if (updateCallbacks.contains(id) && depth == 0)
        {
            updateCallbacks.value(id)();
        }
    }

    // check if we are a top level menu action
    if (AZStd::find(
        topLevelMenuActions.begin(), topLevelMenuActions.end(),
        menu->menuAction()) == topLevelMenuActions.end())
    {
        // if we're not a top level menu action, we want to disable
        // the sub menu if none of the child actions are active, otherwise
        // ensure the menu is returned to an enabled state
        menu->menuAction()->setEnabled(
            defaultActionsSuspended
                ? suspendedActionCounter != actionCount
                : true);
    }
}

void ActionManager::UpdateMenu()
{
    auto menu = qobject_cast<QMenu*>(sender());
    AZ_Assert(menu, "sender() was not convertible to a QMenu*");

    int depth = 0;
    UpdateMenus(
        menu, m_defaultActionsSuspended, m_updateCallbacks,
        m_mainWindow->menuBar()->actions(), depth);
}

void ActionManager::UpdateActions()
{
    for (auto it = m_updateCallbacks.constBegin(); it != m_updateCallbacks.constEnd(); ++it)
    {
        if (ActionSuspended(m_defaultActionsSuspended, GetAction(it.key())))
        {
            continue;
        }

        it.value()();
    }
}

QList<QAction*> ActionManager::GetActions() const
{
    return m_actions.values();
}

bool ActionManager::ActionIsWidget(int id) const
{
    return id >= ID_TOOLBAR_WIDGET_FIRST && id <= ID_TOOLBAR_WIDGET_LAST;
}

// either enable or disable all registered actions
void SetDefaultActionsEnabled(
    const QList<QAction*>& actions, const bool enabled)
{
    AZStd::for_each(
        actions.begin(), actions.end(),
        [enabled](QAction* action)
    {
        if (!action->property(s_reserved).toBool())
        {
            action->setEnabled(enabled);
        }
    });
}

void ActionManager::AddActionViaBus(int id, QAction* action)
{
    AddAction(id, action);
}

void ActionManager::AddActionViaBusCrc(AZ::Crc32 id, QAction* action)
{
    AddAction(id, action);
}

void ActionManager::RemoveActionViaBus(QAction* action)
{
    RemoveAction(action);
}

void ActionManager::EnableDefaultActions()
{
     SetDefaultActionsEnabled(GetActions(), true);
     m_defaultActionsSuspended = false;
}

void ActionManager::DisableDefaultActions()
{
    SetDefaultActionsEnabled(GetActions(), false);
    m_defaultActionsSuspended = true;
}

void ActionManager::AttachOverride(QWidget* object)
{
    m_shortcutDispatcher->AttachOverride(object);
}

void ActionManager::DetachOverride()
{
    m_shortcutDispatcher->DetachOverride();
}

WidgetAction::WidgetAction(int actionId, MainWindow* mainWindow, const QString& name, QObject* parent)
    : QWidgetAction(parent)
    , m_actionId(actionId)
    , m_mainWindow(mainWindow)
{
    setText(name);
}

QWidget* WidgetAction::createWidget(QWidget* parent)
{
    QWidget* w = m_mainWindow->CreateToolbarWidget(m_actionId);
    if (w)
    {
        w->setParent(parent);
    }

    return w;
}

#include <moc_ActionManager.cpp>
