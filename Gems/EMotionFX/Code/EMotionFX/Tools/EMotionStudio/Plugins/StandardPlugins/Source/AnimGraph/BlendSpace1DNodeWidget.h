/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR_DECL

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
        void hideEvent(QHideEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        void PrepareForDrawing(EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        void UpdateDisplayRange(const EMotionFX::BlendSpace1DNode::UniqueData* uniqueData);
        //! Grid line values across the visible range, plus how many lines to skip between labels.
        AZStd::vector<float> GetGridLines(int& outLabelStride) const;
        bool HasMotionsOutsideCustomRange(const EMotionFX::BlendSpace1DNode::UniqueData* uniqueData) const;

        void DrawGrid(QPainter& painter);
        void DrawAxisGrid(QPainter& painter);
        void DrawSelectedMotion(QPainter& painter);
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
        AZ::u32 FindPointAt(int windowX, int windowY) const;
        //! Blend space coordinate under the cursor, kept inside the grid and snapped when the axis asks for it.
        float GridCoordinateAt(int windowX, int windowY);

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
        EMotionFX::BlendSpace1DNode::UniqueData* GetUniqueData();

    private:
        EMotionFX::BlendSpace1DNode*                m_currentNode;
        AnimGraphPlugin*                            m_animGraphPlugin;
        bool                                        m_registeredForPerFrameCallback;
        AZStd::vector<QPointF>                      m_renderPoints;
        AZ::Vector2                                 m_scale;
        AZ::Vector2                                 m_shift;
        float                                       m_displayMin = 0.0f; // range shown in the view
        float                                       m_displayMax = 1.0f;
        float                                       m_gridMin = 0.0f; // range the grid lines, snapping and clamping use
        float                                       m_gridMax = 1.0f;
        bool                                        m_customRange = false;
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
        QPen                                        m_divisionPen;
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


