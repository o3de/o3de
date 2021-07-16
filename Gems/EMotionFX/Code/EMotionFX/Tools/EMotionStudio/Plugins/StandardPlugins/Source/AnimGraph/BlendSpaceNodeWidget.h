/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimGraphNodeWidget.h"
#include <QPainter>


namespace EMStudio
{
    class BlendSpaceNodeWidget
    {
    public:
        BlendSpaceNodeWidget();
        ~BlendSpaceNodeWidget();

        void RenderCurrentSamplePoint(QPainter& painter, const QPointF& samplePoint);
        void RenderSampledMotionPoint(QPainter& painter, const QPointF& point, float weight);
        void RenderTextBox(QPainter& painter, const QPointF& point, const QString& text);

    protected:
        // Current sample point
        static const float s_currentSamplePointWidth;
        static const QColor s_currentSamplePointColor;

        // Sampled motion points
        static const float s_motionPointWidthMin;
        static const float s_motionPointWidthMax;
        static const float s_motionPointAlphaMin;
        static const float s_motionPointAlphaMax;
        static const QColor s_motionPointColor;

        static const char* s_warningOffsetForIcon;

    private:
        void RenderCircle(QPainter& painter, const QPointF& point, const QColor& color, float size);
    };
} // namespace EMStudio
