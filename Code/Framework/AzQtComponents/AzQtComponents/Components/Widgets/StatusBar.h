/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
