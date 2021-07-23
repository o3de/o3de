/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendSpace2DNodeWidget.h"
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>

namespace
{
    inline void DrawTriangle(QPainter& painter, const AZStd::vector<QPointF>& points, uint16_t vert1, uint16_t vert2, uint16_t vert3)
    {
        const QPointF triPoints[3] =
        {
            points[vert1],
            points[vert2],
            points[vert3]
        };
        painter.drawPolygon(triPoints, 3);
    }
}

namespace EMStudio
{
    const int       BlendSpace2DNodeWidget::s_motionPointCircleWidth = 4;
    const int       BlendSpace2DNodeWidget::s_leftMargin = 40;
    const int       BlendSpace2DNodeWidget::s_rightMargin = 20;
    const int       BlendSpace2DNodeWidget::s_topMargin = 20;
    const int       BlendSpace2DNodeWidget::s_bottomMargin = 40;
    const int       BlendSpace2DNodeWidget::s_maxTextDim = 1000; // maximum height/width of text. Used in creating the rectangle for drawText.
    const int       BlendSpace2DNodeWidget::s_textWidthMargin = 60;
    const int       BlendSpace2DNodeWidget::s_textHeightMargin = 25;
    const float     BlendSpace2DNodeWidget::s_maxZoomScale = 10.0f;
    const int       BlendSpace2DNodeWidget::s_subGridSpacing = 10;
    const int       BlendSpace2DNodeWidget::s_gridSpacing = 100;
    const float     BlendSpace2DNodeWidget::s_infoTextGapToPos = 12.0f;

    BlendSpace2DNodeWidget::BlendSpace2DNodeWidget(AnimGraphPlugin* animGraphPlugin, QWidget* parent)
        : AnimGraphNodeWidget(parent)
        , BlendSpaceNodeWidget()
        , m_currentNode(nullptr)
        , m_animGraphPlugin(animGraphPlugin)
        , m_registeredForPerFrameCallback(false)
        , m_zoomFactor(0)
        , m_hoverMotionIndex(MCORE_INVALIDINDEX32)
    {
        AZ_Assert(animGraphPlugin, "AnimGraphPlugin needed to get per frame callbacks");

        QColor color;

        color.setRgb(0xBB, 0xBB, 0xBB);
        m_edgePen.setColor(color);
        m_edgePen.setWidth(1);

        color.setRgb(0xF5, 0xA6, 0x23);
        m_highlightedEdgePen.setColor(color);
        m_highlightedEdgePen.setWidth(2);

        m_highlightedDottedPen.setColor(color);
        m_highlightedDottedPen.setWidthF(0.7);
        m_highlightedDottedPen.setStyle(Qt::DotLine);

        m_gridPen.setColor(QColor(61, 61, 61));
        m_subgridPen.setColor(QColor(55, 55, 55));

        color.setRgb(0xBB, 0xBB, 0xBB);
        m_axisLabelPen.setColor(color);
        m_axisLabelPen.setWidth(2);

        color.setRgb(0xBB, 0xBB, 0xBB);
        m_infoTextPen.setColor(color);
        m_infoTextPen.setWidth(1);

        color.setRgb(0xDD, 0xDD, 0xDD, 0x11);
        m_backgroundRectBrush.setColor(color);
        m_backgroundRectBrush.setStyle(Qt::SolidPattern);

        color.setRgb(0xF5, 0xA6, 0x23, 0x13);
        m_normalPolyBrush.setColor(color);
        m_normalPolyBrush.setStyle(Qt::SolidPattern);

        color.setRgb(0xF5, 0xA6, 0x23, 0x26);
        m_highlightedPolyBrush.setColor(color);
        m_highlightedPolyBrush.setStyle(Qt::SolidPattern);

        color.setRgb(0xBB, 0xBB, 0xBB);
        m_pointBrush.setColor(color);
        m_pointBrush.setStyle(Qt::SolidPattern);

        color.setRgb(0xF5, 0xA6, 0x23);
        m_interpolatedPointBrush.setColor(color);
        m_interpolatedPointBrush.setStyle(Qt::SolidPattern);

        color.setRgb(0x22, 0x22, 0x22);
        m_infoTextBackgroundBrush.setColor(color);
        m_infoTextBackgroundBrush.setStyle(Qt::SolidPattern);

        m_infoTextFont.setPixelSize(8);
        m_infoTextFontMetrics = new QFontMetrics(m_infoTextFont);

        setFocusPolicy((Qt::FocusPolicy)(Qt::ClickFocus | Qt::WheelFocus));
        setMouseTracking(true);
    }

