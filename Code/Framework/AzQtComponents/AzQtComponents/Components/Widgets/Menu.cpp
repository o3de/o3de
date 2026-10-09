/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Menu.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QMenu>
#include <QSettings>
#include <QStyleOption>

#include <limits>

namespace AzQtComponents
{
    Menu::Config Menu::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<int>(settings, QStringLiteral("SubMenuOverlap"), config.subMenuOverlap);

        return config;
    }

    Menu::Config Menu::defaultConfig()
    {
        Config config;

        config.subMenuOverlap = 0;

        return config;
    }

    bool Menu::polish(Style* style, QWidget* widget, const Config& config)
    {
        Q_UNUSED(config);

        auto menu = qobject_cast<QMenu*>(widget);
        if (!menu)
        {
            return false;
        }

        if (StyleManager::stylesheetsDisabled())
        {
            // Menu.qss QMenu { background-color: #222222 }, ::item:disabled color #555555, and
            // the family the sheet gave every QMenu (Open Sans at the platform 9pt size).
            // TOKENIZE: MenuSurfaceColor, MenuDisabledTextColor, MenuFontFamily
            QPalette menuPalette = menu->palette();
            menuPalette.setColor(QPalette::Window, QColor(0x22, 0x22, 0x22));
            menuPalette.setColor(QPalette::Base, QColor(0x22, 0x22, 0x22));
            menuPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x55, 0x55, 0x55));
            menuPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x55, 0x55, 0x55));
            menu->setPalette(menuPalette);

            QFont menuFont = menu->font();
            menuFont.setFamily(QStringLiteral("Open Sans"));
            menu->setFont(menuFont);
        }

        style->repolishOnSettingsChange(widget);

        return true;
    }

    bool Menu::unpolish(Style* style, QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        return qobject_cast<QMenu*>(widget) != nullptr;
    }

    int Menu::subMenuOverlap(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        if (qobject_cast<const QMenu*>(widget))
        {
            return config.subMenuOverlap;
        }

        return std::numeric_limits<int>::lowest();
    }

} // namespace AzQtComponents
