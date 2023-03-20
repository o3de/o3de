/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Internal/RectangleWidget.h>

#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

namespace AzQtComponents::Internal
{
    RectangleWidget::RectangleWidget(QWidget* parent)
        : QWidget(parent)
    {
    }

    void RectangleWidget::setColor(const QColor& color)
    {
        if (color != m_color)
        {
            m_color = color;
            update();
        }
    }

    QColor RectangleWidget::color() const
    {
        return m_color;
    }

    void RectangleWidget::paintEvent(QPaintEvent* event)
    {
        QPainter p(this);
        p.fillRect(event->rect(), m_color);

        QWidget::paintEvent(event);
    }
} // namespace AzQtComponents::Internal
