/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Utilities/ScreenUtilities.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QPainter>
#include <QDockWidget>
#include <QCursor>
#include <QScreen>
#include <QTimer>
#include <QWindow>

namespace AzQtComponents
{
    QRect GetTotalScreenGeometry()
    {
        QRect totalScreenRect;
        int numScreens = QApplication::screens().count();
        for (int i = 0; i < numScreens; ++i)
        {
            totalScreenRect = totalScreenRect.united(QApplication::screens().at(i)->geometry());
        }

        return totalScreenRect;
    }

    void EnsureGeometryWithinScreenTop(QRect& geometry)
    {
        QScreen* screen = Utilities::ScreenAtPoint(geometry.center());
        if (screen)
        {
            QRect screenRect = screen->geometry();
            if (!screenRect.isNull() && geometry.top() < screenRect.top())
            {
                geometry.moveTop(screenRect.top());
            }
        }
    }

    void EnsureWindowWithinScreenGeometry(QWidget* widget)
    {
        // manipulate the window, not the input widget. Might be the same thing
        widget = widget->window();

        // get the edges of the screens
        QRect screenGeometry = GetTotalScreenGeometry();

        // check if we cross over any edges
        QRect geometry = widget->geometry();

        const bool mustBeEntirelyInside = true;
        if (screenGeometry.contains(geometry, mustBeEntirelyInside))
        {
            return;
        }

        if ((geometry.x() + geometry.width()) > screenGeometry.right())
        {
            geometry.moveTo(screenGeometry.right() - geometry.width(), geometry.top());
        }

        if (geometry.x() < screenGeometry.left())
        {
            geometry.moveTo(screenGeometry.left(), geometry.top());
        }

        if ((geometry.y() + geometry.height()) > screenGeometry.bottom())
        {
            geometry.moveTo(geometry.x(), screenGeometry.bottom() - geometry.height());
        }

        if (geometry.y() < screenGeometry.top())
        {
            geometry.moveTo(geometry.x(), screenGeometry.top());
        }

        widget->setGeometry(geometry);
    }

    void SetClipRegionForDockingWidgets(QWidget* widget, QPainter& painter, QMainWindow* mainWindow)
    {
        QRegion clipRegion(widget->rect());

        for (QDockWidget* childDockWidget : mainWindow->findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly))
        {
            if (childDockWidget->isFloating() && childDockWidget->isVisible() && !childDockWidget->isMinimized())
            {
                QRect globalDockRect = childDockWidget->geometry();
                QRect localDockRect(widget->mapFromGlobal(globalDockRect.topLeft()), globalDockRect.size());

                clipRegion = clipRegion.subtracted(QRegion(localDockRect));
            }
        }

        painter.setClipRegion(clipRegion);
        painter.setClipping(true);
    }

    void SetCursorPos(const QPoint& point)
    {
        const QList<QScreen*> screens = QGuiApplication::screens();
        bool finished = false;
        for (int screenIndex = 0; !finished && screenIndex < screens.size(); ++screenIndex)
        {
            QScreen* screen = screens[screenIndex];
            if (screen->geometry().contains(point))
            {
                QCursor::setPos(screen, point);
                finished = true;
            }
        }
    }
    
    void SetCursorPos(int x, int y)
    {
        SetCursorPos(QPoint(x, y));
    }

    void bringWindowToTop(QWidget* widget)
    {
        auto window = widget->window();

        bool wasMaximized = window->isMaximized();

        // this will un-maximize a window that's previously been maximized...
        window->setWindowState(Qt::WindowActive);

        if (wasMaximized)
        {
            // re-maximize it now
            window->showMaximized();
        }
        else if (WindowDecorationWrapper* wrapper = qobject_cast<WindowDecorationWrapper*>(window))
        {
            // otherwise, restore this from any saved settings if we can
            wrapper->showFromSettings();
        }
        else
        {
            window->show();
        }

        window->raise();

        // activateWindow only works if the window is shown, so let's include
        // a slight delay to make sure the events for showing the window
        // have been processed.
        QTimer::singleShot(0, widget, [widget] {
            widget->activateWindow();
            if (QWindow* window = widget->window()->windowHandle())
            {
                window->requestActivate();
            }

            // The Windows OS has all kinds of issues with bringing a window to the front
            // from another application.
            // For the moment, we assume that any other application calling this
            // has called AllowSetForegroundWindow(), otherwise, the code below
            // won't work.
#ifdef Q_OS_WIN
            HWND hwnd = reinterpret_cast<HWND>(widget->winId());
            SetForegroundWindow(hwnd);

            // Hack to get this to show up for sure
            ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif // Q_OS_WIN
        });
    }
} // namespace AzQtComponents

