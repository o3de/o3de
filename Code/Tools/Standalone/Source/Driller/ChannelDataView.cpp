/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Math/MathUtils.h>

#include "ChannelDataView.hxx"
#include <Source/Driller/moc_ChannelDataView.cpp>

#include "ChannelControl.hxx"
#include "ChannelProfilerWidget.hxx"
#include "DrillerAggregator.hxx"
#include "Annotations/Annotations.hxx"

#include <QPainter>
#include <QPen>
#include <QToolTip>
#include <QDesktopWidget>
#include <QScreen>

namespace Driller
{
    static const int k_contractedSize = 28;
    static const int k_expandedSize = 64;
    static const int k_textWidth = 128;

    static const int k_barHeight = 5;

    ////////////////////////
    // AggregatorDataPoint
    ////////////////////////
    AggregatorDataPoint::AggregatorDataPoint()
        : m_isActive(true)
        , m_isShowingOverlay(false)
        , m_shouldOutline(false)
    {
    }

    AggregatorDataPoint::AggregatorDataPoint(QRect rectangle, ChannelProfilerWidget* profilerWidget)
        : m_visualBlock(rectangle)
        , m_isActive(true)
        , m_isShowingOverlay(false)
        , m_shouldOutline(false)

    {
        m_combinedProfilers.insert(profilerWidget);
    }

    void AggregatorDataPoint::Draw(QPainter& painter, int leftEdge, float barWidth)
    {
        QColor drawColor;
        QColor outlineColor;

        m_isActive = false;

        for (ChannelProfilerWidget* currentProfiler : m_combinedProfilers)
        {
            if (currentProfiler->IsActive())
            {
                if (m_isActive)
                {
                    drawColor = QColor(255, 255, 255);
                    outlineColor = QColor(255, 255, 0);
                    break;
                }
                else
                {
                    m_isActive = true;
                    drawColor = currentProfiler->GetAggregator()->GetColor();

                    // right now this should never be used. If it eventually is used, we'll probably
                    // want to add a flow to the aggregators to pass along this color. For now
                    // some silly wrapping to do something noticeable.
                    outlineColor = QColor((drawColor.red() + 100) % 255, (drawColor.green() + 100) % 255, (drawColor.blue() + 100) % 255);
                }
            }
        }

        if (m_isActive)
        {
            const int upAmount = static_cast<int>(ceilf(k_barHeight * 0.5f));

            int centerHeight = m_visualBlock.center().y();
            int topEdge = static_cast<int>(centerHeight - upAmount);

            if (m_shouldOutline)
            {
                static const int k_outlineSize = 1;

                painter.fillRect(
                    leftEdge,
                    topEdge,
                    static_cast<int>(barWidth),
                    k_barHeight,
                    outlineColor);

                painter.fillRect(
                    leftEdge + k_outlineSize,
                    (int)topEdge + k_outlineSize,
                    (int)barWidth - (2 * k_outlineSize),             // overdraw on dense data > 1 event per pixel, optimize later
                    k_barHeight - (2 * k_outlineSize),
                    drawColor);
            }
            else
            {
                painter.fillRect(
                    leftEdge,
                    topEdge,
                    static_cast<int>(barWidth),
                    k_barHeight,
                    drawColor);
            }
        }
    }

    bool AggregatorDataPoint::IntersectsDataPoint(const AggregatorDataPoint& dataPoint)
    {
        return m_visualBlock.intersects(dataPoint.m_visualBlock);
    }

    bool AggregatorDataPoint::ContainsPoint(const QPoint& point)
    {
        // Don't want to collide if we aren't visible.
        if (m_isActive)
        {
            // Our Visual block's horizontal information is only valid when it is created.
            // After that we lazily update information, so the only valid information that the block maintins is the vertical.
            // So we're going to rely on the DataView to manage the horizontal collision, and we'll manage the vertical.
            return (m_visualBlock.bottom() > point.y() && m_visualBlock.top() <= point.y());
        }
        else
        {
            return false;
        }
    }