    BlendSpace2DNodeWidget::~BlendSpace2DNodeWidget()
    {
        UnregisterForPerFrameCallback();
        delete m_infoTextFontMetrics;
    }


    void BlendSpace2DNodeWidget::SetCurrentNode(EMotionFX::AnimGraphNode* node)
    {
        if (m_currentNode)
        {
            m_currentNode->SetInteractiveMode(false);
        }
        m_currentNode = nullptr;

        if (node)
        {
            if (azrtti_typeid(node) == azrtti_typeid<EMotionFX::BlendSpace2DNode>())
            {
                m_currentNode = static_cast<EMotionFX::BlendSpace2DNode*>(node);
                m_currentNode->SetInteractiveMode(true);

                // Once in interactive mode, the GUI is responsible for setting the current position.
                // So, initialize it.
                EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
                if (uniqueData)
                {
                    m_currentNode->SetCurrentPosition(uniqueData->m_currentPosition);
                }
            }
            else
            {
                AZ_Assert(false, "Unexpected node type");
            }
        }

        update();

        if (m_currentNode)
        {
            RegisterForPerFrameCallback();
        }
        else
        {
            UnregisterForPerFrameCallback();
        }
    }

    void BlendSpace2DNodeWidget::ProcessFrame([[maybe_unused]] float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || visibleRegion().isEmpty())
            return;

