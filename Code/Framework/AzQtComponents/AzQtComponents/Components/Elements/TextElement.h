/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <QColor>
#endif

class QFont;

namespace AzQtComponents
{
    //! The font a widget's rules give it, and the colour of its text. Every element sets the family and the weight (the legacy
    //! '*' rule reached every widget); a size or style no rule sets stays Qt's or the widget's code's (m_pixelSize and m_pointSize
    //! 0, m_italic and m_underline -1).
    //! A class's rules are installed as Qt class fonts, which do not reach the widgets inside as setFont would; the matched
    //! elements are set over them at polish. The colour goes into the widget's palette as the sheet put it there, and so reaches
    //! the widget's children: give one to the widgets that draw the text, not to a container.
    struct AZ_QT_COMPONENTS_API TextElement
    {
        const char* m_name = "";                                //!< Scope.Text.Look
        const char* m_family = Foundations::Surface::Family;    //!< the universal rule's
        int m_pixelSize = 0;                                    //!< 0: no rule sets it
        //! 0: no rule sets it. A size in points, for a rule that gave `12pt` (QCss setFontSizeFromValue calls QFont::setPointSizeF):
        //! Qt turns it into pixels at the screen's logical DPI, as it did for the sheet. Taken only where m_pixelSize is 0.
        int m_pointSize = 0;
        int m_weight = Foundations::Surface::Weight;            //!< the universal rule's
        int m_italic = -1;                                      //!< -1: no rule sets it
        //! -1: no rule sets it. 1 for a rule that gave `text-decoration: underline`, 0 for one that gave `none` (QCss extractFont sets
        //! the font's underline). The font keeps what an earlier element set, so a state that ends the underline gives 0.
        int m_underline = -1;
        QColor m_color;                                         //!< The text, enabled; invalid: the widget keeps its palette
        QColor m_disabledColor;                                 //!< The text, disabled; invalid: m_color

        //! Sets this element's attributes over the font; true if it changed
        bool overlay(QFont& font) const;

        // ---- Class fonts
        //! From Style::polish(QApplication*): the class rules as Qt class fonts (setStyle rebuilt Qt's from the platform just before)
        static void installClassFonts();

        // ---- Resolution
        //! The widgets of widgetClass the matcher accepts have this element: module rows before Global ones, each in registration order
        static void registerElement(const TextElement* element, const QMetaObject* widgetClass, ElementMatcher matcher,
            ElementScope scope = ElementScope::Module);
        //! From Style::polish, first: finds the widget's element and sets it over the widget's font
        static void resolve(QWidget* widget);
        //! From Style::polish, last: the colour of the element resolve found, over the palettes the rest of the polish set
        static void applyColor(QWidget* widget);
    };
} // namespace AzQtComponents
