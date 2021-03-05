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

#include <AzQtComponents/Components/Widgets/DragAndDrop.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QColor>
#include <QPainter>
#include <QSettings>
#include <QStyleOption>

namespace AzQtComponents
{
    DragAndDrop::Config DragAndDrop::loadConfig(QSettings& settings)
    {
        ConfigHelpers::GroupGuard dropIndicator(&settings, QStringLiteral("DropIndicator"));

        DragAndDrop::Config config = defaultConfig();

        ConfigHelpers::read<int>(settings, QStringLiteral("rectOutlineWidth"), config.dropIndicator.rectOutlineWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("lineSeparatorOutlineWidth"), config.dropIndicator.lineSeparatorOutlineWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("ballDiameter"), config.dropIndicator.ballDiameter);
        ConfigHelpers::read<int>(settings, QStringLiteral("ballOutlineWidth"), config.dropIndicator.ballOutlineWidth);

        ConfigHelpers::read<QColor>(settings, QStringLiteral("rectOutlineColor"), config.dropIndicator.rectOutlineColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("lineSeparatorColor"), config.dropIndicator.lineSeparatorColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("ballFillColor"), config.dropIndicator.ballFillColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("ballOutlineColor"), config.dropIndicator.ballOutlineColor);

        ConfigHelpers::read<int>(settings, QStringLiteral("rectBorderRadius"), config.dragIndicator.rectBorderRadius);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("rectFillColor"), config.dragIndicator.rectFillColor);

        return config;
    }

    DragAndDrop::Config DragAndDrop::defaultConfig()
    {
        Config config;

        config.dropIndicator.rectOutlineWidth = 1;
        config.dropIndicator.lineSeparatorOutlineWidth = 1;
        config.dropIndicator.ballDiameter = 5;
        config.dropIndicator.ballOutlineWidth = 1;

        config.dropIndicator.rectOutlineColor = QColor("#00A1C9");
        config.dropIndicator.lineSeparatorColor = QColor("#00A1C9");
        config.dropIndicator.ballFillColor = QColor("#444444");
        config.dropIndicator.ballOutlineColor = QColor("#00A1C9");

        config.dragIndicator.rectBorderRadius = 2;
        config.dragIndicator.rectFillColor = QColor("#888888");

        return config;
    }

    bool DragAndDrop::drawDragIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const DragAndDrop::Config& config)
    {
        Q_UNUSED(style)
        Q_UNUSED(widget)

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        painter->setPen(QPen(config.dragIndicator.rectFillColor, 1));
        painter->setBrush(QBrush(config.dragIndicator.rectFillColor));
        painter->drawRoundedRect(option->rect, config.dragIndicator.rectBorderRadius, config.dragIndicator.rectBorderRadius);

        painter->restore();
        return true;
    }

    bool DragAndDrop::drawDropIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const DragAndDrop::Config& config)
    {
        Q_UNUSED(style)
        Q_UNUSED(widget)

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // If true means we're moving the row, not dropping into another one
        const bool inBetweenRows = option->rect.height() == 0;

        if (inBetweenRows)
        {
            //draw circle followed by line for drops between items
            const int ballFillDiameter = config.dropIndicator.ballDiameter - 2*config.dropIndicator.ballOutlineWidth;
            const int paintedDiameter = config.dropIndicator.ballDiameter + config.dropIndicator.ballOutlineWidth;
            const int paintedRadius = paintedDiameter / 2;

            QPen pen(config.dropIndicator.ballOutlineColor);
            pen.setWidth(config.dropIndicator.ballOutlineWidth);
            painter->setPen(pen);
            painter->drawEllipse(option->rect.topLeft() + QPoint(paintedRadius, 0), ballFillDiameter, ballFillDiameter);
            painter->drawEllipse(option->rect.topRight() - QPoint(paintedRadius, 0), ballFillDiameter, ballFillDiameter);

            pen = QPen(config.dropIndicator.lineSeparatorColor);
            pen.setWidth(config.dropIndicator.lineSeparatorOutlineWidth);
            painter->setPen(pen);
            painter->drawLine(option->rect.topLeft() + QPoint(paintedDiameter, 0), option->rect.topRight() - QPoint(paintedDiameter, 0));
        }
        else
        {
            //draw a rounded box for drops on items
            QPen pen(config.dropIndicator.rectOutlineColor);
            pen.setWidth(config.dropIndicator.rectOutlineWidth);
            painter->setPen(pen);
            painter->drawRect(option->rect);
        }

        painter->restore();
        return true;
    }

}

