/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Range.h"

#include <functional>
#include <vector>
#include <QRect>

class QPainter;
class QPalette;

namespace DrawingPrimitives
{
    struct SRulerOptions;
    typedef std::function<void()> TDrawCallback;

    struct SRulerOptions
    {
        QRect m_rect;
        Range m_visibleRange;
        Range m_rulerRange;
        int m_textXOffset;
        int m_textYOffset;
        int m_markHeight;
        int m_shadowSize;

        TDrawCallback m_drawBackgroundCallback;
    };

    struct STick
    {
        bool m_bTenth;
        int m_position;
        float m_value;
    };

    typedef SRulerOptions STickOptions;

    std::vector<STick> CalculateTicks(uint size, Range visibleRange, Range rulerRange, int* pRulerPrecision, Range* pScreenRulerRange);
    void DrawTicks(const std::vector<STick>& ticks, QPainter& painter, const QPalette& palette, const STickOptions& options);
    void DrawTicks(QPainter& painter, const QPalette& palette, const STickOptions& options);
    void DrawRuler(QPainter& painter, const QPalette& palette, const SRulerOptions& options, int* pRulerPrecision);
}
