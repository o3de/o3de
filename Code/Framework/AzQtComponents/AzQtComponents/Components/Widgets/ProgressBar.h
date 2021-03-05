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
