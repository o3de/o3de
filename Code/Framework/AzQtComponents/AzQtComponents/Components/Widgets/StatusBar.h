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

    class AZ_QT_COMPONENTS_API StatusBar
    {
    public:
        static void initialize();

    private:
        friend class Style;

        static bool drawPanelStatusBar(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget);

        inline static QColor s_backgroundColor = QColor(QStringLiteral("#444444"));
        inline static QColor s_borderColor = QColor(QStringLiteral("#111111"));
        inline static qreal s_borderWidth = 2;
    };
} // namespace AzQtComponents
