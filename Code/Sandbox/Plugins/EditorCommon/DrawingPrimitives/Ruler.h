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