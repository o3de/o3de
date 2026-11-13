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

//////////////////////////////////////////////////////////////////////////
//  PAINT COLOURS
//
//  The colour button's border, named so the legacy and the flattened GUI read one value. A named namespace, not an anonymous
//  one: a unity build merges the anonymous namespaces of a target's files.
//////////////////////////////////////////////////////////////////////////
namespace ColorButtonFlat
{
    // ColorButton.cpp:31 (the border of the colour button; the fill is the button's own colour). TOKENIZE: ColorButtonBorder
    const QColor Border(Qt::black);
} // namespace ColorButtonFlat


ColorButton::ColorButton(QWidget* parent /* = nullptr */)
    : QToolButton(parent)
{
    connect(this, &ColorButton::clicked, this, &ColorButton::OnClick);
}

void ColorButton::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_color);
    painter.setPen(ColorButtonFlat::Border);
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

