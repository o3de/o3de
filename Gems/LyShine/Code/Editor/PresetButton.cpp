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
#include "UiCanvasEditor_precompiled.h"

#include "PresetButton.h"

PresetButton::PresetButton(const QString& defaultIconPath,
    const QString& hoverIconPath,
    const QString& selectedIconPath,
    const QSize& fixedButtonAndIconSize,
    const QString& text,
    OnClicked clicked,
    QWidget* parent)
    : QPushButton(QIcon(defaultIconPath), text, parent)
    , m_isHovering(false)
    , m_defaultIcon(defaultIconPath)
    , m_hoverIcon(hoverIconPath)
    , m_selectedIconPath(selectedIconPath)
{
    setAttribute(Qt::WA_Hover, true);
    setCheckable(true);
    setFlat(true);
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(fixedButtonAndIconSize);
    setIconSize(fixedButtonAndIconSize);

    QObject::connect(this, &QPushButton::clicked, this, clicked);

    QObject::connect(this,
        &QAbstractButton::toggled,
        this,
        &PresetButton::UpdateIcon);
}

void PresetButton::enterEvent(QEvent* ev)
{
    m_isHovering = true;

    UpdateIcon(isChecked());

    QPushButton::enterEvent(ev);
}

void PresetButton::leaveEvent(QEvent* ev)
{
    m_isHovering = false;

    UpdateIcon(isChecked());

    QPushButton::leaveEvent(ev);
}

void PresetButton::UpdateIcon(bool isChecked)
{
    if (isChecked)
    {
        setIcon(m_selectedIconPath);
    }
    else
    {
        if (m_isHovering && isEnabled())
        {
            setIcon(m_hoverIcon);
        }
        else
        {
            setIcon(m_defaultIcon);
        }
    }
}

#include <moc_PresetButton.cpp>