    void AggregatorDataPoint::AddAggregatorDataPoint(const AggregatorDataPoint& dataPoint)
    {
        m_drawColor = QColor(255, 255, 255);

        // dataPointBottomRight
        QPoint dpBR = dataPoint.m_visualBlock.bottomRight();

        // dataPointTopLeft
        QPoint dpTL = dataPoint.m_visualBlock.topLeft();

        // visualBlockBottomRight
        QPoint vbBR = m_visualBlock.bottomRight();

        // visualBlockTopLeft
        QPoint vbTL = m_visualBlock.topLeft();

        QPoint bottomRight(AZStd::GetMax(dpBR.x(), vbBR.x()), AZStd::GetMax(dpBR.y(), vbBR.y()));
        QPoint topLeft(AZStd::GetMin(dpTL.x(), vbTL.x()), AZStd::GetMin(dpTL.y(), vbTL.y()));

        m_visualBlock = QRect(topLeft, bottomRight);

        m_combinedProfilers.insert(dataPoint.m_combinedProfilers.begin(), dataPoint.m_combinedProfilers.end());
    }

    bool AggregatorDataPoint::SetOverlayEnabled(bool enabled)
    {
        if (m_isShowingOverlay != enabled)
        {
            m_isShowingOverlay = enabled;

            if (m_isShowingOverlay)
            {
                int activeProfilers = 0;
                QString toolTip = "Multiple Profiler(s)";

                for (ChannelProfilerWidget* channelProfiler : m_combinedProfilers)
                {
                    if (channelProfiler->IsActive())
                    {
                        ++activeProfilers;
                        toolTip.append(QString("<BR> - <I>%1</I>").arg(channelProfiler->GetName()));
                    }
                }

                if (activeProfilers >= 2)
                {
                    m_shouldOutline = true;

                    // Since QToolTip::HideText doesn't seem to actually do anything, I'm trying to show
                    // a second tool tip while the first one is being displayed. However,
                    // if I use the same string for the tool tip, it doesn't actually update the position.
                    // But since the cursor has moved away from the original postion, it begins the countdown
                    // timer to hide the tool tip. Meaning, the new tool tip I was trying to generate hides.
                    // Going to alternatingly append a space to the end to force it to be different to
                    // avoid this nonsense.
                    {
                        static bool k_dumbToolTipHack = false;
                        if (k_dumbToolTipHack)
                        {
                            toolTip.append(" ");
                        }

                        k_dumbToolTipHack = !k_dumbToolTipHack;
                    }

                    QToolTip::showText(QCursor::pos(), toolTip);
                }
                else
                {
                    m_shouldOutline = false;
                }
            }
            else
            {
                m_shouldOutline = false;
                QToolTip::hideText();
            }

            return true;
        }

        return false;
    }

    /////////////////
    // BudgetMarker
    /////////////////

    BudgetMarker::BudgetMarker(float value, QColor& drawColor)
        : m_value(value)
        , m_drawColor(drawColor)
    {
    }

    BudgetMarker::~BudgetMarker()
    {
    }

    float BudgetMarker::GetValue() const
    {
        return m_value;
    }

    const QColor& BudgetMarker::GetColor() const
    {
        return m_drawColor;
    }

    ////////////////////
    // ChannelDataView
    ////////////////////
    ChannelDataView::ChannelDataView(QWidget* parent)
        : QWidget(parent)
        , m_Channel(nullptr)
        , m_ptrAnnotations(nullptr)
        , m_minFrame(-1)
        , m_maxFrame(-1)
        , m_highlightedFrame(-1)
        , m_lastFrame(0)
        , m_xOffset(0)
        , m_initializeDrag(false)
        , m_dragInitialized(false)
        , m_shouldIgnorePoint(false)
        , m_mouseGrabbed(false)
        , m_dirtyGraph(true)
    {
        setAutoFillBackground(true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);

        setMouseTracking(true);
    }

    ChannelDataView::~ChannelDataView()
    {
    }

    void ChannelDataView::RegisterToChannel(ChannelControl* channel, AnnotationsProvider* annotations)
    {
        m_Channel = channel;
        m_ptrAnnotations = annotations;
    }

    int ChannelDataView::FrameToPosition(FrameNumberType frameNumber)
    {
        int localOffset = 0;
        FrameNumberType frameDifference = 0;

        if (frameNumber < m_Channel->m_State.m_FrameOffset)
        {
            frameDifference = frameNumber - m_Channel->m_State.m_FrameOffset;
        }
        else if (frameNumber >= (m_Channel->m_State.m_FrameOffset + (m_Channel->m_State.m_FramesInView - 1)))
        {
            frameDifference = frameNumber - (m_Channel->m_State.m_FrameOffset + (m_Channel->m_State.m_FramesInView - 1));
            localOffset = rect().width();
        }

        localOffset += static_cast<int>(GetBarWidth() * frameDifference);

        return mapToGlobal(QPoint(localOffset, 0)).x();
    }

