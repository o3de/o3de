/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

class QPainter;
class QStyleOption;
class QWidget;

namespace AzQtComponents
{
    //! The elements of the color picker and of the color label in the flattened GUI: ColorPicker.qss and ColorLabel.qss, which the
    //! legacy GUI gave the widgets of both, carried as box elements. Each takes the values the legacy GUI finally rendered for the
    //! widgets it covers; ColorPickerElements.cpp names the declaration that grants every one.
    //!
    //! What they cover: the picker's scroll area and the views of its palettes (no frame), its labels (no margin and no padding, so none
    //! of the label box and the automatic text indent that come with it), the separators between its sections, the frames of the color
    //! grid, the preview, the swatches of a palette and the button that adds one (1px #333333, rounded by 2px, 2px #C8ABFF around a
    //! selected swatch), the swatch of a color label (no border at all, the one of a selected swatch apart), the title of a modified
    //! palette, and the offset of the tool tip of its gradient sliders.
    namespace ColorPickerElements
    {
        // ---- Global box elements: registered with BoxElement as Global ones, so a module's own element for the same widget is
        // checked first
        //! Registers the elements. From Style::polish(QApplication*), in the flattened GUI only; a call in the legacy GUI does
        //! nothing, and so does every call after the first.
        AZ_QT_COMPONENTS_API void registerGlobal();

        //! From Style::polish(QWidget*), after the elements of the widget; the flattened GUI only. What the sheet did again by itself:
        //! - a color picker resolves again every widget under it that was polished already. The flattened GUI resolves an element when a
        //!   widget is polished, against the widgets above it as they are then, and ColorComponentEdit polishes itself and all that is in it,
        //!   its label, spin box and slider (GetRequiredWidth, ColorComponentSliders.cpp:39), while the sliders it sits in have no parent: no
        //!   rule of the picker reached them. The legacy sheet matched a widget against its parents again whenever it was reparented
        //!   (QWidgetPrivate::inheritStyle, qwidget.cpp:2727-2770).
        //! - a palette card polishes its header again when it is modified or saved: ColorPicker.qss:61-64 colors the title of a modified
        //!   palette, and PaletteCard::setModified made the sheet match it again (PaletteCard.cpp:113).
        //! Any other widget is left as it is.
        AZ_QT_COMPONENTS_API void polished(QWidget* widget);

        // ---- the frame a widget draws over its contents. The color grid, the preview, the swatch and the add button ask the style for
        // PE_Frame once they have painted their contents; the sheet drew the border of their rule over them, after the one it drew with
        // the styled background (QStyleSheetStyle::drawPrimitive, PE_Frame: the border of the rule, none where it is the style's own).
        //! From Style::drawPrimitive for PE_Frame: the border of the element of the widget, in the state of the option (a swatch that
        //! is selected has the wider one). False: the widget has none of the elements above (the base style draws its frame), and in
        //! the legacy GUI.
        AZ_QT_COMPONENTS_API bool drawFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget);
    } // namespace ColorPickerElements
} // namespace AzQtComponents
