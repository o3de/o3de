/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/InteractiveWindowGeometryChanger.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

namespace AzQtComponents
{

    enum
    {
        ChangeIncrement = 10 // How many pixels are added each time we move or resize
    };


    InteractiveWindowGeometryChanger::InteractiveWindowGeometryChanger(QWindow* target, QObject *parent)
        : QObject(parent)
        , m_targetWindow(target)
        , m_originalCursorPos(target ? target->mapFromGlobal(QCursor::pos()) : QPoint())
    {
        if (m_targetWindow)
        {
            qApp->setOverrideCursor(Qt::SizeAllCursor);
            qApp->installEventFilter(this);
            m_targetWindow->setMouseGrabEnabled(true);

            // Dismiss the changer if someone does intrusive stuff programatically, like maximizing the window
            connect(target, &QWindow::visibilityChanged, this, &QObject::deleteLater);
            connect(target, &QWindow::windowStateChanged, this, &QObject::deleteLater);
        }
        else
        {
            // this doesn't happen
            qWarning() << Q_FUNC_INFO << "target window is null";
            deleteLater();
            Q_ASSERT(false);
        }
    }

    InteractiveWindowGeometryChanger::~InteractiveWindowGeometryChanger()
    {
        m_targetWindow->setMouseGrabEnabled(false);
        qApp->restoreOverrideCursor();
        // Restore original cursor position
        restoreCursorPosition();
    }

    void InteractiveWindowGeometryChanger::restoreCursorPosition()
    {
        if (m_targetWindow && m_restoreCursorAtExit)
        {
            AzQtComponents::SetCursorPos(m_targetWindow->mapToGlobal(m_originalCursorPos));
        }
    }

    bool InteractiveWindowGeometryChanger::eventFilter(QObject* watched, QEvent* ev)
    {
        if (ev->type() == QEvent::KeyPress)
        {
            if (!m_targetWindow)
            {
                // The window was deleted by external factors, dismiss our changer
                deleteLater();
                return true;
            }

            auto keyPressEv = static_cast<QKeyEvent*>(ev);
            switch (keyPressEv->key())
            {
                case Qt::Key_Left:
                case Qt::Key_Up:
                case Qt::Key_Right:
                case Qt::Key_Down:
                    handleKeyPress(keyPressEv);
                    return true;

                case Qt::Key_Escape:
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    deleteLater();
                    return true; // consume it

                default:
                    return QObject::eventFilter(watched, ev);
            }
        }
        else if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::MouseButtonRelease ||
            ev->type() == QEvent::MouseButtonDblClick)
        {
            // Any mouse click dismisses the geometry changer
            deleteLater();

            // We don't want to restore the cursor pos, the user just clicked somewhere, would be odd to move it
            m_restoreCursorAtExit = false;
        }
        else if (ev->type() == QEvent::MouseMove)
        {
            handleMouseMove(static_cast<QMouseEvent*>(ev));
        }

