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

class QSettings;

namespace AzQtComponents
{
    class Style;

    //! Class to handle styling of ProgressBar controls.
    class AZ_QT_COMPONENTS_API ProgressBar
    {
    public:
        //! Style configuration for the ProgressBar class.
        struct Config
        {
            int height;     //!< Height of the ProgressBar control in pixels.
        };

        //! Sets the ProgressBar style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the ProgressBar.
        static Config loadConfig(QSettings& settings);
        //! Gets the default ProgressBar style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        // methods used by Style
        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const Config& config);
    };
} // namespace AzQtComponents
