/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ClickableLabel.hxx"

namespace AzToolsFramework
{
    ClickableLabel::ClickableLabel(QWidget* parent)
        : QLabel(parent)
        , m_pressed(false)
    {
    }

    ClickableLabel::~ClickableLabel()
    {
    }

    void ClickableLabel::mousePressEvent(QMouseEvent* event)
    {
        QLabel::mousePressEvent(event);

        m_pressed = true;

        grabMouse();
    }

    void ClickableLabel::mouseReleaseEvent(QMouseEvent* event)
    {
        QLabel::mouseReleaseEvent(event);
        releaseMouse();

        AZ_TracePrintf("ClickableLabel", "UnderMouse: %i\n", underMouse());
        AZ_TracePrintf("ClickableLabel", "Pressed: %i\n", m_pressed);

        if (m_pressed && rect().contains(event->pos()))
        {
            emit clicked();
        }

        m_pressed = false;
    }

}

#include "UI/UICore/moc_ClickableLabel.cpp"
