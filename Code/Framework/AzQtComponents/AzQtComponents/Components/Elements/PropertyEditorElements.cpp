/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/PropertyEditorElements.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/IndicatorElement.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/Elements/ToolTipElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CardNotification.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QBrush>
#include <QCheckBox>
#include <QFrame>
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
        const QColor ExpanderFill = Foundations::Clear;             // CheckBox.qss:123 $fill.check.idle
        constexpr int IconMarginRight = 4;                          // Card.qss:219 $space.label.idle.margin-right.Alt2
        constexpr int WarningIconMarginLeft = 4;                    // Card.qss:226 $space.label.idle.margin-left.Alt1
        const QColor IconFill = Foundations::Clear;                 // Card.qss:221 $fill.label.idle.Clear
        const QColor FlatButtonFill = Foundations::Clear;           // PushButton.qss:51 $fill.button.flat
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

        // ---- The widgets below a card header that no rule of their own names, and the tips of a modified header
        const QColor HeaderBodyFill(0x55, 0x55, 0x55);              // Card.qss:88 TOKENIZE: CardHeaderBodyFill
        const QColor ModifiedTextColor(0xE5, 0x78, 0x29);           // Card.qss:174 TOKENIZE: CardModifiedTextColor
        constexpr int ToolTipTextSize = 12;                         // ToolTip.qss:16 TOKENIZE: CardModifiedToolTipTextSize

        // ---- The notification of a card: a block holding a header frame, which holds the icon and the message in a rounded pill
        const QColor NotificationFill(0x55, 0x55, 0x55);            // Card.qss:53 TOKENIZE: CardNotificationFill
        // Card.qss:54, :60 TOKENIZE: CardNotificationSpacing (a frame has nothing to space: the rule only gets a box of it)
        constexpr int NotificationSpacing = 4;
        // Card.qss:59 TOKENIZE: CardNotificationHeaderPadding
        constexpr QMargins NotificationHeaderPadding(8, 0, 8, 0);
        const QColor NotificationPillFill(0x44, 0x44, 0x44);        // Card.qss:67 TOKENIZE: CardNotificationPillFill
        constexpr int NotificationPillRadius = 3;                   // Card.qss:68 TOKENIZE: CardNotificationPillRadius

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
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! AzToolsFramework--PropertyRowWidget QLabel#DefaultLabel: the text a container or an unhandled row shows in place of a
        //! control, 16px high, with the 1px above and below Text.qss gave every label
        const BoxElement PropertyRowDefaultLabel{
            .m_name = "Global.Box.PropertyRowDefaultLabel",
            .m_margin = QMargins(0, Foundations::Label::MarginV, 0, Foundations::Label::MarginV),
            .m_minSize = RowLabelMinSize,
            .m_maxSize = RowLabelMaxSize,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! The name label of a property a prefab instance overrides (the `overridden` property of PrefabOverrideLabelHandler's
        //! ElidingLabel, set in RefreshUI, which repolishes the label): bold over the QLabel's font. With style sheets the handler
        //! gives the label an inline sheet; flattened, none. A label not overridden keeps the QLabel's Global element.
        const TextElement OverriddenPropertyLabel{
            .m_name = "Global.Text.OverriddenPropertyLabel",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = Foundations::Label::Size,
            .m_weight = OverriddenWeight,
        };

        //! AzQtComponents--CardHeader #Expander, the "tree expander" check box: a margin at the right over QCheckBox.Expander's
        //! `margin: 0px`, and a box of 16x16. Its caret is the indicator's.
        const BoxElement CardHeaderExpander{
            .m_name = "Global.Box.CardHeaderExpander",
            .m_margin = ExpanderMargin,
            .m_minSize = IndicatorElement::ExpanderMinSize,
            .m_maxSize = IndicatorElement::ExpanderMaxSize,
            .m_idle = {
                .m_fill = QBrush(ExpanderFill),
            },
        };

        //! AzQtComponents--CardHeader #Icon: the component's icon, a margin at the right, the 1px above and below Text.qss gave every
        //! label, and `border: none` (Card.qss:220 $line.label.idle.style.Alt1). Its `width` and `height` set nothing (Card.qss:217-218):
        //! a label sizes itself from its pixmap.
        const BoxElement CardHeaderIcon{
            .m_name = "Global.Box.CardHeaderIcon",
            .m_margin = QMargins(0, Foundations::Label::MarginV, IconMarginRight, Foundations::Label::MarginV),
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(IconFill),
            },
        };

        //! AzQtComponents--CardHeader #WarningIcon: the icon of a header with a warning, with a margin at the left as well
        const BoxElement CardHeaderWarningIcon{
            .m_name = "Global.Box.CardHeaderWarningIcon",
            .m_margin = QMargins(WarningIconMarginLeft, Foundations::Label::MarginV, IconMarginRight, Foundations::Label::MarginV),
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

        //! AzQtComponents--CardHeader * (Card.qss:86-89) on a frame below a header that no rule of its own names: filled #555555, as
        //! the rule fills it over the header's own fill. The background frame of a header names itself (#Background, Card.qss:91-99,
        //! which is transparent); EMotionFX's RagdollCardHeader clears that name (RagdollNodeWidget.cpp:37), so the frame has the fill
        //! of the rule that reaches every widget. A frame has no margin or padding of its own, and no rule gives it any.
        const BoxElement CardHeaderFrame{
            .m_name = "Global.Box.CardHeaderFrame",
            .m_idle = {
                .m_fill = QBrush(HeaderBodyFill),
            },
        };

        //! The text of a tip shown for a widget of a modified card header: `CardHeader[contentModified=true] * { color: #E57829 }`
        //! (Card.qss:172-175) reaches the tip through the widget it is shown for (QStyleSheetStyle::styleRules, `_q_stylesheet_parent`),
        //! with a specificity over QToolTip's (ToolTip.qss:12). The tip keeps the font of the QToolTip rule (ToolTip.qss:16, :19) and the
        //! weight of '*' (BaseStyleSheet.qss:20), which this element restates: it replaces the tooltip class element whole.
        const TextElement CardHeaderModifiedToolTipText{
            .m_name = "Global.Text.CardHeaderModifiedToolTip",
            .m_pixelSize = ToolTipTextSize,
            .m_color = ModifiedTextColor,
        };

        //! The tips of a modified card header and of the widgets in it
        const ToolTipElement CardHeaderModifiedToolTip{
            .m_name = "Global.ToolTip.CardHeaderModified",
            .m_text = &CardHeaderModifiedToolTipText,
        };

        //! AzQtComponents--CardNotification: the block under a notification (Card.qss:51-55), filled #555555. Its `spacing` sets nothing
        //! for a frame (QStyleSheetStyle::pixelMetric reads it for the items of tool bars and menu bars and the labels of check boxes and
        //! radio buttons), but it gives the rule a box of zero margins and paddings (QRenderRule::hasBox). The border is the style's own.
        const BoxElement NotificationBlock{
            .m_name = "Global.Box.CardNotification",
            .m_spacing = NotificationSpacing,
            .m_idle = {
                .m_fill = QBrush(NotificationFill),
            },
        };

        //! AzQtComponents--CardNotification #HeaderFrame (Card.qss:57-61, :63-70): the pill that holds the icon and the message, a
        //! padding of 8 at each side (the frame's contents, which the layout inside fills), #444444 with round corners and no border
        const BoxElement NotificationPill{
            .m_name = "Global.Box.CardNotificationPill",
            .m_padding = NotificationHeaderPadding,
            .m_spacing = NotificationSpacing,
            .m_nativeBorder = false,                                // Card.qss:68-69 border-radius, border: none
            .m_radius = NotificationPillRadius,
            .m_idle = {
                .m_fill = QBrush(NotificationPillFill),
            },
        };

        //! AzQtComponents--CardNotification #Icon and #Title (Card.qss:63-70): the labels of the pill, filled and rounded as it is (the
        //! fill is not seen over the pill's), with the 1px above and below Text.qss:93-94 gave every label, a box of their own now
        const BoxElement NotificationLabel{
            .m_name = "Global.Box.CardNotificationLabel",
            .m_margin = QMargins(0, Foundations::Label::MarginV, 0, Foundations::Label::MarginV),
            .m_nativeBorder = false,                                // Card.qss:68-69 border-radius, border: none
            .m_radius = NotificationPillRadius,
            .m_idle = {
                .m_fill = QBrush(NotificationPillFill),
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

        //! `AzQtComponents--CardNotification W`
        bool inCardNotification(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::CardNotification");
        }

        //! `AzQtComponents--CardHeader * { }` for a frame that is not a label: a frame with no object name, below a header. (A label
        //! has the box of its typography tier, which an element would replace; no label below a header lacks a name.) The one such
        //! frame is the header's background frame when a header clears its name (RagdollCardHeader), which the header makes as its
        //! own child (CardHeader.cpp:104): the parent decides it. A walk to the root would run for every unnamed frame of the Editor,
        //! every row of a property editor among them.
        bool isUnnamedHeaderFrame(const QWidget* widget)
        {
            return widget->objectName().isEmpty() && !qobject_cast<const QLabel*>(widget)
                && Selectors::childOfClass(widget, "AzQtComponents::CardHeader");
        }

        //! `AzQtComponents--CardHeader[contentModified=true]`: the header, whose rows are on its class
        bool isModifiedCardHeader(const QWidget* widget)
        {
            return Selectors::hasProperty(widget, "contentModified", "true");
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

        // `[overridden="true"]`: the handler's sheet sat on its label only; no other widget has the property
        TextElement::registerElement(&OverriddenPropertyLabel, &ElidingLabel::staticMetaObject,
            [](const QWidget* w) { return Selectors::hasProperty(w, "overridden", "true"); });

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
        BoxElement::registerElement(&CardHeaderFrame, &QFrame::staticMetaObject, &isUnnamedHeaderFrame, global);

        // The tips of the widgets of a header that is modified (the rule reaches them through the widget they are shown for)
        ToolTipElement::registerElement(&CardHeaderModifiedToolTip, &CardHeader::staticMetaObject, &isModifiedCardHeader, global);

        // A notification of a card: the type selector covers the class and what derives from it, the ids any depth below it
        BoxElement::registerElement(&NotificationBlock, &CardNotification::staticMetaObject, [](const QWidget*) { return true; }, global);
        BoxElement::registerElement(&NotificationPill, &QFrame::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "HeaderFrame") && inCardNotification(w); }, global);
        BoxElement::registerElement(&NotificationLabel, label,
            [](const QWidget* w)
            {
                return (Selectors::named(w, "Icon") || Selectors::named(w, "Title")) && inCardNotification(w);
            }, global);
    }
} // namespace AzQtComponents
