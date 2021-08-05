/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ToolBar.h>

#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QEvent>
#include <QApplication>
#include <QSettings>
#include <QStyleOptionToolBar>
#include <QToolBar>
#include <QToolButton>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4251: 'QCss::Declaration::d': class 'QExplicitlySharedDataPointer<QCss::Declaration::DeclarationData>' needs to have dll-interface to be used by clients of struct 'QCss::Declaration'
#include <QtWidgets/private/qstylesheetstyle_p.h>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
static QString g_mainToolBarClass = QStringLiteral("MainToolBar");
static QString g_defaultSpacingClass = QStringLiteral("DefaultSpacing");
static QString g_iconNormalClass = QStringLiteral("IconNormal");
static QString g_iconLargeClass = QStringLiteral("IconLarge");

static void ReadToolBarConfig(QSettings& settings, const QString& name, ToolBar::ToolBarConfig& config)
{
    ConfigHelpers::GroupGuard guard(&settings, name);
    ConfigHelpers::read<int>(settings, QStringLiteral("ItemSpacing"), config.itemSpacing);
}

ToolBar::Config ToolBar::loadConfig(QSettings& settings)
{
    Q_UNUSED(settings);

    Config config = defaultConfig();
    ReadToolBarConfig(settings, QStringLiteral("Primary"), config.primary);
    ReadToolBarConfig(settings, QStringLiteral("Secondary"), config.secondary);
    ConfigHelpers::read<int>(settings, QStringLiteral("DefaultSpacing"), config.defaultSpacing);
    return config;
}

ToolBar::Config ToolBar::defaultConfig()
{
    Config config;

    config.primary.itemSpacing = 12;
    config.secondary.itemSpacing = 12;

    config.defaultSpacing = 4;

    return config;
}

void ToolBar::addMainToolBarStyle(QToolBar* toolbar)
{
    Style::addClass(toolbar, g_mainToolBarClass);

    // QToolBarLayout only gets the spacing and margins from the style in the
    // constructor, or if the QToolBar receives a StyleChangeEvent.
    QEvent event(QEvent::StyleChange);
    QApplication::sendEvent(toolbar, &event);
}

void ToolBar::setToolBarIconSize(QToolBar* toolbar, ToolBarIconSize size)
{
    Style::removeClass(toolbar, g_iconNormalClass);
    Style::removeClass(toolbar, g_iconLargeClass);

    // This is needed for backward compatibility
    // Saving target icon size for UI1.0 on a property, and only setting the icon size through setIconSize
    // when polishing from UI1.0 style. This way the icon size will be read from ToolBar.qss as needed.
    toolbar->setProperty(g_ui10IconSizeProperty,
        size == ToolBarIconSize::IconNormal ? g_ui10DefaultIconSize : g_ui10LargeIconSize);

    // Invalidate any previously set icon size so the toolbar will get the right size through our style.
    toolbar->setIconSize({});

    switch (size) {
        case ToolBarIconSize::IconNormal:
        {
            Style::addClass(toolbar, g_iconNormalClass);
            break;
        }
        case ToolBarIconSize::IconLarge:
        {
            Style::addClass(toolbar, g_iconLargeClass);
            break;
        }
    }

    // Force a layout update
    QEvent event(QEvent::StyleChange);
    QApplication::sendEvent(toolbar, &event);

    // Repolish
    StyleManager::repolishStyleSheet(toolbar);
}

QToolButton* ToolBar::getToolBarExpansionButton(QToolBar* toolBar)
{
    if (!toolBar)
    {
        return nullptr;
    }

    auto children = toolBar->findChildren<QToolButton*>("qt_toolbar_ext_button", Qt::FindDirectChildrenOnly);
    return children.isEmpty() ? nullptr : children.first();
}

bool ToolBar::polish(Style* style, QWidget* widget, const Config& config)
{
    Q_UNUSED(config);

    auto toolbar = qobject_cast<QToolBar*>(widget);
    if (!toolbar)
    {
        return false;
    }

    style->repolishOnSettingsChange(widget);

    return true;
}

int ToolBar::itemSpacing(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config)
{
    Q_UNUSED(option);

    int spacing = -1;

    const auto toolbar = qobject_cast<const QToolBar*>(widget);
    if (!toolbar)
    {
        return spacing;
    }

    spacing = config.secondary.itemSpacing;

    if (style->hasClass(toolbar, g_mainToolBarClass))
    {
        spacing = config.primary.itemSpacing;
    }
    else if (style->hasClass(toolbar, g_defaultSpacingClass))
    {
        spacing = config.defaultSpacing;
    }

    return spacing;
}

} // namespace AzQtComponents