        update();
    }

    void BlendSpace2DNodeWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

        const EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        if (!animGraphInstance)
        {
            painter.drawText(rect(), Qt::AlignCenter, "No anim graph active.");
        }

        EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
        if (!uniqueData)
        {
            return;
        }

        m_zoomScale = MCore::LinearInterpolate<float>(1.0f, s_maxZoomScale, m_zoomFactor);

        // Detect if the node is in an active blend tree. Checking if the parent is ready is
        // more stable since a non-connected blend space node wont be ready
        EMotionFX::AnimGraphNode* nodeThatShouldBeReady = m_currentNode->GetParentNode()
            ? m_currentNode->GetParentNode()
            : m_currentNode;

        const AZStd::vector<AZ::Vector2>& points = uniqueData->m_motionCoordinates;
        const size_t numPoints = points.size();

        if (animGraphInstance
            && m_currentNode
            && !animGraphInstance->GetIsOutputReady(nodeThatShouldBeReady->GetObjectIndex()))
        {
            PrepareForDrawing(uniqueData);
            if (m_scale(0) <= 0)
            {
                // This happens if the window is so small that there is no space to draw after leaving margins
                return;
            }

            DrawBoundRect(painter, uniqueData);
            DrawBlendSpaceInfoText(painter, "The blend tree containing this blend space node is currently not in active state. "
                "To be able to interactively visualize the operation of this blend space, set the blend tree containing this node "
                "to active state.");
        }
        else if (!GetCurrentNode()->GetValidCalculationMethodsAndEvaluators())
        {
            PrepareForDrawing(uniqueData);
            if (m_scale(0) <= 0)
            {
                // This happens if the window is so small that there is no space to draw after leaving margins
                return;
            }

            DrawBoundRect(painter, uniqueData);
            DrawBlendSpaceInfoText(painter, "You will create a blend space by selecting the calculation methods for the axes "
                "and adding motions to blend using the Attributes window below.\n\nFor each axis, you can choose to have the "
                "coordinates of the motions to be calculated automatically or to enter them manually. To have them calculated "
                "automatically, pick one of the available evaluators. The evaluators calculate the coordinate by analyzing the "
                "motion.");
        }
        else 
        {
            DrawGrid(painter);
            m_warningBoundRect.setRect(0, 0, 0, 0);

            if (numPoints < 3)
            {
                DrawBlendSpaceWarningText(painter, "At least three motion coordinates are required.");
            }
            else if (uniqueData->m_triangles.empty())
            {
                DrawBlendSpaceWarningText(painter, "Two or more motions are sharing the same coordinates, which might cause inaccurate blended "
                                          "animations. Please check the coordinates and try again.");
            }
            else if (uniqueData->m_hasDegenerateTriangles)
            {
                DrawBlendSpaceWarningText(painter, "Two or more motions have coordinates too close to each other, which might cause inaccurate "
                                          "blended animations. Please check the coordinates and try again.");
            }

            PrepareForDrawing(uniqueData);
            if (m_scale(0) <= 0)
            {
                // This happens if the window is so small that there is no space to draw after leaving margins
                return;
            }
            DrawBoundRect(painter, uniqueData);

            m_renderPoints.resize(numPoints);
            for (size_t i = 0; i < numPoints; ++i)
            {
                const AZ::Vector2 transformedPt = TransformToScreenCoords(points[i]);
                m_renderPoints[i].setX(transformedPt.GetX());
                m_renderPoints[i].setY(transformedPt.GetY());
            }

            DrawAxisLabels(painter, uniqueData);
            DrawPoints(painter, uniqueData);
            DrawTriangles(painter, uniqueData);
            DrawCurrentPointAndBlendingInfluence(painter, uniqueData);
            DrawHoverMotionInfo(painter, uniqueData);
        }
    }

    void BlendSpace2DNodeWidget::mousePressEvent(QMouseEvent* event)
    {
        if (!m_currentNode)
        {
            return;
        }
        if (event->buttons() &  Qt::LeftButton)
        {
            SetCurrentSamplePoint(event->x(), event->y());
            setCursor(Qt::ClosedHandCursor);  // dragging the hotspot
        }
        else
        {
            setCursor(Qt::ArrowCursor); // not dragging the hotspot
        }
    }

    void BlendSpace2DNodeWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        OnMouseMove(event->x(), event->y());
    }

    void BlendSpace2DNodeWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (!m_currentNode)
        {
            return;
        }
        const AZ::u32 prevHoverMotionIndex = m_hoverMotionIndex;

        if (event->buttons() &  Qt::LeftButton)
        {
            SetCurrentSamplePoint(event->x(), event->y());
            m_hoverMotionIndex = MCORE_INVALIDINDEX32;
        }
        else
        {
            OnMouseMove(event->x(), event->y());
        }

        if (m_hoverMotionIndex != prevHoverMotionIndex)
        {
            update();
        }
    }

    void BlendSpace2DNodeWidget::PrepareForDrawing(EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        const AZ::Vector2& max = uniqueData->m_rangeMax;
        const AZ::Vector2& min = uniqueData->m_rangeMin;

        const float rangeX = std::max(1e-8f, max(0) - min(0));
        const float rangeY = std::max(1e-8f, max(1) - min(1));

        const int w = width();
        const int h = height() - m_warningBoundRect.height();
        const int wAfterMargin = w - s_leftMargin - s_rightMargin;
        const int hAfterMargin = h - s_topMargin - s_bottomMargin;

        const float scaleX = wAfterMargin / rangeX;
        const float scaleY = -hAfterMargin / rangeY; // Negating the scale because, per window convention
        // y increases downwards

        m_drawCenterX = s_leftMargin + wAfterMargin / 2;
        m_drawCenterY = h - s_bottomMargin - hAfterMargin / 2 + m_warningBoundRect.height();

        m_drawRect.setRect(m_drawCenterX - wAfterMargin / 2, m_drawCenterY - hAfterMargin / 2, wAfterMargin, hAfterMargin);

        m_scale.Set(scaleX, scaleY);
        m_shift.Set(m_drawCenterX - uniqueData->m_rangeCenter(0) * scaleX, m_drawCenterY - uniqueData->m_rangeCenter(1) * scaleY);
    }

    void BlendSpace2DNodeWidget::DrawGrid(QPainter& painter)
    {
        QTransform gridTransform;
        gridTransform.scale(m_zoomScale, m_zoomScale);
        painter.setTransform(gridTransform);

        const int winWidth = width();
        const int winHeight = height();
        QPoint upperLeft = gridTransform.inverted().map(QPoint(0, 0));
        QPoint lowerRight = gridTransform.inverted().map(QPoint(winWidth, winHeight));

        // Calculate the start and end ranges in 'zoomed out' coordinates.
        // We need to render grid lines covering that area.
        const int32 startX = (upperLeft.x() / s_subGridSpacing) * s_subGridSpacing - s_subGridSpacing;
        const int32 startY = (upperLeft.y() / s_subGridSpacing) * s_subGridSpacing - s_subGridSpacing;
        const int32 endX = lowerRight.x();
        const int32 endY = lowerRight.y();

        // Draw subgrid lines
        painter.setPen(m_subgridPen);

        for (int32 x = startX; x < endX; x += s_subGridSpacing)
        {
            if (x % s_gridSpacing != 0)
            {
                painter.drawLine(x, startY, x, endY);
            }
        }
        for (int32 y = startY; y < endY; y += s_subGridSpacing)
        {
            if (y % s_gridSpacing != 0)
            {
                painter.drawLine(startX, y, endX, y);
            }
        }

        // Draw grid lines
        painter.setPen(m_gridPen);

        const int32 gridStartX = (startX / s_gridSpacing) * s_gridSpacing;
        const int32 gridStartY = (startY / s_gridSpacing) * s_gridSpacing;
        for (int32 x = gridStartX; x < endX; x += s_gridSpacing)
        {
            painter.drawLine(x, startY, x, endY);
        }
        for (int32 y = gridStartY; y < endY; y += s_gridSpacing)
        {
            painter.drawLine(startX, y, endX, y);
        }

        painter.setTransform(QTransform()); // set the transform back to identity
    }

    void BlendSpace2DNodeWidget::DrawAxisLabels(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        painter.setPen(m_axisLabelPen);

        const int rectLeft = m_drawRect.left();
        const int rectRight = m_drawRect.right();
        const int rectTop = m_drawRect.top();
        const int rectBottom = m_drawRect.bottom();
        const int xValueTop = rectBottom + 4;
        const int yValueRight = rectLeft - 2;
        const int xAxisLabelTop = rectBottom + 15;
        const char numFormat = 'g';
        const int  numPrecision = 4;

        // x axis label
        const char* axisLabelX = m_currentNode->GetAxisLabel(0);
        painter.drawText(QRect(m_drawCenterX - s_maxTextDim / 2, xAxisLabelTop, s_maxTextDim, s_maxTextDim), axisLabelX, Qt::AlignHCenter | Qt::AlignTop);

        // if we are in a situation without points, we want to draw reference axis from 0 to 1
        const bool referenceAxis = uniqueData->m_motionCoordinates.empty();

        // x axis values
        const AZ::Vector2 xAxisLimits = referenceAxis ? AZ::Vector2(0.0f, 1.0f) : AZ::Vector2(uniqueData->m_rangeMin.GetX(), uniqueData->m_rangeMax.GetX());
        m_tempString.setNum(xAxisLimits(0), numFormat, numPrecision);
        painter.drawText(QRect(rectLeft - s_maxTextDim / 2, xValueTop, s_maxTextDim, s_maxTextDim), m_tempString, Qt::AlignHCenter | Qt::AlignTop);
        m_tempString.setNum(xAxisLimits(1), numFormat, numPrecision);
        painter.drawText(QRect(rectRight - s_maxTextDim / 2, xValueTop, s_maxTextDim, s_maxTextDim), m_tempString, Qt::AlignHCenter | Qt::AlignTop);

        // yaxis values
        const AZ::Vector2 yAxisLimits = referenceAxis ? AZ::Vector2(0.0f, 1.0f) : AZ::Vector2(uniqueData->m_rangeMin.GetY(), uniqueData->m_rangeMax.GetY());
        m_tempString.setNum(yAxisLimits(0), numFormat, numPrecision);
        painter.drawText(QRect(yValueRight - s_maxTextDim, rectBottom - s_maxTextDim / 2, s_maxTextDim, s_maxTextDim), m_tempString, Qt::AlignVCenter | Qt::AlignRight);
        m_tempString.setNum(yAxisLimits(1), numFormat, numPrecision);
        painter.drawText(QRect(yValueRight - s_maxTextDim, rectTop - s_maxTextDim / 2, s_maxTextDim, s_maxTextDim), m_tempString, Qt::AlignVCenter | Qt::AlignRight);

        const char* axisLabelY = m_currentNode->GetAxisLabel(1);
        painter.rotate(-90);
        // Since the coordinate system has been rotated -90 degrees, we have to specify the rectangle coordinates accordingly. In
        // particular, the -x and y axes will correspond to the normal y and x axes respectively.
        painter.drawText(QRect(-(m_drawCenterY + s_maxTextDim / 2), rectLeft - 20, s_maxTextDim, s_maxTextDim), axisLabelY, Qt::AlignHCenter | Qt::AlignTop);

        painter.resetTransform();
    }

    void BlendSpace2DNodeWidget::DrawBoundRect(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_backgroundRectBrush);

        if (!uniqueData->m_motionCoordinates.empty())
        {
            const AZ::Vector2 topLeft = TransformToScreenCoords(uniqueData->m_rangeMin);
            const AZ::Vector2 bottomRight = TransformToScreenCoords(uniqueData->m_rangeMax);
            const QRectF rect(QPointF(topLeft(0), topLeft(1)), QPointF(bottomRight(0), bottomRight(1)));
            painter.drawRect(rect);
        }
        else
        {
            // Draw in the whole drawing area
            painter.drawRect(m_drawRect);
        }
    }

    void BlendSpace2DNodeWidget::DrawPoints(QPainter& painter, [[maybe_unused]] EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        painter.setPen(QPen());
        painter.setBrush(m_pointBrush);
        for (size_t i = 0, numPoints = m_renderPoints.size(); i < numPoints; ++i)
        {
            painter.drawEllipse(m_renderPoints[i], s_motionPointCircleWidth, s_motionPointCircleWidth);
        }
    }

    void BlendSpace2DNodeWidget::DrawTriangles(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        const EMotionFX::BlendSpace2DNode::Triangles& triangles = uniqueData->m_triangles;
        const AZ::u32 numTriangles = (AZ::u32)triangles.size();

        painter.setPen(m_edgePen);
        painter.setBrush(m_normalPolyBrush);
        for (AZ::u32 triIdx = 0; triIdx < numTriangles; ++triIdx)
        {
            if (triIdx == uniqueData->m_currentTriangle.m_triangleIndex)
            {
                continue;
            }
            const EMotionFX::BlendSpace2DNode::Triangle& tri = uniqueData->m_triangles[triIdx];
            DrawTriangle(painter, m_renderPoints, tri.m_vertIndices[0], tri.m_vertIndices[1], tri.m_vertIndices[2]);
        }
    }

    void BlendSpace2DNodeWidget::DrawCurrentPointAndBlendingInfluence(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        AZ::Vector2 transformedPt = TransformToScreenCoords(uniqueData->m_currentPosition);
        QPointF samplePoint(transformedPt(0), transformedPt(1));

        const QRectF drawRect = m_drawRect;
        // Clamp the sample point to the m_drawRect (which is the region where the blend space is defined
        samplePoint.setX(
            AZStd::min(
                AZStd::max(samplePoint.x(), drawRect.left()),
                drawRect.right()));
        samplePoint.setY(
            AZStd::min(
                AZStd::max(samplePoint.y(), drawRect.top()),
                drawRect.bottom()));

        if (uniqueData->m_currentTriangle.m_triangleIndex != MCORE_INVALIDINDEX32)
        {
            const EMotionFX::BlendSpace2DNode::Triangle& tri = uniqueData->m_triangles[uniqueData->m_currentTriangle.m_triangleIndex];

            painter.setPen(m_highlightedEdgePen);
            painter.setBrush(m_highlightedPolyBrush);
            DrawTriangle(painter, m_renderPoints, tri.m_vertIndices[0], tri.m_vertIndices[1], tri.m_vertIndices[2]);

            for (int i = 0; i < 3; ++i)
            {
                AZ::u32 pointIdx = tri.m_vertIndices[i];
                const AZ::Vector2& triVert = uniqueData->m_motionCoordinates[pointIdx];
                const QPointF& triScreenVert = m_renderPoints[pointIdx];
                const float blendWeight = uniqueData->m_currentTriangle.m_weights[i];

                RenderSampledMotionPoint(painter, triScreenVert, blendWeight);

                m_tempStrArray.clear();
                EMotionFX::MotionInstance* motionInstance = uniqueData->m_motionInfos[pointIdx].m_motionInstance;
                m_tempStrArray.push_back(motionInstance->GetMotion()->GetName());
                m_tempStrArray.push_back(QString::asprintf("Blend weight: %.1f%%", blendWeight * 100.0f));
                m_tempStrArray.push_back(QString::asprintf("(%g, %g)", triVert(0), triVert(1)));
                DrawInfoText(painter, triScreenVert, samplePoint, m_tempStrArray);
            }
        }
        else if (uniqueData->m_currentEdge.m_edgeIndex != MCORE_INVALIDINDEX32)
        {
            const EMotionFX::BlendSpace2DNode::Edge& edge = uniqueData->m_outerEdges[uniqueData->m_currentEdge.m_edgeIndex];

            painter.setPen(m_highlightedEdgePen);
            painter.drawLine(m_renderPoints[edge.m_vertIndices[0]], m_renderPoints[edge.m_vertIndices[1]]);

            painter.setPen(m_highlightedDottedPen);
            const AZ::Vector2& edgeStart = uniqueData->m_motionCoordinates[edge.m_vertIndices[0]];
            const AZ::Vector2& edgeEnd = uniqueData->m_motionCoordinates[edge.m_vertIndices[1]];
            AZ::Vector2 samplePt = edgeStart.Lerp(edgeEnd, uniqueData->m_currentEdge.m_u);
            AZ::Vector2 transformedSampleLoc = TransformToScreenCoords(samplePt);
            const QPointF transformedSamplePt(transformedSampleLoc(0), transformedSampleLoc(1));
            painter.drawLine(samplePoint, transformedSamplePt);

            painter.setPen(Qt::NoPen);
            painter.setBrush(m_interpolatedPointBrush);
            painter.drawEllipse(transformedSamplePt, s_motionPointCircleWidth, s_motionPointCircleWidth);

            for (int i = 0; i < 2; ++i)
            {
                const AZ::u32 pointIdx = edge.m_vertIndices[i];
                const QPointF& edgeScreenVert = m_renderPoints[pointIdx];
                const float blendWeight = (i == 0) ? (1.0f - uniqueData->m_currentEdge.m_u) : uniqueData->m_currentEdge.m_u;

                RenderSampledMotionPoint(painter, edgeScreenVert, blendWeight);

                m_tempStrArray.clear();
                EMotionFX::MotionInstance* motionInstance = uniqueData->m_motionInfos[pointIdx].m_motionInstance;
                m_tempStrArray.push_back(motionInstance->GetMotion()->GetName());
                m_tempStrArray.push_back(QString::asprintf("Blend weight: %.1f%%", blendWeight * 100.0f));
                const AZ::Vector2& edgeVert = uniqueData->m_motionCoordinates[pointIdx];
                m_tempStrArray.push_back(QString::asprintf("(%g, %g)", edgeVert(0), edgeVert(1)));
                DrawInfoText(painter, edgeScreenVert, samplePoint, m_tempStrArray);
            }
        }

        m_tempStrArray.clear();
        m_tempStrArray.push_back(QString::asprintf("(%g, %g)", uniqueData->m_currentPosition.GetX(), uniqueData->m_currentPosition.GetY()));
        DrawInfoText(painter, samplePoint, m_tempStrArray);

        RenderCurrentSamplePoint(painter, samplePoint);
    }

    void BlendSpace2DNodeWidget::DrawHoverMotionInfo(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        if (m_hoverMotionIndex != MCORE_INVALIDINDEX32)
        {
            m_tempStrArray.clear();
            EMotionFX::MotionInstance* motionInstance = uniqueData->m_motionInfos[m_hoverMotionIndex].m_motionInstance;
            m_tempStrArray.push_back(motionInstance->GetMotion()->GetName());
            DrawInfoText(painter, m_renderPoints[m_hoverMotionIndex], m_tempStrArray);
        }
    }

    void BlendSpace2DNodeWidget::DrawInfoText(QPainter& painter, const QPointF& loc, const QPointF& refPoint, const AZStd::vector<QString>& strArray)
    {
        const int winWidth = width();
        const int winHeight = height();

        // The text is to be displayed near "loc".
        // When possible, we want the text displayed so that it is away from the "refPoint". But, we
        // don't do that if the text is likely to go off the margin.

        int flags = 0;

        const int locX = aznumeric_cast<int>(loc.x());
        if (locX > refPoint.x())
        {
            if ((locX + s_textWidthMargin) < winWidth)
            {
                flags |= Qt::AlignLeft;
            }
            else
            {
                flags |= Qt::AlignRight;
            }
        }
        else
        {
            if (locX >= s_textWidthMargin)
            {
                flags |= Qt::AlignRight;
            }
            else
            {
                flags |= Qt::AlignLeft;
            }
        }

        const int locY = aznumeric_cast<int>(loc.y());
        if (locY > refPoint.y())
        {
            if (locY + s_textHeightMargin < winHeight)
            {
                flags |= Qt::AlignTop;
            }
            else
            {
                flags |= Qt::AlignBottom;
            }
        }
        else
        {
            if (locY > s_textHeightMargin)
            {
                flags |= Qt::AlignBottom;
            }
            else
            {
                flags |= Qt::AlignTop;
            }
        }

        DrawInfoText(painter, loc, strArray, flags);
    }

    void BlendSpace2DNodeWidget::DrawInfoText(QPainter& painter, const QPointF& loc, const AZStd::vector<QString>& strArray)
    {
        const int winWidth = width();

        int flags = Qt::AlignTop;
        // If the text is likely to go off the right margin, align it so that right side is at loc.x.
        // Else, align so that left side is at loc.x
        flags |= ((winWidth - s_textWidthMargin) > loc.x()) ? Qt::AlignLeft : Qt::AlignRight;
        DrawInfoText(painter, loc, strArray, flags);
    }

    void BlendSpace2DNodeWidget::DrawInfoText(QPainter& painter, const QPointF& loc, const AZStd::vector<QString>& strArray, int flags)
    {
        const int numStrings = (int)strArray.size();
        if (numStrings == 0)
        {
            return;
        }

        painter.setFont(m_infoTextFont);

        QString textToDraw = strArray[0];
        for (size_t i = 1; i < numStrings; ++i)
        {
            const QString& str = strArray[i];
            textToDraw += '\n';
            textToDraw += str;
        }

        float left, right, top, bottom;
        if (flags & Qt::AlignLeft)
        {
            left = aznumeric_cast<float>(loc.x() + s_infoTextGapToPos);
            right = left + s_maxTextDim;
        }
        else
        {
            right = aznumeric_cast<float>(loc.x() - s_infoTextGapToPos);
            left = right - s_maxTextDim;
        }
        if (flags & Qt::AlignTop)
        {
            top = aznumeric_cast<float>(loc.y() + s_infoTextGapToPos);
            bottom = top + s_maxTextDim;
        }
        else
        {
            bottom = aznumeric_cast<float>(loc.y() - s_infoTextGapToPos);
            top = bottom - s_maxTextDim;
        }

        QRect rect(QPoint(aznumeric_cast<int>(left), aznumeric_cast<int>(top)), QPoint(aznumeric_cast<int>(right), aznumeric_cast<int>(bottom)));

        QRect boundRect = m_infoTextFontMetrics->boundingRect(rect, flags, textToDraw);
        boundRect.adjust(-3, -3, 3, 3);

        // Draw background rect for the text
        painter.setBrush(m_infoTextBackgroundBrush);
        painter.setPen(Qt::NoPen);
        painter.drawRect(boundRect);

        // Draw the text
        painter.setPen(m_infoTextPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(rect, flags, textToDraw);
    }

    void BlendSpace2DNodeWidget::DrawBlendSpaceInfoText(QPainter& painter, const char* infoText) const
    {
        painter.setPen(m_infoTextPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(m_drawRect, Qt::AlignCenter | Qt::TextWordWrap, infoText);
    }

    void BlendSpace2DNodeWidget::DrawBlendSpaceWarningText(QPainter& painter, const char* warningText)
    {
        const QRect warningRect(10, 10, width() - 20, height() - 20);
        QString offsetWarningText(s_warningOffsetForIcon); // some space for the warning icon
        offsetWarningText.append(warningText);

        // Draw/compute the bounding rect of the warning text. This is a trick to get the proper bounding
        // rect of the text
        painter.setPen(m_infoTextPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(warningRect, Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap, offsetWarningText, &m_warningBoundRect);

        // adjust the bounding rect to give some margins
        m_warningBoundRect.adjust(-10, -5, 10, 5);

        // Draw background rect for the text
        painter.setBrush(m_infoTextBackgroundBrush);
        painter.setPen(Qt::NoPen);
        painter.drawRect(m_warningBoundRect);

        // Draw warning icon
        const QIcon& warningIcon = MysticQt::GetMysticQt()->FindIcon("Images/Icons/Warning.svg");
        const QPoint iconPosition(m_warningBoundRect.x() + 5,
                                  m_warningBoundRect.center().y() - 8);
        painter.drawPixmap(iconPosition, warningIcon.pixmap(16, 16));
    
        painter.setPen(m_infoTextPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(warningRect, Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap, offsetWarningText);
    }

    void BlendSpace2DNodeWidget::SetCurrentSamplePoint(int windowX, int windowY)
    {
        EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
        if (!uniqueData || !animGraphInstance)
        {
            return;
        }

        const AZ::Vector2 screenCoords((float)windowX, (float)windowY);
        AZ::Vector2 currentPosition = TransformFromScreenCoords(screenCoords);
        if (currentPosition != uniqueData->m_currentPosition)
        {
            m_currentNode->SetCurrentPosition(currentPosition);
            update();
        }
    }

    void BlendSpace2DNodeWidget::OnMouseMove(int windowX, int windowY)
    {
        float minDistSqr = FLT_MAX;
        AZ::u32 closestMotionIdx = MCORE_INVALIDINDEX32;

        const AZ::u32 numMotions = (AZ::u32)m_renderPoints.size();
        for (AZ::u32 i = 0; i < numMotions; ++i)
        {
            const float diffX = aznumeric_cast<float>(windowX - m_renderPoints[i].x());
            const float diffY = aznumeric_cast<float>(windowY - m_renderPoints[i].y());
            const float distSqr = diffX * diffX + diffY * diffY;
            if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
                closestMotionIdx = i;
            }
        }

        if ((closestMotionIdx != MCORE_INVALIDINDEX32) && (minDistSqr < 36.0f))
        {
            m_hoverMotionIndex = closestMotionIdx;
        }
        else
        {
            m_hoverMotionIndex = MCORE_INVALIDINDEX32;
        }

        EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
        if (uniqueData && m_drawRect.contains(windowX, windowY))    // Otherwise we cannot change the hotspot therefore keep the cursor as arrow
        {
            AZ::Vector2 transformedPt = TransformToScreenCoords(uniqueData->m_currentPosition);
            const QRectF regionForHotspotCursor(
                transformedPt.GetX() - s_currentSamplePointWidth,
                transformedPt.GetY() - s_currentSamplePointWidth,
                s_currentSamplePointWidth * 2.0f,
                s_currentSamplePointWidth * 2.0f);

            if (regionForHotspotCursor.contains(windowX, windowY))
            {
                setCursor(Qt::OpenHandCursor);  // indicates that the hotspot can be grabbed
            }
            else
            {
                setCursor(Qt::PointingHandCursor); // indicates that we are in the blend space
            }
        }
        else
        {
            setCursor(Qt::ArrowCursor); // indicates that we are not in the blend space
        }
    }

    void BlendSpace2DNodeWidget::RegisterForPerFrameCallback()
    {
        if (m_animGraphPlugin && !m_registeredForPerFrameCallback)
        {
            m_animGraphPlugin->RegisterPerFrameCallback(this);
            m_registeredForPerFrameCallback = true;
        }
    }

    void BlendSpace2DNodeWidget::UnregisterForPerFrameCallback()
    {
        if (m_animGraphPlugin && m_registeredForPerFrameCallback)
        {
            m_animGraphPlugin->UnregisterPerFrameCallback(this);
            m_registeredForPerFrameCallback = false;
        }
    }

    EMotionFX::BlendSpace2DNode* BlendSpace2DNodeWidget::GetCurrentNode() const
    {
        return m_currentNode;
    }

    EMotionFX::BlendSpace2DNode::UniqueData* BlendSpace2DNodeWidget::GetUniqueData() const
    {
        EMotionFX::BlendSpace2DNode* blendSpaceNode = GetCurrentNode();
        if (!blendSpaceNode)
        {
            return nullptr;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        if (!animGraphInstance)
        {
            return nullptr;
        }

        // Check that we are looking at the correct animgrah instance
        const EMotionFX::AnimGraphNode* thisNode = animGraphInstance->GetAnimGraph()->RecursiveFindNodeById(blendSpaceNode->GetId());
        if (thisNode != blendSpaceNode)
        {
            return nullptr;
        }

        return static_cast<EMotionFX::BlendSpace2DNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(blendSpaceNode));
    }
} // namespace EMStudio
