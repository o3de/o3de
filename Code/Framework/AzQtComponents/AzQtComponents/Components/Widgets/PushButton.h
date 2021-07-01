/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        struct Gradient
        {
            QColor start;           //!< Gradient start color (top).
            QColor end;             //!< Gradient end color (bottom).
        };

        //! Structure to specify multiple gradients for different widget states.
        struct ColorSet
        {
            Gradient disabled;
            Gradient sunken;
            Gradient hovered;
            Gradient normal;
        };

        struct Border
        {
            int thickness;
            QColor color;
        };

        struct Frame
        {
            int height;
            int radius;
            int margin;
        };

        struct SmallIcon
        {
            QColor enabledArrowColor;
            QColor disabledArrowColor;
            Frame frame;
            int width;
            int arrowWidth;
        };

        struct IconButton
        {
            QColor activeColor;
            QColor disabledColor;
            QColor selectedColor;
        };

        struct DropdownButton
        {
            QPixmap indicatorArrowDown;
            int menuIndicatorWidth;
            int menuIndicatorPadding;
        };

        //! Style configuration for the PushButton class.
        struct Config
        {
            ColorSet primary;               //!< Background colors for the primary PushButton style.
            ColorSet secondary;             //!< Background colors for the secondary PushButton style.
            Border defaultBorder;
            Border disabledBorder;
            Border focusedBorder;
            Frame defaultFrame; 
            SmallIcon smallIcon;            //!< Style configuration for small icon buttons.
            IconButton iconButton;          //!< Style configuration for icon buttons.
            DropdownButton dropdownButton;  //!< Style configuration for dropdown buttons.
        };

        //! Applies the Primary styling to a QPushButton.
        static void applyPrimaryStyle(QPushButton* button);

        //! Applies the Primary styling to a QToolButton.
        static void applySmallIconStyle(QToolButton* button);

        //! Sets the PushButton style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the PushButton.
        static Config loadConfig(QSettings& settings);
        //! Gets the default PushButton style configuration.
        static Config defaultConfig();

    private:
        friend class Style;

        // methods used by Style
        static bool polish(Style* style, QWidget* widget, const Config& config);

        static int buttonMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static QSize sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const Config& config);

        static int menuButtonIndicatorWidth(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static bool drawPushButtonBevel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool drawToolButton(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);
        static bool drawIndicatorArrowDown(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static bool drawPushButtonFocusRect(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option, const Config& config);

        // internal methods
        static void drawSmallIconButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config, bool drawFrame = true);
        static void drawSmallIconFrame(const Style* style, const QStyleOption* option, const QRect& frame, QPainter* painter, const Config& config);
        static void drawSmallIconLabel(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget, const Config& config);
        static void drawSmallIconArrow(const Style* style, const QStyleOptionToolButton* buttonOption, QStyle::State state, const QRect& buttonArea, QPainter* painter, const QWidget* widget, const Config& config);

        static void drawFilledFrame(QPainter* painter, const QRectF& rect, const QColor& gradientStartColor, const QColor& gradientEndColor, const Border& border, float radius);
        static void selectColors(const QStyleOption* option, const ColorSet& colorSet, bool isDisabled, QColor& gradientStartColor, QColor& gradientEndColor);
    };
} // namespace AzQtComponents
