/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

#include <iostream>

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

bool ShortcutDispatcher::FindCandidateActionAndFire(QShortcutEvent* shortcutEvent)
{

    for (unsigned i = 0; i < m_all_actions.size(); i++)
    {
        if (shortcutEvent->key() == m_all_actions[i].second->shortcut())
        {
            //This method is simpler and we don't need any recursion. There are not many shortcuts so a vector is not that problematic with performance.
            QObject* p_registered = nullptr;
            p_registered = m_all_actions[i].second->parent();

            QObject* p_focused = nullptr;
            p_focused = qApp->focusObject();

            if (m_all_actions[i].second->isEnabled())
            {
                //Checking if the parent is on focus so that we can use it.
                if (p_registered == p_focused)
                {
                    bool isAmbiguous = false;

                    QShortcutEvent newEvent(shortcutEvent->key(), isAmbiguous);
                    if (QApplication::sendEvent(m_all_actions[i].second, &newEvent))
                    {
                        shortcutEvent->accept();
                    }

                    return true;
                }
                else
                    continue;
            }
        }
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

bool ShortcutDispatcher::shortcutFilter([[maybe_unused]] QObject* obj, QShortcutEvent* shortcutEvent)
{
    if (FindCandidateActionAndFire(shortcutEvent))
    {
        return true;
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

void ShortcutDispatcher::AddNewAction(QAction* newAction, AZ::Crc32 reverseUrl)
{
    const int new_id = newAction->data().toInt();

    unsigned size = m_all_actions.size();
    for (unsigned i = 0; i < size; i++)
    {
        if (m_all_actions[i].second->data().toInt() == new_id || (m_all_actions[i].first == reverseUrl && m_all_actions[i].first != AZ::Crc32(0)))
        {
            qWarning() << "ActionManager already contains action with id" << new_id;
            Q_ASSERT(false);
        }
    }

    AZStd::pair<AZ::Crc32, QAction*> new_addition(reverseUrl, newAction);
    
    m_all_actions.push_back(new_addition);
}

#include <moc_ShortcutDispatcher.cpp>
