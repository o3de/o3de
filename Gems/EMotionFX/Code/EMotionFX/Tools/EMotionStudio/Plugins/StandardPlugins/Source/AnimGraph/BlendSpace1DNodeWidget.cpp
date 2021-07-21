/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendSpace1DNodeWidget.h"
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>

namespace EMStudio
{
    const int   BlendSpace1DNodeWidget::s_motionPointCircleWidth = 4;
    const int   BlendSpace1DNodeWidget::s_leftMargin = 35;
    const int   BlendSpace1DNodeWidget::s_rightMargin = 15;
    const int   BlendSpace1DNodeWidget::s_topMargin = 15;
    const int   BlendSpace1DNodeWidget::s_bottomMargin = 35;
    const int   BlendSpace1DNodeWidget::s_maxTextDim = 1000; // maximum height/width of text. Used in creating the rectangle for drawText.
    const int   BlendSpace1DNodeWidget::s_textWidthMargin = 60;
    const float BlendSpace1DNodeWidget::s_maxZoomScale = 10.0f;
    const int   BlendSpace1DNodeWidget::s_subGridSpacing = 10;
    const int   BlendSpace1DNodeWidget::s_gridSpacing = 100;

    BlendSpace1DNodeWidget::BlendSpace1DNodeWidget(AnimGraphPlugin* animGraphPlugin, QWidget* parent)
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

        color.setRgb(0xBB, 0xBB, 0xBB);
        m_pointBrush.setColor(color);
        m_pointBrush.setStyle(Qt::SolidPattern);

        color.setRgb(0x22, 0x22, 0x22);
        m_infoTextBackgroundBrush.setColor(color);
        m_infoTextBackgroundBrush.setStyle(Qt::SolidPattern);

        m_infoTextFont.setPixelSize(8);
        m_infoTextFontMetrics = new QFontMetrics(m_infoTextFont);

