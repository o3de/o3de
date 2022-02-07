/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#if !defined(Q_MOC_RUN)
#include <QObject>

#include <QPointer>
#include <QHash>
#include <QVector>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QIcon>
#include <QWidgetAction>

#include <QSet>

#include <functional>
#include <utility>

#include <AzToolsFramework/Viewport/ActionBus.h>
#endif

class QSignalMapper;
class QPixmap;
class QDockWidget;
class DynamicMenu;
class MainWindow;
class QtViewPaneManager;
class ShortcutDispatcher;

class PatchedAction
    : public QAction
{
    // PatchedAction is a workaround for the fact that Qt doesn't honour Qt::WindowShortcut context for floating dock widgets.

    Q_OBJECT
public:
    explicit PatchedAction(const QString& name, QObject* parent = nullptr);
    bool event(QEvent*) override;
};


class WidgetAction
    : public QWidgetAction
{
    Q_OBJECT
public:
    explicit WidgetAction(int actionId, MainWindow* mainWindow, const QString& name, QObject* parent);
protected:
    QWidget* createWidget(QWidget* parent) override;
private:
    const int m_actionId;
    MainWindow* const m_mainWindow;
};

class ActionManager;

// If the action handler puts up a modal dialog, the event queue will be pumped
// and double clicks on menu items will go through, in some cases.
// This class can be used to guard against that.
class ActionManagerExecutionGuard
{
public:
    ActionManagerExecutionGuard(ActionManager* actionManager, QAction* action);
    ActionManagerExecutionGuard(ActionManager* actionManager, int actionId);
    ~ActionManagerExecutionGuard();

    // Only execute the action of this returns true
    bool CanExecute() const { return m_canExecute; }

private:
    QPointer<ActionManager> m_actionManager;
    int m_actionId;
    bool m_canExecute;
};

