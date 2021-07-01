/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
