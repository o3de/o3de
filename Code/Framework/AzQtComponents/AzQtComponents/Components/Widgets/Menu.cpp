/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Menu.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
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
            // The menu's element, resolved here once and kept on the menu (draws and metrics read it back). Its colours are
            // also what the menu's own palette carries, for the widgets a QWidgetAction hosts in it (Menu.qss:12 QMenu
            // { background-color }, :25 ::item:disabled { color }). The font is the QMenu class font (TextElement::installClassFonts).
            const MenuElement& element = MenuElement::resolve(menu);
            QPalette menuPalette = menu->palette();
            menuPalette.setColor(QPalette::Window, element.m_surface);
            menuPalette.setColor(QPalette::Base, element.m_surface);
            menuPalette.setColor(QPalette::Disabled, QPalette::Text, element.m_disabledText);
            menuPalette.setColor(QPalette::Disabled, QPalette::WindowText, element.m_disabledText);
            menu->setPalette(menuPalette);
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
