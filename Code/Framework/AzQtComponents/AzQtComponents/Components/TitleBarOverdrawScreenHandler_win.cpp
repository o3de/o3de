/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/TitleBarOverdrawScreenHandler_win.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler_win.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <QWindow>
#include <QScreen>
#include <QDockWidget>

namespace AzQtComponents
{

TitleBarOverdrawScreenHandler::TitleBarOverdrawScreenHandler(QWindow* window, QObject* parent)
    : QObject(parent)
    , m_dockWidget(nullptr)
    , m_window(window)
{
    registerWindow(window);
}

TitleBarOverdrawScreenHandler::TitleBarOverdrawScreenHandler(QDockWidget* dockWidget, QObject* parent)
    : QObject(parent)
    , m_dockWidget(dockWidget)
    , m_window(nullptr)
{
    m_dockWidget->installEventFilter(this);
}

void TitleBarOverdrawScreenHandler::registerWindow(QWindow* window)
{
    if (!window)
    {
        return;
    }

    m_window = window;
    connect(m_window, &QObject::destroyed, this, [this] {
        m_window = nullptr;
    });

    connect(window, &QWindow::screenChanged, this, &TitleBarOverdrawScreenHandler::applyOverdrawMargins);
    window->installEventFilter(this);
}

bool TitleBarOverdrawScreenHandler::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::WindowStateChange: // Handles minimize/maximize
        {
            // Don't reapply margins on window maximization for dockwidgets,
            // otherwise the native titlebar would become visible
            if (m_dockWidget)
            {
                return QObject::eventFilter(watched, event);
            }
        }
        // Intentional fall-through
        case QEvent::Show:
        {
            if (qobject_cast<QWindow*>(watched))
            {
                applyOverdrawMargins();
            }
            // Floating dockwidgets' own window becomes available only show.
            // Listen for this event on floating widgets and then proceed as usual with titlebar overdraw
            else if (QDockWidget* dockWidget = qobject_cast<QDockWidget*>(watched))
            {
                if (dockWidget != m_dockWidget)
                {
                    break;
                }

                if (!dockWidget->isFloating())
                {
                    break;
                }

                if (m_window)
                {
                    // Window is already registered. This happens when a minimized window is restored from the
                    // taskbar and a Show event is sent by QWidget before the WindowStateChange event is sent
                    break;
                }

                if (QWindow* dockWindow = dockWidget->windowHandle())
                {
                    registerWindow(dockWindow);
                    applyOverdrawMargins();
                }
            }
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void TitleBarOverdrawScreenHandler::applyOverdrawMargins()
{
    if (!m_window)
    {
        return;
    }

    if (!m_window->handle())
    {
        return;
    }

    if (TitleBarOverdrawHandlerWindows* tbhandle = qobject_cast<TitleBarOverdrawHandlerWindows*>(parent()))
    {
        QPlatformWindow* platformWindow = m_window->handle();
        auto hWnd = (HWND)m_window->winId();
        WINDOWPLACEMENT placement;
        placement.length = sizeof(WINDOWPLACEMENT);
        const bool maximized = GetWindowPlacement(hWnd, &placement) && placement.showCmd == SW_SHOWMAXIMIZED;
        tbhandle->applyOverdrawMargins(platformWindow, hWnd, maximized);
    }
}

} // namespace AzQtComponents
