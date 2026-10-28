/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ColorPickerElements.h>
#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
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
#include <QScrollArea>
#include <QStyleOption>
#include <QWidget>

namespace AzQtComponents
{
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ColorPickerElementsInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------


        // ---- The scroll area, the palette views and the labels of the picker
        const QColor ScrollAreaFill = Foundations::Clear;           // ColorPicker.qss:42 $fill.surface.idle.Clear
        const QColor PaletteViewFill = Foundations::Clear;          // ColorPicker.qss:36 $fill.view.idle.fill.Clear
        constexpr int PaletteTitleWeight = 400;                     // ColorPicker.qss:58 (normal) $type.label.idle.weight.Alt2
        const QColor ModifiedPaletteTitleText(0xFF, 0xA5, 0x00);    // ColorPicker.qss:63 $text.label.modified

        // ---- The separators between its sections
        const QColor SeparatorFill = Foundations::BG;               // ColorPicker.qss:89 $fill.surface.idle.BG
        // ColorPicker.qss:85-86 $line.surface.idle.width.Alt1 $line.surface.idle.color.Alt2 (the top and the bottom edge alike)
        const BoxElement::Edge SeparatorLine{ 1, QColor(0x39, 0x3A, 0x3C) };

        // ---- The frames of the color grid, the preview, the swatches and the button that adds one
        // ColorPicker.qss:108 $line.surface.idle.width.Alt1 $line.surface.idle.color.Low
        const BoxElement::Edge FrameLine{ 1, Foundations::Low };
        constexpr int FrameRadius = 2;                              // ColorPicker.qss:109 $radius.surface.idle.all.Alt3
        // ColorPicker.qss:115 $line.surface.selected.width $line.surface.selected.color
        const BoxElement::Edge SelectedSwatchLine{ 2, QColor(0xC8, 0xAB, 0xFF) };

