/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FixedStateButton.h"

#include <QStyleOptionButton>
#include <QStylePainter>

FixedStateButton::FixedStateButton(QWidget* parent)
    : QPushButton(parent)
{

}

void FixedStateButton::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QStylePainter painter(this);
    QStyleOptionButton option;
    initStyleOption(&option);
    option.state = m_state;
    painter.drawControl(QStyle::CE_PushButton, option);
}

#include <Gallery/moc_FixedStateButton.cpp>
