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
class QStyleOptionSpinBox;
class QWidget;

namespace AzQtComponents
{
    //! The spin box of the flattened GUI: the face its box has in each state, and the glyphs of its two buttons. Every value is
    //! the one the legacy GUI finally rendered, the winner of the cascade of SpinBox.qss over BaseStyleSheet.qss (the entry
    //! controls' rule), VectorInput.qss (a spin box of a vector element) and ColorPicker.qss (a spin box of a color picker);
    //! SpinBoxElement.cpp names the declaration that grants each, and the specificity that decides where rules meet.
    //!
    //! The legacy sheet drew the box as a rule draws it (QRenderRule::drawRule): the fill, a rounded one where the rule has a
    //! radius (a path, antialiased), the background image, the border; then the image of each button, centred in the rect the
    //! sheet's position gave it. The base style's frame and arrows are none of it.
    namespace SpinBoxElement
    {
        //! CC_SpinBox of a QSpinBox or a QDoubleSpinBox, in the flattened GUI. False for any other widget (and for a null option)
        AZ_QT_COMPONENTS_API bool draw(const QStyleOptionSpinBox* option, QPainter* painter, const QWidget* widget);

        //! The width the sheet style added to the base style's size of a QSpinBox or a QDoubleSpinBox (qstylesheetstyle.cpp:5309-5330):
        //! the up button (6px, none in a color picker) and the box of the rule around the contents, the margin of 2 on each side
        //! (none at the left in a vector element) and the padding of 12 on the right. -1: not our widget
        AZ_QT_COMPONENTS_API int extraWidth(const QWidget* widget);
    } // namespace SpinBoxElement
} // namespace AzQtComponents
