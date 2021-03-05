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

#include <AzQtComponents/Components/Widgets/StatusBar.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzCore/Casting/numeric_cast.h>

#include <QPainter>
#include <QSettings>
#include <QStatusBar>
#include <QStyleOption>

namespace AzQtComponents
{

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

    config.backgroundColor = QColor(QStringLiteral("#444444"));
    config.borderColor = QColor(QStringLiteral("#111111"));
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