        setFocusPolicy((Qt::FocusPolicy)(Qt::ClickFocus | Qt::WheelFocus));
        setMouseTracking(true);
    }

    BlendSpace1DNodeWidget::~BlendSpace1DNodeWidget()
    {
        UnregisterForPerFrameCallback();
        delete m_infoTextFontMetrics;
    }

    void BlendSpace1DNodeWidget::SetCurrentNode(EMotionFX::AnimGraphNode* node)
    {
        if (m_currentNode)
        {
            m_currentNode->SetInteractiveMode(false);
        }
        m_currentNode = nullptr;

        if (node)
        {
            if (azrtti_typeid(node) == azrtti_typeid<EMotionFX::BlendSpace1DNode>())
            {
                m_currentNode = static_cast<EMotionFX::BlendSpace1DNode*>(node);
                m_currentNode->SetInteractiveMode(true);

                // Once in interactive mode, the GUI is responsible for setting the current position.
                // So, initialize it.
                EMotionFX::BlendSpace1DNode::UniqueData* uniqueData = GetUniqueData();
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

    void BlendSpace1DNodeWidget::ProcessFrame([[maybe_unused]] float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || visibleRegion().isEmpty())
            return;

        update();
    }

    void BlendSpace1DNodeWidget::paintEvent([[maybe_unused]] QPaintEvent* event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

        const EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        if (!animGraphInstance)
        {
            painter.drawText(rect(), Qt::AlignCenter, "No anim graph active.");
        }

        EMotionFX::BlendSpace1DNode::UniqueData* uniqueData = GetUniqueData();
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

        const AZStd::vector<float>& points = uniqueData->m_motionCoordinates;
        const size_t numPoints = points.size();

        if (animGraphInstance
            && m_currentNode
            && !animGraphInstance->GetIsOutputReady(nodeThatShouldBeReady->GetObjectIndex()))
        {
            PrepareForDrawing(uniqueData);
            if (m_scale.GetX() <= 0)
            {
                // This happens if the window is so small that there is no space to draw after leaving margins
                return;
            }

            DrawBoundRect(painter, uniqueData);
            DrawBlendSpaceInfoText(painter, "The blend tree containing this blend space node is currently not in active state. "
                "To be able to interactively visualize the operation of this blend space, set the blend tree containing this node "
                "to active state.");
        }
        else if (uniqueData->m_motionCoordinates.empty()
                 || !GetCurrentNode()->GetValidCalculationMethodAndEvaluator())
        {
            PrepareForDrawing(uniqueData);
            if (m_scale(0) <= 0)
            {
                // This happens if the window is so small that there is no space to draw after leaving margins
                return;
            }

            DrawBoundRect(painter, uniqueData);
            DrawBlendSpaceInfoText(painter, "You will create a blend space by selecting the calculation method for the axis "
                "and adding motions to blend using the Attributes window below.\n\nFor the axis, you can choose to have the "
                "coordinates of the motions to be calculated automatically or to enter them manually. To have it calculated "
                "automatically, pick one of the available evaluators. The evaluators calculate the coordinate by analyzing the "
                "motion.");
        }
        else 
        {
            DrawGrid(painter);
            m_warningBoundRect.setRect(0, 0, 0, 0);

            if (numPoints < 2) 
            {
                DrawBlendSpaceWarningText(painter, "At least two motion coordinates are required.");
            }
            else if (uniqueData->m_hasOverlappingCoordinates)
            {
                DrawBlendSpaceWarningText(painter, "Two or more motions are sharing the same coordinates, which might cause inaccurate blended "
                    "animations. Please check the coordinates and try again.");
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
                AZ::Vector2 transformedPt = TransformToScreenCoords(points[i]);
                m_renderPoints[i].setX(transformedPt.GetX());
                m_renderPoints[i].setY(transformedPt.GetY());
            }

            DrawAxisLabels(painter, uniqueData);
            DrawMotionsLine(painter, uniqueData);
            DrawPoints(painter, uniqueData);
            DrawCurrentPointAndBlendingInfluence(painter, uniqueData);
            DrawHoverMotionInfo(painter, uniqueData);
        }
    }

    void BlendSpace1DNodeWidget::mousePressEvent(QMouseEvent* event)
    {
        if (!m_currentNode)
        {
            return;
        }
        if (event->buttons() &  Qt::LeftButton)
        {
            SetCurrentSamplePosition(event->x(), event->y());
            setCursor(Qt::ClosedHandCursor);  // dragging the hotspot
        }
        else
        {
            setCursor(Qt::ArrowCursor); // not dragging the hotspot
        }
    }

    void BlendSpace1DNodeWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        OnMouseMove(event->x(), event->y());
    }

    void BlendSpace1DNodeWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (!m_currentNode)
        {
            return;
        }
        const AZ::u32 prevHoverMotionIndex = m_hoverMotionIndex;

        if (event->buttons() &  Qt::LeftButton)
        {
            SetCurrentSamplePosition(event->x(), event->y());
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

    void BlendSpace1DNodeWidget::PrepareForDrawing(EMotionFX::BlendSpace1DNode::UniqueData* uniqueData)
    {
        const float max = uniqueData->GetRangeMax();
        const float min = uniqueData->GetRangeMin();

        const float rangeX = std::max(1e-8f, max - min);
        const float rangeY = 2; // always from -1 to +1

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

        const float centerX = (min + max) / 2;
        m_scale.Set(scaleX, scaleY);
        m_shift.Set(m_drawCenterX - centerX * scaleX, aznumeric_cast<float>(m_drawCenterY));
    }

    void BlendSpace1DNodeWidget::DrawGrid(QPainter& painter)
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

    void BlendSpace1DNodeWidget::DrawAxisLabels(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData)
    {
        painter.setPen(m_axisLabelPen);

        const int rectLeft = m_drawRect.left();
        const int rectRight = m_drawRect.right();
        const int rectBottom = m_drawRect.bottom();
        const int xValueTop = rectBottom + 4;
        const int xAxisLabelTop = m_drawCenterY + 15;
        const char numFormat = 'g';
        const int  numPrecision = 4;

        // x axis label
        const char* axislabel = m_currentNode->GetAxisLabel();
        painter.drawText(QRect(m_drawCenterX - s_maxTextDim / 2, xAxisLabelTop, s_maxTextDim, s_maxTextDim), axislabel, Qt::AlignHCenter | Qt::AlignTop);

        // if we are in a situation without points, we want to draw reference axis from 0 to 1
        const bool referenceAxis = uniqueData->m_motionCoordinates.empty();

        // x axis values
        const AZ::Vector2 axisLimits = referenceAxis ? AZ::Vector2(0.0f, 1.0f) : AZ::Vector2(uniqueData->GetRangeMin(), uniqueData->GetRangeMax());

        QString tempStr;
        tempStr.setNum(axisLimits(0), numFormat, numPrecision);
        painter.drawText(QRect(rectLeft - s_maxTextDim / 2, xAxisLabelTop, s_maxTextDim, s_maxTextDim), tempStr, Qt::AlignHCenter | Qt::AlignTop);
        tempStr.setNum(axisLimits(1), numFormat, numPrecision);
        painter.drawText(QRect(rectRight - s_maxTextDim / 2, xAxisLabelTop, s_maxTextDim, s_maxTextDim), tempStr, Qt::AlignHCenter | Qt::AlignTop);
    }

    void BlendSpace1DNodeWidget::DrawBoundRect(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* /* uniqueData */)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_backgroundRectBrush);
        painter.drawRect(m_drawRect);
    }

    void BlendSpace1DNodeWidget::DrawMotionsLine(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData)
    {
        const size_t numMotions = m_renderPoints.size();
        if (numMotions < 2)
        {
            return;
        }
        const QPointF& lineStart = m_renderPoints[uniqueData->m_sortedMotions.front()];
        const QPointF& lineEnd = m_renderPoints[uniqueData->m_sortedMotions.back()];

        painter.setPen(m_edgePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(lineStart, lineEnd);
    }

    void BlendSpace1DNodeWidget::DrawPoints(QPainter& painter, [[maybe_unused]] EMotionFX::BlendSpace1DNode::UniqueData* uniqueData)
    {
        painter.setPen(QPen());
        painter.setBrush(m_pointBrush);
        for (size_t i = 0, numPoints = m_renderPoints.size(); i < numPoints; ++i)
        {
            painter.drawEllipse(m_renderPoints[i], s_motionPointCircleWidth, s_motionPointCircleWidth);
        }
    }

    void BlendSpace1DNodeWidget::DrawCurrentPointAndBlendingInfluence(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData)
    {
        AZ::Vector2 transformedPt = TransformToScreenCoords(uniqueData->m_currentPosition);
        QPointF samplePoint(transformedPt(0), transformedPt(1));

        if (uniqueData->m_currentSegment.m_segmentIndex != MCORE_INVALIDINDEX32)
        {
            const AZ::u32 segIndex = uniqueData->m_currentSegment.m_segmentIndex;
            AZ::u32 endVerts[2] = { uniqueData->m_sortedMotions[segIndex], uniqueData->m_sortedMotions[segIndex + 1] };

            painter.setPen(m_highlightedEdgePen);
            painter.drawLine(m_renderPoints[endVerts[0]], m_renderPoints[endVerts[1]]);

            for (int i = 0; i < 2; ++i)
            {
                AZ::u32 pointIdx = endVerts[i];
                const QPointF& endScreenVert = m_renderPoints[pointIdx];
                const float blendWeight = (i == 0) ? (1.0f - uniqueData->m_currentSegment.m_weightForSegmentEnd) : uniqueData->m_currentSegment.m_weightForSegmentEnd;

                RenderSampledMotionPoint(painter, endScreenVert, blendWeight);

                m_tempStrArray.clear();
                const EMotionFX::MotionInstance* motionInstance = uniqueData->m_motionInfos[pointIdx].m_motionInstance;
                m_tempStrArray.push_back(motionInstance->GetMotion()->GetName());
                m_tempStrArray.push_back(QString::asprintf("Blend weight: %.1f%%", blendWeight * 100.0f));
                m_tempStrArray.push_back(QString::asprintf("(%g)", uniqueData->m_motionCoordinates[pointIdx]));
                DrawInfoText(painter, endScreenVert, m_tempStrArray);
            }
        }
        else
        {
            const float rangeMin = uniqueData->GetRangeMin();
            AZ::u32 pointIdx = (uniqueData->m_currentPosition <= rangeMin) ? uniqueData->m_sortedMotions.front() : uniqueData->m_sortedMotions.back();
            const QPointF& endScreenVert = m_renderPoints[pointIdx];

            RenderSampledMotionPoint(painter, endScreenVert, 1.0f);

            m_tempStrArray.clear();
            const EMotionFX::MotionInstance* motionInstance = uniqueData->m_motionInfos[pointIdx].m_motionInstance;
            m_tempStrArray.push_back(motionInstance->GetMotion()->GetName());
            m_tempStrArray.push_back(QString::asprintf("Blend weight: 100%%"));
            m_tempStrArray.push_back(QString::asprintf("(%g)", uniqueData->m_motionCoordinates[pointIdx]));
            DrawInfoText(painter, endScreenVert, m_tempStrArray);
        }

        m_tempStrArray.clear();
        m_tempStrArray.push_back(QString::asprintf("(%g)", uniqueData->m_currentPosition));
        DrawInfoText(painter, samplePoint, m_tempStrArray);

        RenderCurrentSamplePoint(painter, samplePoint);
    }

    void BlendSpace1DNodeWidget::DrawHoverMotionInfo(QPainter& painter, EMotionFX::BlendSpace1DNode::UniqueData* uniqueData)
    {
        if (m_hoverMotionIndex != MCORE_INVALIDINDEX32)
        {
            m_tempStrArray.clear();
            EMotionFX::MotionInstance* motionInstance = uniqueData->m_motionInfos[m_hoverMotionIndex].m_motionInstance;
            m_tempStrArray.push_back(motionInstance->GetMotion()->GetName());
            DrawInfoText(painter, m_renderPoints[m_hoverMotionIndex], m_tempStrArray);
        }
    }

    void BlendSpace1DNodeWidget::DrawInfoText(QPainter& painter, const QPointF& loc, const AZStd::vector<QString>& strArray)
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

        QRect rect(QPoint(aznumeric_cast<int>(loc.x() - s_maxTextDim), aznumeric_cast<int>(loc.y() - s_maxTextDim)), QPoint(aznumeric_cast<int>(loc.x() + s_maxTextDim), m_drawCenterY - 13));

        // avoid the text occluding the motion point
        rect.translate(0, -s_motionPointCircleWidth);

        const int flags = Qt::AlignBottom | Qt::AlignHCenter;

        QRect boundRect = m_infoTextFontMetrics->boundingRect(rect, flags, textToDraw);
        boundRect.adjust(-3, -3, 3, 4);

        // Draw background rect for the text
        painter.setBrush(m_infoTextBackgroundBrush);
        painter.setPen(Qt::NoPen);
        painter.drawRect(boundRect);

        // Draw the text
        painter.setPen(m_infoTextPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(rect, flags, textToDraw);
    }

    void BlendSpace1DNodeWidget::DrawBlendSpaceInfoText(QPainter& painter, const char* infoText) const
    {
        painter.setPen(m_infoTextPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(m_drawRect, Qt::AlignCenter | Qt::TextWordWrap, infoText);
    }

    void BlendSpace1DNodeWidget::DrawBlendSpaceWarningText(QPainter& painter, const char* warningText)
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

    void BlendSpace1DNodeWidget::SetCurrentSamplePosition(int windowX, int windowY)
    {
        EMotionFX::AnimGraphInstance* animGraphInstance = m_modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
        EMotionFX::BlendSpace1DNode::UniqueData* uniqueData = GetUniqueData();
        if (!uniqueData || !animGraphInstance)
        {
            return;
        }

        const AZ::Vector2 screenCoords((float)windowX, (float)windowY);
        const float currentPosition = TransformFromScreenCoords(screenCoords);
        if (currentPosition != uniqueData->m_currentPosition)
        {
            m_currentNode->SetCurrentPosition(currentPosition);
            update();
        }
    }

    void BlendSpace1DNodeWidget::OnMouseMove(int windowX, int windowY)
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

        EMotionFX::BlendSpace1DNode::UniqueData* uniqueData = GetUniqueData();
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

    void BlendSpace1DNodeWidget::RegisterForPerFrameCallback()
    {
        if (m_animGraphPlugin && !m_registeredForPerFrameCallback)
        {
            m_animGraphPlugin->RegisterPerFrameCallback(this);
            m_registeredForPerFrameCallback = true;
        }
    }

    void BlendSpace1DNodeWidget::UnregisterForPerFrameCallback()
    {
        if (m_animGraphPlugin && m_registeredForPerFrameCallback)
        {
            m_animGraphPlugin->UnregisterPerFrameCallback(this);
            m_registeredForPerFrameCallback = false;
        }
    }

    EMotionFX::BlendSpace1DNode* BlendSpace1DNodeWidget::GetCurrentNode() const
    {
        return m_currentNode;
    }

    EMotionFX::BlendSpace1DNode::UniqueData* BlendSpace1DNodeWidget::GetUniqueData() const
    {
        EMotionFX::BlendSpace1DNode* blendSpaceNode = GetCurrentNode();
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

        return static_cast<EMotionFX::BlendSpace1DNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(blendSpaceNode));
    }
} // namespace EMStudio
