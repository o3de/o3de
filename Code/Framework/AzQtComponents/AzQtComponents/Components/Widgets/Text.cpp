/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>

#include <QLabel>
#include <QSettings>
#include <QString>

namespace AzQtComponents
{
    static QString g_headlineClass = QStringLiteral("Headline");
    static QString g_titleClass = QStringLiteral("Title");
    static QString g_subtitleClass = QStringLiteral("Subtitle");
    static QString g_menuClass = QStringLiteral("Menu");
    static QString g_labelClass = QStringLiteral("Label");
    static QString g_paragraphClass = QStringLiteral("Paragraph");
    static QString g_tooltipClass = QStringLiteral("Tooltip");
    static QString g_buttonClass = QStringLiteral("Button");

    static QString g_primaryTextClass = QStringLiteral("primaryText");
    static QString g_secondaryTextClass = QStringLiteral("secondaryText");
    static QString g_highlightedTextClass = QStringLiteral("highlightedText");
    static QString g_blackTextClass = QStringLiteral("blackText");

    Text::Config Text::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();
        ConfigHelpers::GroupGuard guard(&settings, QStringLiteral("Hyperlink"));
        ConfigHelpers::read<QColor>(settings, QStringLiteral("Color"), config.hyperlinkColor);

        return config;
    }

    Text::Config Text::defaultConfig()
    {
        Config config;
        config.hyperlinkColor = QStringLiteral("#94D2FF");

        return config;
    }

    void Text::addHeadlineStyle(QLabel* text)
    {
        Style::addClass(text, g_headlineClass);
    }

    void Text::addTitleStyle(QLabel* text)
    {
        Style::addClass(text, g_titleClass);
    }

    void Text::addSubtitleStyle(QLabel* text)
    {
        Style::addClass(text, g_subtitleClass);
    }

    void Text::addMenuStyle(QLabel* text)
    {
        Style::addClass(text, g_menuClass);
    }

    void Text::addLabelStyle(QLabel* text)
    {
        Style::addClass(text, g_labelClass);
    }

    void Text::addParagraphStyle(QLabel* text)
    {
        Style::addClass(text, g_paragraphClass);
    }

    void Text::addTooltipStyle(QLabel* text)
    {
        Style::addClass(text, g_tooltipClass);
    }

    void Text::addButtonStyle(QLabel* text)
    {
        Style::addClass(text, g_buttonClass);
    }

    void Text::addPrimaryStyle(QLabel* text)
    {
        Style::addClass(text, g_primaryTextClass);
    }

    void Text::addSecondaryStyle(QLabel* text)
    {
        Style::addClass(text, g_secondaryTextClass);
    }

    void Text::addHighlightedStyle(QLabel* text)
    {
        Style::addClass(text, g_highlightedTextClass);
    }

    void Text::addBlackStyle(QLabel* text)
    {
        Style::addClass(text, g_blackTextClass);
    }
} // namespace AzQtComponents
