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

#include <QColor>

class QPainter;
class QSettings;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    class Style;

    class AZ_QT_COMPONENTS_API StatusBar
    {
    public:

        struct Config
        {
            QColor backgroundColor;
            QColor borderColor;
            qreal borderWidth;
        };

        /*!
        * Loads the status bar config data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default status bar config data.
        */
        static Config defaultConfig();

    private:
        friend class Style;

        static bool drawPanelStatusBar(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);
    };
} // namespace AzQtComponents
