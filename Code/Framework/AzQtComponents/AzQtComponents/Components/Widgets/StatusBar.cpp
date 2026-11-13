/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/StatusBar.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzCore/Casting/numeric_cast.h>

#include <QPainter>
#include <QSettings>
#include <QStatusBar>
#include <QStyleOption>

namespace AzQtComponents
{

//! Everything private to this file that the flattened GUI adds. A named namespace, not an anonymous one: a unity build joins this file
//! with others into one translation unit, where anonymous namespaces merge.
namespace StatusBarFlat
{
    // ------------------------------------------------------------------------------
    // The Config defaults of StatusBar::defaultConfig: StatusBarConfig.ini holds no line, so these are the live values of both rungs
    // ------------------------------------------------------------------------------
    constexpr QColor PanelFill(0x44, 0x44, 0x44);   // StatusBar.cpp:36. TOKENIZE: StatusBarPanelFill
    constexpr QColor TopLine(0x11, 0x11, 0x11);     // StatusBar.cpp:37. TOKENIZE: StatusBarTopLine
} // namespace StatusBarFlat

StatusBar::Config StatusBar::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ConfigHelpers::read<QColor>(settings, QStringLiteral("BackgroundColor"), config.backgroundColor);
    ConfigHelpers::read<QColor>(settings, QStringLiteral("BorderColor"), config.borderColor);
    ConfigHelpers::read<qreal>(settings, QStringLiteral("BorderWidth"), config.borderWidth);

    return config;
}

StatusBar::Config StatusBar::defaultConfig()
{
    Config config;

    config.backgroundColor = StatusBarFlat::PanelFill;
    config.borderColor = StatusBarFlat::TopLine;
    config.borderWidth = 2;

    return config;
}

bool StatusBar::drawPanelStatusBar(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
{
    Q_UNUSED(style);

    const auto statusBar = qobject_cast<const QStatusBar*>(widget);
    if (!statusBar)
    {
        return false;
    }

    painter->save();

    painter->setPen(Qt::NoPen);
    painter->setBrush(config.backgroundColor);

    painter->drawRect(option->rect);

    painter->setPen({config.borderColor, config.borderWidth});
    painter->setBrush(Qt::NoBrush);

    auto line = QLine(option->rect.topLeft(), option->rect.topRight());
    const int offset = aznumeric_cast<int>(config.borderWidth * 0.5);
    line.translate(0, offset);
    painter->drawLine(line);

    painter->restore();

    return true;
}

} // namespace AzQtComponents
