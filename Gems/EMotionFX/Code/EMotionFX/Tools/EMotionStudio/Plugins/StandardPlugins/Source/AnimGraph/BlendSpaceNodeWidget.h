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
