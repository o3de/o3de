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
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

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
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace PropertyEditorElementsInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        const QColor SurfaceFill(0x00, 0x00, 0x00, 0x00);           // BaseStyleSheet.qss:17 $fill.surface.idle.Clear
        // Text.qss:93-94 $space.label.idle.margin-top.Alt1 $space.label.idle.margin-bottom.Alt1 (every QLabel)
        constexpr int LabelMarginV = 1;
        constexpr const char* LabelFamily = "Open Sans";            // Text.qss:91 $type.label.idle.family.Alt1
        constexpr int LabelSize = 12;                               // Text.qss:92 $type.label.idle.size.Alt1

        // ---- The labels of a property row
        // ReflectedPropertyEditor.qss:14 (left), :13 (the others) $space.label.idle.margin-left.Alt1 $space.label.idle.margin.Alt1
        constexpr QMargins RowNameMargin(4, 0, 0, 0);
        // ReflectedPropertyEditor.qss:11, :19 $size.label.idle.min-height.Alt1 (no width)
        constexpr QSize RowLabelMinSize(-1, 16);
        // ReflectedPropertyEditor.qss:12, :20 $size.label.idle.max-height.Alt1 (no width)
        constexpr QSize RowLabelMaxSize(-1, 16);
        // PrefabOverrideLabelHandler::PrefabOverrideLabelHandler (its inline sheet: bold) $type.label.overridden.weight
        constexpr int OverriddenWeight = 700;

        // ---- The parts of a card header: its layout has no spacing, the gaps are the parts' margins
        constexpr QMargins ExpanderMargin(0, 0, 4, 0);              // Card.qss:211 $space.check.idle.margin.Alt2
        // CheckBox.qss:129, :127 $size.check.idle.min-width.Alt1 $size.check.idle.min-height
        constexpr QSize ExpanderMinSize(16, 16);
        // CheckBox.qss:130, :128 $size.check.idle.max-width $size.check.idle.max-height
        constexpr QSize ExpanderMaxSize(16, 16);
        const QColor ExpanderFill(0x00, 0x00, 0x00, 0x00);          // CheckBox.qss:123 $fill.check.idle
        constexpr int IconMarginRight = 4;                          // Card.qss:219 $space.label.idle.margin-right.Alt2
        constexpr int WarningIconMarginLeft = 4;                    // Card.qss:226 $space.label.idle.margin-left.Alt1
        const QColor IconFill(0x00, 0x00, 0x00, 0x00);              // Card.qss:221 $fill.label.idle.Clear
        const QColor FlatButtonFill(0x00, 0x00, 0x00, 0x00);        // PushButton.qss:51 $fill.button.flat
        constexpr bool ButtonFlat = true;                           // Card.qss:233, :243, :253 $other.button.idle.flat
        // Card.qss:251 $asset.button.idle.icon.Alt4
        constexpr const char* HelpIcon = ":/Cards/img/UI20/Cards/help.svg";
        constexpr QSize HelpIconSize(16, 16);                       // Card.qss:252 $asset.button.idle.iconsize.Alt1
        constexpr QMargins HelpMargin(0, 0, 4, 0);                  // Card.qss:254 $space.button.idle.margin.Alt2
        // Card.qss:255-256 $size.button.idle.max-width.Alt3 $size.button.idle.max-height.Alt3
        constexpr QSize HelpMaxSize(16, 16);
        // Card.qss:231 $asset.button.idle.icon.Alt1
        constexpr const char* ContextMenuIcon = ":/Cards/img/UI20/Cards/menu_ico.svg";
        constexpr QSize ContextMenuIconSize(18, 18);                // Card.qss:232, :242 $asset.button.idle.iconsize.Alt4
        // Card.qss:235-236 $size.button.idle.max-width.Alt4 $size.button.idle.max-height.Alt5
        constexpr QSize ContextMenuMaxSize(28, 18);
        constexpr const char* ContextMenuPlusIcon = ":/Gallery/Add.svg";    // Card.qss:241 $asset.button.idle.icon.Alt7
        // Card.qss:245-246 $size.button.idle.max-width.Alt14 $size.button.idle.max-height.Alt5
        constexpr QSize ContextMenuPlusMaxSize(18, 18);

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! AzToolsFramework--PropertyRowWidget QLabel#Name: 4px at the left "to match CardHeader", no margin above or below where
        //! Text.qss gave every label 1px, and 16px high whatever the text needs. The row's name label is an ElidingLabel, which
        //! measures its hint with a hidden QLabel that ElidingLabel::setObjectName gives the same name: the rule reached both, so the
        //! hint counts the left margin twice.
        const BoxElement PropertyRowName{
            .m_name = "Global.Box.PropertyRowName",
            .m_margin = RowNameMargin,
            .m_minSize = RowLabelMinSize,
            .m_maxSize = RowLabelMaxSize,
            .m_idle = {
                .m_fill = QBrush(SurfaceFill),
            },
        };

        //! AzToolsFramework--PropertyRowWidget QLabel#DefaultLabel: the text a container or an unhandled row shows in place of a
        //! control, 16px high, with the 1px above and below Text.qss gave every label
        const BoxElement PropertyRowDefaultLabel{
            .m_name = "Global.Box.PropertyRowDefaultLabel",
            .m_margin = QMargins(0, LabelMarginV, 0, LabelMarginV),
            .m_minSize = RowLabelMinSize,
            .m_maxSize = RowLabelMaxSize,
            .m_idle = {
                .m_fill = QBrush(SurfaceFill),
            },
        };

        //! The name label of a property a prefab instance overrides (the `overridden` property of PrefabOverrideLabelHandler's
        //! ElidingLabel, set in RefreshUI, which repolishes the label): bold over the QLabel's font. With style sheets the handler
        //! gives the label an inline sheet; flattened, none. A label not overridden keeps the QLabel's Global element.
        const TextElement OverriddenPropertyLabel{
            .m_name = "Global.Text.OverriddenPropertyLabel",
            .m_family = LabelFamily,
            .m_pixelSize = LabelSize,
            .m_weight = OverriddenWeight,
        };

        //! AzQtComponents--CardHeader #Expander, the "tree expander" check box: a margin at the right over QCheckBox.Expander's
        //! `margin: 0px`, and a box of 16x16. Its caret is the indicator's.
        const BoxElement CardHeaderExpander{
            .m_name = "Global.Box.CardHeaderExpander",
            .m_margin = ExpanderMargin,
            .m_minSize = ExpanderMinSize,
            .m_maxSize = ExpanderMaxSize,
            .m_idle = {
                .m_fill = QBrush(ExpanderFill),
            },
        };

        //! AzQtComponents--CardHeader #Icon: the component's icon, a margin at the right, the 1px above and below Text.qss gave every
        //! label, and `border: none` (Card.qss:220 $line.label.idle.style.Alt1). Its `width` and `height` set nothing (Card.qss:217-218):
        //! a label sizes itself from its pixmap.
        const BoxElement CardHeaderIcon{
            .m_name = "Global.Box.CardHeaderIcon",
            .m_margin = QMargins(0, LabelMarginV, IconMarginRight, LabelMarginV),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(IconFill),
            },
        };

        //! AzQtComponents--CardHeader #WarningIcon: the icon of a header with a warning, with a margin at the left as well
        const BoxElement CardHeaderWarningIcon{
            .m_name = "Global.Box.CardHeaderWarningIcon",
            .m_margin = QMargins(WarningIconMarginLeft, LabelMarginV, IconMarginRight, LabelMarginV),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(IconFill),
            },
        };

        //! The same two icons inside a color picker: `AzQtComponents--ColorPicker QLabel` gives a label no margin (ColorPicker.qss:48
        //! $space.label.idle.margin.Alt1) and outranks Text.qss's `QLabel`, so the 1px above and below are gone; the margins of the ids
        //! stay (an id outranks both)
        const BoxElement CardHeaderIconInPicker{
            .m_name = "Global.Box.CardHeaderIconInPicker",
            .m_margin = QMargins(0, 0, IconMarginRight, 0),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(IconFill),
            },
        };

        const BoxElement CardHeaderWarningIconInPicker{
            .m_name = "Global.Box.CardHeaderWarningIconInPicker",
            .m_margin = QMargins(WarningIconMarginLeft, 0, IconMarginRight, 0),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(IconFill),
            },
        };

        //! AzQtComponents--CardHeader #Help: a flat button with the help icon. Qt gave the limits the native border of a push button
        //! as the rule polished it, before qproperty-flat: the box counts it (BoxElement::resolve).
        const BoxElement CardHeaderHelp{
            .m_name = "Global.Box.CardHeaderHelp",
            .m_margin = HelpMargin,
            .m_maxSize = HelpMaxSize,
            .m_idle = {
                .m_fill = QBrush(FlatButtonFill),
            },
            .m_properties = {
                { "icon", QString(HelpIcon) },
                { "iconSize", HelpIconSize },
                { "flat", ButtonFlat },
            },
        };

        //! AzQtComponents--CardHeader #ContextMenu: a flat button with the menu icon, and a margin of 0 (Card.qss:234
        //! $space.button.idle.margin.Alt1), which gives it a box
        const BoxElement CardHeaderContextMenu{
            .m_name = "Global.Box.CardHeaderContextMenu",
            .m_hasBox = true,
            .m_maxSize = ContextMenuMaxSize,
            .m_idle = {
                .m_fill = QBrush(FlatButtonFill),
            },
            .m_properties = {
                { "icon", QString(ContextMenuIcon) },
                { "iconSize", ContextMenuIconSize },
                { "flat", ButtonFlat },
            },
        };

        //! AzQtComponents--CardHeader #ContextMenuPlusIcon: the same button with a plus (CardHeader::setContextMenuIcon renames it), with
        //! a margin of 0 (Card.qss:244 $space.button.idle.margin.Alt1)
        const BoxElement CardHeaderContextMenuPlus{
            .m_name = "Global.Box.CardHeaderContextMenuPlus",
            .m_hasBox = true,
            .m_maxSize = ContextMenuPlusMaxSize,
            .m_idle = {
                .m_fill = QBrush(FlatButtonFill),
            },
            .m_properties = {
                { "icon", QString(ContextMenuPlusIcon) },
                { "iconSize", ContextMenuIconSize },
                { "flat", ButtonFlat },
            },
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! `AzToolsFramework--PropertyRowWidget W`
        bool inPropertyRow(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzToolsFramework::PropertyRowWidget");
        }

        //! `[overridden="true"]`: an attribute selector compares the property's text (a bool reads "true")
        bool overriddenProperty(const QWidget* widget)
        {
            return widget->property("overridden").toString() == QLatin1String("true");
        }

        //! `AzQtComponents--CardHeader W`
        bool inCardHeader(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::CardHeader");
        }

        //! `AzQtComponents--ColorPicker W`: a label of a card header in a color picker has the picker's box
        bool inColorPicker(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::ColorPicker");
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
        constexpr ElementScope global = ElementScope::Global;

        BoxElement::registerElement(&PropertyRowName, label,
            [](const QWidget* w) { return Selectors::named(w, "Name") && inPropertyRow(w); }, global);
        BoxElement::registerElement(&PropertyRowDefaultLabel, label,
            [](const QWidget* w) { return Selectors::named(w, "DefaultLabel") && inPropertyRow(w); }, global);

        // The handler's sheet sat on its label only; no other widget has the property
        TextElement::registerElement(&OverriddenPropertyLabel, &ElidingLabel::staticMetaObject,
            [](const QWidget* w) { return overriddenProperty(w); });

        BoxElement::registerElement(&CardHeaderExpander, checkBox,
            [](const QWidget* w)
            {
                // the class tag is the rule's too (QCheckBox.Expander)
                return Selectors::named(w, "Expander") && Selectors::tagged(w, "Expander") && inCardHeader(w);
            }, global);
        BoxElement::registerElement(&CardHeaderIcon, label,
            [](const QWidget* w) { return Selectors::named(w, "Icon") && inCardHeader(w) && !inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderWarningIcon, label,
            [](const QWidget* w) { return Selectors::named(w, "WarningIcon") && inCardHeader(w) && !inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderIconInPicker, label,
            [](const QWidget* w) { return Selectors::named(w, "Icon") && inCardHeader(w) && inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderWarningIconInPicker, label,
            [](const QWidget* w) { return Selectors::named(w, "WarningIcon") && inCardHeader(w) && inColorPicker(w); }, global);
        BoxElement::registerElement(&CardHeaderHelp, button,
            [](const QWidget* w) { return Selectors::named(w, "Help") && inCardHeader(w); }, global);
        BoxElement::registerElement(&CardHeaderContextMenu, button,
            [](const QWidget* w) { return Selectors::named(w, "ContextMenu") && inCardHeader(w); }, global);
        BoxElement::registerElement(&CardHeaderContextMenuPlus, button,
            [](const QWidget* w) { return Selectors::named(w, "ContextMenuPlusIcon") && inCardHeader(w); }, global);
    }
} // namespace AzQtComponents
