/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QProxyStyle>

class QSettings;
class QSize;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    class Style;

    //! Class to handle styling and painting of QToolButton controls.
    class AZ_QT_COMPONENTS_API ToolButton
    {
    public:
        //! Style configuration for the ToolButton class.
        struct Config
        {
            int buttonIconSize;                     //!< The default size of button icons in pixels.
            int defaultButtonMargin;                //!< Margin around ToolButton controls, in pixels. All directions get the same margin.
            int menuIndicatorWidth;                 //!< Width of the menu indicator arrow in pixels.
            QColor checkedStateBackgroundColor;     //!< Background color for checkable ToolButtons set to the checked state.
            QString menuIndicatorIcon;              //!< Path to the indicator icon. Svg images recommended.
            QSize menuIndicatorIconSize;            //!< Size of the indicator icon. Size must be proportional to the icon's ratio.
        };

        //! Sets the ToolButton style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the ToolButton.
        static Config loadConfig(QSettings& settings);
        //! Gets the default ToolButton style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        static int buttonIconSize(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentSize, const QWidget* widget, const Config& config);

        static QRect subControlRect(const Style* style, const QStyleOptionComplex* option, QStyle::SubControl subControl, const QWidget* widget, const Config& config);

        static bool drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

    };
} // namespace AzQtComponents