    FrameNumberType ChannelDataView::PositionToFrame(const QPoint& pt)
    {
        QRect wrect = rect();

        FrameNumberType frame = m_Channel->m_State.m_FrameOffset + m_Channel->m_State.m_FramesInView - 1;
        frame = frame <= m_Channel->m_State.m_EndFrame ? frame : m_Channel->m_State.m_EndFrame;

        float pct = (float)pt.x() / (float)wrect.width();
        FrameNumberType rCell = m_Channel->m_State.m_FramesInView - 1 - (int)((float)m_Channel->m_State.m_FramesInView * pct);
        FrameNumberType retFrame = frame - rCell;

        return retFrame;
    }

    FrameNumberType ChannelDataView::FramesPerPixel()
    {
        FrameNumberType range = PositionToFrame(QPoint(0, 0)) - PositionToFrame(QPoint(1, 0));
        return(range > 0 ? range : 1);
    }

    float ChannelDataView::GetBarWidth()
    {
        return ((float)(rect().width()) / (float)(m_Channel->m_State.m_FramesInView));
    }

    void ChannelDataView::paintEvent(QPaintEvent* event)
    {
        (void)event;

        if (m_dirtyGraph)
        {
            m_dirtyGraph = false;
            RecalculateGraphedPoints();
        }

        QPen pen;
        pen.setWidth(1);
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        pen.setBrush(brush);

        QPainter painter(this);

        painter.setPen(pen);
        painter.fillRect(rect(), Qt::black);

        FrameNumberType frame = m_Channel->m_State.m_FrameOffset + m_Channel->m_State.m_FramesInView - 1;
        frame = frame <= m_Channel->m_State.m_EndFrame ? frame : m_Channel->m_State.m_EndFrame;

        QRect wrect = rect();

        float barWidth = GetBarWidth();
        int barWidthHalf = (int)(barWidth / 2.0f);
        float drawBarWidth = ((barWidth - 1.0f) < 1.0f ? 1.0f : (barWidth - 1.0f));

        if (m_Channel->IsActive())
        {
            // PLAYBACK LOOP MARKERS
            if (m_Channel->m_State.m_LoopBegin >= frame - m_Channel->m_State.m_FramesInView)
            {
                float l = rect().right() - barWidth / 2.0f - 1 - barWidth * (frame - m_Channel->m_State.m_LoopBegin);
                painter.fillRect((int)l, 0, 2, rect().height(), QColor(255, 255, 0, 255));
            }
            if (m_Channel->m_State.m_LoopEnd >= frame - m_Channel->m_State.m_FramesInView)
            {
                float l = rect().right() - barWidth / 2.0f - 1 - barWidth * (frame - m_Channel->m_State.m_LoopEnd);
                painter.fillRect((int)l, 0, 2, rect().height(), QColor(255, 255, 0, 255));
            }
        }

        brush.setStyle(Qt::Dense2Pattern);
        brush.setColor(Qt::red);
        int wrectHeight = wrect.height() / (m_Channel->m_State.m_ContractedHeight ? 2 : 1);

        if (m_Channel->IsActive() && m_Channel->m_State.m_EndFrame)
        {
            // SCRUBBER
            if (m_Channel->m_State.m_ScrubberFrame >= frame - m_Channel->m_State.m_FramesInView)
            {
                float l = rect().right()-GetBarWidth()/2.0f - 1 - GetBarWidth() * (frame - m_Channel->m_State.m_ScrubberFrame);
                painter.fillRect( (int)l, 0, 2, rect().height(), brush );
            }

            float rightEdgeOfBar = (float)wrect.right();
            float leftEdgeOfBar = rightEdgeOfBar - barWidth;

            AZStd::list< ChannelProfilerWidget* > activeProfilers;

            for (ChannelProfilerWidget* profiler : m_Channel->GetProfilers())
            {
                if (profiler->IsActive())
                {
                    activeProfilers.push_back(profiler);
                }
            }

            while (frame >= 0 && rightEdgeOfBar >= wrect.left())
            {
                int actualLeftEdge = (int)floorf(leftEdgeOfBar);

                // Graph out precomputed points.
                DataPointList& dataPointList = m_graphedPoints[frame];

                for (AggregatorDataPoint& dataPoint : dataPointList)
                {
                    dataPoint.Draw(painter, actualLeftEdge, drawBarWidth);
                }

                // annotations?
                AnnotationsProvider::ConstAnnotationIterator it = m_ptrAnnotations->GetFirstAnnotationForFrame(frame);
                AnnotationsProvider::ConstAnnotationIterator endIt = m_ptrAnnotations->GetEnd();

                if (it != endIt)
                {
                    painter.fillRect(actualLeftEdge + barWidthHalf, 0, 1, wrectHeight, m_ptrAnnotations->GetColorForChannel(it->GetChannelCRC()));
                    //++it;
                }

                --frame;
                rightEdgeOfBar -= barWidth;
                leftEdgeOfBar -= barWidth;
            }

            // Styling the budget markers.
            // Mildly ugly, but not a lot of pixels to work with
            // to make it look better.
            pen = QPen(Qt::black);
            pen.setWidth(1);

            painter.setPen(pen);
            brush.setStyle(Qt::BrushStyle::SolidPattern);

            // Budget Markers
            for (BudgetMarkerMap::value_type& mapPair : m_budgetMarkers)
            {
                BudgetMarker& budgetMarker = mapPair.second;

                QColor drawColor = budgetMarker.GetColor();

                QRect sizeRect = rect();
                int x = rect().left();
                float normalizedValue = ((budgetMarker.GetValue() + 1.0f) / 2.0f);
                int y = static_cast<int>(rect().bottom() - (rect().height() * normalizedValue));
                int width = rect().width();
                int height = 4;
                brush.setColor(drawColor);

                // Want to make sure we always draw the entire box.
                if (y > rect().bottom() - height)
                {
                    y = rect().bottom() - height;
                }
                
                painter.fillRect(
                    x,
                    y,
                    width,
                    height,
                    brush);

                painter.drawRect(
                    x,
                    y,
                    width,
                    height);
            }
        }
    }

