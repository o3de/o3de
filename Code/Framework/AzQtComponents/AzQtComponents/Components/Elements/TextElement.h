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
#include <QColor>
#endif

class QFont;
struct QMetaObject;
class QWidget;

namespace AzQtComponents
{
    //! A text element of the flattened GUI: the font attributes the legacy GUI finally renders for the
    //! widgets it covers (the winners of its stylesheet cascade); each definition cites the declaration
    //! that grants every value. Family and weight are set by every element: the legacy '*' rule
    //! (BaseStyleSheet.qss:19-20) reached every widget. A size or style no rule sets is left to Qt and to
    //! the widget's own code (m_pixelSize 0, m_italic -1). Sites that render identically share one
    //! element, and no element is built from another.
    //!
    //! Two ways in, both resolved once (never per paint):
    //! - class fonts: an element for every widget of a class, installed as a Qt class font. Unlike
    //!   setFont, a class font does not reach the widgets inside (a style sheet never did either).
    //! - widget elements: an element for the widgets a matcher picks, applied when a widget is polished.
    //! Global elements are defined in TextElement.cpp; a window or feature module defines its own and
    //! registers them, checked before the Global ones (as TabElement::registerElement works).
    //!
    //! A widget element may also carry the colour its rules gave the text. The colour goes into the widget's
    //! palette as the sheet put it there, so it reaches the widget's children as a palette does: give one
    //! to the widgets that draw the text (a label, a button, an entry), not to a container.
    struct AZ_QT_COMPONENTS_API TextElement
    {
        const char* m_name = "";                //!< Scope.Text.Look
        const char* m_family = "Open Sans";     //!< BaseStyleSheet.qss:19. TOKENIZE: TextFamily
        int m_pixelSize = 0;                    //!< 0: no rule sets it
        int m_weight = 400;                     //!< BaseStyleSheet.qss:20. TOKENIZE: TextWeight
        int m_italic = -1;                      //!< -1: no rule sets it

        // ---- colour: the `color` of the element's rules (invalid: no rule sets one, the widget keeps its palette)
        QColor m_color;                         //!< the enabled states (the Active and Inactive colour groups)
        QColor m_disabledColor;                 //!< the Disabled group; invalid: m_color reaches it, as a rule without :disabled does

        //! The attributes this element sets, over the font. True if the font changed.
        bool overlay(QFont& font) const;

        // ---- class fonts (flattened mode)
        //! From Style::polish(QApplication*): Qt rebuilds its class fonts from the platform on every
        //! setStyle (qapplication.cpp:1019), right before it calls that polish (qapplication.cpp:1022).
        static void installClassFonts();
        //! A module's element for every widget of one class (and of its subclasses).
        static void registerClassFont(const TextElement* element, const char* className);

        // ---- widget elements (flattened mode). Widgets polished before a registration are resolved
        // again, so the order of calls does not matter.
        using Matcher = bool (*)(const QWidget* widget);
        static void registerElement(const TextElement* element, const QMetaObject* widgetClass, Matcher matcher);
        //! From Style::polish(QWidget*), first: the element of the widget, over its font.
        static void apply(QWidget* widget);
        //! From Style::polish(QWidget*), last: the colour of the element apply found, over the palettes the rest of the
        //! flattened polish set (a module's rule won over the Global ones in the sheet's cascade).
        static void applyColor(QWidget* widget);

        // ---- selectors for matchers (the QSS each stands for)
        static bool named(const QWidget* widget, const char* objectName);              // #name
        static bool childOf(const QWidget* widget, const char* parentObjectName);      // #name > W
        static bool within(const QWidget* widget, const char* ancestorObjectName);     // #name W
        static bool withinClass(const QWidget* widget, const char* ancestorClass);     // Class W
        static bool tagged(const QWidget* widget, const char* token);                  // .token
    };
} // namespace AzQtComponents
