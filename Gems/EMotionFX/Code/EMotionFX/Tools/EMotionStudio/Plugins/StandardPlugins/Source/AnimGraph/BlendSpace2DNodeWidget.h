/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendSpaceNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <QBrush>
#include <QPen>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QFontMetrics)


namespace EMStudio
{
    class BlendSpace2DNodeWidget
        : public AnimGraphNodeWidget
        , public AnimGraphPerFrameCallback
        , public BlendSpaceNodeWidget
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpace2DNodeWidget(AnimGraphPlugin* animGraphPlugin, QWidget* parent = nullptr);
        ~BlendSpace2DNodeWidget();

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
        void PrepareForDrawing(EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        void UpdateDisplayRange(const EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        //! Grid line values of an axis across the visible range, plus how many lines to skip between labels.
        AZStd::vector<float> GetGridLines(int axis, int& outLabelStride);
        bool HasMotionsOutsideCustomRange(const EMotionFX::BlendSpace2DNode::UniqueData* uniqueData) const;

        void DrawGrid(QPainter& painter);
        void DrawAxisGrid(QPainter& painter);
        void DrawSelectedMotion(QPainter& painter);
        void DrawAxisLabels(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        void DrawBoundRect(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        void DrawPoints(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        void DrawTriangles(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        void DrawCurrentPointAndBlendingInfluence(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);
        void DrawHoverMotionInfo(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData);

        void DrawInfoText(QPainter& painter, const QPointF& loc, const QPointF& refPoint, const AZStd::vector<QString>& strArray);
        void DrawInfoText(QPainter& painter, const QPointF& loc, const AZStd::vector<QString>& strArray);
        void DrawInfoText(QPainter& painter, const QPointF& loc, const AZStd::vector<QString>& strArray, int flags);

        void DrawBlendSpaceInfoText(QPainter& painter, const char* infoText) const;
        void DrawBlendSpaceWarningText(QPainter& painter, const char* warningText);

        void SetCurrentSamplePoint(int windowX, int windowY);

        void OnMouseMove(int windowX, int windowY);
        AZ::u32 FindPointAt(int windowX, int windowY) const;
        //! Blend space coordinates under the cursor, kept inside the grid and snapped when the axis asks for it.
        AZ::Vector2 GridCoordinatesAt(int windowX, int windowY);

        void RegisterForPerFrameCallback();
        void UnregisterForPerFrameCallback();

        AZ::Vector2 TransformToScreenCoords(const AZ::Vector2& inValue)
        {
            return (inValue * m_scale) + m_shift;
        }

        AZ::Vector2 TransformFromScreenCoords(const AZ::Vector2& screenCoords)
        {
            return (screenCoords - m_shift) / m_scale;
        }

        EMotionFX::BlendSpace2DNode* GetCurrentNode() const;
        EMotionFX::BlendSpace2DNode::UniqueData* GetUniqueData();

    private:
        EMotionFX::BlendSpace2DNode*                m_currentNode;
        AnimGraphPlugin*                            m_animGraphPlugin;
        bool                                        m_registeredForPerFrameCallback;
        AZStd::vector<QPointF>                      m_renderPoints;
        AZ::Vector2                                 m_scale;
        AZ::Vector2                                 m_shift;
        AZ::Vector2                                 m_displayMin = AZ::Vector2(0.0f, 0.0f); // range shown in the view
        AZ::Vector2                                 m_displayMax = AZ::Vector2(1.0f, 1.0f);
        AZ::Vector2                                 m_gridMin = AZ::Vector2(0.0f, 0.0f); // range the grid lines, snapping and clamping use
        AZ::Vector2                                 m_gridMax = AZ::Vector2(1.0f, 1.0f);
        bool                                        m_customRange[2] = { false, false };
        float                                       m_zoomFactor;// 0 for farthest zoom, 1 for closest zoom
        float                                       m_zoomScale;
        QRect                                       m_drawRect; // Rectangle where parameter space is displayed
        QRect                                       m_warningBoundRect; // Rectangle where the warning is displayed
        int                                         m_drawCenterX;
        int                                         m_drawCenterY;

        AZ::u32                                     m_hoverMotionIndex;

        QPen                                        m_edgePen;
        QPen                                        m_highlightedEdgePen;
        QPen                                        m_highlightedDottedPen;
        QPen                                        m_gridPen;
        QPen                                        m_subgridPen;
        QPen                                        m_divisionPen;
        QPen                                        m_axisLabelPen;
        QPen                                        m_infoTextPen;
        QBrush                                      m_backgroundRectBrush;
        QBrush                                      m_normalPolyBrush;
        QBrush                                      m_highlightedPolyBrush;
        QBrush                                      m_pointBrush;
        QBrush                                      m_interpolatedPointBrush;
        QBrush                                      m_infoTextBackgroundBrush;

        QFont                                       m_infoTextFont;
        QFontMetrics*                               m_infoTextFontMetrics;

        QString                                     m_tempString;
        AZStd::vector<QString>                      m_tempStrArray;

        static const int       s_motionPointCircleWidth;
        static const int       s_leftMargin;
        static const int       s_rightMargin;
        static const int       s_topMargin;
        static const int       s_bottomMargin;
        static const int       s_maxTextDim; // maximum height/width of text. Used in creating the rectangle for drawText.
        static const int       s_textWidthMargin;
        static const int       s_textHeightMargin;
        static const float     s_maxZoomScale;
        static const int       s_subGridSpacing;
        static const int       s_gridSpacing;
        static const float     s_infoTextGapToPos; // gap to the position being passed when drawing info text (x and y)
    };
} // namespace EMStudio


