/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxModelingWindow.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>

namespace WhiteBox
{
    WhiteBoxModelingWindow::WhiteBoxModelingWindow(const QString& title, QWidget* parent)
        : QDialog(parent, Qt::Tool | Qt::FramelessWindowHint)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_WindowPropagation); // Tool windows inherit the editor's palette and font too.
        setWindowTitle(title);
        setModal(false);
        setMinimumWidth(300);
        // Inherit the application/parent palette, fonts and widget styles, including theme changes.
        setAutoFillBackground(true);
    }

    void WhiteBoxModelingWindow::ShowNearCursor()
    {
        if (!isVisible())
        {
            adjustSize();
            QPoint position = QCursor::pos() + QPoint(12, 18);
            if (auto* screen = QGuiApplication::screenAt(position))
            {
                const QRect available = screen->availableGeometry();
                position.setX(qBound(available.left(), position.x(),
                    qMax(available.left(), available.right() - width())));
                position.setY(qBound(available.top(), position.y(),
                    qMax(available.top(), available.bottom() - height())));
            }
            move(position);
        }
        show();
        raise();
        activateWindow();
    }

    void WhiteBoxModelingWindow::Dismiss()
    {
        hide();
        deleteLater();
    }

    void WhiteBoxModelingWindow::SetDragHandle(QWidget* widget)
    {
        m_dragHandle = widget;
        widget->setCursor(Qt::SizeAllCursor);
    }

    void WhiteBoxModelingWindow::mousePressEvent(QMouseEvent* event)
    {
        // Use the actual title geometry so theme font sizes never make controls draggable.
        if (event->button() == Qt::LeftButton && m_dragHandle &&
            m_dragHandle->rect().contains(m_dragHandle->mapFromGlobal(event->globalPos())))
        {
            m_dragging = true;
            m_dragOffset = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
        QDialog::mousePressEvent(event);
    }

    void WhiteBoxModelingWindow::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_dragging)
        {
            move(event->globalPos() - m_dragOffset);
            event->accept();
            return;
        }
        QDialog::mouseMoveEvent(event);
    }

    void WhiteBoxModelingWindow::mouseReleaseEvent(QMouseEvent* event)
    {
        m_dragging = false;
        QDialog::mouseReleaseEvent(event);
    }
}
