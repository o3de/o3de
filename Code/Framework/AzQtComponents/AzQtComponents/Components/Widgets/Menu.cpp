/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Menu.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QStyleOption>
#include <QWindow>
#include <QSurfaceFormat>
#include <QTimer>
#include <QWindow>

#include <qdrawutil.h>

#include <limits>

namespace AzQtComponents
{
    static void ReadMargins(QSettings& settings, const QString& name, Menu::Margins& margins)
    {
        ConfigHelpers::GroupGuard guard(&settings, name);
        ConfigHelpers::read<int>(settings, QStringLiteral("Top"), margins.top);
        ConfigHelpers::read<int>(settings, QStringLiteral("Right"), margins.right);
        ConfigHelpers::read<int>(settings, QStringLiteral("Bottom"), margins.bottom);
        ConfigHelpers::read<int>(settings, QStringLiteral("Left"), margins.left);
    }

    Menu::Config Menu::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<QPixmap>(settings, QStringLiteral("PixmapPath"), config.shadowPixmap);
        ReadMargins(settings, QStringLiteral("PixmapMargins"), config.shadowMargins);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("BackgroundColor"), config.backgroundColor);
        ConfigHelpers::read<int>(settings, QStringLiteral("Radius"), config.radius);
        ConfigHelpers::read<int>(settings, QStringLiteral("HorizontalMargin"), config.horizontalMargin);
        ConfigHelpers::read<int>(settings, QStringLiteral("VerticalMargin"), config.verticalMargin);
        ConfigHelpers::read<int>(settings, QStringLiteral("HorizontalPadding"), config.horizontalPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("VerticalPadding"), config.verticalPadding);
        ConfigHelpers::read<int>(settings, QStringLiteral("SubMenuOverlap"), config.subMenuOverlap);

        return config;
    }

    Menu::Config Menu::defaultConfig()
    {
        Config config;

        config.shadowPixmap = QPixmap(QStringLiteral(":/stylesheet/img/UI20/shadows/menu-shadow.png"));
        // 5px margin + 2px radius = 7px
        config.shadowMargins.top = 7;
        config.shadowMargins.right = 7;
        config.shadowMargins.bottom = 7;
        config.shadowMargins.left = 7;
        config.backgroundColor = QStringLiteral("#222222");
        config.radius = 2;
        config.horizontalMargin = 5;
        config.verticalMargin = 5;
        config.horizontalPadding = 0;
        config.verticalPadding = 4;
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

        style->repolishOnSettingsChange(widget);

        // QMenu::popup results in the QWindow being created before polish is called. If the QWindow
        // is created with the wrong flags, window tranparency does not work. Check if the window
        // surface format supports an alpha. If not, destroy it.
        if (QWindow* window = menu->windowHandle())
        {
            if (!window->format().hasAlpha())
            {
                window->destroy();
            }
        }

        // Set the widget attributes we need to enable window transparency. Do this before calling
        // QWidget::setWindowFlags as the QWindow is created there if it does not exist.
        menu->setAttribute(Qt::WA_TranslucentBackground, true);
        menu->setAttribute(Qt::WA_OpaquePaintEvent, true);
        menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

        return true;
    }

    bool Menu::unpolish(Style* style, QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        auto menu = qobject_cast<QMenu*>(widget);
        if (!menu)
        {
            return false;
        }

        menu->setWindowFlags(menu->windowFlags());
        menu->setAttribute(Qt::WA_TranslucentBackground, false);
        menu->setAttribute(Qt::WA_OpaquePaintEvent, false);

        return true;
    }

    int Menu::horizontalMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        if (qobject_cast<const QMenu*>(widget))
        {
            return config.horizontalMargin + config.horizontalPadding;
        }

        return -1;
    }

    int Menu::verticalMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        if (qobject_cast<const QMenu*>(widget))
        {
            return config.verticalMargin + config.verticalPadding;
        }

        return -1;
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

    int Menu::verticalShadowMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        if (qobject_cast<const QMenu*>(widget))
        {
            return -config.verticalMargin;
        }

        return std::numeric_limits<int>::lowest();
    }

    int Menu::horizontalShadowMargin(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(option);

        if (qobject_cast<const QMenu*>(widget))
        {
            return -config.horizontalMargin;
        }

        return std::numeric_limits<int>::lowest();
    }

    bool Menu::drawFrame(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Menu::Config& config)
    {
        Q_UNUSED(style);

        auto menu = qobject_cast<const QMenu*>(widget);
        if (!menu)
        {
            return false;
        }

        const auto& m = config.shadowMargins;
        const QMargins shadowMargins(m.left, m.top, m.right, m.bottom);

        qDrawBorderPixmap(painter, option->rect, shadowMargins, config.shadowPixmap);

        painter->setPen(Qt::NoPen);
        painter->setBrush(config.backgroundColor);
        const auto hMargin = config.horizontalMargin;
        const auto vMargin = config.verticalMargin;
        painter->drawRoundedRect(option->rect.adjusted(hMargin, vMargin, -hMargin, -vMargin), config.radius, config.radius);

        return true;
    }
} // namespace AzQtComponents
