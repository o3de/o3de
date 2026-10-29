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
    //! The Global elements of the color picker and the color label: the boxes ColorPicker.qss and ColorLabel.qss gave their widgets,
    //! and the title of a modified palette. They cover the picker's scroll area and palette views (no frame), its labels (no margin,
    //! no padding), the separators between its sections, the frames of the color grid, the preview, the swatches and the button that
    //! adds one, the swatch of a color label, and the tool tip offsets of the gradient sliders.
    namespace ColorPickerElements
    {
        //! Registers the elements as Global ones (a module's own element for the same widget is checked first). From
        //! Style::polish(QApplication*), in the flattened GUI only; any other call does nothing.
        AZ_QT_COMPONENTS_API void registerGlobal();

        //! From Style::polish(QWidget*), after the elements of the widget (flattened GUI only): what the sheet did again by itself.
        //! - A color picker resolves again every widget under it that was polished already. A widget is resolved against its parents
        //!   as they are when it is polished, and ColorComponentEdit polishes its label, spin box and slider before they have a parent
        //!   (GetRequiredWidth, ColorComponentSliders.cpp); the sheet matched a widget again whenever it was reparented
        //!   (QWidgetPrivate::inheritStyle).
        //! - A palette card polishes its header again when it is modified or saved: the title of a modified palette has a colour of
        //!   its own, and PaletteCard::setModified made the sheet match it again.
        AZ_QT_COMPONENTS_API void polished(QWidget* widget);

        //! PE_Frame of the color grid, the preview, a swatch and the add button: they ask for it after painting their contents, and
        //! the sheet drew their rule's border over them again (QStyleSheetStyle::drawPrimitive, PE_Frame), in the option's state (a
        //! selected swatch has the wider border). False: none of these elements, and the base style draws the frame.
        AZ_QT_COMPONENTS_API bool drawFrame(const QStyleOption* option, QPainter* painter, const QWidget* widget);
    } // namespace ColorPickerElements
} // namespace AzQtComponents
