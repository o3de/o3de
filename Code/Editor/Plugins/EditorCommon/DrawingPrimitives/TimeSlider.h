/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
