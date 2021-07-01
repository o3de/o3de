/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ShortcutDispatcher.h"

#include <assert.h>


// Qt
#include <QScopedValueRollback>
#include <QAction>
#include <QDockWidget>
#include <QDebug>
#include <QMainWindow>
#include <QMenuBar>

// AzQtComponents
#include <AzQtComponents/Buses/ShortcutDispatch.h>

// Editor
#include "ActionManager.h"
#include "QtViewPaneManager.h"

const char* FOCUSED_VIEW_PANE_EVENT_NAME = "FocusedViewPaneEvent";    //Sent when view panes are focused
const char* FOCUSED_VIEW_PANE_ATTRIBUTE_NAME = "FocusedViewPaneName"; //Name of the current focused view pane

// Build sets _DEBUG when we're in debug mode; define SHOW_ACTION_INFO_IN_DEBUGGER so that when stepping through the debugger,
// we can see what the actions are and what their keyboard shortcuts are
#define SHOW_ACTION_INFO_IN_DEBUGGER _DEBUG

static QPointer<QWidget> s_lastFocus;

#if AZ_TRAIT_OS_PLATFORM_APPLE
class MacNativeShortcutFilter : public QObject
{
    /* mac's native toolbar doesn't generate shortcut events, it calls the action directly.
     * It doesn't even honour shortcut contexts.
     *
     * To remedy this, we catch the QMetaCallEvent that triggers the menu item activation
     * and suppress it if it was triggered via key combination, and send a QShortcutEvent.
     *
     * The tricky part is to find out if the menu item was triggered via mouse or shortcut.
     * If the previous event was a ShortcutOverride then it means key press.
     */

public:
    explicit MacNativeShortcutFilter(QObject* parent)
        : QObject(parent)
        , m_lastShortcutOverride(QEvent::KeyPress, 0, Qt::NoModifier) // dummy init
    {
        qApp->installEventFilter(this);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {

        switch (event->type())
        {
            case QEvent::ShortcutOverride:
            {
                auto ke = static_cast<QKeyEvent*>(event);
                m_lastEventWasShortcutOverride = true;
                m_lastShortcutOverride = QKeyEvent(*ke);
                break;
            }
            case QEvent::MetaCall:
            {
                if (m_lastEventWasShortcutOverride)
                {
                    m_lastEventWasShortcutOverride = false;
                    const QMetaObject* mo = watched->metaObject();
                    const bool isMenuItem = mo && qstrcmp(mo->className(), "QPlatformMenuItem") == 0;
                    if (isMenuItem)
                    {
                        QWidget* focusWidget = ShortcutDispatcher::focusWidget();
                        if (focusWidget)
                        {
                            QShortcutEvent se(QKeySequence(m_lastShortcutOverride.key() + m_lastShortcutOverride.modifiers()), /*ambiguous=*/false);
                            se.setAccepted(false);
                            QApplication::sendEvent(focusWidget, &se);
                            return se.isAccepted();
                        }
                    }
                }

                break;
            }
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
                m_lastEventWasShortcutOverride = false;
                break;
            default:
                break;
        }

        return false;
    }

    bool m_lastEventWasShortcutOverride = false;
    QKeyEvent m_lastShortcutOverride;
};
#endif

// Returns either a top-level or a dock widget (regardless of floating)
// This way when docking a main window Qt::WindowShortcut still works
QWidget* ShortcutDispatcher::FindParentScopeRoot(QWidget* widget)
{
    QWidget* w = widget;
    // if the current scope root is a QDockWidget, we want to bubble out
    // so we move to the parent immediately
    if (qobject_cast<QDockWidget*>(w) != nullptr || qobject_cast<QMainWindow*>(w) != nullptr)
    {
        w = w->parentWidget();
    }

    QWidget* newScopeRoot = w;
    while (newScopeRoot && newScopeRoot->parent() && !qobject_cast<QDockWidget*>(newScopeRoot) && !qobject_cast<QMainWindow*>(newScopeRoot))
    {
        newScopeRoot = newScopeRoot->parentWidget();
    }

    // This method should always return a parent scope root; if it can't find one
    // it returns null
    if (newScopeRoot == widget)
    {
        newScopeRoot = nullptr;

        if (w != nullptr)
        {
            // we couldn't find a valid parent; broadcast a message to see if something else wants to tell us about one
            AzQtComponents::ShortcutDispatchBus::EventResult(newScopeRoot, w, &AzQtComponents::ShortcutDispatchBus::Events::GetShortcutDispatchScopeRoot, w);
        }
    }

    return newScopeRoot;
}

// Returns true if a widget is ancestor of another widget
bool ShortcutDispatcher::IsAContainerForB(QWidget* a, QWidget* b)
{
    if (!a || !b)
    {
        return false;
    }

    while (b && (a != b))
    {
        b = b->parentWidget();
    }

    return (a == b);
}

// Returns the list of QActions which have this specific key shortcut
// Only QActions under scopeRoot are considered.
QList<QAction*> ShortcutDispatcher::FindCandidateActions(QObject* scopeRoot, const QKeySequence& sequence, QSet<QObject*>& previouslyVisited, bool checkVisibility)
{
    QList<QAction*> actions;
    if (!scopeRoot)
    {
        return actions;
    }

    if (previouslyVisited.contains(scopeRoot))
    {
        return actions;
    }
    previouslyVisited.insert(scopeRoot);

    QWidget* scopeRootWidget = qobject_cast<QWidget*>(scopeRoot);
    if (scopeRootWidget && ((checkVisibility && !scopeRootWidget->isVisible()) || !scopeRootWidget->isEnabled()))
    {
        return actions;
    }

#ifdef SHOW_ACTION_INFO_IN_DEBUGGER
    QString matchingAgainst = sequence.toString();
    (void) matchingAgainst; // avoid an unused variable warning; want this for debugging
#endif

    // Don't just call scopeRoot->actions()! It doesn't always return the proper list, especially with the dock widgets
    for (QAction* action : scopeRoot->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly))   // Width first
    {
#ifdef SHOW_ACTION_INFO_IN_DEBUGGER
        QString actionName = action->text();
        (void)actionName; // avoid an unused variable warning; want this for debugging
        QString shortcut = action->shortcut().toString();
        (void)shortcut; // avoid an unused variable warning; want this for debugging
#endif

        if (action->shortcut() == sequence)
        {
            actions << action;
        }
    }

