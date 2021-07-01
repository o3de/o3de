/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SHORTCUT_DISPATCHER_H
#define SHORTCUT_DISPATCHER_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#endif

template<typename T> class QSet;
template<typename T> class QList;
template<typename T> class QPointer;

class QWidget;
class QShortcutEvent;
class QKeyEvent;

/* This class provides a workaround against Qt's buggy implementation of shortcut contexts when using dock widgets.

   There are several problems with Qt's implementation:

   - Qt::WindowShortcut doesn't work well with floating docking widgets.
     Qt gives a warning about ambiguous shortcut even though both actions are in different windows.
   - A Qt::WindowShortcut in a docked widget will conflict when the widget is embedded (not floating).
   - Qt::WidgetWithChildrenShortcut doesn't work well on menus since they can't have focus
     Implement our own conflict resolution and shortcut dispatch.
     Deliver to the action in the most inner scope, for our purposes a scope is either a window or a dockwidget (regardless of floating)

     Beware that shortcut handling is complex and try not to change this class too much, as it's hard to test
     and hard to verify all edge cases.
*/


/*
  More documentation on Qt shortcuts
  -------------------------------------------

  Here's some more detailed info regarding shortcuts in Qt. Not specific Open 3D Engine but
  useful as not explained in Qt docs much.

  P.S.: The following text details the strategy used in an earlier Open 3D Engine version. Not sure which
        shortcut context type it uses nowadays, but eitherway, the following text is educational,
        and all the traps still exist in current Qt (5.11).

     Some applications have a QMainWindow and also secondary main windows which can dock into the main QMainWindow.
     All these main windows have menu bars containing actions with shortcuts.

     So, which shortcut context should be used ?

     - Qt::ApplicationShortcut
       Obviously not, would create conflicting shortcuts and you only want local shortcuts anyway.

     - Qt::WindowShortcut ?
       This is supposed to only work if the shortcut's parent is in the focused window. However,
       there a bug with floating dock widgets: doing a key sequence in the dock widget
       triggers the main QMainWindow's shortcut.

     - Qt::WidgetShortcut
       docs say: "The shortcut is active when its parent widget has focus"
       The QAction's parent is a QMenuBar, which doesn't get focus, so this is useless.

     - Qt::WidgetWithChildrenShortcut ?
       docs say: "The shortcut is active when its parent widget, or any of its children has focus"
       The QAction's parent is a QMenuBar, which doesn't get focus or has any focused children. Useless.

     - Qt::WidgetWithChildrenShortcut (Round2!)
       Actually it's not the QAction's parent that counts but the associated widget! (Misleading docs)
       So if you also add the action to the window, it works:
       QAction *action = menu->addAction("Del");
       myWindow->addAction(action); // success!

       Are we happy ?
       Not yet.

       Qt::WidgetWithChildrenShortcut is working pretty well, but as soon as you dock your secondary QMainWindow into your main QMainWindow we get:
       "QAction::eventFilter: Ambiguous shortcut overload: Ctrl+O". Some widget inside main window 2 is focused, but it's also a child of main window 1 further up the hierarchy, so both QActions would apply.

       So now what we need is simply a global event filter, catch QEvent::Shortcut, check if shortcut->isAmbiguous(), and if yes dispatch the shortcut
       manually (sendEvent), otherwise Qt would just bail out. To which QAction you send it to is up to you. I chose to imagine each dock widget was a scope and dispatch the shortcut to the most
       inner scope that contains the focused widget.

       If you've read this far you can now press 'Ctrl+Q' and hope it closes your editor ;)
 */

class ShortcutDispatcher
    : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutDispatcher(QObject* parent = nullptr);
    ~ShortcutDispatcher();
    bool eventFilter(QObject* obj, QEvent* ev) override;

    static QWidget* focusWidget();

    /// Assign the widget responsible for getting first attempt
    /// at every shortcut routed through the ShortcutDispatcher.
    void AttachOverride(QWidget* object);
    /// Detach the widget responsible for intercepting Actions
    /// routed through the ShortcutDispatcher.
    void DetachOverride();

private:
    bool shortcutFilter(QObject* obj, QShortcutEvent* ev);
    void setNewFocus(QObject* obj);

    QWidget* FindParentScopeRoot(QWidget* w);
    bool IsAContainerForB(QWidget* a, QWidget* b);
    QList<QAction*> FindCandidateActions(QObject* scopeRoot, const QKeySequence& sequence, QSet<QObject*>& previouslyVisited, bool checkVisibility = true);
    bool FindCandidateActionAndFire(QObject* focusWidget, QShortcutEvent* shortcutEvent, QList<QAction*>& candidates, QSet<QObject*>& previouslyVisited);

    static bool IsShortcutSearchBreak(QWidget* widget);

    bool m_currentlyHandlingShortcut;

    QString m_previousWidgetName;

    QWidget* m_actionOverrideObject = nullptr; /**< (if set/not null) The widget responsible for getting first attempt
                                                 *   at every shortcut routed through the ShortcutDispatcher. */
};

#endif
