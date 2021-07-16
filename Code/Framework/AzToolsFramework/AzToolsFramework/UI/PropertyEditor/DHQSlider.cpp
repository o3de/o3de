/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DHQSlider.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QAbstractSpinBox>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QWheelEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    void DHQSlider::wheelEvent(QWheelEvent* e)
    {
        if (hasFocus())
        {
            QSlider::wheelEvent(e);
        }
        else
        {
            e->ignore();
        }
    }

    void InitializeSliderPropertyWidgets(QSlider* slider, QAbstractSpinBox* spinbox)
    {
        if (slider == nullptr || spinbox == nullptr)
        {
            return;
        }

        // A 2:1 ratio between spinbox and slider gives the slider more room,
        // but leaves some space for the spin box to expand.
        const int spinBoxStretch = 1;
        const int sliderStretch = 2;

        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(spinBoxStretch);
        spinbox->setSizePolicy(sizePolicy);
        spinbox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        spinbox->setFixedHeight(PropertyQTConstant_DefaultHeight);
        spinbox->setFocusPolicy(Qt::StrongFocus);

        sizePolicy.setHorizontalStretch(sliderStretch);
        slider->setSizePolicy(sizePolicy);
        slider->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        slider->setFixedHeight(PropertyQTConstant_DefaultHeight);
        slider->setFocusPolicy(Qt::StrongFocus);
        slider->setFocusProxy(spinbox);
    }
}
