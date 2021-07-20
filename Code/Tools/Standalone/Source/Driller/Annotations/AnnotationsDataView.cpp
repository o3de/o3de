/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "AnnotationsDataView.hxx"
#include "AnnotationHeaderView.hxx"
#include "Annotations.hxx"

#include <QPen>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

namespace Driller
{
    static const float adv_arrow_width = 8.0f;

    AnnotationsDataView::AnnotationsDataView(QWidget* parent)
        : QWidget(parent)
        , m_ptrHeaderView(nullptr)
        , m_ptrAnnotations(nullptr)
    {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setMouseTracking(true);
    }

    AnnotationsDataView::~AnnotationsDataView()
    {
    }

    void AnnotationsDataView::RegisterAnnotationHeaderView(AnnotationHeaderView* header, AnnotationsProvider* annotations)
    {
        m_ptrHeaderView = header;
        m_ptrAnnotations = annotations;
    }

    int AnnotationsDataView::PositionToFrame(const QPoint& pt)
    {
        QRect wrect = rect();

        int frame = m_ptrHeaderView->GetState().m_FrameOffset + m_ptrHeaderView->GetState().m_FramesInView - 1;
        frame = frame <= m_ptrHeaderView->GetState().m_EndFrame ? frame : m_ptrHeaderView->GetState().m_EndFrame;

        int rOffset = wrect.width() - pt.x();
        int rCell = (int)((float)rOffset / GetBarWidth());
        int retFrame = frame - rCell;

        //AZ_TracePrintf("Driller","Click Frame Raw Input = %d\n", retFrame);

        return retFrame;
    }

    float AnnotationsDataView::GetBarWidth()
    {
        return ((float)(rect().width()) / (float)(m_ptrHeaderView->GetState().m_FramesInView));
    }

    void AnnotationsDataView::paintEvent(QPaintEvent* event)
    {
        (void)event;

        m_ClickableAreas.clear();

        QPen pen;
        pen.setWidth(1);
        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        pen.setBrush(brush);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        painter.setPen(pen);
        painter.fillRect(rect(), Qt::black);

        int frame = m_ptrHeaderView->GetState().m_FrameOffset + m_ptrHeaderView->GetState().m_FramesInView - 1;
        frame = frame <= m_ptrHeaderView->GetState().m_EndFrame ? frame : m_ptrHeaderView->GetState().m_EndFrame;

        QRect wrect = rect();

        float barWidth = GetBarWidth();
        int barWidthHalf = (int)(barWidth / 2.0f);

        QPen fatPen(QColor(255, 255, 255, 255));
        fatPen.setWidth(2);
        fatPen.setCapStyle(Qt::FlatCap);

        if (m_ptrHeaderView->GetState().m_EndFrame)
        {
            float rightEdgeOfBar = (float)wrect.right();
            float leftEdgeOfBar = rightEdgeOfBar - barWidth;

            while (frame >= 0 && rightEdgeOfBar >= wrect.left())
            {
                int actualLeftEdge = (int)floorf(leftEdgeOfBar);

                float center = (float)(actualLeftEdge + barWidthHalf) + 0.5f;

                // annotations?
                AnnotationsProvider::ConstAnnotationIterator it = m_ptrAnnotations->GetFirstAnnotationForFrame(frame);
                AnnotationsProvider::ConstAnnotationIterator endIt = m_ptrAnnotations->GetEnd();

                while ((it != endIt) && (it->GetFrameIndex() == frame))
                {
                    QPainterPath newPath;
                    QPolygonF newPolygon;
                    newPolygon << QPointF(center - adv_arrow_width, 1.0f) << QPointF(center, wrect.height() - 1.0f) << QPointF(center + adv_arrow_width, 1.0f);
                    newPath.addPolygon(newPolygon);
                    newPath.closeSubpath();

                    if (m_eventsToHighlight.find(it->GetEventIndex()) != m_eventsToHighlight.end())
                    {
                        painter.setPen(fatPen);
                        painter.setBrush(m_ptrAnnotations->GetColorForChannel(it->GetChannelCRC()));
                    }
                    else
                    {
                        painter.setPen(QColor(0, 0, 0, 0));
                        painter.setBrush(m_ptrAnnotations->GetColorForChannel(it->GetChannelCRC()));
                    }
                    painter.drawPath(newPath);
                    m_ClickableAreas[it->GetEventIndex()] = newPath;
                    ++it;
                }

                --frame;
                rightEdgeOfBar -= barWidth;
                leftEdgeOfBar -= barWidth;
            }
        }
    }

    void AnnotationsDataView::mouseMoveEvent(QMouseEvent* event)
    {
        AZStd::unordered_set<AZ::s64> newEventsToHighlight;

        for (auto it = m_ClickableAreas.begin(); it != m_ClickableAreas.end(); ++it)
        {
            if (it->second.contains(event->pos()))
            {
                auto annot = m_ptrAnnotations->GetAnnotationForEvent(it->first);
                if (annot != m_ptrAnnotations->GetEnd())
                {
                    newEventsToHighlight.insert(annot->GetEventIndex());
                    emit InformOfMouseOverAnnotation(*annot);
                }
            }
        }

        bool doUpdate = false;
        // did our highlight change?
        for (auto it = newEventsToHighlight.begin(); it != newEventsToHighlight.end(); ++it)
        {
            if (m_eventsToHighlight.find(*it) == m_eventsToHighlight.end())
            {
                doUpdate = true;
                break;
            }
        }

        // did our highlight change?
        if (!doUpdate)
        {
            for (auto it = m_eventsToHighlight.begin(); it != m_eventsToHighlight.end(); ++it)
            {
                if (newEventsToHighlight.find(*it) == newEventsToHighlight.end())
                {
                    doUpdate = true;
                    break;
                }
            }
        }

        if (doUpdate)
        {
            newEventsToHighlight.swap(m_eventsToHighlight);
            update();
        }

        // find the first annotation within a margin:
        event->ignore();
    }

    void AnnotationsDataView::mousePressEvent(QMouseEvent* event)
    {
        for (auto it = m_ClickableAreas.begin(); it != m_ClickableAreas.end(); ++it)
        {
            if (it->second.contains(event->pos()))
            {
                auto annot = m_ptrAnnotations->GetAnnotationForEvent(it->first);
                if (annot != m_ptrAnnotations->GetEnd())
                {
                    emit InformOfClickAnnotation(*annot);
                }
            }
        }
        event->ignore();
    }

    void AnnotationsDataView::mouseReleaseEvent(QMouseEvent* event)
    {
        event->ignore();
    }
}

#include <Source/Driller/Annotations/moc_AnnotationsDataView.cpp>
