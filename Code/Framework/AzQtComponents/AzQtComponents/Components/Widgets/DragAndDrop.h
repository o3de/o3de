/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QColor>

class QPainter;
class QSettings;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    class Style;

    /**
    * Special class to handle styling and painting of Drag and Drop operations.
    * You can use DragAndDrop.ini to configure colors and line widths.
    */

    class AZ_QT_COMPONENTS_API DragAndDrop
    {
    public:
        struct DropIndicator
        {
            int rectOutlineWidth;
            int lineSeparatorOutlineWidth;
            int ballDiameter;
            int ballOutlineWidth;
            QColor rectOutlineColor;
            QColor lineSeparatorColor;
            QColor ballFillColor;
            QColor ballOutlineColor;
        };

        struct DragIndicator
        {
            int rectBorderRadius;
            QColor rectFillColor;
        };

        struct Config {
            DropIndicator dropIndicator;
            DragIndicator dragIndicator;
        };

        //! Loads the button config data from a settings object.
        static DragAndDrop::Config loadConfig(QSettings& settings);

        //! Returns default config data.
        static DragAndDrop::Config defaultConfig();

        //! Draw the drag indicator.
        static bool drawDragIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const DragAndDrop::Config& config);

        //! Draw the drop indicator.
        static bool drawDropIndicator(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const DragAndDrop::Config& config);
    };
}