class ActionManager
    : public QObject
    , private AzToolsFramework::EditorActionRequestBus::Handler
{
    Q_OBJECT

public:
    class ActionWrapper
    {
    public:
        ActionWrapper& SetText(const QString& text) { m_action->setText(text); return *this; }
        ActionWrapper& SetIcon(const QIcon& icon) { m_action->setIcon(icon); return *this; }
        ActionWrapper& SetIcon(const QPixmap& icon) { m_action->setIcon(QIcon(icon)); return *this; }
        ActionWrapper& SetShortcut(const QString& shortcut) { m_action->setShortcut(shortcut); return *this; }
        ActionWrapper& SetShortcut(const QKeySequence& shortcut) { m_action->setShortcut(shortcut); return *this; }
        ActionWrapper& SetToolTip(const QString& toolTip) { m_action->setToolTip(toolTip); return *this; }
        ActionWrapper& SetStatusTip(const QString& statusTip) { m_action->setStatusTip(statusTip); return *this; }
        ActionWrapper& SetCheckable(bool value) { m_action->setCheckable(value); return *this; }
        ActionWrapper& SetReserved(); // if reserved, the action should not be allowed to be disabled or overridden

        //ActionWrapper &SetMenu(QMenu *menu) { m_action->setMenu(menu); return *this; }
        //ActionWrapper &SetMenu(QMenu *menu) { m_action->setMenu(menu); return *this; }
        template <typename Func1, typename Func2>
        ActionWrapper& Connect(Func1 signal, Func2 slot)
        {
            // The ActionWrapper class is stack based usually so
            // the lambda below can't use the this pointer.
            // As a result, wrap the actionManager and the action in
            // a QPointer, and capture them in the lambda
            QPointer<ActionManager> actionManager = m_actionManager;
            QPointer<QAction> action = m_action;

            QObject::connect(m_action, signal, m_action, [action, actionManager, slot]()
                {
                    ActionManagerExecutionGuard guard(actionManager.data(), action.data());

                    if (!GetIEditor()->IsInGameMode() && guard.CanExecute())
                    {
                        slot();
                    }
                });
            return *this;
        }

        template <typename Func1, typename Object, typename Func2>
        ActionWrapper& Connect(Func1 signal, Object* context, const Func2 slot, Qt::ConnectionType type = Qt::AutoConnection)
        {
            // The ActionWrapper class is stack based usually so
            // the lambda below can't use the this pointer.
            // As a result, wrap the actionManager and the action in
            // a QPointer, and capture them in the lambda
            QPointer<ActionManager> actionManager = m_actionManager;
            QPointer<QAction> action = m_action;

            QObject::connect(m_action, signal, context, [action, actionManager, context, slot]()
                {
                    ActionManagerExecutionGuard guard(actionManager.data(), action.data());

                    if (!GetIEditor()->IsInGameMode() && guard.CanExecute())
                    {
                        (context->*slot)();
                    }
                }, type);

            return *this;
        }

        ActionWrapper& SetMenu(DynamicMenu* menu);

        operator QAction*() const {
            return m_action;
        }
        QAction* operator->() const { return m_action; }

        template<typename T>
        ActionWrapper& RegisterUpdateCallback(T* object, void (T::* method)(QAction*))
        {
            m_actionManager->RegisterUpdateCallback(m_action->data().toInt(), object, method);
            return *this;
        }

        template<typename Fn>
        ActionWrapper& RegisterUpdateCallback(Fn&& fn)
        {
            m_actionManager->RegisterUpdateCallback(m_action->data().toInt(), AZStd::forward<Fn>(fn));
            return *this;
        }

    private:
        friend ActionManager;
        friend DynamicMenu;
        ActionWrapper(QAction* action, ActionManager* am)
            : m_action(action)
            , m_actionManager(am) { Q_ASSERT(m_action); }

        QAction* m_action;
        ActionManager* m_actionManager;
    };

    class MenuWrapper
    {
    public:
        MenuWrapper() = default;
        MenuWrapper(QMenu* menu, ActionManager* am)
            : m_menu(menu)
            , m_actionManager(am) {}

        MenuWrapper& SetTitle(const QString& text) { m_menu->setTitle(text); return *this; }
        MenuWrapper& SetIcon(const QIcon& icon) { m_menu->setIcon(icon); return *this; }

        QAction* AddAction(int id)
        {
            auto action = m_actionManager->GetAction(id);
            Q_ASSERT(action);
            m_menu->addAction(action);

            return action;
        }

        QAction* AddSeparator()
        {
            return m_menu->addSeparator();
        }

        MenuWrapper AddMenu(const QString& name, const QString& menuId = QString())
        {
            auto menu = m_actionManager->AddMenu(name, menuId);
            m_menu->addMenu(menu);
            return menu;
        }

        bool isNull() const
        {
            return !m_menu;
        }

        operator QMenu*()
        {
            return m_menu;
        }

        QMenu* operator->()
        {
            return m_menu;
        }

        QMenu* Get()
        {
            return m_menu;
        }

    private:
        friend ActionManager;

        QPointer<QMenu> m_menu = nullptr;
        ActionManager* m_actionManager = nullptr;
    };

    class ToolBarWrapper
    {
    public:
        QAction* AddAction(int id)
        {
            auto action = m_actionManager->GetAction(id);
            Q_ASSERT(action);
            m_toolBar->addAction(action);

            return action;
        }

        QAction* AddSeparator()
        {
            QAction* action = m_toolBar->addSeparator();

            // For the Dnd to work:
            action->setData(ID_TOOLBAR_SEPARATOR);
            QWidget* w = m_toolBar->widgetForAction(action);
            w->addAction(action);

            return action;
        }

        void AddWidget(QWidget* widget)
        {
            m_toolBar->addWidget(widget);
        }

        operator QToolBar*() const {
            return m_toolBar;
        }
        QToolBar* operator->() const { return m_toolBar; }

    private:
        friend ActionManager;
        ToolBarWrapper(QToolBar* toolBar, ActionManager* am)
            : m_toolBar(toolBar)
            , m_actionManager(am) { Q_ASSERT(m_toolBar); }

        QToolBar* m_toolBar;
        ActionManager* m_actionManager;
    };

public:
    explicit ActionManager(
        MainWindow* parent, QtViewPaneManager* qtViewPaneManager,
        ShortcutDispatcher* shortcutDispatcher);
    ~ActionManager();
    void AddMenu(QMenu* menu);
    MenuWrapper AddMenu(const QString& title, const QString& menuId);
    MenuWrapper FindMenu(const QString& menuId);
    bool ActionIsWidget(int actionId) const;

    void AddToolBar(QToolBar* toolBar);
    ToolBarWrapper AddToolBar(int id);

    void AddAction(QAction* action);
    void AddAction(int id, QAction* action);
    void AddAction(AZ::Crc32 id, QAction* action);

    void RemoveAction(QAction* action);

    ActionWrapper AddAction(int id, const QString& name);
    ActionWrapper AddAction(AZ::Crc32, const QString& name);

    bool HasAction(QAction*) const;
    bool HasAction(int id) const;

    QAction* GetAction(int id) const;
    QList<QAction*> GetActions() const;

    // AzToolsFramework::EditorActionRequests
    void AddActionViaBus(int id, QAction* action) override;
    void AddActionViaBusCrc(AZ::Crc32 id, QAction* action) override;
    void RemoveActionViaBus(QAction* action) override;
    void EnableDefaultActions() override;
    void DisableDefaultActions() override;
    void AttachOverride(QWidget* object) override;
    void DetachOverride() override;

    template<typename T>
    void RegisterUpdateCallback(int id, T* object, void (T::*method)(QAction*))
    {
        Q_ASSERT(m_actions.contains(id));
        m_updateCallbacks[id] = [action = m_actions.value(id), object, method] { AZStd::invoke(method, object, action); };
    }

    template<typename Fn>
    void RegisterUpdateCallback(int id, Fn&& fn)
    {
        Q_ASSERT(m_actions.contains(id));
        m_updateCallbacks[id] = [action = m_actions.value(id), fn] { fn(action); };
    }

    template<typename T>
    void RegisterActionHandler(int id, T method)
    {
        m_actionHandlers[id] = method;
    }
    template<typename T>
    void RegisterActionHandler(int id, T* object, void (T::* method)())
    {
        m_actionHandlers[id] = std::bind(method, object);
    }
    template<typename T>
    void RegisterActionHandler(int id, T* object, void (T::* method)(UINT))
    {
        m_actionHandlers[id] = std::bind(method, object, id);
    }

    bool eventFilter(QObject* watched, QEvent* event) override;

    // returns false if the action was already inserted, indicating that the action should not be processed again
    bool InsertActionExecuting(int id);

    // returns true if the action was inserted with InsertActionExecuting previously
    bool RemoveActionExecuting(int id);

Q_SIGNALS:
    void SendMetricsSignal(const char* viewPaneName, const char* openLocation);

public slots:
    void ActionTriggered(int id);

private slots:
    void UpdateMenu();
    void UpdateActions();

private:
    QHash<int, QAction*> m_actions;
    QVector<QMenu*> m_menus;
    QVector<QToolBar*> m_toolBars;
    QSignalMapper* m_actionMapper;
    QHash<int, std::function<void()> > m_updateCallbacks;
    QHash<int, std::function<void()> > m_actionHandlers;
    MainWindow* const m_mainWindow;

    QtViewPaneManager* m_qtViewPaneManager;
    ShortcutDispatcher* m_shortcutDispatcher = nullptr;

    // for sending shortcut metrics events
    bool m_isShortcutEvent = false;

    // for sending toolbar metrics events
    QSet<int> m_editorToolbarIds;

    // for sending main menu metrics events
    QSet<int> m_registeredViewPaneIds;

    // update the registered view pane Ids when the registered view pane list is modified
    void RebuildRegisteredViewPaneIds();

    // Guard against recursive actions being triggered by double clicks on menu items
    // while modal dialogs are in the process of popping up
    QSet<int> m_executingIds;

    // have all default actions (not including reserved actions) been suspended
    bool m_defaultActionsSuspended = false;
};

class DynamicMenu
    : public QObject
{
    Q_OBJECT

public:
    explicit DynamicMenu(QObject* parent = nullptr);
    void SetAction(QAction* action, ActionManager* am);
    void SetParentMenu(QMenu* menu, ActionManager* am);

protected:
    virtual void CreateMenu() = 0;
    virtual void OnMenuUpdate(int id, QAction* action) = 0;
    virtual void OnMenuChange(int id, QAction* action) = 0;

    void AddAction(int id, QAction* action);
    ActionManager::ActionWrapper AddAction(int id, const QString& name);
    ActionManager::ActionWrapper AddAction(AZ::Crc32 id, const QString& name);
    void AddSeparator();

    void UpdateAllActions();
    ActionManager* m_actionManager;

private slots:
    void ShowMenu();
    void TriggerAction(int id);

private:
    QAction* m_action;
    QMenu* m_menu;
    QSignalMapper* m_actionMapper;
    QHash<int, QAction*> m_actions;
};

#endif // ACTIONMANAGER_H
