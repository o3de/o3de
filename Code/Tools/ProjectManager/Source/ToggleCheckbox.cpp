/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ToggleCheckbox.h>

#include <QPaintEvent>
#include <QPainter>

namespace O3DE::ProjectManager
{
    static QSize s_toggleButtonSize = QSize(32, 16);
    static int s_toggleButtonBorderRadius = 8;
    static int s_toggleButtonCircleRadius = 6;
    static QColor s_toggleButtonEnabledColor = "#1E70EB";
    static QColor s_toggleButtonDisabledColor = "#3C4D65";
    static QColor s_toggleButtonCircleColor = "#FFFFFF";
    static QColor s_toggleButtonDisabledCircleColor = "#AAAAAA";

    ToggleCheckbox::ToggleCheckbox(QWidget* parent)
        : QCheckBox(parent)
    {
        setMinimumSize(s_toggleButtonSize);
    }

    void ToggleCheckbox::paintEvent(QPaintEvent* e)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPoint circleCenter;

        QRect toggleRect(e->rect().topLeft(), s_toggleButtonSize);
        QColor primaryColor, secondaryColor;

        if (isEnabled())
        {
            primaryColor = s_toggleButtonEnabledColor;
            secondaryColor = s_toggleButtonCircleColor;
        }
        else
        {
            primaryColor = s_toggleButtonDisabledColor;
            secondaryColor = s_toggleButtonDisabledCircleColor;
        }

        if (isChecked())
        {
            painter.setBrush(primaryColor);
            painter.setPen(primaryColor);

            circleCenter = toggleRect.center() + QPoint(toggleRect.width() / 2 - s_toggleButtonBorderRadius + 1, 1);
        }
        else
        {
            painter.setPen(secondaryColor);

            circleCenter = toggleRect.center() + QPoint(-toggleRect.width() / 2 + s_toggleButtonBorderRadius + 1, 1);
        }

        // Rounded rect
        painter.drawRoundedRect(toggleRect, s_toggleButtonBorderRadius, s_toggleButtonBorderRadius);

        // Circle
        painter.setBrush(secondaryColor);
        painter.drawEllipse(circleCenter, s_toggleButtonCircleRadius, s_toggleButtonCircleRadius);
    }

    bool ToggleCheckbox::hitButton(const QPoint& pos) const
    {
        Q_UNUSED(pos);
        return true;
    }
} // namespace O3DE::ProjectManager
