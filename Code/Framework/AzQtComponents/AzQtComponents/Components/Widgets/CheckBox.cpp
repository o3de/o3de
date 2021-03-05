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

#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Style.h>

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
    Q_UNUSED(style);
    Q_UNUSED(widget);
    Q_UNUSED(config);

    return false;
}

QSize CheckBox::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const CheckBox::Config& config)
{
    Q_UNUSED(config);

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
    Q_UNUSED(style);
    Q_UNUSED(widget);
    Q_UNUSED(config);
    Q_UNUSED(option);
    Q_UNUSED(painter);

    return false;
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
