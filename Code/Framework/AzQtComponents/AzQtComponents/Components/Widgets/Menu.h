/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QPixmap>

class QLineEdit;
class QPainter;
class QSettings;
class QStyleOption;

namespace AzQtComponents
{
    class Style;

    //! Class to handle styling and painting of QMenu controls.
    class AZ_QT_COMPONENTS_API Menu
    {
    public:
        //! Settings for margins around the menu.
        struct Margins
        {
            int top;
            int right;
            int bottom;
            int left;
        };

        //! Style configuration for the Menu class.
        struct Config
        {
            QPixmap shadowPixmap;       //!< Pixmap for the shadow shown around the Menu.
            Margins shadowMargins;      //!< Margins for drop shadow around the Menu.
            QColor backgroundColor;     //!< Background color for the Menu.
            int radius;                 //!< Corner radius for the Menu rect.
            int horizontalMargin;       //!< Margin for the Menu on the horizontal directions (left, right).
            int verticalMargin;         //!< Margin for the Menu on the vertical directions (top, bottom).
            int horizontalPadding;      //!< Padding for the Menu on the horizontal directions (left, right).
            int verticalPadding;        //!< Padding for the Menu on the vertical directions (top, bottom).
            int subMenuOverlap;         //!< Amount of pixels submenus overlap with their parent Menu.
        };

        //! Sets the Menu style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the Menu.
        static Config loadConfig(QSettings& settings);
        //! Gets the default Menu style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);

        static int horizontalMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int verticalMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int subMenuOverlap(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int verticalShadowMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int horizontalShadowMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static bool drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };

} // namespace AzQtComponents
