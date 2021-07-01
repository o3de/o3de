/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ProgressBar.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QSettings>
#include <QStyleFactory>

namespace AzQtComponents
{

ProgressBar::Config ProgressBar::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();

    ConfigHelpers::read<int>(settings, QStringLiteral("Height"), config.height);

    return config;
}

ProgressBar::Config ProgressBar::defaultConfig()
{
    Config config;

    config.height = 5;

    return config;
}

QSize ProgressBar::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const ProgressBar::Config& config)
{
    Q_UNUSED(config);

    QSize sz = style->QProxyStyle::sizeFromContents(type, option, size, widget);

    sz.setHeight(config.height);

    return sz;
}

} // namespace AzQtComponents
