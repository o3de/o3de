/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QRect>
#include <QSize>

class QPainter;
class QStyleOptionSpinBox;
class QWidget;

namespace AzQtComponents
{
    //! The spin box: the face of its box in each state and the glyphs of its two buttons, the winners of SpinBox.qss over the entry
    //! rule of BaseStyleSheet.qss, VectorInput.qss (in a vector element) and ColorPicker.qss (in a color picker). The box is drawn as a
    //! rule draws it (QRenderRule::drawRule, with BoxElement's fill and edges), then each button's image; the base style's frame and
    //! arrows are none of it.
    namespace SpinBoxElement
    {
        //! CC_SpinBox of a QSpinBox or a QDoubleSpinBox. False for any other widget, and for a null option.
        AZ_QT_COMPONENTS_API bool draw(const QStyleOptionSpinBox* option, QPainter* painter, const QWidget* widget);

        //! What the sheet added to the base style's size (QStyleSheetStyle::sizeFromContents, CT_SpinBox): the box around the contents in
        //! the state of the option, the padding and the up button (none in a color picker, none for a spin box with NoButtons).
        //! Invalid: not a spin box, or no option.
        AZ_QT_COMPONENTS_API QSize extraSize(const QStyleOptionSpinBox* option, const QWidget* widget);

        //! The edit field of a spin box in the rect of the option: the contents of its box in the state of the option, less the buttons
        //! (QStyleSheetStyle::subControlRect, SC_SpinBoxEditField; none for a spin box with NoButtons). Null: not a spin box, or no option.
        AZ_QT_COMPONENTS_API QRect editFieldRect(const QStyleOptionSpinBox* option, const QWidget* widget);

        //! The up or the down button of a spin box, where the sheet puts it (QStyleSheetStyle::subControlRect, CC_SpinBox: the position
        //! rect of the button's rule): the button's box at the right of the padding box, centered vertically and moved left by its
        //! inset. The box is the glyph's, 6 by 10 (SpinBox.qss:81-82), side by side with the other button, the down one to the left of
        //! the up one (SpinBox.qss:88, :96); in a color picker it is 0 wide (ColorPicker.qss:74-81). Such a rect is not valid and holds
        //! no point, so no press at the right of the field is a button's, and the field is dragged. Null: not a spin box.
        AZ_QT_COMPONENTS_API QRect buttonRect(const QStyleOptionSpinBox* option, bool up, const QWidget* widget);

        //! SC_SpinBoxFrame: the rect of the option less the margin of the face the state gives the box (QStyleSheetStyle::subControlRect,
        //! CC_SpinBox: QRenderRule::borderRect). The margin is not the box's: a point in it is the frame's of no spin box
        //! (QCommonStyle::hitTestComplexControl). Null: not a spin box.
        AZ_QT_COMPONENTS_API QRect frameRect(const QStyleOptionSpinBox* option, const QWidget* widget);
    } // namespace SpinBoxElement
} // namespace AzQtComponents
