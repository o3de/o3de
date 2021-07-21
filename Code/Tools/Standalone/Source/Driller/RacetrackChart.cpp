/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "RacetrackChart.hxx"
#include <Source/Driller/moc_RacetrackChart.cpp>
#include "DrillerMainWindowMessages.h"
#include "Axis.hxx"
#include "ChartNumberFormats.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QPen>
#include <QPainter>

namespace Racetrack
{
    //////////////////////////////////////////////////////////////////////////
    DataRacetrack::DataRacetrack(QWidget* parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
        , m_Axis(NULL)
        , m_InsetL(2)
        , m_InsetR(2)
        , m_InsetT(2)
        , m_InsetB(32)
        , m_IsDragging(false)
        , m_ZoomLimit(15)
        , m_IsLeftDragging(false)
        , m_ZeroBasedAxisDisplay(false)
    {
        m_Axis = aznew Charts::Axis(this);
        connect(m_Axis, SIGNAL(Invalidated()), this, SLOT(OnAxisInvalidated()));
        this->setMouseTracking(true);
        m_iChannelHighlight = -1;
    }

    Charts::Axis* DataRacetrack::GetAxis() const
    {
        return m_Axis;
    }

    void DataRacetrack::OnAxisInvalidated()
    {
        update();
    }

    DataRacetrack::~DataRacetrack()
    {
    }

    void DataRacetrack::SetZoomLimit(float limit)
    {
        m_ZoomLimit = limit;
    }

    int DataRacetrack::AddChannel(QString name)
    {
        int id = (int)m_Channels.size();
        m_Channels.push_back();
        m_Channels[id].SetName(name);

        return id;
    }

    void DataRacetrack::SetChannelColor(int channelID, QColor color)
    {
        m_Channels[channelID].SetColor(color);
    }

    void DataRacetrack::SetZeroBasedAxisNumbering(bool tf)
    {
        m_ZeroBasedAxisDisplay = tf;
        update();
    }

    void DataRacetrack::SetMarkerColor(QColor qc)
    {
        m_MarkerColor = qc;
        update();
    }
    void DataRacetrack::SetMarkerPosition(float qposn)
    {
        m_MarkerPosition = qposn;
        update();
    }


    void DataRacetrack::AddData(int channelID, float h, float v)
    {
        m_Channels[channelID].m_Data.push_back();
        m_Channels[channelID].m_Data.back().first = h;
        m_Channels[channelID].m_Data.back().second = v;
    }

    void DataRacetrack::Clear()
    {
        m_Axis->Clear();
        m_Channels.clear();
    }

    void DataRacetrack::ClearData(int channelID)
    {
        if (channelID < m_Channels.size())
        {
            m_Channels[channelID].m_Data.clear();
        }
    }


    void DataRacetrack::SetupAxis(QString label, float minimum, float maximum, bool locked)
    {
        m_Axis->SetLabel(label);
        m_Axis->SetAxisRange(minimum, maximum);
        m_Axis->SetLockedRange(locked);
    }

    void DataRacetrack::Zoom(QPoint pt, int steps)
    {
        if (m_Axis->GetValid())
        {
            if (m_Inset.intersects(QRect(pt, pt)))
            {
                float ratio = float(pt.x() - m_Inset.left()) / float(m_Inset.width());
                if (m_Axis->GetLockedRight())
                {
                    ratio = 1.0f;
                }
                if (!m_Axis->GetLockedRange())
                {
                    m_Axis->SetAutoWindow(false);

                    float testMin = m_Axis->GetWindowMin();
                    float testMax = m_Axis->GetWindowMax();

                    testMin -= float(m_Axis->GetWindowMax() - m_Axis->GetWindowMin()) * 0.05f * ratio * float(-steps);
                    testMax += float(m_Axis->GetWindowMax() - m_Axis->GetWindowMin()) * 0.05f * (1.0f - ratio) * float(-steps);
                    if ((testMax - testMin) > 0.0f)
                    {
                        if (testMax > m_Axis->GetRangeMax())
                        {
                            float offset = m_Axis->GetRangeMax() - testMax;
                            testMax += offset;
                            testMin += offset;
                        }
                        if (testMin < m_Axis->GetRangeMin())
                        {
                            float offset = testMin - m_Axis->GetRangeMin();
                            testMax -= offset;
                            testMin -= offset;
                        }
                        if ((float)m_Inset.width() / (testMax - testMin) < m_ZoomLimit)
                        {
                            m_Axis->SetWindowMin(testMin);
                            m_Axis->SetWindowMax(testMax);
                        }

                        if ((testMax - testMin) > (m_Axis->GetRangeMax() - m_Axis->GetRangeMin()))
                        {
                            m_Axis->SetViewFull();
                        }
                    }
                }
            }
        }
    }

    void DataRacetrack::Drag(int deltaX)
    {
        if (m_Axis->GetValid())
        {
            if (!m_Axis->GetLockedRange() && !m_Axis->GetLockedRight())
            {
                // delta is in pixels.  Convert to domain units:

                float pixelWidth = (float)m_Inset.width();
                float domainWidth = m_Axis->GetWindowMax() - m_Axis->GetWindowMin();
                float domainPerPixel = domainWidth / pixelWidth;
                float deltaInDomain = domainPerPixel * (float)deltaX;

                if (m_Axis->GetWindowMin() + deltaInDomain > m_Axis->GetRangeMin() && m_Axis->GetWindowMax() + deltaInDomain < m_Axis->GetRangeMax())
                {
                    m_Axis->SetAutoWindow(false);
                    m_Axis->UpdateWindowRange((float)deltaInDomain);
                }
            }
        }
    }

    QPoint DataRacetrack::Transform(float v)
    {
        QPoint pt;

        if (m_Axis->GetValid())
        {
            if ((v >= m_Axis->GetWindowMin()) && (v <= m_Axis->GetWindowMax()))
            {
                float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());

                float ratio = float(v - m_Axis->GetWindowMin()) / fullRange;
                pt.setY(m_Inset.bottom() - int((float(m_Inset.height()) * ratio)));
            }
        }

        return pt;
    }

