/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include <AzCore/Debug/Profiler.h>

#include "StripChart.hxx"
#include <Source/Driller/moc_StripChart.cpp>
#include "DrillerMainWindowMessages.h"

#include "ChartNumberFormats.h"
#include <Source/Driller/Axis.hxx>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QPen>
#include <QPainter>

namespace StripChart
{
    int DataStrip::s_invalidChannelId = -1;

    //////////////////////////////////////////////////////////////////////////
    DataStrip::DataStrip(QWidget* parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
        , m_Axis(nullptr)
        , m_DependentAxis(nullptr)
        , m_InsetL(56)
        , m_InsetR(16)
        , m_InsetT(16)
        , m_InsetB(24)
        , m_IsDragging(false)
        , m_InBatchMode(false)
        , m_ZoomLimit(15)
        , m_MarkerPosition(0)
        , m_MarkerColor(Qt::white)
        , m_ptrFormatter(nullptr)
        , m_MouseWasDragged(false)
        , m_bLeftDown(false)
        , m_isDataDirty(true)
    {
        this->setStyleSheet(QString("QToolTip {   border: 1px solid white; padding: 1px; background: black; color: white; }"));
        setMouseTracking(true);
    }

    DataStrip::~DataStrip()
    {
        delete m_Axis;
        delete m_DependentAxis;
    }

    void DataStrip::SetAxisTextFormatter(Charts::QAbstractAxisFormatter* target)
    {
        if (m_ptrFormatter)
        {
            disconnect(m_ptrFormatter, SIGNAL(destroyed(QObject*)), this, SLOT(OnDestroyAxisFormatter(QObject*)));
        }

        m_ptrFormatter = target;
        if (m_ptrFormatter)
        {
            connect(m_ptrFormatter, SIGNAL(destroyed(QObject*)), this, SLOT(OnDestroyAxisFormatter(QObject*)));
        }
    }

    void DataStrip::OnDestroyAxisFormatter(QObject* pDestroyed)
    {
        if ((QObject*)pDestroyed == (QObject*)m_ptrFormatter)
        {
            m_ptrFormatter = nullptr; // signals will disconnect automatically.
        }
    }

    void DataStrip::Reset()
    {
        delete m_Axis;
        m_Axis = nullptr;

        delete m_DependentAxis;
        m_DependentAxis = nullptr;

        m_Channels.clear();
    }

    void DataStrip::SetDataDirty()
    {
        m_isDataDirty = true;
        update();
    }

    void DataStrip::SetZoomLimit(float limit)
    {
        m_ZoomLimit = limit;
    }

    int DataStrip::AddChannel(QString name)
    {
        int id = (int)m_Channels.size();
        m_Channels.push_back();
        m_Channels[id].SetName(name);
        m_Channels[id].SetID(id);
        m_Channels[id].m_Data.reserve(65536);

        return id;
    }

    void DataStrip::SetChannelColor(int channelID, QColor color)
    {
        if (IsValidChannelId(channelID))
        {
            m_Channels[channelID].SetColor(color);
        }
    }
    void DataStrip::SetChannelStyle(int channelID, Channel::ChannelStyle style)
    {
        if (IsValidChannelId(channelID))
        {
            m_Channels[channelID].SetStyle(style);
        }
    }

    void DataStrip::SetChannelHighlight(int channelID, bool highlight)
    {
        if (IsValidChannelId(channelID))
        {
            m_Channels[channelID].SetHighlight(highlight);
            update();
        }
    }

    void DataStrip::SetChannelSampleHighlight(int channelID, AZ::u64 sampleID,  bool highlight)
    {
        if (IsValidChannelId(channelID))
        {
            m_Channels[channelID].SetHighlightedSample(highlight, sampleID);
            update();
        }
    }

    void DataStrip::SetViewFull()
    {
        m_Axis->SetViewFull();
        m_DependentAxis->SetViewFull();
        update();
    }

    void DataStrip::SetLockRight(bool tf)
    {
        if (m_Axis)
        {
            m_Axis->SetLockedRight(tf);
        }
        update();
    }
    void DataStrip::SetMarkerColor(QColor qc)
    {
        m_MarkerColor = qc;
        update();
    }
    void DataStrip::SetMarkerPosition(float qposn)
    {
        m_MarkerPosition = qposn;
        update();
    }

    void DataStrip::AddData(int channelID, AZ::u64 sampleID, float h, float v)
    {
        AZ_Error("Standalone Tools", !m_InBatchMode, "AddData should not called during a BatchData session.");

        if (IsValidChannelId(channelID))
        {
            m_Channels[channelID].m_Data.push_back(Channel::Sample(sampleID, h, v));

            m_Axis->AddAxisRange(h);
            if (m_DependentAxis)
            {
                m_DependentAxis->AddAxisRange(v);
            }
            update();
        }
    }

    void DataStrip::StartBatchDataAdd()
    {
        m_InBatchMode = true;
    }

    void DataStrip::AddBatchedData(int channelId, AZ::u64 sampleId, float h, float v)
    {
        AZ_Error("StandaloneTools", m_InBatchMode, "AddBatchedData should only be called during a BatchData session.");

        if (IsValidChannelId(channelId))
        {
            m_Channels[channelId].m_Data.push_back(Channel::Sample(sampleId, h, v));
        }
    }

    void DataStrip::EndBatchDataAdd()
    {
        if (m_InBatchMode)
        {
            m_InBatchMode = false;
            update();
        }
    }

    void DataStrip::ClearData(int channelID)
    {
        if (IsValidChannelId(channelID))
        {
            m_Channels[channelID].m_Data.clear();
        }
    }

    void DataStrip::ClearAxisRange()
    {
        m_Axis->Clear();
        if (m_DependentAxis)
        {
            m_DependentAxis->Clear();
        }
    }

    bool DataStrip::AddAxis(QString label, float minimum, float maximum, bool lockedZoom, bool lockedRange)
    {
        Charts::Axis* a = nullptr;

        if (!m_Axis)
        {
            a = m_Axis = aznew Charts::Axis();
        }
        else if (!m_DependentAxis)
        {
            a = m_DependentAxis = aznew Charts::Axis();
        }
        else
        {
            AZ_Assert(false, "ERROR: Creating 3 axis's for a single graph.");
        }

        if (a)
        {
            a->SetLabel(label);
            a->SetLockedZoom(lockedZoom);
            a->SetAxisRange(minimum, maximum);
            a->SetWindowMin(minimum);
            a->SetWindowMax(maximum);
            a->SetLockedRange(lockedRange);
            update();
        }

        return a != nullptr;
    }

    bool DataStrip::GetAxisRange(Charts::AxisType whichAxis, float& minValue, float& maxValue)
    {
        bool foundAxis = false;

        Charts::Axis* axis = nullptr;
        switch (whichAxis)
        {
        case Charts::AxisType::Horizontal:
            axis = m_Axis;
            break;
        case Charts::AxisType::Vertical:
            axis = m_DependentAxis;
            break;
        default:
            AZ_Assert(false, "ERROR: Invalid Axis(%i) given to GetAxisRange", whichAxis);
            break;
        }



        if (axis)
        {
            foundAxis = true;
            minValue = axis->GetRangeMin();
            maxValue = axis->GetRangeMax();
        }

        return foundAxis;
    }

    bool DataStrip::GetWindowRange(Charts::AxisType whichAxis, float& minValue, float& maxValue)
    {
        bool foundAxis = false;

        Charts::Axis* axis = nullptr;
        switch (whichAxis)
        {

        case Charts::AxisType::Horizontal:
            axis = m_Axis;
            break;
        case Charts::AxisType::Vertical:
            axis = m_DependentAxis;
            break;
        default:
            AZ_Assert(false, "ERROR: Invalid Axis(%i) given to GetWindowRange", whichAxis);
            break;
        }

        if (axis)
        {
            foundAxis = true;
            minValue = axis->GetWindowMin();
            maxValue = axis->GetWindowMax();
        }

        return foundAxis;
    }

    void DataStrip::SetWindowRange(Charts::AxisType whichAxis, float minValue, float maxValue)
    {
        Charts::Axis* axis = nullptr;
        switch (whichAxis)
        {
        case Charts::AxisType::Horizontal:
            axis = m_Axis;
            break;
        case Charts::AxisType::Vertical:
            axis = m_DependentAxis;
            break;
        default:
            AZ_Assert(false, "ERROR: Invalid Axis(%i) given to SetWindowRange", whichAxis);
            break;
        }

        if (axis)
        {
            axis->SetAxisRange(minValue, maxValue);
        }
    }

    void DataStrip::AddWindowRange(Charts::AxisType whichAxis, float minValue, float maxValue)
    {
        Charts::Axis* axis = nullptr;
        switch (whichAxis)
        {
        case Charts::AxisType::Horizontal:
            axis = m_Axis;
            break;
        case Charts::AxisType::Vertical:
            axis = m_DependentAxis;
            break;
        default:
            AZ_Assert(false, "ERROR: Invalid Axis(%i) given to SetWindowRange", whichAxis);
            break;
        }

        if (axis)
        {
            axis->AddAxisRange(minValue);
            axis->AddAxisRange(maxValue);
        }
    }

    void DataStrip::Drag(Charts::Axis* axis, int deltaX, int deltaY)
    {
        if (!axis->GetLockedRange() && !axis->GetLockedRight())
        {
            if (axis->GetWindowMin() + deltaX > axis->GetRangeMin() && axis->GetWindowMax() + deltaX < axis->GetRangeMax())
            {
                axis->SetAutoWindow(false);
                axis->UpdateWindowRange((float)deltaX);
            }
        }

        Drag(m_DependentAxis, deltaY);
    }

    void DataStrip::Drag(Charts::Axis* axis, int deltaY)
    {
        if (!axis->GetLockedRange() && !axis->GetLockedRight())
        {
            axis->SetAutoWindow(false);
            axis->UpdateWindowRange((float)deltaY);
        }
    }

    QPoint DataStrip::TransformHoriz(Charts::Axis* axis, float h)
    {
        QPoint pt(0, 0);

        if (axis)
        {
            //if ((v >= axis->m_WindowMin) && (v <= axis->m_WindowMax))
            {
                float fullRange = fabs(axis->GetWindowRange());

                float ratio = float(h - axis->GetWindowMin()) / fullRange;
                pt.setX(m_Inset.left() + int((float(m_Inset.width()) * ratio)));
            }
        }

        return pt;
    }

    QPoint DataStrip::TransformVert(Charts::Axis* axis, float v)
    {
        QPoint pt(0, 0);

        if (axis)
        {
            //if ((v >= axis->m_WindowMin) && (v <= axis->m_WindowMax))
            {
                float fullRange = fabs(axis->GetWindowRange());

                float ratio = float(v - axis->GetWindowMin()) / fullRange;
                pt.setY(m_Inset.bottom() - int((float(m_Inset.height()) * ratio)));
            }
        }

        return pt;
    }

    TransformResult DataStrip::Transform(Charts::Axis* axis, float h, float v, QPoint& outPt)
    {
        TransformResult tr = INVALID_RANGE;
        QPoint pt(0, 0);

        if (axis)
        {
            if (h < axis->GetWindowMin())
            {
                tr = OUTSIDE_LEFT;
                pt.setX(m_Inset.left());
            }
            else if (h > axis->GetWindowMax())
            {
                tr = OUTSIDE_RIGHT;
                pt.setX(m_Inset.right());
            }
            else
            {
                tr = INSIDE_RANGE;

                if (axis->GetWindowMax() != axis->GetWindowMin())
                {
                    float fullRange = fabs(axis->GetWindowMax() - axis->GetWindowMin());
                    float ratio = float(h - axis->GetWindowMin()) / fullRange;
                    pt.setX(m_Inset.left() + int((float(m_Inset.width()) * ratio)));
                    pt += TransformVert(m_DependentAxis, v);
                }
                else
                {
                    pt.setX(m_Inset.left() + m_Inset.width() / 2);
                }
            }
        }

        outPt = pt;
        return tr;
    }

    void DataStrip::wheelEvent(QWheelEvent* event)
    {
        int numDegrees = event->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        // +step := zoom IN
        // -step := zoom OUT
        QPoint zoomPt = event->position().toPoint() - m_Inset.topLeft();

        float zoomRatioX = (float)zoomPt.x() / (float)m_Inset.width();
        float zoomRatioY = (1.0f - ((float)zoomPt.y() / (float)m_Inset.height())); // because up is higher

        // give it some grace area.  This makes it so if your mouse is "close" to the edge, its basically at the edge.
        if (zoomRatioX < 0.1f)
        {
            zoomRatioX = 0.0f;
        }

        if (zoomRatioX > 0.9f)
        {
            zoomRatioX = 1.0f;
        }

        if (zoomRatioY < 0.1f)
        {
            zoomRatioY = 0.0f;
        }

        if (zoomRatioY > 0.9f)
        {
            zoomRatioY = 1.0f;
        }

        // the zoom limit is the smallest possible range you'd like to represent.

        m_Axis->Zoom(zoomRatioX, (float)numSteps, m_ZoomLimit);
        m_DependentAxis->Zoom(zoomRatioY, (float)numSteps, 1.0f);

        update();

        event->accept();
    }
    void DataStrip::mouseMoveEvent(QMouseEvent* event)
    {
        if (m_IsDragging)
        {
            m_MouseWasDragged = true;

            float pixelWidth = (float)m_Inset.width();
            float pixelHeight = (float)m_Inset.height();
            float domainWidth = m_Axis->GetWindowRange();
            float domainHeight = m_DependentAxis->GetWindowRange();
            float domainPerPixelX = domainWidth / pixelWidth;
            float domainPerPixelY = domainHeight / pixelHeight;

            QPoint deltaPoint = event->pos() - m_DragTracker;

            float deltaInDomainX = -domainPerPixelX * (float)(deltaPoint.x());
            float deltaInDomainY = domainPerPixelY * (float)(deltaPoint.y());

            m_Axis->Drag(deltaInDomainX);
            m_DependentAxis->Drag(deltaInDomainY);

            m_DragTracker = event->pos();
            update();
        }
        else if (m_bLeftDown)
        {
            float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());
            float ratio = (float)(event->pos().x() - m_InsetL) / (float)(width() - m_InsetL - m_InsetR);
            float localValue = (fullRange * ratio) + m_Axis->GetWindowMin();
            emit onMouseLeftDragDomainValue(localValue);
        }
        else
        {
            // mouse move with no buttons -- tooltip event
            bool hitsomething = false;
            HitArea closestHitArea;
            int minimumDistance = 10;

            for (auto iter = m_hitAreas.begin(); iter != m_hitAreas.end(); ++iter)
            {
                QPoint hitboxCenter = iter->m_hitBoxCenter;

                int distance = (hitboxCenter - event->pos()).manhattanLength();
                if (distance < minimumDistance)
                {
                    closestHitArea = *iter;
                    hitsomething = true;
                    minimumDistance = distance;
                }
            }

            if (hitsomething)
            {
                emit onMouseOverDataPoint(closestHitArea.m_ptrChannel->m_channelID, closestHitArea.m_sampleID, closestHitArea.m_primaryAxisValue, closestHitArea.m_dependentAxisValue);
            }
            else
            {
                // transform the point into the window range:
                QPoint localPt = event->pos() - m_Inset.topLeft();
                localPt -= m_Inset.topLeft();
                float ratioX = (float)localPt.x() / (float)m_Inset.width();
                float ratioY = (1.0f - ((float)localPt.y() / (float)m_Inset.height()));
                emit onMouseOverNothing((ratioX * m_Axis->GetWindowRange())  + m_Axis->GetWindowMin(), (ratioY * m_DependentAxis->GetWindowRange())  + m_DependentAxis->GetWindowMin());
            }
        }
    }
    void DataStrip::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            m_MouseWasDragged = false;
            m_IsDragging = true;
            m_DragTracker = event->pos();
            update();
        }
        else if (event->button() == Qt::LeftButton)
        {
            m_bLeftDown = true;

            float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());
            float ratio = (float)(event->pos().x() - m_InsetL) / (float)(width() - m_InsetL - m_InsetR);
            float localValue = (fullRange * ratio) + m_Axis->GetWindowMin();
            emit onMouseLeftDownDomainValue(localValue);
        }

        event->accept();
    }
    void DataStrip::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton)
        {
            m_IsDragging = false;
        }
        else if (event->button() == Qt::LeftButton)
        {
            if (m_bLeftDown)
            {
                m_bLeftDown = false;
                float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());
                float ratio = (float)(event->pos().x() - m_InsetL) / (float)(width() - m_InsetL - m_InsetR);
                float localValue = (fullRange * ratio) + m_Axis->GetWindowMin();
                emit onMouseLeftUpDomainValue(localValue);
            }
        }

        update();
        event->accept();
    }
    void DataStrip::resizeEvent(QResizeEvent* event)
    {
        RecalculateInset();
        event->ignore();
    }

    void DataStrip::RecalculateInset()
    {
        m_Inset = QRect(m_InsetL, m_InsetT, rect().width() - m_InsetL - m_InsetR, rect().height() - m_InsetT - m_InsetB);
    }

    void DataStrip::AttachDataSourceWidget(QWidget* widget)
    {
        connect(this, SIGNAL(ProcureData(StripChart::DataStrip*)), widget, SLOT(ProvideData(StripChart::DataStrip*)));
    }

    void DataStrip::paintEvent(QPaintEvent* event)
    {
        (void)event; 

        //
        // pull data from owner
        // e.g. in the profiler case the ChartTimeHistory() with cached parameters should end up getting called
        // ChartTimeHistory( m_cachedChart, m_cachedFrame, m_cachedFar, m_cachedColumn );
        //
        if (m_isDataDirty)
        {
            emit ProcureData(this);
            m_isDataDirty = false;
        }

        QPen pen;
        pen.setWidth(1);
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        pen.setBrush(brush);

        QPainter p(this);
        p.setPen(pen);

        QFont currentFont = p.font();

        p.fillRect(rect(), QColor(32, 32, 32, 255));
        p.fillRect(m_Inset, Qt::black);

        brush.setColor(QColor(255, 255, 0, 255));
        pen.setColor(QColor(0, 255, 255, 255));
        p.setPen(pen);

        m_hitAreas.clear();
        // HORIZ
        if (m_Axis)
        {
            m_Axis->PaintAxis(Charts::AxisType::Horizontal, &p, rect(), m_Inset, m_ptrFormatter);

            // VERT
            if (m_DependentAxis)
            {                
                m_DependentAxis->PaintAxis(Charts::AxisType::Vertical, &p, rect(), m_Inset, m_ptrFormatter);
            }

            // +-1 allows data at the outer envelope to render
            p.setClipRect(m_InsetL, m_InsetT - 1, rect().width() - m_InsetR - m_InsetL, rect().height() - m_InsetB - m_InsetT + 1);

            for (Channels::iterator chiter = m_Channels.begin(); chiter != m_Channels.end(); ++chiter)
            {
                Channel& cptr = *chiter;

                pen.setStyle(Qt::SolidLine);
                brush.setColor(chiter->m_Color);
                pen.setColor(chiter->m_Color);
                if (chiter->m_highlighted)
                {
                    pen.setWidth(3);
                }
                else
                {
                    pen.setWidth(1);
                }
                p.setPen(pen);

                switch (chiter->m_Style)
                {
                case Channel::STYLE_POINT:
                {
                    auto datiter = cptr.m_Data.begin();
                    while (datiter != cptr.m_Data.end())
                    {
                        QPoint pt;
                        if (Transform(m_Axis, datiter->m_domainValue, datiter->m_dependentValue, pt) == INSIDE_RANGE)
                        {
                            if ((chiter->m_highlightSample) && (chiter->m_highlightedSampleID == datiter->m_sampleID))
                            {
                                // if the channel itself wasn't highlighte we need to set the pen
                                if (!chiter->m_highlighted)
                                {
                                    pen.setWidth(3);
                                    p.setPen(pen);
                                }

                                p.drawEllipse(pt, 5, 5);

                                if (!chiter->m_highlighted)
                                {
                                    // if the channel itself wasn't highlighted we need to restore the pen
                                    p.setPen(pen);
                                    pen.setWidth(1);
                                }
                            }
                            else
                            {
                                p.drawEllipse(pt, 3, 3);
                            }

                            m_hitAreas.push_back(HitArea(datiter->m_domainValue, datiter->m_dependentValue, pt, &cptr, datiter->m_sampleID));
                        }
                        ++datiter;
                    }
                }
                break;
                case Channel::STYLE_PLUSMINUS:
                {
                    auto datiter = cptr.m_Data.begin();
                    while (datiter != cptr.m_Data.end())
                    {
                        QPoint pt;
                        if (Transform(m_Axis, datiter->m_domainValue, datiter->m_dependentValue, pt) == INSIDE_RANGE)
                        {
                            m_hitAreas.push_back(HitArea(datiter->m_domainValue, datiter->m_dependentValue, pt, &cptr, datiter->m_sampleID));

                            int plussize = 3;

                            if ((chiter->m_highlightSample) && (chiter->m_highlightedSampleID == datiter->m_sampleID))
                            {
                                if (!chiter->m_highlighted)
                                {
                                    // if the channel itself wasn't highlighte we need to set the pen
                                    pen.setWidth(3);
                                    p.setPen(pen);
                                }
                                plussize = 5;
                            }

                            p.drawLine(pt.x() - plussize, pt.y(), pt.x() + plussize, pt.y());
                            if (datiter->m_dependentValue > 0.0f)
                            {
                                p.drawLine(pt.x(), pt.y() - plussize, pt.x(), pt.y() + plussize);
                            }

                            if ((chiter->m_highlightSample) && (chiter->m_highlightedSampleID == datiter->m_sampleID))
                            {
                                if (!chiter->m_highlighted)
                                {
                                    // restore pen
                                    pen.setWidth(1);
                                    p.setPen(pen);
                                }
                            }
                        }
                        ++datiter;
                    }
                }
                break;
                case Channel::STYLE_CONNECTED_LINE:
                {
                    auto datiter = cptr.m_Data.begin();
                    auto onebehind = cptr.m_Data.begin();
                    if (datiter != cptr.m_Data.end())
                    {
                        ++datiter;
                    }
                    while (datiter != cptr.m_Data.end())
                    {
                        QPoint pt1;
                        TransformResult tr1 = Transform(m_Axis, onebehind->m_domainValue, onebehind->m_dependentValue, pt1);
                        QPoint pt2;
                        TransformResult tr2 = Transform(m_Axis, datiter->m_domainValue, datiter->m_dependentValue, pt2);

                        if (tr1 == INSIDE_RANGE && tr2 == INSIDE_RANGE)
                        {
                            m_hitAreas.push_back(HitArea(datiter->m_domainValue, datiter->m_dependentValue, pt2, &cptr, datiter->m_sampleID));


                            if ((chiter->m_highlightSample) && (chiter->m_highlightedSampleID == datiter->m_sampleID))
                            {
                                if (!chiter->m_highlighted)
                                {
                                    // if the channel itself wasn't highlighted we need to set the pen
                                    pen.setWidth(3);
                                    p.setPen(pen);
                                }

                                p.drawLine(pt1, pt2);
                                p.drawEllipse(pt2, 3, 3);
                                if (!chiter->m_highlighted)
                                {
                                    // restore the pen
                                    pen.setWidth(1);
                                    p.setPen(pen);
                                }
                            }
                            else
                            {
                                // not highlighted..
                                p.drawLine(pt1, pt2);       // just draw a line
                            }
                        }
                        ++onebehind;
                        ++datiter;
                    }
                }
                break;
                }
            }

            pen.setStyle(Qt::SolidLine);
            brush.setStyle(Qt::Dense2Pattern);
            brush.setColor(m_MarkerColor);
            pen.setColor(m_MarkerColor);
            p.setPen(pen);
            QPoint markerPt;
            if (Transform(m_Axis, m_MarkerPosition, 0.0f, markerPt) == INSIDE_RANGE)
            {
                p.drawLine(markerPt.x(), 0, markerPt.x(), m_Inset.y() + m_Inset.height());
            }
        }
    }

    void DataStrip::RenderHorizCallouts(QPainter* painter)
    {
        float textSpaceRequired = (float)painter->fontMetrics().horizontalAdvance("9,999,999.99");
        int fontH = painter->fontMetrics().height();

        AZStd::vector<float> divisions;
        divisions.reserve(10);
        float divisionSize = m_Axis->ComputeAxisDivisions((float)m_Inset.width(), divisions, textSpaceRequired, textSpaceRequired);

        QPen dottedPen;
        dottedPen.setStyle(Qt::DotLine);
        dottedPen.setColor(QColor(64, 64, 64, 255));
        dottedPen.setWidth(1);
        QBrush solidBrush;
        QPen solidPen;
        solidPen.setStyle(Qt::SolidLine);
        solidPen.setColor(QColor(0, 255, 255, 255));
        solidPen.setWidth(1);

        for (auto it = divisions.begin(); it != divisions.end(); ++it)
        {
            float currentUnit = *it;
            QPoint leftEdge = TransformHoriz(m_Axis, currentUnit);

            QPoint leftline((int)leftEdge.x(), m_Inset.bottom());
            QPoint leftend = leftline - QPoint(0, m_Inset.height());
            painter->setPen(dottedPen);
            painter->drawLine(leftline, leftend);

            QString text;
            if (m_ptrFormatter)
            {
                text = m_ptrFormatter->convertAxisValueToText(Charts::AxisType::Horizontal, currentUnit, divisions.front(), divisions.back(), divisionSize);
            }
            else
            {
                text = QString("%1").arg((AZ::s64)currentUnit);
            }

            int textW = painter->fontMetrics().horizontalAdvance(text);

            painter->setPen(solidPen);
            painter->drawText((int)leftEdge.x() - textW / 2, m_Inset.bottom() + fontH, text);
        }
    }

    void DataStrip::RenderVertCallouts(QPainter* painter)
    {
        if (!m_DependentAxis)
        {
            return;
        }

        int fontH = painter->fontMetrics().height();


        QPen dottedPen;
        dottedPen.setStyle(Qt::DotLine);
        dottedPen.setColor(QColor(64, 64, 64, 255));
        dottedPen.setWidth(1);
        QBrush solidBrush;
        QPen solidPen;
        solidPen.setStyle(Qt::SolidLine);
        solidPen.setColor(QColor(0, 255, 255, 255));
        solidPen.setWidth(1);
        AZStd::vector<float> divisions;
        divisions.reserve(10);
        float divisionSize = m_DependentAxis->ComputeAxisDivisions((float)m_Inset.height(), divisions, fontH * 2.0f, fontH * 2.0f);

        for (auto it = divisions.begin(); it != divisions.end(); ++it)
        {
            float currentUnit = *it;

            // where is that in the inset?
            QPoint leftEdge = TransformVert(m_DependentAxis, currentUnit);

            painter->setPen(dottedPen);
            QPoint leftline(m_Inset.left(), leftEdge.y());
            QPoint leftend = leftline + QPoint(m_Inset.width(), 0);
            painter->drawLine(leftline, leftend);

            QString text;
            if (m_ptrFormatter)
            {
                text = m_ptrFormatter->convertAxisValueToText(Charts::AxisType::Vertical, currentUnit, divisions.front(), divisions.back(), divisionSize);
            }
            else
            {
                text = QString("%1").arg((AZ::s64)currentUnit);
            }

            int textW = painter->fontMetrics().horizontalAdvance(text);
            painter->setPen(solidPen);
            painter->drawText(m_Inset.left() - textW - 2, (int)leftEdge.y() + fontH / 2, text);
        }
    }

    void DataStrip::DrawRotatedText(QString text, QPainter* painter, float degrees, int x, int y, float scale)
    {
        painter->save();
        painter->translate(x, y);
        painter->scale(scale, scale);
        painter->rotate(degrees);
        painter->drawText(0, 0, text);
        painter->restore();
    }

    void DataStrip::ZoomExtents(Charts::AxisType axis)
    {
        switch (axis)
        {
        case Charts::AxisType::Horizontal:
            if (m_Axis)
            {
                m_Axis->SetViewFull();
            }
            break;
        case Charts::AxisType::Vertical:
            if (m_DependentAxis)
            {
                m_DependentAxis->SetViewFull();
            }
            break;
        default:
            AZ_Assert(false, "ERROR: Unkown axis(%i) in ZoomExtents", axis);
        }
    }

    void DataStrip::ZoomManual(Charts::AxisType axis, float minValue, float maxValue)
    {
        switch (axis)
        {
        case Charts::AxisType::Horizontal:
            if (m_Axis)
            {
                m_Axis->ZoomToRange(minValue, maxValue, false);
            }
            break;
        case Charts::AxisType::Vertical:
            if (m_DependentAxis)
            {
                m_DependentAxis->ZoomToRange(minValue, maxValue, false);
            }
            break;
        default:
            AZ_Assert(false, "ERROR: Unkown axis(%i) in ZoomExtents", axis);
        }
    }

    bool DataStrip::IsValidChannelId(int channelId) const
    {
        return channelId >= 0 && channelId < m_Channels.size();
    }
}
