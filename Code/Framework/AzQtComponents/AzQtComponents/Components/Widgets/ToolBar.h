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

class QSettings;
class QStyleOption;
class QToolBar;
class QToolButton;
class QWidget;

namespace AzQtComponents
{
    class Style;

     //! Class to provide extra functionality for working with ToolBar controls.
    class AZ_QT_COMPONENTS_API ToolBar
    {
    public:
        //! Style configuration for a specific ToolBar group.
        struct ToolBarConfig
        {
            int itemSpacing;            //!< Spacing value for the ToolBar items, in pixels. All directions get the same spacing.
            QColor backgroundColor;     //!< Background color for this ToolBar group.
            qreal borderWidth;          //!< ToolBar border width, in pixels.
            QColor borderColor;         //!< Border color for this ToolBar group.
        };

        //! Style configuration for the ToolBar class.
        struct Config
        {
            ToolBarConfig primary;      //!< Style configuration for primary toolbars
            ToolBarConfig secondary;    //!< Style configuration for secondary toolbars
            int defaultSpacing;         //!< Spacing value for default ToolBar items, in pixels. All directions get the same spacing.
        };

        //! Enum used to specify the ToolBar icon size. Icon sizing is also affected by the toolbar group (primary/secondary).
        enum class ToolBarIconSize
        {
            IconNormal,
            IconLarge,
            Default = IconNormal
        };

        //! Sets the ToolBar style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the ToolBar.
        static Config loadConfig(QSettings& settings);
        //! Gets the default ToolBar style configuration.
        static Config defaultConfig();

        //! Applies the primary styling to a ToolBar.
        static void addMainToolBarStyle(QToolBar* toolbar);

        //! Sets the icon size on the toolbar to the size provided.
        static void setToolBarIconSize(QToolBar* toolbar, ToolBarIconSize size);

        //! Returns a pointer to the toolbar's expander button.
        //! Used to override the default toolbar overflow behavior.
        static QToolButton* getToolBarExpansionButton(QToolBar* toolBar);

    private:
        friend class Style;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        static int itemSpacing(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static constexpr const int g_ui10DefaultIconSize = 20;
        static constexpr const int g_ui10LargeIconSize = 32;

        static constexpr const char* g_ui10IconSizeProperty = "Ui10IconSize";
    };

} // namespace AzQtComponents