    // also have to check the actions on the object directly, without looking at children
    // specifically for the base Editor MainWindow
    if (scopeRootWidget)
    {
        for (QAction* action : scopeRootWidget->actions())
        {
#ifdef SHOW_ACTION_INFO_IN_DEBUGGER
            QString actionName = action->text();
            (void)actionName; // avoid an unused variable warning; want this for debugging
            QString shortcut = action->shortcut().toString();
            (void)shortcut; // avoid an unused variable warning; want this for debugging
#endif

            if (action->shortcut() == sequence)
            {
                actions << action;
            }
        }
    }

    // Menubars have child widgets that have actions
    // But menu bar child widgets (menu items) are only visible when they've been clicked on
    // so we don't want to test visibility for child widgets of menubars
    if (qobject_cast<QMenuBar*>(scopeRoot))
    {
        checkVisibility = false;
    }

    // check the dock's central widget and the main window's
    // In some cases, they aren't in the scopeRoot's children, despite having the scopeRoot as their parent
    QDockWidget* dockWidget = qobject_cast<QDockWidget*>(scopeRoot);
    if (dockWidget)
    {
        actions << FindCandidateActions(dockWidget->widget(), sequence, previouslyVisited, checkVisibility);
    }

    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(scopeRoot);
    if (mainWindow)
    {
        actions << FindCandidateActions(mainWindow->centralWidget(), sequence, previouslyVisited, checkVisibility);
    }

