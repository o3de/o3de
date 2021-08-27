/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>
#include <platform.h>

#include "ColorButton.h"

// Qt
#include <QPainter>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/ColorPicker.h>  // for AzQtComponents::ColorPicker
#include <AzQtComponents/Utilities/Conversions.h>           // for AzQtComponents::toQColor


ColorButton::ColorButton(QWidget* parent /* = nullptr */)
    : QToolButton(parent)
{
    connect(this, &ColorButton::clicked, this, &ColorButton::OnClick);
}

void ColorButton::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_color);
    painter.setPen(Qt::black);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

#if AZ_TRAIT_OS_PLATFORM_APPLE
void macRaiseWindowDelayed(QWidget* window);
#endif

void ColorButton::OnClick()
{
    const QColor color = AzQtComponents::toQColor(
        AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGB,
        AzQtComponents::fromQColor(m_color),
        tr("Select Color")));

#if AZ_TRAIT_OS_PLATFORM_APPLE
    macRaiseWindowDelayed(window());
#endif

    if (color != m_color)
    
    {
        m_color = color;
        update();
        emit ColorChanged(m_color);
    }
}

#include <QtUI/moc_ColorButton.cpp>
