/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QBrush>
#include <QCheckBox>
#include <QLabel>
#include <QMargins>
#include <QPushButton>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others
    //! into one translation unit, where anonymous namespaces merge (TextElement.cpp does the same).
    namespace PropertyEditorElementsInternal
    {
        // ------------------------------------------------------------------------------
        // Property row labels: ReflectedPropertyEditor.qss over Text.qss and BaseStyleSheet.qss
        //
        // A property row is an AzToolsFramework::PropertyRowWidget. Its name label is an ElidingLabel, which measures its size
        // hint with a hidden QLabel of its own that ElidingLabel::setObjectName gives the same name: the rule reached both,
        // so each carries the box, and the label's hint counts the left margin twice (the hidden label's and its own).
        // ------------------------------------------------------------------------------

        //! AzToolsFramework--PropertyRowWidget QLabel#Name (ReflectedPropertyEditor.qss:9): 4px at the left, "to match
        //! CardHeader" (:14), and no margin above or below, where Text.qss gave every label 1px (the rule's `margin: 0px`,
        //! :13); 16px high whatever the text needs (:11-12). TOKENIZE: PropertyRowLabelIndent, PropertyRowLabelHeight
        const BoxElement PropertyRowName{
            .m_name = "Global.Box.PropertyRowName",
            .m_margin = QMargins(4, 0, 0, 0),                       // ReflectedPropertyEditor.qss:14 (left), :13 (the others)
            .m_minSize = QSize(-1, 16),                             // ReflectedPropertyEditor.qss:11
            .m_maxSize = QSize(-1, 16),                             // ReflectedPropertyEditor.qss:12
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // BaseStyleSheet.qss:17 (*)
            },
        };

        //! AzToolsFramework--PropertyRowWidget QLabel#DefaultLabel (ReflectedPropertyEditor.qss:17): the text a container or an
        //! unhandled row shows in place of a control, 16px high (:19-20), with the 1px above and below Text.qss gave every label.
        const BoxElement PropertyRowDefaultLabel{
            .m_name = "Global.Box.PropertyRowDefaultLabel",
            .m_margin = QMargins(0, 1, 0, 1),                       // Text.qss:93-94 (QLabel)
            .m_minSize = QSize(-1, 16),                             // ReflectedPropertyEditor.qss:19
            .m_maxSize = QSize(-1, 16),                             // ReflectedPropertyEditor.qss:20
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // BaseStyleSheet.qss:17 (*)
            },
        };

        // ------------------------------------------------------------------------------
        // Card header parts: Card.qss over Text.qss, CheckBox.qss, PushButton.qss and BaseStyleSheet.qss
        //
        // The header's layout has no spacing (CardHeader.cpp:152): the gaps are the margins of the parts.
        // ------------------------------------------------------------------------------

        //! AzQtComponents--CardHeader #Expander (Card.qss:209), the "tree expander" check box (CardHeader.cpp:108-110): 4px of
        //! margin at the right (:211), over QCheckBox.Expander's `margin: 0px` (CheckBox.qss:125), and a box of 16x16
        //! (CheckBox.qss:127-130), 20 wide with the margin. Its caret is the indicator's.
        //! TOKENIZE: CardHeaderPartSpacing, ExpanderSize
        const BoxElement CardHeaderExpander{
            .m_name = "Global.Box.CardHeaderExpander",
            .m_margin = QMargins(0, 0, 4, 0),                       // Card.qss:211
            .m_minSize = QSize(16, 16),                             // CheckBox.qss:129 (width), :127 (height)
            .m_maxSize = QSize(16, 16),                             // CheckBox.qss:130 (width), :128 (height)
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // CheckBox.qss:123
            },
        };

        //! AzQtComponents--CardHeader #Icon (Card.qss:214): the component's icon, 4px of margin at the right (:219) and the
        //! 1px above and below of Text.qss:93-94, no border (:220). Its `width` and `height` of 16px (:217-218) set nothing: a
        //! label sizes itself from its pixmap.
        const BoxElement CardHeaderIcon{
            .m_name = "Global.Box.CardHeaderIcon",
            .m_margin = QMargins(0, 1, 4, 1),                       // Card.qss:219 (right), Text.qss:93-94 (the others)
            .m_nativeBorder = false,                                // Card.qss:220 (border: none)
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // Card.qss:221
            },
        };

        //! AzQtComponents--CardHeader #WarningIcon (Card.qss:215): the icon of a header with a warning, as the icon with 4px of
        //! margin at the left as well (:224-227).
        const BoxElement CardHeaderWarningIcon{
            .m_name = "Global.Box.CardHeaderWarningIcon",
            .m_margin = QMargins(4, 1, 4, 1),                       // Card.qss:226 (left), :219 (right), Text.qss:93-94 (the others)
            .m_nativeBorder = false,                                // Card.qss:220
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // Card.qss:221
            },
        };

        //! The same two labels inside a color picker: `AzQtComponents--ColorPicker QLabel` (ColorPicker.qss:45-49) gives a label no margin
        //! at all, and its selector is more specific than Text.qss's `QLabel` (:93-94), so the 1px above and below are gone; the margins of
        //! the ids stay (an id beats both: Card.qss:219 at the right, :226 at the left of the warning icon). UNVERIFIED: no capture shows
        //! a header icon in a picker; the tests' sheet twin is the proof of the numbers.
        const BoxElement CardHeaderIconInPicker{
            .m_name = "Global.Box.CardHeaderIconInPicker",
            .m_margin = QMargins(0, 0, 4, 0),                       // Card.qss:219 (right), ColorPicker.qss:48 (the others)
            .m_nativeBorder = false,                                // Card.qss:220
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // Card.qss:221
            },
        };

        const BoxElement CardHeaderWarningIconInPicker{
            .m_name = "Global.Box.CardHeaderWarningIconInPicker",
            .m_margin = QMargins(4, 0, 4, 0),                       // Card.qss:226 (left), :219 (right), ColorPicker.qss:48 (the others)
            .m_nativeBorder = false,                                // Card.qss:220
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // Card.qss:221
            },
        };

        //! AzQtComponents--CardHeader #Help (Card.qss:249): a flat button with the help icon, 16x16 at most (:255-256) and
        //! 4px of margin at the right (:254). Qt gave the limits the native border of a push button as the rule polished the
        //! button, before qproperty-flat: the box counts it (BoxElement::applyLimits). TOKENIZE: HelpIconSize
        const BoxElement CardHeaderHelp{
            .m_name = "Global.Box.CardHeaderHelp",
            .m_margin = QMargins(0, 0, 4, 0),                       // Card.qss:254
            .m_maxSize = QSize(16, 16),                             // Card.qss:255-256
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // PushButton.qss:51 (QPushButton:flat)
            },
            .m_properties = {
                { "icon", QString(":/Cards/img/UI20/Cards/help.svg") },     // Card.qss:251
                { "iconSize", QSize(16, 16) },                              // Card.qss:252
                { "flat", true },                                           // Card.qss:253
            },
        };

        //! AzQtComponents--CardHeader #ContextMenu (Card.qss:229): a flat button with the menu icon, 28x18 at most
        //! (:235-236), and a margin of 0 (:234). TOKENIZE: ContextMenuIconSize
        const BoxElement CardHeaderContextMenu{
            .m_name = "Global.Box.CardHeaderContextMenu",
            .m_hasBox = true,                                       // a margin of 0: Card.qss:234
            .m_maxSize = QSize(28, 18),                             // Card.qss:235-236
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // PushButton.qss:51 (QPushButton:flat)
            },
            .m_properties = {
                { "icon", QString(":/Cards/img/UI20/Cards/menu_ico.svg") }, // Card.qss:231
                { "iconSize", QSize(18, 18) },                              // Card.qss:232
                { "flat", true },                                           // Card.qss:233
            },
        };

        //! AzQtComponents--CardHeader #ContextMenuPlusIcon (Card.qss:239): the same button with a plus (CardHeader::setContextMenuIcon
        //! renames it), 18x18 at most (:245-246). UNVERIFIED: no capture shows one.
        const BoxElement CardHeaderContextMenuPlus{
            .m_name = "Global.Box.CardHeaderContextMenuPlus",
            .m_hasBox = true,                                       // a margin of 0: Card.qss:244
            .m_maxSize = QSize(18, 18),                             // Card.qss:245-246
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // PushButton.qss:51 (QPushButton:flat)
            },
            .m_properties = {
                { "icon", QString(":/Gallery/Add.svg") },                   // Card.qss:241
                { "iconSize", QSize(18, 18) },                              // Card.qss:242
                { "flat", true },                                           // Card.qss:243
            },
        };

        // ------------------------------------------------------------------------------
        // Selectors: what each element covers is what its rule's selector covered
        // ------------------------------------------------------------------------------

        //! `AzToolsFramework--PropertyRowWidget W`
        bool inPropertyRow(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzToolsFramework::PropertyRowWidget");
        }

        //! `AzQtComponents--CardHeader W`
        bool inCardHeader(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzQtComponents::CardHeader");
        }

        //! `AzQtComponents--ColorPicker W`: a label of a card header in a color picker has the box of the picker's elements, not of
        //! the others (ColorPicker.qss:45-49)
        bool inColorPicker(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzQtComponents::ColorPicker");
        }
    } // namespace PropertyEditorElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void PropertyEditorElements::registerGlobal()
    {
        using namespace PropertyEditorElementsInternal;

        static bool registered = false;
        if (registered || !StyleManager::stylesheetsDisabled())
        {
            return;
        }
        registered = true;

        // The selectors name no state: qproperty-flat comes from the rule itself, so a button is matched before it is flat
        const QMetaObject* label = &QLabel::staticMetaObject;
        const QMetaObject* button = &QPushButton::staticMetaObject;
        const QMetaObject* checkBox = &QCheckBox::staticMetaObject;
        constexpr BoxElement::Scope global = BoxElement::Scope::Global;

        BoxElement::registerElement(&PropertyRowName, label,
            [](const QWidget* w) { return TextElement::named(w, "Name") && inPropertyRow(w); }, global);
        BoxElement::registerElement(&PropertyRowDefaultLabel, label,
            [](const QWidget* w) { return TextElement::named(w, "DefaultLabel") && inPropertyRow(w); }, global);

        BoxElement::registerElement(&CardHeaderExpander, checkBox,
            [](const QWidget* w)
            {
                // the class tag is the rule's too (QCheckBox.Expander, CheckBox.qss:121)
                return TextElement::named(w, "Expander") && TextElement::tagged(w, "Expander") && inCardHeader(w);
            }, global);
        BoxElement::registerElement(&CardHeaderIcon, label,
            [](const QWidget* w) { return TextElement::named(w, "Icon") && inCardHeader(w) && !inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderWarningIcon, label,
            [](const QWidget* w) { return TextElement::named(w, "WarningIcon") && inCardHeader(w) && !inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderIconInPicker, label,
            [](const QWidget* w) { return TextElement::named(w, "Icon") && inCardHeader(w) && inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderWarningIconInPicker, label,
            [](const QWidget* w) { return TextElement::named(w, "WarningIcon") && inCardHeader(w) && inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderHelp, button,
            [](const QWidget* w) { return TextElement::named(w, "Help") && inCardHeader(w); }, global);
        BoxElement::registerElement(&CardHeaderContextMenu, button,
            [](const QWidget* w) { return TextElement::named(w, "ContextMenu") && inCardHeader(w); }, global);
        BoxElement::registerElement(&CardHeaderContextMenuPlus, button,
            [](const QWidget* w) { return TextElement::named(w, "ContextMenuPlusIcon") && inCardHeader(w); }, global);
    }
} // namespace AzQtComponents
