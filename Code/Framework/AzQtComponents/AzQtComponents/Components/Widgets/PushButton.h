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

class QPushButton;
class QRectF;
class QSettings;
class QStyleOptionComplex;
class QStyleOptionToolButton;
class QToolButton;

namespace AzQtComponents
{
    class Style;

    /**
     * Special class to handle styling and painting of PushButtons.
     * 
     * We have to use default QPushButton's throughout the engine, because QMessageBox's and other built-in
     * Qt things don't provide an easy hook to override the type used.
     * So, this class, full of static member functions, provides a place to collect all of the logic related
     * to painting/styling QPushButton's
     * 
     * The built-in Qt version of QPushButton painting is not very good. The edges of the frame aren't anti-aliased
     * and don't look quite right.
     * 
     * Unfortunately, Qt (as of 5.6) doesn't provide any hooks to query stylesheet values once loaded.
     * To avoid writing and maintaining our own css parsing and rule engine, we just override the painting
     * directly, and allow settings to be customized via an ini file.
     *
     * Some of the styling of QPushButton and QToolButton is done via CSS, in PushButton.qss
     * Everything else that can be configured is in PushButtonConfig.ini
     *
     * There are currently three built-in styles:
     *   Our default QPushButton style
     *   Our Primary QPushButton style, specified via pushButton->setDefault(true);
     *   Small Icon QToolButton style, specified via PushButton::applySmallIconStyle(toolButton);
     *
     */

    //! Class to handle styling and painting of QPushButton controls.
    class AZ_QT_COMPONENTS_API PushButton
    {
    public:
        //! Applies the Primary styling to a QPushButton.
        static void applyPrimaryStyle(QPushButton* button);

        //! Applies the Primary styling to a QToolButton.
        static void applySmallIconStyle(QToolButton* button);

        static void initialize();

    private:
        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget);

        static int buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget);

        static int menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget);

        static bool drawPushButtonBevel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget);

        static bool drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget);
        static bool drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget);

        static bool drawPushButtonFocusRect(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget);

        static QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option);

        // internal methods
        static void drawSmallIconButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, bool drawFrame = true);
        static void drawSmallIconFrame(const Style* style, const QStyleOption* option, const QRect& frame, QPainter* painter);
        static void drawSmallIconLabel(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget);
        static void drawSmallIconArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget);

        static void drawFilledFrame(
            QPainter* painter, const QRectF& rect, const QColor& gradientStartColor, const QColor& gradientEndColor, const QColor& borderColor, const int borderThickness, float radius);
        static void selectColors(
            const QStyleOption* option, bool isPrimary, bool isDisabled, QColor& gradientStartColor, QColor& gradientEndColor);

        inline static int s_buttonFrameHeight = 24;
        inline static int s_buttonFrameRadius = 4;
        inline static int s_buttonFrameMargin = 8;

        inline static QColor s_buttonSmallIconEnabledArrowColor = QColor("#FFFFFF");
        inline static QColor s_buttonSmallIconDisabledArrowColor = QColor("#BBBBBB");
        inline static int s_buttonSmallIconWidth = 24;
        inline static int s_buttonSmallIconArrowWidth = 10;

        inline static QColor s_buttonPrimaryColorDisabledStart = QColor("#2A4875");
        inline static QColor s_buttonPrimaryColorDisabledEnd = QColor("#2A4875");
        inline static QColor s_buttonPrimaryColorSunkenStart = QColor("#1E70EB");
        inline static QColor s_buttonPrimaryColorSunkenEnd = QColor("#1E70EB");
        inline static QColor s_buttonPrimaryColorHoveredStart = QColor("#4ABAFF");
        inline static QColor s_buttonPrimaryColorHoveredEnd = QColor("#94D2FF");
        inline static QColor s_buttonPrimaryColorNormalStart = QColor("#0095F2");
        inline static QColor s_buttonPrimaryColorNormalEnd = QColor("#1E70EB");

        inline static QColor s_buttonColorDisabledStart = QColor("#666666");
        inline static QColor s_buttonColorDisabledEnd = QColor("#666666");
        inline static QColor s_buttonColorSunkenStart = QColor("#444444");
        inline static QColor s_buttonColorSunkenEnd = QColor("#444444");
        inline static QColor s_buttonColorHoveredStart = QColor("#888888");
        inline static QColor s_buttonColorHoveredEnd = QColor("#777777");
        inline static QColor s_buttonColorNormalStart = QColor("#777777");
        inline static QColor s_buttonColorNormalEnd = QColor("#666666");

        inline static int s_buttonBorderThickness = 1;
        inline static QColor s_buttonBorderColor = QColor("#3D000000");
        inline static int s_buttonBorderDisabledThickness = 1;
        inline static QColor s_buttonBorderDisabledColor = QColor("#3D000000");
        inline static int s_buttonBorderFocusedThickness = 1;
        inline static QColor s_buttonBorderFocusedColor = QColor("#4B8CEF");

        inline static QColor s_buttonIconColor = QColor("#FFFFFF");
        inline static QColor s_buttonIconActiveColor = QColor("#FF0000");
        inline static QColor s_buttonIconDisabledColor = QColor("#999999");
        inline static QColor s_buttonIconSelectedColor = QColor("#FFFFFF");

        //! TODO - Not sure if this should even be configurable?
        inline static QString s_buttonDropdownIndicatorArrowDown = QStringLiteral(":/stylesheet/img/UI20/dropdown-button-arrow.svg");
        inline static int s_buttonDropdownMenuIndicatorWidth = 16;
        inline static int s_buttonDropdownMenuIndicatorPadding = 4;
    };
} // namespace AzQtComponents
