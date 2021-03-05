/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "AzToolsFramework_precompiled.h"

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