    for (QWidget* child : scopeRoot->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
    {
        bool isMenu = (qobject_cast<QMenu*>(child) != nullptr);

        if ((child->windowFlags() & Qt::Window) && !isMenu)
        {
            // When going down the hierarchy stop at window boundaries, to not accidentally trigger
            // shortcuts from unfocused windows. Windows might be parented to this scope for purposes
            // "centering within parent" or lifetime.
            // Don't stop at menus though, as they are flagged as Qt::Window but are often the only thing that
            // actions are attached to.
            continue;
        }

        bool isDockWidget = (qobject_cast<QDockWidget*>(child) != nullptr);
        if ((isDockWidget && !actions.isEmpty()) || IsShortcutSearchBreak(child))
        {
            // If we already found a candidate, don't go into dock widgets, they have lower priority
            // since they are not focused.
            // Also never go into viewpanes; viewpanes are their own separate shortcut context and they never take shortcuts from the main window
            continue;
        }

        actions << FindCandidateActions(child, sequence, previouslyVisited, checkVisibility);
    }

    return actions;
}

bool ShortcutDispatcher::FindCandidateActionAndFire(QObject* focusWidget, QShortcutEvent* shortcutEvent, QList<QAction*>& candidates, QSet<QObject*>& previouslyVisited)
{
    candidates = FindCandidateActions(focusWidget, shortcutEvent->key(), previouslyVisited);
    QSet<QAction*> candidateSet = QSet<QAction*>(candidates.begin(), candidates.end());
    QAction* chosenAction = nullptr;
    int numCandidates = candidateSet.size();
    if (numCandidates == 1)
    {
        chosenAction = candidates.first();
    }
    else if (numCandidates > 1)
    {
        // If there are multiple candidates, choose the one that is parented to the ActionManager
        // since there are cases where panes with their own menu shortcuts that are docked
        // in the main window can be found in the same parent scope
        for (QAction* action : candidateSet)
        {
            if (qobject_cast<ActionManager*>(action->parent()))
            {
                chosenAction = action;
                break;
            }
        }
    }
    if (chosenAction)
    {
        if (chosenAction->isEnabled())
        {
            // has to be send, not post, or the dispatcher will get the event again and won't know that it was the one that queued it
            bool isAmbiguous = false;
            QShortcutEvent newEvent(shortcutEvent->key(), isAmbiguous);
            QApplication::sendEvent(chosenAction, &newEvent);
        }
        shortcutEvent->accept();
        return true;
    }

    return false;
}

ShortcutDispatcher::ShortcutDispatcher(QObject* parent)
    : QObject(parent)
    , m_currentlyHandlingShortcut(false)
{
    qApp->installEventFilter(this);
#if AZ_TRAIT_OS_PLATFORM_APPLE
    new MacNativeShortcutFilter(this);
#endif
}

ShortcutDispatcher::~ShortcutDispatcher()
{
}

bool ShortcutDispatcher::eventFilter(QObject* obj, QEvent* ev)
{
    switch (ev->type())
    {
    case QEvent::ShortcutOverride:
        // QActions default "autoRepeat" to true, which is not an ideal user experience
        // We globally disable that behavior here - in the unlikely event a shortcut needs to
        // replicate it, its owner can instead implement a keyEvent handler
        if (static_cast<QKeyEvent*>(ev)->isAutoRepeat())
        {
            ev->accept();
            return true;
        }
        break;

    case QEvent::Shortcut:
        return shortcutFilter(obj, static_cast<QShortcutEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        if (!s_lastFocus || !IsAContainerForB(qobject_cast<QWidget*>(obj), s_lastFocus))
        {
            setNewFocus(obj);
        }
        break;

    case QEvent::FocusIn:
        setNewFocus(obj);
        break;

        // we don't really care about focus out, because something should always have the focus
        // but I'm leaving this here, so that it's clear that this is intentional
        //case QEvent::FocusOut:
        //    break;
    }

    return false;
}

QWidget* ShortcutDispatcher::focusWidget()
{
    QWidget* focusWidget = s_lastFocus; // check the widget we tracked last
    if (!focusWidget)
    {
        // we don't have anything, so fall back to using the focus object
        focusWidget = qobject_cast<QWidget*>(qApp->focusObject()); // QApplication::focusWidget() doesn't always work
    }

    return focusWidget;
}

bool ShortcutDispatcher::shortcutFilter(QObject* obj, QShortcutEvent* shortcutEvent)
{
    if (m_currentlyHandlingShortcut)
    {
        return false;
    }

    QScopedValueRollback<bool> recursiveCheck(m_currentlyHandlingShortcut, true);

    // prioritize m_actionOverrideObject if active
    if (m_actionOverrideObject != nullptr)
    {
        QList<QAction*> childActions =
            m_actionOverrideObject->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly);

        // attempt to find shortcut in override
        const auto childActionIt = AZStd::find_if(
            childActions.begin(), childActions.end(), [shortcutEvent](QAction* child)
        {
            return child->shortcut() == shortcutEvent->key();
        });

        // trigger shortcut
        if (childActionIt != childActions.end())
        {
            // has to be send, not post, or the dispatcher will get the event again
            // and won't know that it was the one that queued it
            const bool isAmbiguous = false;
            QShortcutEvent newEvent(shortcutEvent->key(), isAmbiguous);

            QAction* action = *childActionIt;
            QApplication::sendEvent(action, &newEvent);

            shortcutEvent->accept();
            return true;
        }
    }

    QWidget* currentFocusWidget = focusWidget(); // check the widget we tracked last
    if (!currentFocusWidget)
    {
        qWarning() << Q_FUNC_INFO << "No focus widget"; // Defensive. Doesn't happen.
        return false;
    }

    // Shortcut is ambiguous, lets resolve ambiguity and give preference to QActions in the most inner scope

    // Try below the focusWidget first:
    QSet<QObject*> previouslyVisited;
    QList<QAction*> candidates;
    if (FindCandidateActionAndFire(currentFocusWidget, shortcutEvent, candidates, previouslyVisited))    {
        return true;
    }

    // Now incrementally try bigger scopes. This handles complex cases several levels docking nesting

    QWidget* correctedTopLevel = nullptr;
    QWidget* p = currentFocusWidget;
    correctedTopLevel = FindParentScopeRoot(p);
    while (correctedTopLevel)
    {
        if (FindCandidateActionAndFire(correctedTopLevel, shortcutEvent, candidates, previouslyVisited))
        {
            return true;
        }

        p = correctedTopLevel;
        correctedTopLevel = FindParentScopeRoot(p);
    }


    // Nothing else to do... shortcut is really ambiguous, or there's no actions, something for the developer to fix.
    // Here's some debug info :

    if (candidates.isEmpty())
    {
        qWarning() << Q_FUNC_INFO << "No candidate QActions found";
    }
    else
    {
        qWarning() << Q_FUNC_INFO << "Ambiguous shortcut:" << shortcutEvent->key() << "; focusWidget="
            << qApp->focusWidget() << "Candidates=" << candidates << "; obj = " << obj
            << "Focused top-level=" << currentFocusWidget;
        for (auto ambiguousAction : candidates)
        {
            qWarning() << "action=" << ambiguousAction << "; action->parentWidget=" << ambiguousAction->parentWidget()
                << "; associatedWidgets=" << ambiguousAction->associatedWidgets()
                << "; shortcut=" << ambiguousAction->shortcut();
        }
    }

    return false;
}

void ShortcutDispatcher::setNewFocus(QObject* obj)
{
    // Unless every widget has strong focus, mouse clicks don't change the current focus widget
    // which is a little unintuitive, compared to how we expect focus to work, right now.
    // So instead of putting strong focus on everything, we detect focus change and mouse clicks

    QWidget* widget = qobject_cast<QWidget*>(obj);

    // we only watch widgets
    if (widget == nullptr)
    {
        return;
    }

    // track it for later
    s_lastFocus = widget;
}

bool ShortcutDispatcher::IsShortcutSearchBreak(QWidget* widget)
{
    return widget->property(AzQtComponents::SHORTCUT_DISPATCHER_CONTEXT_BREAK_PROPERTY).toBool();
}

void ShortcutDispatcher::AttachOverride(QWidget* object)
{
    m_actionOverrideObject = object;
}

void ShortcutDispatcher::DetachOverride()
{
    m_actionOverrideObject = nullptr;
}

#include <moc_ShortcutDispatcher.cpp>
