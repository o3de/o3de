/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/TextElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Swatch.h>
#include <AzQtComponents/Components/Widgets/GradientSlider.h>

#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QMargins>
#include <QScrollArea>
#include <QSize>
#include <QStyleOption>
#include <QWidget>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file with others
    //! into one translation unit, where anonymous namespaces merge (TextElement.cpp does the same).
    namespace ColorPickerElementsInternal
    {
        // ------------------------------------------------------------------------------
        // The scroll area of the picker: ColorPicker.qss over its own `*` rule
        // ------------------------------------------------------------------------------

        //! AzQtComponents--ColorPicker QScrollArea (ColorPicker.qss:39-43): `border: none`, a border of its own, so the frame of the base
        //! style (the 1px line of a StyledPanel) is not drawn, takes no room, and the viewport fills the widget: n2 07_color-picker__dialog0
        //! has the scroll area 302 x 531 with contents margins 0, the viewport at its origin (the flat one was 304 x 539, margins 1). A
        //! transparent background: what the dialog is painted with shows through.
        const BoxElement PickerScrollArea{
            .m_name = "Global.Box.PickerScrollArea",
            .m_nativeBorder = false,                                // ColorPicker.qss:41
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // ColorPicker.qss:42
            },
        };

        //! AzQtComponents--PaletteView (ColorPicker.qss:33-37, `background-color: transparent`) and AzQtComponents--ColorPicker
        //! AzQtComponents--PaletteView (:97-100, `border: none`): the view of a palette (only the picker's palette cards have one) has no
        //! frame and no color of its own, so its viewport fills it and what the dialog is painted with shows through (n2
        //! 07_color-picker__dialog0: the view 270 x 16, contents margins 0, the viewport at its origin and as big). Style::polish takes the
        //! frame off every item view, so the element changes nothing there; it is what keeps the base style's frame (1px, a StyledPanel)
        //! off a view that is given one.
        const BoxElement PickerPaletteView{
            .m_name = "Global.Box.PickerPaletteView",
            .m_nativeBorder = false,                                // ColorPicker.qss:99
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // ColorPicker.qss:36
            },
        };

        // ------------------------------------------------------------------------------
        // The labels of the picker: ColorPicker.qss over Text.qss
        // ------------------------------------------------------------------------------

        //! AzQtComponents--ColorPicker QLabel (ColorPicker.qss:45-49): `padding: 0px; margin: 0px`. Its selector has two type names and
        //! outranks Text.qss:89's `QLabel` (one), so the 1px above and below Text.qss gave every label are gone, and with them the label's
        //! frame box: a QLabel with a frame width adds half an 'x' of indent to its text (qlabel.cpp, QLabelPrivate::sizeForWidth), which
        //! is what made the flat labels 2px taller and 6px wider than Native's (n2 07_color-picker__dialog0: the label of the hex field
        //! 8 x 17 with contents margins 0, flat 14 x 19; the title of the quick palette 74 x 17, flat 80 x 21). A rule that gives a margin
        //! and a padding, even 0, gives the label a box (hasBox) whose contents are all of it.
        const BoxElement PickerLabel{
            .m_name = "Global.Box.PickerLabel",
            .m_hasBox = true,                                       // margin and padding of 0: ColorPicker.qss:47-48
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // BaseStyleSheet.qss:17 (*)
            },
        };

        //! AzQtComponents--PaletteCard[modified="true"] AzQtComponents--CardHeader #Title (ColorPicker.qss:61-64): the title of a palette that
        //! has unsaved changes is orange, in the weight a palette card gives its title (:56-59: normal, as TextElement's Body, which the
        //! 12px of :16-18 makes it). UNVERIFIED by capture: no n2 screen has a palette card; the legacy twin is the proof.
        //! TOKENIZE: PickerModifiedPaletteTitle
        const TextElement ModifiedPaletteTitle{
            .m_name = "Global.Text.PickerModifiedPaletteTitle",
            .m_pixelSize = 12,                                      // ColorPicker.qss:18
            .m_color = QColor(0xFF, 0xA5, 0x00),                    // ColorPicker.qss:63
        };

        // ------------------------------------------------------------------------------
        // The separators between the sections of the picker
        // ------------------------------------------------------------------------------

        const QColor SeparatorFill(0x22, 0x22, 0x22);               // ColorPicker.qss:89. TOKENIZE: PickerSeparatorFill
        const QColor SeparatorLine(0x39, 0x3A, 0x3C);               // ColorPicker.qss:85-86. TOKENIZE: PickerSeparatorLine

        //! AzQtComponents--ColorPicker .HorizontalSeparator (ColorPicker.qss:83-90): a QFrame HLine (ColorPicker.cpp:1331-1335) with
        //! a 1px line above and below and none at the sides, #222222 between. A class tag replaces the class name in the selector match
        //! (qstylesheetstyle.cpp:1545-1551), so BaseStyleSheet.qss's `.QFrame[frameShape]` fill never reached it. Its box is the two
        //! lines: contents margins 0, 1, 0, 1 (n2: the flat frame had 1 on every side). The sheet draws it once, with the styled
        //! background (CE_ShapedFrame draws nothing for a border of the rule's own, qstylesheetstyle.cpp:4502-4511); the lines are
        //! antialiased fills, so at a fractional device ratio the edge rows blend (the profile of the n2 rows is the one of the tests).
        const BoxElement PickerSeparator{
            .m_name = "Global.Box.PickerSeparator",
            .m_nativeBorder = false,                                // ColorPicker.qss:85-88
            .m_idle = {
                .m_fill = QBrush(SeparatorFill),                    // ColorPicker.qss:89
                .m_edges = { { 1, SeparatorLine }, {}, { 1, SeparatorLine }, {} },  // ColorPicker.qss:85 (top), :86 (bottom); :87-88 (none)
            },
        };

        // ------------------------------------------------------------------------------
        // The frames of the color grid, the preview, the swatches and the button that adds one
        // ------------------------------------------------------------------------------

        const QColor FrameLine(0x33, 0x33, 0x33);                   // ColorPicker.qss:108. TOKENIZE: PickerFrameLine
        const QColor SelectedSwatchLine(0xC8, 0xAB, 0xFF);          // ColorPicker.qss:115. TOKENIZE: PickerSelectedSwatchLine
        constexpr int FrameRadius = 2;                              // ColorPicker.qss:109. TOKENIZE: PickerFrameRadius

        //! AzQtComponents--ColorGrid, --ColorPreview, --Swatch and --ColorPicker #addColorButton (ColorPicker.qss:102-110): `border: 1px
        //! solid #333333; border-radius: 2px`, and no fill (the comment of :107 says a swatch ignores any background set there: it paints
        //! its own color). The border is the box: contents margins 1 on every side (n2: grid, preview and add button 1, flat 0; the
        //! preview 218 x 22 with the hint 202 x 22, flat 218 x 20), so the grid's pixmap and the colors of a preview are laid out inside
        //! it. The sheet draws the border twice, once with the styled background and once at the PE_Frame these widgets ask for after
        //! their contents (drawFrame): at 1.5 the half row of the line is two 50% draws, 76% in n2.
        const BoxElement PickerFrame{
            .m_name = "Global.Box.PickerFrame",
            .m_nativeBorder = false,                                // ColorPicker.qss:108-109
            .m_radius = FrameRadius,                                // ColorPicker.qss:109
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // BaseStyleSheet.qss:17 (*)
                .m_edges = BoxElement::border(1, FrameLine),        // ColorPicker.qss:108
            },
        };

        //! AzQtComponents--Swatch:selected (ColorPicker.qss:112-116): `border: 2px solid #C8ABFF` over the rule above, which keeps its
        //! radius. Only the border of a selected swatch is wider: the contents rect, which the swatch fills its color in, is the box
        //! Qt read when the widget was polished (the state Enabled alone), 1px in. A pseudo-class counts more than any number of type
        //! names (qcssparser.cpp:1979-1991: 0x10 against 1 each), so this rule outranks the one of the frames (:102-105) and the one of
        //! the swatch of a color label (ColorLabel.qss:16-25) alike: LabelSwatch has no border while the swatch is not selected, and
        //! this one while it is. UNVERIFIED: no capture shows a selected swatch; the tests' sheet twin is the proof of the numbers.
        const BoxElement PickerSelectedSwatch{
            .m_name = "Global.Box.PickerSelectedSwatch",
            .m_nativeBorder = false,                                // ColorPicker.qss:115
            .m_radius = FrameRadius,                                // ColorPicker.qss:109
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // BaseStyleSheet.qss:17 (*)
                .m_edges = BoxElement::border(2, SelectedSwatchLine),   // ColorPicker.qss:115
            },
        };

        //! AzQtComponents--ColorLabel AzQtComponents--Swatch (ColorLabel.qss:16-25): the swatch of a color label has no border (`border:
        //! 0px`, over the rule above: the later, more specific one), no margin and no padding, so it is a flat box of its color. The flat
        //! swatch drew the base style's 1px frame over it, and at 1.5 the frame landed on the bottom and right rows the color did not
        //! cover (n2 06_color-label: 48 x 24 device pixels of #ff0000, nothing around them). Its size, 32 x 16 (ColorLabel.qss:20-23), is
        //! set by ColorLabel.cpp. The border is the rule's own, none, not the style's: the box has no room for a frame the swatch is given.
        //! A selected swatch (the palette view of a picker opened from the label paints its swatches so, and the dialog has the label
        //! among its parents) has the border of PickerSelectedSwatch all the same.
        const BoxElement LabelSwatch{
            .m_name = "Global.Box.PickerLabelSwatch",
            .m_hasBox = true,                                       // margin and padding of 0: ColorLabel.qss:18-19
            .m_nativeBorder = false,                                // ColorLabel.qss:24
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                  // BaseStyleSheet.qss:17 (*)
            },
        };

        // ------------------------------------------------------------------------------
        // The tool tip of a gradient slider: qproperty-* of ColorPicker.qss
        // ------------------------------------------------------------------------------

        //! AzQtComponents--GradientSlider.HorizontalSlider (ColorPicker.qss:118-122): the tool tip 24px above the pointer. The offsets of
        //! the three elements are UNVERIFIED by capture (no n2 screen shows a tool tip); the tests' sheet twin is the proof of the numbers.
        const BoxElement HorizontalSliderToolTip{
            .m_name = "Global.Box.PickerHorizontalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", 0 },                            // ColorPicker.qss:120
                { "toolTipOffsetY", -24 },                          // ColorPicker.qss:121
            },
        };

        //! AzQtComponents--HSLSliders AzQtComponents--GradientSlider.HorizontalSlider, and the same of the HSV sliders (ColorPicker.qss:124-129):
        //! the sliders of those two tabs are taller, so their tool tip is 40px above it (the more specific rule, so it is checked first)
        const BoxElement TallHorizontalSliderToolTip{
            .m_name = "Global.Box.PickerTallHorizontalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", 0 },                            // ColorPicker.qss:127
                { "toolTipOffsetY", -40 },                          // ColorPicker.qss:128
            },
        };

        //! AzQtComponents--GradientSlider.VerticalSlider (ColorPicker.qss:131-135): the tool tip 8px right of the pointer, 24px above it
        const BoxElement VerticalSliderToolTip{
            .m_name = "Global.Box.PickerVerticalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", 8 },                            // ColorPicker.qss:133
                { "toolTipOffsetY", -24 },                          // ColorPicker.qss:134
            },
        };

        // ------------------------------------------------------------------------------
        // Selectors: what each element covers is what its rule's selector covered
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--ColorPicker W`
        bool inColorPicker(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzQtComponents::ColorPicker");
        }

        //! `AzQtComponents--ColorLabel W`
        bool inColorLabel(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzQtComponents::ColorLabel");
        }

        //! A label of the picker whose box another rule gives: a tier of Text.qss (`.Headline`, `.Title`, ... [class~=tag], which outranks
        //! the picker's two type names: its margin above and below stays, Style.cpp's flattenedLabelFrameContents gives it), or the
        //! icons of a card header (their ids outrank both, Card.qss:214-227: PropertyEditorElements has them)
        bool hasOtherLabelBox(const QWidget* widget)
        {
            for (const char* tier : { "Headline", "Title", "Subtitle", "Menu", "Label", "Paragraph", "Tooltip", "Button" })
            {
                if (TextElement::tagged(widget, tier))
                {
                    return true;
                }
            }
            return (TextElement::named(widget, "Icon") || TextElement::named(widget, "WarningIcon"))
                && TextElement::withinClass(widget, "AzQtComponents::CardHeader");
        }

        //! `AzQtComponents--HSLSliders W` and `AzQtComponents--HSVSliders W`
        bool inHslOrHsvSliders(const QWidget* widget)
        {
            return TextElement::withinClass(widget, "AzQtComponents::HSLSliders") || TextElement::withinClass(widget, "AzQtComponents::HSVSliders");
        }

        //! `AzQtComponents--PaletteCard[modified="true"] AzQtComponents--CardHeader #Title`: the title of the header of the nearest palette
        //! card above, if it is modified (a property selector compares the text of the value: a bool is "true", qcssparser.cpp)
        bool isTitleOfModifiedPaletteCard(const QWidget* widget)
        {
            if (!TextElement::named(widget, "Title") || !TextElement::withinClass(widget, "AzQtComponents::CardHeader"))
            {
                return false;
            }
            for (const QWidget* ancestor = widget->parentWidget(); ancestor; ancestor = ancestor->parentWidget())
            {
                if (ancestor->inherits("AzQtComponents::PaletteCard"))
                {
                    return ancestor->property("modified").toBool();
                }
            }
            return false;
        }

        //! The elements that draw their border at PE_Frame
        bool drawsFrame(const BoxElement* element)
        {
            return element == &PickerFrame || element == &LabelSwatch;
        }

        // ------------------------------------------------------------------------------
        // A palette card whose modified state changes
        // ------------------------------------------------------------------------------

        //! The name of the property that marks a palette card whose changes of state are watched
        constexpr const char* WatchedProperty = "_o3de_pickerModifiedWatch";

        //! The sheet matched `[modified="true"]` again whenever the property changed (PaletteCard::setModified recomputes the sheet of the
        //! header, PaletteCard.cpp:113, which the flattened GUI has none of): the header and everything in it is polished again, as
        //! StyleHelpers::repolishWhenPropertyChanges does for a property of a widget
        void repolishHeaderOf(PaletteCard* card)
        {
            for (CardHeader* header : card->findChildren<CardHeader*>())
            {
                QStyle* style = header->style();
                style->unpolish(header);
                style->polish(header);
                for (QWidget* child : header->findChildren<QWidget*>())
                {
                    style->unpolish(child);
                    style->polish(child);
                }
            }
        }

        void watchModified(PaletteCard* card)
        {
            if (card->property(WatchedProperty).toBool())
            {
                return;
            }
            card->setProperty(WatchedProperty, true);
            QObject::connect(card, &PaletteCard::modifiedChanged, card, [card]() { repolishHeaderOf(card); });
        }
    } // namespace ColorPickerElementsInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void ColorPickerElements::registerGlobal()
    {
        using namespace ColorPickerElementsInternal;

        static bool registered = false;
        if (registered || !StyleManager::stylesheetsDisabled())
        {
            return;
        }
        registered = true;

        constexpr BoxElement::Scope global = BoxElement::Scope::Global;
        const auto always = [](const QWidget*) { return true; };

        BoxElement::registerElement(&PickerScrollArea, &QScrollArea::staticMetaObject,
            [](const QWidget* w) { return inColorPicker(w); }, global);
        BoxElement::registerElement(&PickerPaletteView, &PaletteView::staticMetaObject,
            [](const QWidget* w) { return inColorPicker(w); }, global);
        BoxElement::registerElement(&PickerLabel, &QLabel::staticMetaObject,
            [](const QWidget* w) { return inColorPicker(w) && !hasOtherLabelBox(w); }, global);
        BoxElement::registerElement(&PickerSeparator, &QFrame::staticMetaObject,
            [](const QWidget* w)
            {
                return w->metaObject() == &QFrame::staticMetaObject && TextElement::tagged(w, "HorizontalSeparator") && inColorPicker(w);
            },
            global);

        // The swatch of a color label first: its rule is the more specific of the two that reach a swatch
        BoxElement::registerElement(&LabelSwatch, &Swatch::staticMetaObject, [](const QWidget* w) { return inColorLabel(w); }, global);
        BoxElement::registerElement(&PickerFrame, &Swatch::staticMetaObject, always, global);
        BoxElement::registerElement(&PickerFrame, &ColorGrid::staticMetaObject, always, global);
        BoxElement::registerElement(&PickerFrame, &ColorPreview::staticMetaObject, always, global);
        BoxElement::registerElement(&PickerFrame, &Internal::AddSwatch::staticMetaObject,
            [](const QWidget* w) { return TextElement::named(w, "addColorButton") && inColorPicker(w); }, global);

        // The sliders of the HSL and HSV tabs before the others: of two rules that reach a slider the more specific wins
        BoxElement::registerElement(&TallHorizontalSliderToolTip, &GradientSlider::staticMetaObject,
            [](const QWidget* w) { return TextElement::tagged(w, "HorizontalSlider") && inHslOrHsvSliders(w); }, global);
        BoxElement::registerElement(&HorizontalSliderToolTip, &GradientSlider::staticMetaObject,
            [](const QWidget* w) { return TextElement::tagged(w, "HorizontalSlider"); }, global);
        BoxElement::registerElement(&VerticalSliderToolTip, &GradientSlider::staticMetaObject,
            [](const QWidget* w) { return TextElement::tagged(w, "VerticalSlider"); }, global);

        // The title of a modified palette: a text element, checked before the Global ones
        TextElement::registerElement(&ModifiedPaletteTitle, &QLabel::staticMetaObject, isTitleOfModifiedPaletteCard);
    }

    // ----------------------------------------------------------------------------------
    // Late work, after a widget is polished
    // ----------------------------------------------------------------------------------

    void ColorPickerElements::polished(QWidget* widget)
    {
        using namespace ColorPickerElementsInternal;

        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }

        if (auto* card = qobject_cast<PaletteCard*>(widget))
        {
            watchModified(card);
        }
        else if (widget->inherits("AzQtComponents::ColorPicker"))
        {
            // An element that does not change leaves the widget as it was; a widget not polished yet is resolved when it is. (What the
            // spin boxes of the picker look like inside it is read when they are drawn: SpinBoxElement::draw.)
            const QList<QWidget*> descendants = widget->findChildren<QWidget*>();
            for (QWidget* descendant : descendants)
            {
                if (descendant->testAttribute(Qt::WA_WState_Polished))
                {
                    BoxElement::apply(descendant);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // The frame over the contents
    // ----------------------------------------------------------------------------------

    bool ColorPickerElements::drawFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget)
    {
        using namespace ColorPickerElementsInternal;

        const BoxElement* element = StyleManager::stylesheetsDisabled() ? BoxElement::forWidget(widget) : nullptr;
        if (!drawsFrame(element))
        {
            return false;
        }

        // PE_Frame draws the border of the rule of the state (qstylesheetstyle.cpp:4639-4650); a swatch the view gives State_Selected
        // takes the border of ColorPicker.qss:115 (its rule is read when the swatch draws: it is hidden, the delegate paints with it),
        // whichever of the two elements is the swatch's: `Swatch:selected` outranks both rules (see PickerSelectedSwatch). The grid, the
        // preview and the add button are no swatch: a state of the same name does nothing to them.
        const bool selected = qobject_cast<const Swatch*>(widget) && option->state.testFlag(QStyle::State_Selected);
        (selected ? PickerSelectedSwatch : *element).draw(painter, option, widget);
        return true;
    }
} // namespace AzQtComponents
