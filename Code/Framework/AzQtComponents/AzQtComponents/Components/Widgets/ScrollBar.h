/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QPointer>
#include <QStyle>

class QSettings;
class QWidget;
class QAbstractScrollArea;
class QColor;

namespace AzQtComponents
{
    class ScrollBarWatcher;
    class Style;
    class AssetFolderThumbnailView;
    class TableView;

    //! Class to provide extra functionality for working with ScrollBar controls.
    class AZ_QT_COMPONENTS_API ScrollBar
    {
    public:
        //! Enum used to determine the display mode of the ScrollBar.
        enum class ScrollBarMode
        {
            AlwaysShow = 0,     //!< Always shows scrollbars when content overflows (default).
            ShowOnHover         //!< Only show scrollbars when the parent widget is being hovered.
        };

        //! Style configuration for the ScrollBar class.
        struct Config
        {
            ScrollBarMode defaultMode;  //!< ScrollBar display mode.
        };

        //! Applies the qss classes to the QAbstractScrollArea to display the scrollbars in their dark style.
        //! Used to make the scrollbars visible in widgets with light backgrounds.
        static void applyDarkStyle(QAbstractScrollArea* scrollArea);

        //! Resets the qss classes to the QAbstractScrollArea to display the scrollbars in their light style.
        //! This is the default styling, so it only needs to be called to revert to it after applyDarkStyle is used.
        static void applyLightStyle(QAbstractScrollArea* scrollArea);

        //! Sets the ScrollBar style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the ScrollBar.
        static Config loadConfig(QSettings& settings);

        //! Gets the default ScrollBar style configuration.
        static Config defaultConfig();

        //! Sets the ScrollBar display mode for the QAbstractScrollArea specified.
        static void setDisplayMode(QAbstractScrollArea* scrollArea, ScrollBarMode mode);

        //! Gives a QAbstractScrollArea (a console, a text output) a background colour that covers its viewport and the lanes of its
        //! scroll bars. A scroll area set its own with an inline style sheet, `setStyleSheet("background: <colour>")`, which the legacy
        //! GUI drew with the scroll bars of ScrollBar.qss. In the flattened GUI such a sheet keeps a style sheet style alive on the area
        //! and on every scroll bar inside it, which draws them as the squared bars of the base style. So: with style sheets, this sets
        //! that same sheet; flattened, it sets the colour as the Base and Window roles of the area's palette (the area fills its own
        //! background) and leaves the scroll bars to the style.
        static void setBackgroundColor(QAbstractScrollArea* scrollArea, const QColor& color);

    private:
        friend class Style;
        friend class AssetFolderThumbnailView;
        friend class TableView;
        friend class TreeView;

        static void initializeWatcher();
        static void uninitializeWatcher();

        //! Flattened GUI, from Style::polish after everything else sets a palette: gives a scroll area the background colour
        //! setBackgroundColor gave it. An inline sheet outranked every rule of the application sheet, so this does too.
        static void applyBackgroundColor(QWidget* widget);

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool unpolish(Style* style, QWidget* widget, const Config& config);
        static bool drawScrollBar(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config);

        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'AzQtComponents::ScrollBar::s_scrollBarWatcher': class 'QPointer<AzQtComponents::ScrollBarWatcher>' needs to have dll-interface to be used by clients of class 'AzQtComponents::ScrollBar'
            static QPointer<ScrollBarWatcher> s_scrollBarWatcher;
        AZ_POP_DISABLE_WARNING
        static unsigned int s_watcherReferenceCount;
    };

} // namespace AzQtComponents

Q_DECLARE_METATYPE(AzQtComponents::ScrollBar::ScrollBarMode)
