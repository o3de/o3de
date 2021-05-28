/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