    TransformResult DataRacetrack::Transform(float h, QPoint& outPoint)
    {
        TransformResult tr = INVALID_RANGE;
        QPoint pt(0, 0);

        if (m_Axis->GetValid())
        {
            if (h < m_Axis->GetWindowMin())
            {
                tr = OUTSIDE_LEFT;
                pt.setX(m_Inset.left());
            }
            else if (h > m_Axis->GetWindowMax())
            {
                tr = OUTSIDE_RIGHT;
                pt.setX(m_Inset.left() + m_Inset.width());
            }
            else
            {
                tr = INSIDE_RANGE;

                float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());
                float ratio = float(h - m_Axis->GetWindowMin()) / fullRange;
                pt.setX(m_Inset.left() + int((float(m_Inset.width()) * ratio)));
            }
        }

        outPoint = pt;
        return tr;
    }

    void DataRacetrack::wheelEvent (QWheelEvent* event)
    {
        if (!m_Axis->GetValid())
        {
            return;
        }

        int numDegrees = event->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        // +step := zoom IN
        // -step := zoom OUT
        QPoint zoomPt = event->position().toPoint() - m_Inset.topLeft();

        Zoom(zoomPt, numSteps);

        update();

        event->accept();
    }
    void DataRacetrack::mouseMoveEvent (QMouseEvent* event)
    {
        if (!m_Axis->GetValid())
        {
            return;
        }
        if (m_IsDragging)
        {
            // far far did we move in the DOMAIN?
            QPoint pt = m_DragTracker - event->pos();
            Drag(pt.x());
            m_DragTracker = event->pos();
            update();
        }
        else
        {
            if (m_IsLeftDragging)
            {
                float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());
                float ratio = (float)(event->pos().x() - m_InsetL) / (float)(width() - m_InsetL - m_InsetR);
                float localValue = fullRange * ratio;
                Driller::EventNumberType globalEvtID = (Driller::EventNumberType)(m_Axis->GetWindowMin() + localValue);

                emit EventRequestEventFocus(globalEvtID);
            }
            else
            {
                int oldChannelHighlight = m_iChannelHighlight;
                m_iChannelHighlight = -1;
                if (m_Channels.size() > 0)
                {
                    if (m_Inset.contains(event->pos()))
                    {
                        QPoint offset = event->pos() - m_Inset.topLeft();
                        float ratio = (float)offset.y() / m_Inset.height();
                        if (ratio < 1.0f)
                        {
                            // so which channel is it over?
                            int channel = (int)(ratio * (float)m_Channels.size());
                            if (m_iChannelHighlight != channel)
                            {
                                m_iChannelHighlight = channel;
                            }
                        }
                    }
                }

                if (oldChannelHighlight != m_iChannelHighlight)
                {
                    update();
                }
            }
        }
    }
    void DataRacetrack::mousePressEvent (QMouseEvent* event)
    {
        if (!m_Axis->GetValid())
        {
            return;
        }

        if (event->button() == Qt::RightButton)
        {
            m_IsDragging = true;
            m_DragTracker = event->pos();
        }
        else if (event->button() == Qt::LeftButton)
        {
            m_IsLeftDragging = true;

            float fullRange = fabs(m_Axis->GetWindowMax() - m_Axis->GetWindowMin());
            float ratio = (float)(event->pos().x() - m_Inset.x()) / (float)(width() - m_InsetL - m_InsetR);
            float localValue = fullRange * ratio;
            Driller::EventNumberType globalEvtID = (Driller::EventNumberType)(m_Axis->GetWindowMin() + localValue);

            emit EventRequestEventFocus(globalEvtID);
        }

        event->accept();
    }

    void DataRacetrack::leaveEvent(QEvent*)
    {
        if (m_iChannelHighlight != -1)
        {
            m_iChannelHighlight = -1;
            update();
        }
    }

    void DataRacetrack::mouseReleaseEvent (QMouseEvent* event)
    {
        if (!m_Axis->GetValid())
        {
            return;
        }

        if (event->button() == Qt::RightButton)
        {
            m_IsDragging = false;
        }
        else
        {
            if (m_IsLeftDragging)
            {
                m_IsLeftDragging = false;
            }
        }

        event->accept();
    }
    void DataRacetrack::resizeEvent(QResizeEvent* event)
    {
        RecalculateInset();
        event->ignore();
    }

    void DataRacetrack::RecalculateInset()
    {
        m_Inset = QRect(m_InsetL, m_InsetT, rect().width() - m_InsetL - m_InsetR, rect().height() - m_InsetT - m_InsetB);
    }

    void DataRacetrack::paintEvent(QPaintEvent* event)
    {
        (void)event;

        QPen pen;
        pen.setWidth(1);
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        pen.setBrush(brush);

        QPainter p(this);
        p.setPen(pen);

        p.fillRect(rect(), QColor(32, 32, 32, 255));
        p.fillRect(m_Inset, Qt::black);

        brush.setColor(QColor(255, 255, 0, 255));
        pen.setColor(QColor(0, 255, 255, 255));
        p.setPen(pen);

        if (m_Channels.empty())
        {
            return;
        }

        if (!m_Axis->GetValid())
        {
            return;
        }

        // HORIZ
        if (m_Axis)
        {
            int barHeight = m_Inset.height() / (int)m_Channels.size() - (int)m_Channels.size();

            p.drawText(0, 0, rect().width(), rect().height(), Qt::AlignHCenter | Qt::AlignBottom, m_Axis->GetLabel());

            pen.setStyle(Qt::DashDotLine);
            pen.setColor(QColor(72, 72, 72, 255));
            p.setPen(pen);
            RenderHorizCallouts(&p);

            TransformResult tr1, tr2;
            QPoint pt1, pt2;
            tr1 = Transform(m_Axis->GetWindowMin(), pt1);
            tr2 = Transform(m_Axis->GetWindowMin() + 1.0f, pt2);
            int drawWidth = pt2.x() - pt1.x() + 1;

            int chidx = 0;
            for (Channels::iterator chiter = m_Channels.begin(); chiter != m_Channels.end(); ++chiter, ++chidx)
            {
                Channel& cptr = *chiter;


                pen.setStyle(Qt::SolidLine);
                brush.setColor(chiter->m_Color);
                brush.setStyle(Qt::SolidPattern);
                pen.setColor(chiter->m_Color);
                pen.setBrush(brush);
                p.setPen(pen);

                {
                    float start(0.0f), last(0.0f), current(0.0f);
                    TransformResult startTR, lastTR;

                    AZStd::vector< AZStd::pair<float, float> >::iterator datiter = cptr.m_Data.begin();
                    if (datiter != cptr.m_Data.end())
                    {
                        start = datiter->first;
                        last = datiter->first;
                        current = datiter->first;
                        ++datiter;
                        while (datiter != cptr.m_Data.end())
                        {
                            current = datiter->first;
                            ++datiter;
                            if ((current == last + 1) && (datiter != cptr.m_Data.end()))
                            {
                                last = current;
                            }
                            else
                            {
                                QPoint drawPtS, drawPtL;
                                startTR = Transform(start, drawPtS);
                                lastTR = Transform(last, drawPtL);

                                if ((startTR == INSIDE_RANGE && lastTR == INSIDE_RANGE) || (startTR != lastTR))
                                {
                                    p.fillRect(m_Inset.x() + drawPtS.x(), m_Inset.y() + chidx * barHeight + 1, drawPtL.x() - drawPtS.x() + drawWidth, barHeight, chiter->m_Color);
                                }

                                start = current;
                                last = current;
                            }
                        }

                        QPoint drawPtS, drawPtL;
                        startTR = Transform(start, drawPtS);
                        lastTR = Transform(last, drawPtL);
                        if ((startTR == INSIDE_RANGE && lastTR == INSIDE_RANGE) || (startTR != lastTR))
                        {
                            p.fillRect(m_Inset.x() + drawPtS.x(), m_Inset.y() + chidx * barHeight + 1, drawPtL.x() - drawPtS.x() + drawWidth, barHeight, chiter->m_Color);
                        }
                    }
                }

                // draw the name of the current channel as a highlight:
                if (chidx == m_iChannelHighlight)
                {
                    QRect textRect(m_Inset.x() + 8, m_Inset.y() + chidx * barHeight + 1, m_Inset.width() - 16, barHeight);
                    p.setPen(QPen(QColor(255, 255, 255, 255)));
                    QRect bound = p.boundingRect(textRect, Qt::AlignVCenter | Qt::AlignLeft, cptr.m_Name);
                    bound.adjust(-2, -2, 2, 2);
                    p.fillRect(bound, QColor(0, 0, 0, 128));
                    p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,  cptr.m_Name);
                }
            }

            pen.setStyle(Qt::SolidLine);
            brush.setStyle(Qt::SolidPattern);
            brush.setColor(Qt::black);
            pen.setColor(Qt::black);
            p.setPen(pen);

            if (drawWidth >= m_ZoomLimit)
            {
                for (float tickWalker = floorf(m_Axis->GetWindowMin()); tickWalker < ceilf(m_Axis->GetWindowMax()); tickWalker += 1.0f)
                {
                    QPoint markerPt;
                    if (Transform(tickWalker, markerPt) == INSIDE_RANGE)
                    {
                        p.drawLine(m_Inset.x() + markerPt.x(), 0, m_Inset.x() + markerPt.x(), m_Inset.y() + m_Inset.height());
                    }
                }
            }

            brush.setStyle(Qt::Dense2Pattern);
            brush.setColor(m_MarkerColor);
            pen.setColor(m_MarkerColor);
            p.setPen(pen);

            QPoint markerPt;
            if (Transform(m_MarkerPosition + 0.5f, markerPt) == INSIDE_RANGE)
            {
                int xDrawPos = m_Inset.x() + markerPt.x();
                int yDrawPos = m_Inset.y() + m_Inset.height();
                p.drawLine(xDrawPos, 0, xDrawPos, m_Inset.y() + m_Inset.height());

                // event ID overlay at the bottom of the bar
                const float frameWidth = 10.0f;
                QPen selPen (QColor(255, 255, 255, 255));
                selPen.setWidth(1);
                p.setPen(selPen);
                p.setBrush(QColor(0, 0, 0, 255));
                int xOffset = xDrawPos - (int)frameWidth < 0 ? xDrawPos + (int)frameWidth : 0;
                xOffset = xDrawPos + xOffset + (int)(frameWidth * 7.0f) > m_Inset.width() ? (int)(-frameWidth * 7.0f) : xOffset;
                p.drawRect(xDrawPos - (int)frameWidth + xOffset, yDrawPos - 8, (int)(frameWidth * 7.0f), 16);
                p.setBrush(QColor(255, 255, 255, 255));
                int eventNum = (int)(m_MarkerPosition);

                QString frameText = DrillerCharts::FriendlyFormat((AZ::s64)eventNum);

                p.drawText(xDrawPos - (int)frameWidth + 2 + xOffset, yDrawPos + 4, frameText);
            }
        }
    }

    void DataRacetrack::RenderHorizCallouts(QPainter* painter)
    {
        float textSpaceRequired = (float)painter->fontMetrics().horizontalAdvance("9,999,999.99");
        int fontH = painter->fontMetrics().height();

        AZStd::vector<float> divisions;
        divisions.reserve(10);
        m_Axis->ComputeAxisDivisions((float)m_Inset.width(), divisions, textSpaceRequired, textSpaceRequired, false);

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
            QPoint leftEdge;

            // offset by a half becuase we want to slice through the middle of these event tracks.
            currentUnit += 0.5f;

            Transform(currentUnit, leftEdge);

            currentUnit -= 0.5f;

            leftEdge += m_Inset.topLeft();

            QPoint leftline((int)leftEdge.x(), m_Inset.bottom());
            QPoint leftend = leftline - QPoint(0, m_Inset.height());
            painter->setPen(dottedPen);
            painter->drawLine(leftline, leftend);

            QString text;
            text = QString("%1").number(currentUnit, 'f', 0);

            int textW = painter->fontMetrics().horizontalAdvance(text);

            painter->setPen(solidPen);
            painter->drawText((int)leftEdge.x() - textW / 2, m_Inset.bottom() + fontH, text);
        }
    }


    void DataRacetrack::DrawRotatedText(QString text, QPainter* painter, float degrees, int x, int y, float scale)
    {
        painter->save();
        painter->translate(x, y);
        painter->scale(scale, scale);
        painter->rotate(degrees);
        painter->drawText(0, 0, text);
        painter->restore();
    }
}
