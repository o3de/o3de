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

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendSpaceNodeWidget.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <QBrush>
#include <QPen>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QFontMetrics)

namespace EMStudio
{
    class BlendSpace1DNodeWidget
        : public AnimGraphNodeWidget
        , public AnimGraphPerFrameCallback
        , public BlendSpaceNodeWidget
    {
    public:
        BlendSpace1DNodeWidget(AnimGraphPlugin* animGraphPlugin, QWidget* parent = nullptr);
        ~BlendSpace1DNodeWidget();

    public:
        // AnimGraphNodeWidget overrides
        void SetCurrentNode(EMotionFX::AnimGraphNode* node) override;

    public:
        // AnimGraphPerFrameCallback
        void ProcessFrame(float timePassedInSeconds) override;

    public:
        // QWidget overrides
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;

    private:
        void PrepareForDrawing(EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);

        void DrawGrid(QPainter& painter);
        void DrawAxisLabels(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        void DrawBoundRect(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        void DrawMotionsLine(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        void DrawPoints(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        void DrawCurrentPointAndBlendingInfluence(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        void DrawHoverMotionInfo(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);

        void DrawInfoText(QPainter& painter, const QPointF& loc, const AZStd::vector<QString>& strArray);

        void DrawBlendSpaceInfoText(QPainter& painter, const char* infoText) const;
        void DrawBlendSpaceWarningText(QPainter& painter, const char* warningText);

        void SetCurrentSamplePosition(int windowX, int windowY);

        void OnMouseMove(int windowX, int windowY);

        void RegisterForPerFrameCallback();
        void UnregisterForPerFrameCallback();

        AZ::Vector2 TransformToScreenCoords(float inValue)
        {
            AZ::Vector2 value(inValue, 0);
            return (value * m_scale) + m_shift;
        }

        float TransformFromScreenCoords(const AZ::Vector2& screenCoords)
        {
            AZ::Vector2 value = (screenCoords - m_shift) / m_scale;
            return value(0);
        }

        EMotionFX::BlendSpace1DNode* GetCurrentNode() const;
        EMotionFX::BlendSpace1DNode::UniqueData* GetUniqueData() const;

    private:
        EMotionFX::BlendSpace1DNode*                m_currentNode;
        AnimGraphPlugin*                            m_animGraphPlugin;
        bool                                        m_registeredForPerFrameCallback;
        AZStd::vector<QPointF>                      m_renderPoints;
        AZ::Vector2                                 m_scale;
        AZ::Vector2                                 m_shift;
        float                                       m_zoomFactor;// 0 for farthest zoom, 1 for closest zoom
        float                                       m_zoomScale;
        QRect                                       m_drawRect; // Rectangle where parameter space is displayed
        QRect                                       m_warningBoundRect; // Rectangle where the warning is displayed
        int                                         m_drawCenterX;
        int                                         m_drawCenterY;

        AZ::u32                                     m_hoverMotionIndex;

        QPen                                        m_edgePen;
        QPen                                        m_highlightedEdgePen;
        QPen                                        m_gridPen;
        QPen                                        m_subgridPen;
        QPen                                        m_axisLabelPen;
        QPen                                        m_infoTextPen;
        QBrush                                      m_backgroundRectBrush;
        QBrush                                      m_pointBrush;
        QBrush                                      m_infoTextBackgroundBrush;

        QFont                                       m_infoTextFont;
        QFontMetrics*                               m_infoTextFontMetrics;

        QString                                     m_tempString;
        AZStd::vector<QString>                      m_tempStrArray;

        static const int   s_motionPointCircleWidth;
        static const int   s_leftMargin;
        static const int   s_rightMargin;
        static const int   s_topMargin;
        static const int   s_bottomMargin;
        static const int   s_maxTextDim; // maximum height/width of text. Used in creating the rectangle for drawText.
        static const int   s_textWidthMargin;
        static const float s_maxZoomScale;
        static const int   s_subGridSpacing;
        static const int   s_gridSpacing;
    };
} // namespace EMStudio


