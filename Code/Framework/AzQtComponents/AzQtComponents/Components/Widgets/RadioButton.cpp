/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/RadioButton.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QStyleFactory>

#include <QRadioButton>
#include <QPainter>
#include <QStyleOption>
#include <QVariant>
#include <QSettings>

namespace AzQtComponents
{

bool RadioButton::polish(Style* style, QWidget* widget, const RadioButton::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);

    if (!StyleManager::stylesheetsDisabled() || !qobject_cast<QRadioButton*>(widget))
    {
        return false;
    }

    // The widget's indicator element is resolved once, here
    IndicatorElement::resolve(widget);
    return true;
}

QSize RadioButton::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const RadioButton::Config& config)
{
    Q_UNUSED(config);

    if (StyleManager::stylesheetsDisabled())
    {
        // The indicator element: the box (indicator and margin) and the label spacing around the text
        if (const IndicatorElement* indicator = IndicatorElement::forWidget(widget))
        {
            return indicator->sizeFromContents(size, style->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, option, widget));
        }
    }

    QSize sz = style->QProxyStyle::sizeFromContents(type, option, size, widget);

    return sz;
}

RadioButton::Config RadioButton::loadConfig(QSettings& settings)
{
    Q_UNUSED(settings);

    Config config = defaultConfig();

    return config;
}

RadioButton::Config RadioButton::defaultConfig()
{
    Config config;

    return config;
}

bool RadioButton::drawRadioButton(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const RadioButton::Config& config)
{
    Q_UNUSED(config);

    // The indicator element draws the indicator and the label where the legacy sheet laid them out, and no
    // focus rectangle (RadioButton.qss:14 `outline: 0`); the base style's own layout does not match it
    return StyleManager::stylesheetsDisabled()
        && IndicatorElement::draw(style, qstyleoption_cast<const QStyleOptionButton*>(option), painter, widget);
}

bool RadioButton::drawRadioButtonLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const RadioButton::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(widget);
    Q_UNUSED(config);
    Q_UNUSED(option);
    Q_UNUSED(painter);

    return false;
}

} // namespace AzQtComponents