    void ChannelDataView::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_Channel->IsInCaptureMode(CaptureMode::Capturing))
        {
            return;
        }

        if (m_Channel->IsActive())
        {
            if (m_mouseGrabbed)
            {
                if (m_shouldIgnorePoint && event->globalPos() == m_centerPoint)
                {
                    m_shouldIgnorePoint = false;
                    return;
                }

                QPoint mousePoint = event->globalPos();
                int mouseDelta = (mousePoint.x() - m_centerPoint.x());

                if (m_initializeDrag)
                {
                    m_initializeDrag = false;
                    m_dragInitialized = true;

                    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

                    QRect screenGeometry = QApplication::primaryScreen()->geometry();
                    m_centerPoint = screenGeometry.center();

                    mouseDelta = (mousePoint.x() - mapToGlobal(m_simulatedPoint).x());
                }

                m_simulatedPoint.setX(m_simulatedPoint.x() + mouseDelta);

                QPoint framePoint = m_simulatedPoint;
                framePoint.setX(framePoint.x() + m_xOffset);

                FrameNumberType frame = PositionToFrame(framePoint);
                FrameNumberType framesPerPixel = FramesPerPixel();

                // raw frame, sanitized by the controller
                emit InformOfMouseMove(frame, framesPerPixel, event->modifiers());
                event->ignore();

                QRect boundingRect = rect();

                m_shouldIgnorePoint = true;
                QCursor::setPos(m_centerPoint);
                m_xOffset += mouseDelta;

                if (!boundingRect.contains(m_simulatedPoint))
                {
                    m_simulatedPoint.setX(AZ::GetClamp(m_simulatedPoint.x(), boundingRect.left(), boundingRect.right()));
                }
                else
                {
                    m_xOffset = 0;
                }

                if (m_lastFrame != frame)
                {
                    m_lastFrame = frame;

                    if (framesPerPixel == 1)
                    {
                        int barWidth = static_cast<int>(ceilf(GetBarWidth()));
                        m_xOffset %= barWidth;
                    }
                    else
                    {
                        m_xOffset = 0;
                    }
                }

                // Can't scroll beyond the minimum frame.
                if (m_lastFrame == 0 && m_xOffset < 0)
                {
                    m_xOffset = 0;
                }
                // Can't scroll beyond the maximum frame.
                else if (m_lastFrame == m_Channel->m_State.m_EndFrame && m_xOffset > 0)
                {
                    m_xOffset = 0;
                }
            }
            // If we aren't dragging, we need to deal with highlighting
            else
            {
                QPoint pos = event->localPos().toPoint();

                // This guy expects relative position to parent. Not local space.
                FrameNumberType frameNumber = PositionToFrame(event->pos());

                FramePointMapping::iterator dataPointIter = m_graphedPoints.find(frameNumber);

                if (dataPointIter != m_graphedPoints.end())
                {
                    if (frameNumber != m_highlightedFrame)
                    {
                        RemoveHighlight();
                    }

                    bool needsUpdate = false;

                    DataPointList& dataPointList = dataPointIter->second;
                    AggregatorDataPoint* overlayPoint = nullptr;

                    for (AggregatorDataPoint& dataPoint : dataPointList)
                    {
                        if (dataPoint.ContainsPoint(pos))
                        {
                            m_highlightedFrame = frameNumber;
                            overlayPoint = &dataPoint;
                        }
                        else if (dataPoint.SetOverlayEnabled(false))
                        {
                            needsUpdate = true;
                        }
                    }

                    if (overlayPoint)
                    {
                        if (overlayPoint->SetOverlayEnabled(true))
                        {
                            needsUpdate = true;
                        }
                    }
                    else
                    {
                        m_highlightedFrame = -1;
                    }

                    if (needsUpdate)
                    {
                        update();
                    }
                }
                else
                {
                    RemoveHighlight();
                }
            }
        }
    }

    void ChannelDataView::mousePressEvent(QMouseEvent* event)
    {
        if (!m_Channel->IsInCaptureMode(CaptureMode::Capturing) && m_Channel->IsActive())
        {
            emit InformOfMouseClick(event->button(), PositionToFrame(event->pos()), FramesPerPixel(), event->modifiers());
            event->ignore();

            // Only want to perform dragging actions on left clicks.
            if (event->button() == Qt::LeftButton)
            {
                grabMouse();

                m_initializeDrag = true;
                m_simulatedPoint = event->pos();

                m_mouseGrabbed = true;
                m_xOffset = 0;

                m_lastFrame = m_Channel->m_State.m_ScrubberFrame;
            }
        }
    }

    void ChannelDataView::mouseReleaseEvent(QMouseEvent* event)
    {
        if (!m_Channel->IsInCaptureMode(CaptureMode::Capturing) && m_Channel->IsActive())
        {
            if (m_mouseGrabbed)
            {
                FrameNumberType frame = PositionToFrame(m_simulatedPoint);
                emit InformOfMouseRelease(event->button(), frame, FramesPerPixel(), event->modifiers());
                event->ignore();

                m_mouseGrabbed = false;

                if (m_dragInitialized)
                {
                    m_shouldIgnorePoint = true;

                    QCursor::setPos(mapToGlobal(m_simulatedPoint));
                    QApplication::restoreOverrideCursor();
                }

                releaseMouse();
            }
        }
    }

    void ChannelDataView::wheelEvent(QWheelEvent* event)
    {
        if (event->angleDelta().y() == 0)
        {
            event->accept();
            return;
        }

        emit InformOfMouseWheel(PositionToFrame(event->position().toPoint()), event->angleDelta().y(), FramesPerPixel(), event->modifiers());
        event->ignore();
    }

    void ChannelDataView::leaveEvent(QEvent* event)
    {
        (void)event;
        RemoveHighlight();
    }

    void ChannelDataView::DirtyGraphData()
    {
        m_dirtyGraph = true;
    }

    void ChannelDataView::RefreshGraphData()
    {
        for (ChannelProfilerWidget* profilerWidget : m_Channel->GetProfilers())
        {
            profilerWidget->GetAggregator()->AnnotateChannelView(this);
        }

        m_graphedPoints.clear();
        DirtyGraphData();
        update();
    }

    BudgetMarkerTicket ChannelDataView::AddBudgetMarker(float value, QColor color)
    {
        ++m_budgetMarkerCounter;

        if (m_budgetMarkerCounter == 0 || m_budgetMarkers.find(m_budgetMarkerCounter) != m_budgetMarkers.end())
        {
            BudgetMarkerTicket startTicket = m_budgetMarkerCounter;

            do
            {
                ++m_budgetMarkerCounter;
            } while ((m_budgetMarkerCounter == 0 || m_budgetMarkers.find(m_budgetMarkerCounter) != m_budgetMarkers.end()) && m_budgetMarkerCounter != startTicket);

            AZ_Assert(m_budgetMarkers.find(m_budgetMarkerCounter) == m_budgetMarkers.end(),"Ran out of tickets inside of budget marker creation.");
        }

        m_budgetMarkers.insert(BudgetMarkerMap::value_type(m_budgetMarkerCounter, BudgetMarker(value, color)));
        return m_budgetMarkerCounter;
    }

    void ChannelDataView::RemoveBudgetMarker(BudgetMarkerTicket ticket)
    {
        m_budgetMarkers.erase(ticket);
    }

    void ChannelDataView::resizeEvent(QResizeEvent* newSize)
    {
        QWidget::resizeEvent(newSize);

        // Graph data relies on the size of the graph, so we need to update it whenever we resize.
        DirtyGraphData();
    }

    void ChannelDataView::RecalculateGraphedPoints()
    {
        FrameNumberType frame = m_Channel->m_State.m_FrameOffset + m_Channel->m_State.m_FramesInView - 1;
        frame = AZ::GetMin(frame, m_Channel->m_State.m_EndFrame);

        QRect wrect = rect();
        int wrectHeight = wrect.height() / (m_Channel->m_State.m_ContractedHeight ? 2 : 1);

        float barWidth = GetBarWidth();

        if (m_Channel->m_State.m_EndFrame)
        {
            float vRange = (float)(wrectHeight - k_barHeight);
            float half = vRange / 2.0f;

            float rectBarWidth = (barWidth < 1.0f ? 1.0f : barWidth);

            float rightEdgeOfBar = (float)wrect.right();
            float leftEdgeOfBar = rightEdgeOfBar - barWidth;

            FrameNumberType newMax = frame;

            while (frame >= 0 && rightEdgeOfBar >= wrect.left())
            {
                // If the frame is a new frame we need to parse it's data.
                if (m_graphedPoints.find(frame) == m_graphedPoints.end())
                {
                    int actualLeftEdge = (int)floorf(leftEdgeOfBar);
                    int actualWidth = (int)floorf(rectBarWidth - 1.0f);

                    if (actualWidth < 1)
                    {
                        actualWidth = 1;
                    }

                    AZStd::list<AggregatorDataPoint> dataPoints;

                    for (ChannelProfilerWidget* currentProfiler : m_Channel->GetProfilers())
                    {
                        float vaf = currentProfiler->GetAggregator()->ValueAtFrame(frame);
                        int topOfBar = (int)(half - (vaf * half));

                        QRect drawRect(actualLeftEdge, topOfBar, actualWidth, k_barHeight);

                        dataPoints.emplace_back(drawRect, currentProfiler);
                    }

                    while (!dataPoints.empty())
                    {
                        AggregatorDataPoint currentPoint = dataPoints.front();
                        dataPoints.pop_front();

                        bool intersected = false;
                        AZStd::list<AggregatorDataPoint>::iterator aggregatorIter = dataPoints.begin();

                        do
                        {
                            intersected = false;
                            aggregatorIter = dataPoints.begin();

                            while (aggregatorIter != dataPoints.end())
                            {
                                if (currentPoint.IntersectsDataPoint((*aggregatorIter)))
                                {
                                    intersected = true;
                                    currentPoint.AddAggregatorDataPoint((*aggregatorIter));
                                    aggregatorIter = dataPoints.erase(aggregatorIter);
                                }
                                else
                                {
                                    ++aggregatorIter;
                                }
                            }
                        } while (intersected);

                        m_graphedPoints[frame].push_back(currentPoint);
                    }
                }

                --frame;
                rightEdgeOfBar -= barWidth;
                leftEdgeOfBar -= barWidth;
            }

            FrameNumberType newMin = AZ::GetMax(0, frame + 1);

            if (m_minFrame >= 0)
            {
                while (m_minFrame < newMin)
                {
                    m_graphedPoints.erase(m_minFrame);
                    ++m_minFrame;
                }
            }

            m_minFrame = newMin;

            if (m_maxFrame >= 0)
            {
                while (m_maxFrame >= 0 && m_maxFrame > newMax)
                {
                    m_graphedPoints.erase(m_maxFrame);
                    --m_maxFrame;
                }
            }

            m_maxFrame = newMax;
        }
        else
        {
            m_minFrame = -1;
            m_maxFrame = -1;
            m_graphedPoints.clear();
        }
    }

    void ChannelDataView::RemoveHighlight()
    {
        if (m_highlightedFrame >= 0)
        {
            FramePointMapping::iterator dataPointIter = m_graphedPoints.find(m_highlightedFrame);

            if (dataPointIter != m_graphedPoints.end())
            {
                bool needsUpdate = false;
                DataPointList& dataPointList = dataPointIter->second;

                for (AggregatorDataPoint& dataPoint : dataPointList)
                {
                    needsUpdate = dataPoint.SetOverlayEnabled(false) || needsUpdate;
                }

                if (needsUpdate)
                {
                    update();
                }
            }

            m_highlightedFrame = -1;
        }
    }
}
