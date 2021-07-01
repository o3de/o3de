/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZ_Q_SLIDER_HXX
#define AZ_Q_SLIDER_HXX

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QSlider>


#pragma once

class QAbstractSpinBox;

namespace AzToolsFramework
{
    class DHQSlider
        : public QSlider
    {
    public:
        AZ_CLASS_ALLOCATOR(DHQSlider, AZ::SystemAllocator, 0);

        explicit DHQSlider(QWidget* parent = 0)
            : QSlider(parent) {}

        DHQSlider(Qt::Orientation orientation, QWidget* parent = 0)
            : QSlider(orientation, parent) {}

        void wheelEvent(QWheelEvent* e);
    };

    // Share widget initialization code between double and int based slider properties.
    void InitializeSliderPropertyWidgets(QSlider*, QAbstractSpinBox*);
}

#endif
