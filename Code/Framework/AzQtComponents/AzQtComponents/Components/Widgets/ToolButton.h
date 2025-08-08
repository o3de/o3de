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
        static void initialize();

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget);

        static int buttonIconSize(const Style* style, const QStyleOption* option, const QWidget* widget);
        static int buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget);
        static int menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& contentSize, const QWidget* widget);

        static QRect subControlRect(const Style* style, const QStyleOptionComplex* option, QStyle::SubControl subControl, const QWidget* widget);

        static bool drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget);
        static bool drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget);

        //! The default size of button icons in pixels.
        inline static int s_buttonIconSize = 16;
        //! Margin around ToolButton controls, in pixels. All directions get the same margin.
        inline static int s_defaultButtonMargin = 1;
        //! Width of the menu indicator arrow in pixels.
        inline static int s_menuIndicatorWidth = 10;
        //! Background color for checkable ToolButtons set to the checked state.
        inline static QColor s_checkedStateBackgroundColor = QStringLiteral("#1E70EB");
        //! Path to the indicator icon. Svg images recommended.
        inline static QString s_menuIndicatorIcon = QStringLiteral(":/stylesheet/img/UI20/menu-indicator.svg");
        //! Size of the indicator icon. Size must be proportional to the icon's ratio.
        inline static QSize s_menuIndicatorIconSize = QSize(6, 3);
    };
} // namespace AzQtComponents
