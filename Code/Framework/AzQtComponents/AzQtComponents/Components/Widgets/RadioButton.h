/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QStyle>

class QSettings;

namespace AzQtComponents
{
    class Style;

     //! Class to handle styling and painting of radio button controls.
    class AZ_QT_COMPONENTS_API RadioButton
    {
    public:
        //! Style configuration for the RadioButton class.
        struct Config
        {
        };

        //! Sets the RadioButton style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the RadioButton.
        static Config loadConfig(QSettings& settings);
        //! Gets the default RadioButton style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const Config& config);

        static bool drawRadioButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawRadioButtonLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };
} // namespace AzQtComponents
