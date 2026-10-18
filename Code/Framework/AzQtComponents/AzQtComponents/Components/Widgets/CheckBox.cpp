/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QStyleFactory>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QPainter>
#include <QStyleOption>
#include <QVariant>
#include <QSettings>

namespace AzQtComponents
{

static QString g_toggleSwitchClass = QStringLiteral("ToggleSwitch");
static QString g_expanderClass = QStringLiteral("Expander");
static QString g_visibilityModeClass = QStringLiteral("VisibilityMode");

void CheckBox::applyToggleSwitchStyle(QCheckBox* checkBox)
{
    Style::addClass(checkBox, g_toggleSwitchClass);
}

void CheckBox::applyExpanderStyle(QCheckBox* checkBox)
{
    Style::addClass(checkBox, g_expanderClass);
}

void CheckBox::setVisibilityMode(QAbstractItemView* view, bool enabled)
{
    if (enabled)
    {
        Style::addClass(view, g_visibilityModeClass);
    }
    else
    {
        Style::removeClass(view, g_visibilityModeClass);
    }
}

bool CheckBox::polish(Style* style, QWidget* widget, const CheckBox::Config& config)
{
    Q_UNUSED(config);

    if (!StyleManager::stylesheetsDisabled() || !qobject_cast<QCheckBox*>(widget))
    {
        return false;
    }

    // The widget's indicator element is resolved once, here (Style::addClass polishes a widget again when its class
    // tags change)
    IndicatorElement::resolve(widget);

    if (style->hasClass(widget, g_expanderClass))
    {
        // CheckBox.qss:127-130 QCheckBox.Expander { min-height, max-height, min-width, max-width: 16px }.
        // TOKENIZE: ExpanderSize
        widget->setFixedSize(16, 16);
    }

    return true;
}

QSize CheckBox::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const CheckBox::Config& config)
{
    Q_UNUSED(config);

    if (StyleManager::stylesheetsDisabled() && style->hasClass(widget, g_expanderClass))
    {
        // CheckBox.qss QCheckBox.Expander: min/max-height 16, min-width 16, no text.
        // TOKENIZE: ExpanderSize
        return QSize(16, 16);
    }

    if (StyleManager::stylesheetsDisabled())
    {
        // The indicator element: the box (indicator and margin) and the label spacing around the text
        if (const IndicatorElement* indicator = IndicatorElement::forWidget(widget))
        {
            return indicator->sizeFromContents(size, style->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, option, widget));
        }
    }

    QSize sz = style->QProxyStyle::sizeFromContents(type, option, size, widget);

    return sz;
}

CheckBox::Config CheckBox::loadConfig(QSettings& settings)
{
    Q_UNUSED(settings);

    Config config = defaultConfig();

    return config;
}

CheckBox::Config CheckBox::defaultConfig()
{
    Config config;

    return config;
}

bool CheckBox::drawCheckBox(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const CheckBox::Config& config)
{
    Q_UNUSED(config);

    // The indicator element draws the indicator and the label where the legacy sheet laid them out, and no
    // focus rectangle (CheckBox.qss:15 `outline: 0`); the base style's own layout does not match it
    return StyleManager::stylesheetsDisabled()
        && IndicatorElement::draw(style, qstyleoption_cast<const QStyleOptionButton*>(option), painter, widget);
}

bool CheckBox::drawCheckBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const CheckBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(widget);
    Q_UNUSED(config);
    Q_UNUSED(option);
    Q_UNUSED(painter);

    return false;
}

} // namespace AzQtComponents
