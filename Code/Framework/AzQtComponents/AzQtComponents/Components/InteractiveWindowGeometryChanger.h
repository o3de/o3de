/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QObject>
#include <QPointer>
#include <QWindow>

class QKeyEvent;

namespace AzQtComponents
{
    /* This class is used to implement window resizing and moving through the keyboard.
    * This implement the "Size" and "Move" functionality that can be used by right clicking
    * the title bar.
    */
    class AZ_QT_COMPONENTS_API InteractiveWindowGeometryChanger
        : public QObject
    {
        Q_DISABLE_COPY(InteractiveWindowGeometryChanger)
    public:
        // The ctor changes the mouse cursor and installs a global event filter
        explicit InteractiveWindowGeometryChanger(QWindow* target, QObject* parent);

        // The dtor restores the mouse cursor and uninstalls the event filter
        ~InteractiveWindowGeometryChanger();
    protected:
        bool eventFilter(QObject* watched, QEvent* ev) override;
        virtual void handleKeyPress(QKeyEvent*) = 0;
        virtual void handleMouseMove(QMouseEvent*) = 0;
        void restoreCursorPosition();
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::InteractiveWindowGeometryChanger::m_targetWindow': class 'QPointer<QWindow>' needs to have dll-interface to be used by clients of class 'AzQtComponents::InteractiveWindowGeometryChanger'
        QPointer<QWindow> m_targetWindow;
        AZ_POP_DISABLE_WARNING
        QPoint m_originalCursorPos;
        bool m_restoreCursorAtExit = true;
    };

    // This implementation handles arrow key presses and resizes the window accordingly
    class AZ_QT_COMPONENTS_API InteractiveWindowResizer : public InteractiveWindowGeometryChanger
    {
    public:
        /*
        * Windows resizing works like this, right click title bar, choose "Size"
        * then the first arrow key you press will determine if you're resizing the left, top, bottom or right
        * the second arrow key press will effectively resize the window.
        */
        enum SideToResize
        {
            NoneSide = 0,
            LeftSide = 1,
            RightSide = 2,
            TopSide = 4,
            BottomSide = 8,
        };
        Q_DECLARE_FLAGS(SidesToResize, SideToResize)

        explicit InteractiveWindowResizer(QWindow* target, QObject* parent);

    protected:
        void handleKeyPress(QKeyEvent*) override;
        void handleMouseMove(QMouseEvent*) override;

    private:
        void updateCursor();
        SideToResize keyToSide(int key) const;
        bool sideIsVertical(SidesToResize) const;
        bool sideIsHorizontal(SidesToResize) const;
        bool sideIsCorner(SidesToResize) const;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::InteractiveWindowResizer::m_sideToResize': class 'QFlags<AzQtComponents::InteractiveWindowResizer::SideToResize>' needs to have dll-interface to be used by clients of class 'AzQtComponents::InteractiveWindowResizer'
        SidesToResize m_sideToResize = NoneSide;
        AZ_POP_DISABLE_WARNING
    };

    // This implementation handles arrow key presses and moves the window accordingly
    class AZ_QT_COMPONENTS_API InteractiveWindowMover : public InteractiveWindowGeometryChanger
    {
    public:
        explicit InteractiveWindowMover(QWindow* target, QObject* parent);
    protected:
        void handleKeyPress(QKeyEvent*) override;
        void handleMouseMove(QMouseEvent*) override;
        bool m_arrowAlreadyPressed = false;
    };
} // namespace AzQtComponents