        // ---- The tool tip of a gradient slider: its offset from the pointer
        constexpr int ToolTipOffsetX = 0;                           // ColorPicker.qss:120 $other.slider.idle.tooltipoffsetx.Alt3
        constexpr int ToolTipOffsetY = -24;                         // ColorPicker.qss:121 $other.slider.idle.tooltipoffsety.Alt2
        constexpr int TallToolTipOffsetX = 0;                       // ColorPicker.qss:127 $other.slider.idle.tooltipoffsetx.Alt3
        constexpr int TallToolTipOffsetY = -40;                     // ColorPicker.qss:128 $other.slider.idle.tooltipoffsety.Alt3
        constexpr int VerticalToolTipOffsetX = 8;                   // ColorPicker.qss:133 $other.slider.idle.tooltipoffsetx.Alt2
        constexpr int VerticalToolTipOffsetY = -24;                 // ColorPicker.qss:134 $other.slider.idle.tooltipoffsety.Alt2

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! AzQtComponents--ColorPicker QScrollArea: transparent, and `border: none` (ColorPicker.qss:41 $line.surface.idle.style.Alt1),
        //! a border of its own: the base style's frame (the 1px line of a StyledPanel) is not drawn and takes no room, so the viewport
        //! fills the widget
        const BoxElement PickerScrollArea{
            .m_name = "Global.Box.PickerScrollArea",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(ScrollAreaFill),
            },
        };

        //! AzQtComponents--PaletteView, transparent, and `border: none` in a picker (ColorPicker.qss:99 $line.view.idle.style.Alt1): no
        //! frame and no colour of its own, so its viewport fills it. Style::polish takes the frame off every item view; the element
        //! keeps the base style's frame off a view that is given one.
        const BoxElement PickerPaletteView{
            .m_name = "Global.Box.PickerPaletteView",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(PaletteViewFill),
            },
        };

        //! AzQtComponents--ColorPicker QLabel: `padding: 0px; margin: 0px` (ColorPicker.qss:47-48 $space.label.idle.padding
        //! $space.label.idle.margin.Alt1). Its two type names outrank Text.qss's `QLabel`, so the 1px above and below that rule gave
        //! every label is gone, and with it the label's frame box: a QLabel with a frame width indents its text by half an 'x'
        //! (QLabelPrivate::sizeForWidth). A margin and a padding, even of 0, give the label a box whose contents are all of it.
        const BoxElement PickerLabel{
            .m_name = "Global.Box.PickerLabel",
            .m_hasBox = true,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! AzQtComponents--PaletteCard[modified="true"] AzQtComponents--CardHeader #Title: the title of a palette with unsaved changes,
        //! orange, in the picker's size and the weight a palette card gives its title
        const TextElement ModifiedPaletteTitle{
            .m_name = "Global.Text.PickerModifiedPaletteTitle",
            .m_family = Foundations::Label::Family,
            .m_pixelSize = Foundations::Surface::Size,
            .m_weight = PaletteTitleWeight,
            .m_color = ModifiedPaletteTitleText,
        };

        //! AzQtComponents--ColorPicker .HorizontalSeparator: a QFrame HLine (ColorPicker::makeSeparator) with a line above and below
        //! and none at the sides (ColorPicker.qss:87-88 $line.surface.idle.style.Alt1), filled between. A class tag stands in for the
        //! class name in a match (QStyleSheetStyleSelector::attributeValue), so BaseStyleSheet.qss's `.QFrame[frameShape]` fill never
        //! reached it. Its box is the two lines. The sheet drew it once, with the styled background (QStyleSheetStyle::drawControl
        //! draws no CE_ShapedFrame for a border of the rule's own).
        const BoxElement PickerSeparator{
            .m_name = "Global.Box.PickerSeparator",
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(SeparatorFill),
                .m_edges = { SeparatorLine, {}, SeparatorLine, {} },
            },
        };

        //! AzQtComponents--ColorGrid, --ColorPreview, --Swatch and --ColorPicker #addColorButton: a rounded border and no fill (a swatch
        //! paints its own colour). The border is the box: the grid's pixmap and the colours of a preview are laid out inside it. The
        //! sheet drew the border twice, with the styled background and at the PE_Frame these widgets ask for after their contents
        //! (drawFrame).
        const BoxElement PickerFrame{
            .m_name = "Global.Box.PickerFrame",
            .m_nativeBorder = false,
            .m_radius = FrameRadius,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
                .m_edges = { FrameLine, FrameLine, FrameLine, FrameLine },
            },
        };

        //! AzQtComponents--Swatch:selected: the wider border over the frames' rule, which keeps its radius. Only the border changes: the
        //! contents rect, which the swatch fills with its colour, is the box Qt read at polish (the state Enabled alone), 1px in. A
        //! pseudo-class counts more than any number of type names (Selector::specificity), so the rule outranks the frames' and the
        //! color label swatch's alike.
        const BoxElement PickerSelectedSwatch{
            .m_name = "Global.Box.PickerSelectedSwatch",
            .m_nativeBorder = false,
            .m_radius = FrameRadius,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
                .m_edges = { SelectedSwatchLine, SelectedSwatchLine, SelectedSwatchLine, SelectedSwatchLine },
            },
        };

        //! AzQtComponents--ColorLabel AzQtComponents--Swatch: no margin, no padding (ColorLabel.qss:18-19 $space.surface.idle.margin.Alt2
        //! $space.surface.idle.padding.Alt6) and `border: 0px` (ColorLabel.qss:24 $line.surface.idle.width.Alt3), over the frames' rule
        //! (the later, more specific one): a flat box of its colour, with no room for a frame. Its size is set by ColorLabel. A
        //! selected swatch has PickerSelectedSwatch's border all the same.
        const BoxElement LabelSwatch{
            .m_name = "Global.Box.PickerLabelSwatch",
            .m_hasBox = true,
            .m_nativeBorder = false,
            .m_idle = {
                .m_fill = QBrush(Foundations::Surface::Fill),
            },
        };

        //! AzQtComponents--GradientSlider.HorizontalSlider: the tool tip above the pointer
        const BoxElement HorizontalSliderToolTip{
            .m_name = "Global.Box.PickerHorizontalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", ToolTipOffsetX },
                { "toolTipOffsetY", ToolTipOffsetY },
            },
        };

        //! The same in the HSL and HSV tabs (AzQtComponents--HSLSliders and --HSVSliders): taller sliders, the tool tip higher (the
        //! more specific rule, so it is checked first)
        const BoxElement TallHorizontalSliderToolTip{
            .m_name = "Global.Box.PickerTallHorizontalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", TallToolTipOffsetX },
                { "toolTipOffsetY", TallToolTipOffsetY },
            },
        };

        //! AzQtComponents--GradientSlider.VerticalSlider: the tool tip right of the pointer and above it
        const BoxElement VerticalSliderToolTip{
            .m_name = "Global.Box.PickerVerticalSliderToolTip",
            .m_properties = {
                { "toolTipOffsetX", VerticalToolTipOffsetX },
                { "toolTipOffsetY", VerticalToolTipOffsetY },
            },
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! `AzQtComponents--ColorPicker W`
        bool inColorPicker(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::ColorPicker");
        }

        //! `AzQtComponents--ColorLabel W`
        bool inColorLabel(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::ColorLabel");
        }

        //! A label of the picker whose box another rule gives: a tier of Text.qss (`.Headline`, `.Title`, ... outrank the picker's two
        //! type names: its margin above and below stays), or the icons of a card header (their ids outrank both: PropertyEditorElements)
        bool hasOtherLabelBox(const QWidget* widget)
        {
            for (const char* tier : { "Headline", "Title", "Subtitle", "Menu", "Label", "Paragraph", "Tooltip", "Button" })
            {
                if (Selectors::tagged(widget, tier))
                {
                    return true;
                }
            }
            return (Selectors::named(widget, "Icon") || Selectors::named(widget, "WarningIcon"))
                && Selectors::withinClass(widget, "AzQtComponents::CardHeader");
        }

        //! `AzQtComponents--HSLSliders W` and `AzQtComponents--HSVSliders W`
        bool inHslOrHsvSliders(const QWidget* widget)
        {
            return Selectors::withinClass(widget, "AzQtComponents::HSLSliders")
                || Selectors::withinClass(widget, "AzQtComponents::HSVSliders");
        }

        //! `AzQtComponents--PaletteCard[modified="true"] AzQtComponents--CardHeader #Title`: the title of the header of the nearest palette
        //! card above, if it is modified (an attribute selector compares the value's text: a bool reads "true")
        bool isTitleOfModifiedPaletteCard(const QWidget* widget)
        {
            if (!Selectors::named(widget, "Title") || !Selectors::withinClass(widget, "AzQtComponents::CardHeader"))
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

        //! The dynamic property that marks a palette card whose modified state is watched
        constexpr const char* WatchedProperty = "_o3de_pickerModifiedWatch";

        //! The sheet matched `[modified="true"]` again whenever the property changed (PaletteCard::setModified recomputes its header's
        //! sheet, which the flattened GUI has none of): the header and everything in it are polished again, as
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

        constexpr ElementScope global = ElementScope::Global;
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
                return w->metaObject() == &QFrame::staticMetaObject && Selectors::tagged(w, "HorizontalSeparator") && inColorPicker(w);
            },
            global);

        // The swatch of a color label first: its rule is the more specific of the two that reach a swatch
        BoxElement::registerElement(&LabelSwatch, &Swatch::staticMetaObject, [](const QWidget* w) { return inColorLabel(w); }, global);
        BoxElement::registerElement(&PickerFrame, &Swatch::staticMetaObject, always, global);
        BoxElement::registerElement(&PickerFrame, &ColorGrid::staticMetaObject, always, global);
        BoxElement::registerElement(&PickerFrame, &ColorPreview::staticMetaObject, always, global);
        BoxElement::registerElement(&PickerFrame, &Internal::AddSwatch::staticMetaObject,
            [](const QWidget* w) { return Selectors::named(w, "addColorButton") && inColorPicker(w); }, global);

        // The sliders of the HSL and HSV tabs before the others: of two rules that reach a slider the more specific wins
        BoxElement::registerElement(&TallHorizontalSliderToolTip, &GradientSlider::staticMetaObject,
            [](const QWidget* w) { return Selectors::tagged(w, "HorizontalSlider") && inHslOrHsvSliders(w); }, global);
        BoxElement::registerElement(&HorizontalSliderToolTip, &GradientSlider::staticMetaObject,
            [](const QWidget* w) { return Selectors::tagged(w, "HorizontalSlider"); }, global);
        BoxElement::registerElement(&VerticalSliderToolTip, &GradientSlider::staticMetaObject,
            [](const QWidget* w) { return Selectors::tagged(w, "VerticalSlider"); }, global);

        // The title of a modified palette: a text element, checked before the Global ones
        TextElement::registerElement(&ModifiedPaletteTitle, &QLabel::staticMetaObject, isTitleOfModifiedPaletteCard);
    }

    // ----------------------------------------------------------------------------------
    // After a widget is polished
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
            // An element that does not change leaves the widget as it was; a widget not polished yet is resolved when it is (the
            // picker's spin boxes are read when they are drawn: SpinBoxElement::draw)
            const QList<QWidget*> descendants = widget->findChildren<QWidget*>();
            for (QWidget* descendant : descendants)
            {
                if (descendant->testAttribute(Qt::WA_WState_Polished))
                {
                    BoxElement::resolve(descendant);
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

        // PE_Frame draws the border of the rule of the state (QStyleSheetStyle::drawPrimitive). A swatch the view gives State_Selected
        // takes the selected border whichever of the two elements it has (`Swatch:selected` outranks both); the grid, the preview and
        // the add button are no swatch, and a state of that name does nothing to them.
        const bool selected = qobject_cast<const Swatch*>(widget) && option->state.testFlag(QStyle::State_Selected);
        (selected ? PickerSelectedSwatch : *element).draw(painter, option, widget);
        return true;
    }
} // namespace AzQtComponents
