/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QProxyStyle>

class QAbstractItemView;
class QCheckBox;
class QSettings;

namespace AzQtComponents
{
    class Style;

    //! Class to handle styling and painting of Checkboxes, including ToggleSwitches.
    //! Instantiate a regular QCheckbox, then call this class to change style as needed.
    //! There are currently two built-in styles:
    //! - Plain checkboxes
    //! - Toggle Switches
    class AZ_QT_COMPONENTS_API CheckBox
    {
    public:
        //! Style configuration for the CheckBox class.
        struct Config
        {
        };

        //! Applies the "ToggleSwitch" style class to a QCheckBox.
        static void applyToggleSwitchStyle(QCheckBox* checkBox);
        //! Applies the "Expander" style class to a QCheckBox.
        static void applyExpanderStyle(QCheckBox* checkBox);
        //! Applies the "Visibility" style class to checkable items in an item view.
        static void setVisibilityMode(QAbstractItemView* view, bool enabled);

        //! Sets the CheckBox style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the CheckBox.
        static Config loadConfig(QSettings& settings);
        //! Gets the default CheckBox style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const Config& config);

        static bool drawCheckBox(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawCheckBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };
} // namespace AzQtComponents
