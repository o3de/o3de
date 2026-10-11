/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendSpace2DNodeWidget.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <AzCore/std/math.h>
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QFontMetrics>

#include <AzCore/Math/MathUtils.h>

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
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpace2DNodeWidget, AZ::SystemAllocator)

    const int       BlendSpace2DNodeWidget::s_motionPointCircleWidth = 4;
    const int       BlendSpace2DNodeWidget::s_leftMargin = 70;
    const int       BlendSpace2DNodeWidget::s_rightMargin = 20;
    const int       BlendSpace2DNodeWidget::s_topMargin = 55;
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
        m_divisionPen.setColor(QColor(90, 90, 90));

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
        InitializePreviewControls(this);
    }

    BlendSpace2DNodeWidget::~BlendSpace2DNodeWidget()
    {
        UnregisterForPerFrameCallback();
        ResetPreview();
        delete m_infoTextFontMetrics;
    }


    void BlendSpace2DNodeWidget::SetCurrentNode(EMotionFX::AnimGraphNode* node)
    {
        EndMotionDrag();
        m_selectedPointIndex = MCORE_INVALIDINDEX32;
        if (m_currentNode)
        {
            m_currentNode->SetInteractiveMode(false);
        }
        ResetPreview();
        m_renderPoints.clear();
        m_hoverMotionIndex = MCORE_INVALIDINDEX32;
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
                    SetPreviewPosition(uniqueData->m_currentPosition);
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

        EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
        if (!uniqueData)
        {
            m_renderPoints.clear();
            m_hoverMotionIndex = MCORE_INVALIDINDEX32;
            painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, GetPreviewStatus());
            return;
        }
        m_zoomScale = AZ::Lerp(1.0f, s_maxZoomScale, m_zoomFactor);

        const AZStd::vector<AZ::Vector2>& points = uniqueData->m_motionCoordinates;
        const size_t numPoints = points.size();
        UpdateDisplayRange(uniqueData);

        if (!GetCurrentNode()->GetValidCalculationMethodsAndEvaluators())
        {
            m_renderPoints.clear();
            m_hoverMotionIndex = MCORE_INVALIDINDEX32;
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

            if (numPoints > 0 && numPoints < 3)
            {
                DrawBlendSpaceWarningText(painter, "At least three motion coordinates are required.");
            }
            else if (numPoints >= 3 && uniqueData->m_triangles.empty())
            {
                DrawBlendSpaceWarningText(painter, "Two or more motions are sharing the same coordinates, which might cause inaccurate blended "
                                          "animations. Please check the coordinates and try again.");
            }
            else if (uniqueData->m_hasDegenerateTriangles)
            {
                DrawBlendSpaceWarningText(painter, "Two or more motions have coordinates too close to each other, which might cause inaccurate "
                                          "blended animations. Please check the coordinates and try again.");
            }
            else if (HasMotionsOutsideCustomRange(uniqueData))
            {
                DrawBlendSpaceWarningText(painter, "Some motions lie outside the custom axis range. Drag them into the highlighted area or widen the range.");
            }

            PrepareForDrawing(uniqueData);
            if (m_scale(0) <= 0)
            {
                // This happens if the window is so small that there is no space to draw after leaving margins
                return;
            }
            DrawBoundRect(painter, uniqueData);
            DrawAxisGrid(painter);

            m_renderPoints.resize(numPoints);
            for (size_t i = 0; i < numPoints; ++i)
            {
                const AZ::Vector2 transformedPt = TransformToScreenCoords(points[i]);
                m_renderPoints[i].setX(transformedPt.GetX());
                m_renderPoints[i].setY(transformedPt.GetY());
            }

            DrawAxisLabels(painter, uniqueData);
            if (numPoints == 0)
            {
                DrawBlendSpaceInfoText(painter, "Right-click the grid to add a motion at that position.");
                return;
            }
            DrawPoints(painter, uniqueData);
            DrawTriangles(painter, uniqueData);
            DrawCurrentPointAndBlendingInfluence(painter, uniqueData);
            DrawSelectedMotion(painter);
            DrawHoverMotionInfo(painter, uniqueData);
        }
    }

    void BlendSpace2DNodeWidget::mousePressEvent(QMouseEvent* event)
    {
        if (!m_currentNode)
        {
            return;
        }
        if (event->button() == Qt::LeftButton)
        {
            const int windowX = aznumeric_cast<int>(event->position().x());
            const int windowY = aznumeric_cast<int>(event->position().y());

            // Clicking a motion selects it and starts dragging it, clicking elsewhere moves the sample point.
            const AZ::u32 pointIndex = FindPointAt(windowX, windowY);
            if (pointIndex != MCORE_INVALIDINDEX32 && BeginMotionDrag(m_currentNode, pointIndex))
            {
                setCursor(Qt::SizeAllCursor);
                update();
                return;
            }

            m_selectedPointIndex = MCORE_INVALIDINDEX32;
            SetCurrentSamplePoint(windowX, windowY);
            setCursor(Qt::ClosedHandCursor);  // dragging the hotspot
        }
        else
        {
            setCursor(Qt::ArrowCursor); // not dragging the hotspot
        }
    }

    void BlendSpace2DNodeWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && IsDraggingMotion())
        {
            EndMotionDrag();
        }
        OnMouseMove(aznumeric_cast<int>(event->position().x()), aznumeric_cast<int>(event->position().y()));
        update();
    }

    void BlendSpace2DNodeWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        if (!m_currentNode || IsDraggingMotion() || !GetUniqueData() || !m_currentNode->GetValidCalculationMethodsAndEvaluators())
        {
            return;
        }
        const QPoint pos = event->pos();
        const AZ::u32 pointIndex = FindPointAt(pos.x(), pos.y());
        if (pointIndex == MCORE_INVALIDINDEX32 && !m_drawRect.contains(pos))
        {
            return;
        }
        m_selectedPointIndex = pointIndex;
        update();
        ShowGridContextMenu(m_currentNode, event->globalPos(), pointIndex, GridCoordinatesAt(pos.x(), pos.y()));
        update();
    }

    void BlendSpace2DNodeWidget::keyPressEvent(QKeyEvent* event)
    {
        const bool deleteKey = event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace;
        if (deleteKey && m_currentNode && !IsDraggingMotion() && m_selectedPointIndex < m_renderPoints.size())
        {
            RemoveMotionAtPoint(m_currentNode, m_selectedPointIndex);
            update();
            event->accept();
            return;
        }
        AnimGraphNodeWidget::keyPressEvent(event);
    }

    void BlendSpace2DNodeWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (!m_currentNode)
        {
            return;
        }
        if (IsDraggingMotion())
        {
            UpdateMotionDrag(GridCoordinatesAt(aznumeric_cast<int>(event->position().x()), aznumeric_cast<int>(event->position().y())));
            update();
            return;
        }
        const AZ::u32 prevHoverMotionIndex = m_hoverMotionIndex;

        if (event->buttons() &  Qt::LeftButton)
        {
            SetCurrentSamplePoint(event->position().x(), event->position().y());
            m_hoverMotionIndex = MCORE_INVALIDINDEX32;
        }
        else
        {
            OnMouseMove(event->position().x(), event->position().y());
        }

        if (m_hoverMotionIndex != prevHoverMotionIndex)
        {
            update();
        }
    }

    void BlendSpace2DNodeWidget::UpdateDisplayRange(const EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        // Keep the view still while a motion is dragged, so the grid does not rescale under the cursor.
        if (IsDraggingMotion())
        {
            return;
        }
        const bool hasMotions = !uniqueData->m_motionCoordinates.empty();
        for (int axis = 0; axis < 2; ++axis)
        {
            float minValue = 0.0f;
            float maxValue = 1.0f;
            m_customRange[axis] = m_currentNode->GetAxis(axis).GetCustomRange(minValue, maxValue);
            const float motionMin = hasMotions ? uniqueData->m_rangeMin.GetElement(axis) : minValue;
            const float motionMax = hasMotions ? uniqueData->m_rangeMax.GetElement(axis) : maxValue;
            if (m_customRange[axis])
            {
                m_gridMin.SetElement(axis, minValue);
                m_gridMax.SetElement(axis, maxValue);

                // Grow past the custom range only as far as needed to show the motions outside it.
                const float padding = (maxValue - minValue) * 0.05f;
                if (motionMin < minValue)
                {
                    minValue = motionMin - padding;
                }
                if (motionMax > maxValue)
                {
                    maxValue = motionMax + padding;
                }
            }
            else
            {
                if (hasMotions)
                {
                    // Fit the motions, with room around them to place or drag motions further out.
                    const float padding = motionMax > motionMin ? (motionMax - motionMin) * 0.1f : 0.5f;
                    minValue = motionMin - padding;
                    maxValue = motionMax + padding;
                }
                m_gridMin.SetElement(axis, minValue);
                m_gridMax.SetElement(axis, maxValue);
            }
            m_displayMin.SetElement(axis, minValue);
            m_displayMax.SetElement(axis, maxValue);
        }
    }

    AZStd::vector<float> BlendSpace2DNodeWidget::GetGridLines(int axis, int& outLabelStride)
    {
        AZStd::vector<float> lines;
        outLabelStride = 1;
        const AZ::u32 divisions = AZStd::max<AZ::u32>(m_currentNode->GetAxis(axis).m_gridDivisions, 1);
        const float gridMin = m_gridMin.GetElement(axis);
        const float step = (m_gridMax.GetElement(axis) - gridMin) / divisions;
        const float displaySpan = m_displayMax.GetElement(axis) - m_displayMin.GetElement(axis);
        if (step <= 0.0f || displaySpan <= 0.0f)
        {
            return lines;
        }

        // Keep the grid spacing past a custom range, thinning the lines if motions lie far outside it.
        const int first = aznumeric_cast<int>(AZStd::ceil((m_displayMin.GetElement(axis) - gridMin) / step - 0.001f));
        const int last = aznumeric_cast<int>(AZStd::floor((m_displayMax.GetElement(axis) - gridMin) / step + 0.001f));
        const int lineStride = AZStd::max(1, (last - first) / 200 + 1);
        // Start on a multiple of the stride so the custom range edges keep their lines.
        const int start = first + ((-first % lineStride) + lineStride) % lineStride;
        for (int i = start; i <= last; i += lineStride)
        {
            lines.push_back(gridMin + i * step);
        }

        const float pixelsPerLine = (axis == 0 ? m_drawRect.width() : m_drawRect.height()) * step * lineStride / displaySpan;
        const float minPixels = axis == 0 ? 40.0f : 20.0f;
        outLabelStride = pixelsPerLine >= minPixels ? 1 : aznumeric_cast<int>(AZStd::ceil(minPixels / AZStd::max(pixelsPerLine, 0.001f)));
        return lines;
    }

    bool BlendSpace2DNodeWidget::HasMotionsOutsideCustomRange(const EMotionFX::BlendSpace2DNode::UniqueData* uniqueData) const
    {
        if (uniqueData->m_motionCoordinates.empty())
        {
            return false;
        }
        for (int axis = 0; axis < 2; ++axis)
        {
            const float tolerance = (m_gridMax.GetElement(axis) - m_gridMin.GetElement(axis)) * 0.0001f;
            if (m_customRange[axis] && (uniqueData->m_rangeMin.GetElement(axis) < m_gridMin.GetElement(axis) - tolerance
                || uniqueData->m_rangeMax.GetElement(axis) > m_gridMax.GetElement(axis) + tolerance))
            {
                return true;
            }
        }
        return false;
    }

    void BlendSpace2DNodeWidget::PrepareForDrawing([[maybe_unused]] EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        const AZ::Vector2& max = m_displayMax;
        const AZ::Vector2& min = m_displayMin;

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

        const AZ::Vector2 center = (min + max) * 0.5f;
        m_scale.Set(scaleX, scaleY);
        m_shift.Set(m_drawCenterX - center.GetX() * scaleX, m_drawCenterY - center.GetY() * scaleY);
    }

    void BlendSpace2DNodeWidget::DrawAxisGrid(QPainter& painter)
    {
        painter.setPen(m_divisionPen);
        painter.setBrush(Qt::NoBrush);
        for (int axis = 0; axis < 2; ++axis)
        {
            int labelStride = 1;
            for (const float value : GetGridLines(axis, labelStride))
            {
                const AZ::Vector2 screen = TransformToScreenCoords(axis == 0 ? AZ::Vector2(value, m_displayMin.GetY()) : AZ::Vector2(m_displayMin.GetX(), value));
                if (axis == 0)
                {
                    painter.drawLine(QPointF(screen.GetX(), m_drawRect.top()), QPointF(screen.GetX(), m_drawRect.bottom()));
                }
                else
                {
                    painter.drawLine(QPointF(m_drawRect.left(), screen.GetY()), QPointF(m_drawRect.right(), screen.GetY()));
                }
            }
        }
    }

    void BlendSpace2DNodeWidget::DrawSelectedMotion(QPainter& painter)
    {
        if (m_selectedPointIndex >= m_renderPoints.size())
        {
            return;
        }
        painter.setPen(m_highlightedEdgePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_renderPoints[m_selectedPointIndex], s_motionPointCircleWidth + 4, s_motionPointCircleWidth + 4);
    }

    AZ::u32 BlendSpace2DNodeWidget::FindPointAt(int windowX, int windowY) const
    {
        float minDistSqr = 36.0f;
        AZ::u32 closestMotionIdx = MCORE_INVALIDINDEX32;
        for (AZ::u32 i = 0; i < m_renderPoints.size(); ++i)
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
        return closestMotionIdx;
    }

    AZ::Vector2 BlendSpace2DNodeWidget::GridCoordinatesAt(int windowX, int windowY)
    {
        AZ::Vector2 coordinates = TransformFromScreenCoords(AZ::Vector2(aznumeric_cast<float>(windowX), aznumeric_cast<float>(windowY)));
        for (int axis = 0; axis < 2; ++axis)
        {
            const float minValue = m_gridMin.GetElement(axis);
            const float maxValue = m_gridMax.GetElement(axis);
            float value = AZ::GetClamp(coordinates.GetElement(axis), minValue, maxValue);
            const EMotionFX::BlendSpaceNode::BlendSpaceAxis& axisSettings = m_currentNode->GetAxis(axis);
            if (axisSettings.m_snapToGrid)
            {
                value = axisSettings.SnapToGrid(value, minValue, maxValue);
            }
            coordinates.SetElement(axis, value);
        }
        return coordinates;
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

    void BlendSpace2DNodeWidget::DrawAxisLabels(QPainter& painter, [[maybe_unused]] EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        painter.setPen(m_axisLabelPen);

        const int rectLeft = m_drawRect.left();
        const int rectBottom = m_drawRect.bottom();
        const int xValueTop = rectBottom + 4;
        const int yValueRight = rectLeft - 2;
        const int xAxisLabelTop = rectBottom + 20;
        const char numFormat = 'g';
        const int  numPrecision = 4;

        // x axis label
        const char* axisLabelX = m_currentNode->GetAxisLabel(0);
        painter.drawText(QRect(m_drawCenterX - s_maxTextDim / 2, xAxisLabelTop, s_maxTextDim, s_maxTextDim), axisLabelX, Qt::AlignHCenter | Qt::AlignTop);

        // x axis values, skipping grid lines that are too close together to label
        int labelStrideX = 1;
        const AZStd::vector<float> linesX = GetGridLines(0, labelStrideX);
        for (size_t i = 0; i < linesX.size(); i += labelStrideX)
        {
            const float value = linesX[i];
            const int screenX = aznumeric_cast<int>(TransformToScreenCoords(AZ::Vector2(value, m_displayMin.GetY())).GetX());
            m_tempString.setNum(value, numFormat, numPrecision);
            painter.drawText(QRect(screenX - s_maxTextDim / 2, xValueTop, s_maxTextDim, s_maxTextDim), m_tempString, Qt::AlignHCenter | Qt::AlignTop);
        }

        // y axis values
        int labelStrideY = 1;
        const AZStd::vector<float> linesY = GetGridLines(1, labelStrideY);
        for (size_t i = 0; i < linesY.size(); i += labelStrideY)
        {
            const float value = linesY[i];
            const int screenY = aznumeric_cast<int>(TransformToScreenCoords(AZ::Vector2(m_displayMin.GetX(), value)).GetY());
            m_tempString.setNum(value, numFormat, numPrecision);
            painter.drawText(QRect(yValueRight - s_maxTextDim, screenY - s_maxTextDim / 2, s_maxTextDim, s_maxTextDim), m_tempString, Qt::AlignVCenter | Qt::AlignRight);
        }

        const char* axisLabelY = m_currentNode->GetAxisLabel(1);
        painter.rotate(-90);
        // Since the coordinate system has been rotated -90 degrees, we have to specify the rectangle coordinates accordingly. In
        // particular, the -x and y axes will correspond to the normal y and x axes respectively.
        painter.drawText(QRect(-(m_drawCenterY + s_maxTextDim / 2), rectLeft - 62, s_maxTextDim, s_maxTextDim), axisLabelY, Qt::AlignHCenter | Qt::AlignTop);

        painter.resetTransform();
    }

    void BlendSpace2DNodeWidget::DrawBoundRect(QPainter& painter, EMotionFX::BlendSpace2DNode::UniqueData* uniqueData)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_backgroundRectBrush);

        const bool hasMotions = !uniqueData->m_motionCoordinates.empty();
        if (hasMotions || m_customRange[0] || m_customRange[1])
        {
            // The custom range where an axis has one, otherwise the extent of the motions.
            AZ::Vector2 boundMin = hasMotions ? uniqueData->m_rangeMin : m_displayMin;
            AZ::Vector2 boundMax = hasMotions ? uniqueData->m_rangeMax : m_displayMax;
            for (int axis = 0; axis < 2; ++axis)
            {
                if (m_customRange[axis])
                {
                    boundMin.SetElement(axis, m_gridMin.GetElement(axis));
                    boundMax.SetElement(axis, m_gridMax.GetElement(axis));
                }
            }
            const AZ::Vector2 topLeft = TransformToScreenCoords(boundMin);
            const AZ::Vector2 bottomRight = TransformToScreenCoords(boundMax);
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
        if (m_hoverMotionIndex < m_renderPoints.size() && m_hoverMotionIndex < uniqueData->m_motionInfos.size())
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
        const QRect warningRect(10, 45, width() - 20, height() - 55);
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
        EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
        if (!uniqueData || m_renderPoints.empty() || !m_drawRect.contains(windowX, windowY))
        {
            return;
        }

        const AZ::Vector2 screenCoords((float)windowX, (float)windowY);
        AZ::Vector2 currentPosition = TransformFromScreenCoords(screenCoords);
        if (currentPosition != uniqueData->m_currentPosition)
        {
            m_currentNode->SetCurrentPosition(currentPosition);
            SetPreviewPosition(currentPosition);
            update();
        }
    }

    void BlendSpace2DNodeWidget::OnMouseMove(int windowX, int windowY)
    {
        m_hoverMotionIndex = FindPointAt(windowX, windowY);

        EMotionFX::BlendSpace2DNode::UniqueData* uniqueData = GetUniqueData();
        if (m_hoverMotionIndex != MCORE_INVALIDINDEX32)
        {
            setCursor(Qt::SizeAllCursor); // indicates that the motion can be dragged
        }
        else if (uniqueData && m_drawRect.contains(windowX, windowY))    // Otherwise we cannot change the hotspot therefore keep the cursor as arrow
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

    void BlendSpace2DNodeWidget::hideEvent(QHideEvent* event)
    {
        EndMotionDrag();
        StopPreviewPlayback();
        AnimGraphNodeWidget::hideEvent(event);
    }

    EMotionFX::BlendSpace2DNode::UniqueData* BlendSpace2DNodeWidget::GetUniqueData()
    {
        return static_cast<EMotionFX::BlendSpace2DNode::UniqueData*>(GetBlendSpaceData(m_currentNode, m_modelIndex));
    }
} // namespace EMStudio
