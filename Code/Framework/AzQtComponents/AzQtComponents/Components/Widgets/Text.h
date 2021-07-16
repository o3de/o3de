/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QColor>

class QLabel;
class QSettings;

namespace AzQtComponents
{
    /**
     * Special class to handle styling and painting of Text (labels, to be specific).
     *
     * All of the styling of QLabel objects is done via CSS, in Text.qss
     *
     * There are 4 css text styles that can be added to QLabel objects (Headline, Title, Subtitle, and Paragraph). Use the appropriate
     * method below to apply one.
     *
     * Note that a QLabel without a custom css "class" still gets styled according to the Text.qss file.
     * Note also that Qt doesn't properly support the line-height css property, so if non-default Qt
     * line-height spacing is required for multi-line blocks of text, some extra effort will be required
     * beyond the scope of this class/code.
     *
     * There are also 4 css color styles that can be added to QLabel objects (Primary, Secondary, Highlighted and Black). Use the
     * appropriate method below to apply one.
     *
     **/
    class AZ_QT_COMPONENTS_API Text
    {
    public:
        struct Config
        {
            QColor hyperlinkColor;
        };

        /*!
         * Loads the Text config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default Text config data.
         */
        static Config defaultConfig();

        /*!
        * Applies the Headline styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "Headline");
        */
        static void addHeadlineStyle(QLabel* text);
        
        /*!
        * Applies the Title styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "Title");
        */
        static void addTitleStyle(QLabel* text);

        /*!
        * Applies the Subtitle styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "Subtitle");
        */
        static void addSubtitleStyle(QLabel* text);
        
        /*!
         * Applies the Data/Menu styling to a QLabel.
         * Same as
         *   AzQtComponents::Style::addClass(label, "Menu");
         */
        static void addMenuStyle(QLabel* text);

        /*!
         * Applies the Label styling to a QLabel.
         * Same as
         *   AzQtComponents::Style::addClass(label, "Label");
         */
        static void addLabelStyle(QLabel* text);

        /*!
        * Applies the Paragraph styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "Paragraph");
        */
        static void addParagraphStyle(QLabel* text);

        /*!
         * Applies the Tooltip styling to a QLabel.
         * Same as
         *   AzQtComponents::Style::addClass(label, "Tooltip");
         */
        static void addTooltipStyle(QLabel* text);

        /*!
         * Applies the Button styling to a QLabel.
         * Same as
         *   AzQtComponents::Style::addClass(label, "Button");
         */
        static void addButtonStyle(QLabel* text);

        /*!
        * Applies the Primary color styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "primaryText");
        */
        static void addPrimaryStyle(QLabel* text);
        
        /*!
        * Applies the Secondary color styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "secondaryText");
        */
        static void addSecondaryStyle(QLabel* text);

        /*!
        * Applies the Highlighted color styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "highlightedText");
        */
        static void addHighlightedStyle(QLabel* text);

        /*!
        * Applies the Black color styling to a QLabel.
        * Same as
        *   AzQtComponents::Style::addClass(label, "blackText");
        */
        static void addBlackStyle(QLabel* text);
    };
} // namespace AzQtComponents
