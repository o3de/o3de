/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorCommon_precompiled.h"
#include "TimeSlider.h"

#include <QPainter>
#include <QPalette>

namespace DrawingPrimitives
{
    void DrawTimeSlider(QPainter& painter, const QPalette& palette, const STimeSliderOptions& options)
    {
        QString text = QString::number(options.m_time, 'f', options.m_precision + 1);

        QFontMetrics fm(painter.font());
        const int textWidth = fm.horizontalAdvance(text) + fm.height();
        const int markerHeight = fm.height();

        const int thumbX = options.m_position;
        const bool fits = thumbX + textWidth < options.m_rect.right();

        const QRect timeRect(fits ? thumbX : thumbX - textWidth, 3, textWidth, fm.height());
        painter.fillRect(timeRect.adjusted(fits ? 0 : -1, 0, fits ? 1 : 0, 0), options.m_bHasFocus ? palette.highlight() : palette.shadow());
        painter.setPen(palette.color(QPalette::HighlightedText));
        painter.drawText(timeRect.adjusted(fits ? 0 : aznumeric_cast<int>(markerHeight * 0.2f), -1, fits ? aznumeric_cast<int>(-markerHeight * 0.2f) : 0, 0), text, QTextOption(fits ? Qt::AlignRight : Qt::AlignLeft));

        painter.setPen(palette.color(QPalette::Text));
        painter.drawLine(QPointF(thumbX, 0), QPointF(thumbX, options.m_rect.height()));
        QPointF points[3] =
        {
            QPointF(thumbX,  markerHeight),
            QPointF(thumbX - markerHeight * 0.66f, 0),
            QPointF(thumbX + markerHeight * 0.66f, 0)
        };

        painter.setBrush(palette.base());
        painter.setPen(palette.color(QPalette::Text));
        painter.drawPolygon(points, 3);
    }
}
