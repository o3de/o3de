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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "Range.h"

#include <QRect>

class QPainter;
class QPalette;

namespace DrawingPrimitives
{
    struct STimeSliderOptions
    {
        QRect m_rect;
        int m_precision;
        int m_position;
        float m_time;
        bool m_bHasFocus;
    };

    void DrawTimeSlider(QPainter& painter, const QPalette& palette, const STimeSliderOptions& options);
}
