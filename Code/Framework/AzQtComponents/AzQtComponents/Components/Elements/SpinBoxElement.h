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

        //! What the sheet added to the base style's size (QStyleSheetStyle::sizeFromContents, CT_SpinBox): the box around the contents,
        //! the padding and the up button (none in a color picker). Invalid: not a spin box.
        AZ_QT_COMPONENTS_API QSize extraSize(const QWidget* widget);

        //! The edit field of a spin box in rect: the contents of its box less the buttons (QStyleSheetStyle::subControlRect,
        //! SC_SpinBoxEditField). Null: not a spin box.
        AZ_QT_COMPONENTS_API QRect editFieldRect(const QRect& rect, const QWidget* widget);
    } // namespace SpinBoxElement
} // namespace AzQtComponents
