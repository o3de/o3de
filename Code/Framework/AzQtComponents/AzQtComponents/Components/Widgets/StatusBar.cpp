/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/StatusBar.h>

#include <AzCore/Casting/numeric_cast.h>

#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/StyleManagerInterface.h>

#include <QPainter>
#include <QSettings>
#include <QStatusBar>
#include <QStyleOption>

namespace AzQtComponents
{

void StatusBar::initialize()
{
    auto styleManagerInterface = AZ::Interface<StyleManagerInterface>::Get();
    AZ_Assert(styleManagerInterface, "ToolButton - could not get StyleManagerInterface on ToolButton initialization.");

    if (styleManagerInterface->IsStylePropertyDefined("BackgroundColor"))
    {
        s_backgroundColor = styleManagerInterface->GetStylePropertyAsColor("BackgroundColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("SeparatorColor"))
    {
        s_borderColor = styleManagerInterface->GetStylePropertyAsInteger("SeparatorColor");
    }
    if (styleManagerInterface->IsStylePropertyDefined("SeparatorThickness"))
    {
        s_borderWidth = styleManagerInterface->GetStylePropertyAsInteger("SeparatorThickness");
    }
}

bool StatusBar::drawPanelStatusBar(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget)
{
    Q_UNUSED(style);

    const auto statusBar = qobject_cast<const QStatusBar*>(widget);
    if (!statusBar)
    {
        return false;
    }

    painter->save();

    painter->setPen(Qt::NoPen);
    painter->setBrush(s_backgroundColor);

    painter->drawRect(option->rect);

    painter->setPen({ s_borderColor, s_borderWidth });
    painter->setBrush(Qt::NoBrush);

    auto line = QLine(option->rect.topLeft(), option->rect.topRight());
    const int offset = aznumeric_cast<int>(s_borderWidth * 0.5);
    line.translate(0, offset);
    painter->drawLine(line);

    painter->restore();

    return true;
}

} // namespace AzQtComponents