        return QObject::eventFilter(watched, ev);
    }

    InteractiveWindowResizer::InteractiveWindowResizer(QWindow* target, QObject* parent)
        : InteractiveWindowGeometryChanger(target, parent)
    {
    }

    InteractiveWindowResizer::SideToResize InteractiveWindowResizer::keyToSide(int key) const
    {
        switch (key)
        {
            case Qt::Key_Left:
                return LeftSide;
            case Qt::Key_Up:
                return TopSide;
            case Qt::Key_Right:
                return RightSide;
            case Qt::Key_Down:
                return BottomSide;
            default:
                return NoneSide;
        }
    }

    bool InteractiveWindowResizer::sideIsVertical(SidesToResize side) const
    {
        return side & (TopSide | BottomSide);
    }

    bool InteractiveWindowResizer::sideIsHorizontal(SidesToResize side) const
    {
        return side & (LeftSide | RightSide);
    }

    bool InteractiveWindowResizer::sideIsCorner(SidesToResize side) const
    {
        return sideIsHorizontal(side) && sideIsVertical(side);
    }

    void InteractiveWindowResizer::handleKeyPress(QKeyEvent* ev)
    {
        if (m_sideToResize == NoneSide)
        {
            // First arrow press just determines which side we're going to resize
            m_sideToResize = SidesToResize(keyToSide(ev->key()));
            updateCursor();
            return;
        }
        else if (!sideIsCorner(m_sideToResize))
        {
            // When resizing left or right and the user presses up or down then we start resizing
            // a corner instead
            if (sideIsHorizontal(m_sideToResize) && sideIsVertical(keyToSide(ev->key())))
            {
                // We're reisizing left or right, but user pressed up or down, so we go to corner instead
                m_sideToResize |= keyToSide(ev->key());
                updateCursor();
                return;
            }
            else if (sideIsVertical(m_sideToResize) && sideIsHorizontal(keyToSide(ev->key())))
            {
                // We're resizing top or bottom, but user pressed left or right, so we go to corner instead
                m_sideToResize |= keyToSide(ev->key());
                updateCursor();
                return;
            }
        }

        // Now we do the actual resizing:
        QRect geometry = m_targetWindow->geometry();
        // const QRect originalGeometry = geometry;

        int dx1 = 0, dx2 = 0, dy1 = 0, dy2 = 0;

        bool horizontal = false;
        int signedness = 0;

        switch (ev->key())
        {
            case Qt::Key_Left:
                horizontal = true;
                signedness = -1;
                break;
            case Qt::Key_Right:
                horizontal = true;
                signedness = 1;
                break;
            case Qt::Key_Down:
                horizontal = false;
                signedness = 1;
                break;
            case Qt::Key_Up:
                horizontal = false;
                signedness = -1;
                break;
            default:
                break;
        }

        if (horizontal)
        {
            if (m_sideToResize & LeftSide)
            {
                dx1 = ChangeIncrement * signedness;
            }
            else if (m_sideToResize & RightSide)
            {
                dx2 = ChangeIncrement * signedness;
            }
        }
        else
        {
            if (m_sideToResize & TopSide)
            {
                dy1 = ChangeIncrement * signedness;
            }
            else if (m_sideToResize & BottomSide)
            {
                dy2 = ChangeIncrement * signedness;
            }
        }

        geometry.adjust(dx1, dy1, dx2, dy2);

        if (geometry.height() >= m_targetWindow->minimumHeight() &&
            geometry.width() >= m_targetWindow->minimumWidth() &&
            geometry.height() <= m_targetWindow->maximumHeight() &&
            geometry.width() <= m_targetWindow->maximumWidth())
        {
            EnsureGeometryWithinScreenTop(geometry);
            m_targetWindow->setGeometry(geometry);
            updateCursor(); // Position changed, move cursor to border again
        }
    }

    void InteractiveWindowResizer::handleMouseMove(QMouseEvent* ev)
    {
        if (m_sideToResize == NoneSide)
        {
            // First arrow press just determines which side we're going to resize, so nothing will happen here
            return;
        }

        // Now we do the actual resizing:
        QRect geometry = m_targetWindow->geometry();
        if (m_sideToResize & LeftSide)
        {
            geometry.setLeft(ev->globalX() - 1);
        }
        if (m_sideToResize & RightSide)
        {
            geometry.setRight(ev->globalX());
        }
        if (m_sideToResize & TopSide)
        {
            geometry.setTop(ev->globalY() - 1);
        }
        if (m_sideToResize & BottomSide)
        {
            geometry.setBottom(ev->globalY());
        }

        if (geometry.height() >= m_targetWindow->minimumHeight() &&
            geometry.width() >= m_targetWindow->minimumWidth() &&
            geometry.height() <= m_targetWindow->maximumHeight() &&
            geometry.width() <= m_targetWindow->maximumWidth())
        {
            EnsureGeometryWithinScreenTop(geometry);
            m_targetWindow->setGeometry(geometry);
            updateCursor(); // Position changed, move cursor to border again
        }
     }

    void InteractiveWindowResizer::updateCursor()
    {
        // When pressing "Size" in the context menu, the first arrow key press will change the cursor
        // shape and position it in one of the edges, or corner

        const auto s = m_sideToResize; // Less verbose

        QPoint newPos = m_targetWindow->position();
        const int x = newPos.x();
        const int y = newPos.y();
        const int width = m_targetWindow->width();
        const int height = m_targetWindow->height();

        // Restore our previous cursor override so there's only one override stacked
        qApp->restoreOverrideCursor();

        // The magic +1/-1 bellow is because of QTBUG-58590, shape isn't set otherwise for modal windows

        if (((s & LeftSide) && (s & TopSide)) ||
            ((s & BottomSide) && (s & RightSide))) // Corner
        {
            qApp->setOverrideCursor(Qt::SizeFDiagCursor);
            newPos.setX((s & LeftSide) ? (x + 1) : (x + width - 1));
            newPos.setY((s & TopSide) ? (y + 1) : (y + height - 1));
        }
        else if (((s & LeftSide) && (s & BottomSide)) ||
            ((s & RightSide) && (s & TopSide))) // Corner
        {
            qApp->setOverrideCursor(Qt::SizeBDiagCursor);
            newPos.setX((s & LeftSide) ? (x + 1) : (x + width - 1));
            newPos.setY((s & TopSide) ? (y + 1) : (y + height - 1));
        }
        else if (s & (LeftSide | RightSide))
        {
            qApp->setOverrideCursor(Qt::SizeHorCursor);
            newPos.setY(y + height / 2);
            newPos.setX((s & LeftSide) ? (x + 1) : (x + width - 1));
        }
        else if (s & (TopSide | BottomSide))
        {
            qApp->setOverrideCursor(Qt::SizeVerCursor);
            newPos.setX(x + width / 2);
            newPos.setY((s & TopSide) ? (y + 1) : (y + height - 1));
        }

        AzQtComponents::SetCursorPos(newPos);
    }

    InteractiveWindowMover::InteractiveWindowMover(QWindow* target, QObject* parent)
        : InteractiveWindowGeometryChanger(target, parent)
    {
    }

    void InteractiveWindowMover::handleKeyPress(QKeyEvent* ev)
    {
        m_arrowAlreadyPressed = true;
        QPoint offset(0, 0);

        switch (ev->key())
        {
            case Qt::Key_Left:
                offset += QPoint(-ChangeIncrement, 0);
                break;
            case Qt::Key_Up:
                offset += QPoint(0, -ChangeIncrement);
                break;
            case Qt::Key_Right:
                offset += QPoint(ChangeIncrement, 0);
                break;
            case Qt::Key_Down:
                offset += QPoint(0, ChangeIncrement);
                break;
            default:
                Q_ASSERT(false);
                return;
        }

        QRect geometry = m_targetWindow->geometry().translated(offset);
        EnsureGeometryWithinScreenTop(geometry);
        m_targetWindow->setGeometry(geometry);

        // Mouse cursor travels too while we use the arrow keys
        restoreCursorPosition();
    }

    void InteractiveWindowMover::handleMouseMove(QMouseEvent*)
    {
        if (!m_arrowAlreadyPressed)
        {
            // Mouse only moves the window if one arrow has been pressed. That's how Windows does it.
            return;
        }

        const QPoint newPos = m_targetWindow->mapFromGlobal(QCursor::pos());
        const QPoint offset = newPos - m_originalCursorPos;

        QRect geometry = m_targetWindow->geometry().translated(offset);
        EnsureGeometryWithinScreenTop(geometry);
        m_targetWindow->setGeometry(geometry);
    }

} // namespace AzQtComponents
