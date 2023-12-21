/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TimeTrackElement.h"
#include "TimeViewPlugin.h"
#include "TimeTrack.h"
#include <QPainter>
#include <QPaintEvent>
#include <MysticQt/Source/MysticQtConfig.h>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/Algorithms.h>


namespace EMStudio
{
    // statics
    QColor TimeTrackElement::s_textColor             = QColor(30, 30, 30);
    QColor TimeTrackElement::s_highlightedTextColor  = QColor(0, 0, 0);
    QColor TimeTrackElement::s_highlightedColor      = QColor(255, 128, 0);
    int32  TimeTrackElement::s_tickHalfWidth         = 7;

    // constructor
    TimeTrackElement::TimeTrackElement(const char* name, TimeTrack* timeTrack, size_t elementNumber, QColor color)
    {
        m_track              = timeTrack;
        m_name               = name;
        m_isSelected         = false;
        m_showTimeHandles    = false;
        m_isHighlighted      = false;
        m_startTime          = 0.0;
        m_endTime            = 0.0;
        m_color              = color;
        m_elementNumber      = elementNumber;
        m_isCut              = false;

        // init font
        m_font.setPixelSize(10);
    }


    // destructor
    TimeTrackElement::~TimeTrackElement()
    {
    }


    // calculate the dimensions in pixels
    void TimeTrackElement::CalcDimensions(int32* outStartX, int32* outStartY, int32* outWidth, int32* outHeight) const
    {
        TimeViewPlugin* plugin = m_track->GetPlugin();

        *outStartX  = aznumeric_cast<int32>(plugin->TimeToPixel(m_startTime));
        int32 endX  = aznumeric_cast<int32>(plugin->TimeToPixel(m_endTime));
        *outStartY  = m_track->GetStartY() + 1;
        *outWidth   = (endX - *outStartX);
        *outHeight  = m_track->GetHeight() - 1;
    }


    // calculate the rect of an element
    QRect TimeTrackElement::CalcRect()
    {
        // calculate the dimensions
        int32 startX, startY, width, height;
        CalcDimensions(&startX, &startY, &width, &height);

        return QRect(startX, startY, width, height);
    }


    // render the element
    void TimeTrackElement::Render(QPainter& painter, bool isTrackEnabled)
    {
        if (GetIsVisible() == false)
        {
            return;
        }

        // calculate the dimensions
        int32 startX, startY, width, height;
        CalcDimensions(&startX, &startY, &width, &height);

        const bool isTickElement = width < 1 ? true : false;

        // create the rect
        QRect rect(startX, startY, width + 1, height);

        QColor color        = m_color;
        QColor borderColor(30, 30, 30);
        QColor textColor    = s_textColor;
        if (m_isSelected)
        {
            color       = s_highlightedColor;
            borderColor = s_highlightedColor;
            textColor   = s_highlightedTextColor;
        }

        // in case the track is disabled
        if (isTrackEnabled == false)
        {
            borderColor.setAlpha(25);
            color.setAlpha(25);
            textColor.setAlpha(150);
        }

        // make the colors a bit lighter so that we see some highlighting effect
        if (m_isHighlighted)
        {
            borderColor = borderColor.lighter(130);
            color       = color.lighter(130);
        }

        // in case the track is cutted
        if (m_isCut)
        {
            borderColor.setAlpha(90);
            color.setAlpha(90);
            textColor.setAlpha(200);
        }


        // setup the gradient
        //  const QColor gradientColor = QColor( color.red()/5, color.green()/5, color.blue()/5, color.alpha());
        const QColor gradientColor = QColor(color.red() / 2, color.green() / 2, color.blue() / 2, color.alpha());
        QLinearGradient gradient(0, startY, 0, startY + height);
        gradient.setColorAt(0.0f, color);
        gradient.setColorAt(1.0f, gradientColor);


        // render the background
        if (isTickElement == false)
        {
            painter.setPen(borderColor);
            painter.setBrush(gradient);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawRoundedRect(startX, startY + 1, width, height - 2, 3.0f, 3.0f);
            painter.setRenderHint(QPainter::Antialiasing, false);

            // draw the text
            QTextOption options;
            options.setWrapMode(QTextOption::NoWrap);
            options.setAlignment(Qt::AlignCenter);

            painter.setPen(textColor);
            painter.setFont(m_font);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawText(rect, m_name, options);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }
        else
        {
            height--;
            m_tickPoints[0] = QPoint(startX,                 startY);
            m_tickPoints[1] = QPoint(startX + s_tickHalfWidth,  startY + height / 2);
            m_tickPoints[2] = QPoint(startX + s_tickHalfWidth,  startY + height);
            m_tickPoints[3] = QPoint(startX - s_tickHalfWidth,  startY + height);
            m_tickPoints[4] = QPoint(startX - s_tickHalfWidth,  startY + height / 2);
            m_tickPoints[5] = QPoint(startX,                 startY);

            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            //painter.setBrush( color );
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolygon(m_tickPoints, 5, Qt::WindingFill);
            painter.setRenderHint(QPainter::Antialiasing, false);

            painter.setBrush(Qt::NoBrush);
            painter.setPen(borderColor);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolyline(m_tickPoints, 6);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }
    }


    // check if the given point is inside
    bool TimeTrackElement::CheckIfIsInside(int32 x, int32 y) const
    {
        if (GetIsVisible() == false)
        {
            return false;
        }

        // calculate the dimensions
        int32 startX, startY, width, height;
        CalcDimensions(&startX, &startY, &width, &height);

        bool isTickElement = width < 1 ? true : false;
        if (isTickElement)
        {
            startX -= s_tickHalfWidth;
            width  += 2 * s_tickHalfWidth;
        }

        // take scrolling into account
        startX = aznumeric_cast<int32>(startX + m_track->GetPlugin()->GetScrollX());

        // check if we're inside the area of the element
        if (MCore::InRange<int32>(x, startX, startX + width) && MCore::InRange<int32>(y, startY, startY + height))
        {
            return true;
        }

        // we're not inside
        return false;
    }



    // snap to the start and end time
    bool TimeTrackElement::SnapTime(double* inOutTime, double snapThreshold) const
    {
        if (GetIsVisible() == false)
        {
            return false;
        }

        // calculate the time distances between the snap points
        const double elementStartDif    = *inOutTime - GetStartTime();
        const double elementEndDif      = *inOutTime - GetEndTime();

        // check if it's close enough to the start time
        if (MCore::Math::Abs(aznumeric_cast<float>(elementStartDif)) < snapThreshold)
        {
            *inOutTime = GetStartTime();
            return true;
        }
        else
        {
            // check if it's close enough to the end time
            if (MCore::Math::Abs(aznumeric_cast<float>(elementEndDif)) < snapThreshold)
            {
                *inOutTime = GetEndTime();
                return true;
            }
        }

        // no snapping has occurred
        return false;
    }



    // move with a given delta
    void TimeTrackElement::MoveRelative(double timeDelta)
    {
        // don't allow it to start before zero
        if (m_startTime + timeDelta < 0.0)
        {
            m_startTime -= m_startTime;
            m_endTime -= m_startTime;
        }
        else
        {
            m_startTime  += timeDelta;
            m_endTime    += timeDelta;
        }
    }


    // check if we are close to the start or end
    bool TimeTrackElement::FindResizePoint(int32 x, int32 y, uint32* outID)
    {
        if (GetIsVisible() == false)
        {
            return false;
        }

        // calculate the dimensions
        int32 startX, startY, width, height;
        CalcDimensions(&startX, &startY, &width, &height);

        bool isTickElement = width < 1 ? true : false;
        if (isTickElement)
        {
            startX -= s_tickHalfWidth;
            width  += 2 * s_tickHalfWidth;
        }

        int32 endX = startX + width;
        if (MCore::InRange<int32>(x, endX - 2, endX) && MCore::InRange<int32>(y, startY, startY + height))
        {
            *outID = RESIZEPOINT_END;
            return true;
        }

        if (MCore::InRange<int32>(x, startX, startX + 2) && MCore::InRange<int32>(y, startY, startY + height))
        {
            *outID = RESIZEPOINT_START;
            return true;
        }

        // not close to a resize point
        return false;
    }


    // handle the resize of a given point ID
    uint32 TimeTrackElement::HandleResize(uint32 id, double timeDelta, double snapThreshold)
    {
        switch (id)
        {
        // resize the start point
        case RESIZEPOINT_START:
        {
            double newStartTime = m_startTime + timeDelta;
            m_track->GetPlugin()->SnapTime(&newStartTime, this, snapThreshold);
            m_startTime = newStartTime;

            if (newStartTime > m_endTime)
            {
                m_startTime  = m_endTime;
                m_endTime    = newStartTime;
                return RESIZEPOINT_END;
            }

            return RESIZEPOINT_START;
        }
        break;

        // resize the end point
        case RESIZEPOINT_END:
        {
            double newEndTime = m_endTime + timeDelta;
            m_track->GetPlugin()->SnapTime(&newEndTime, this, snapThreshold);
            m_endTime = newEndTime;

            if (newEndTime < m_startTime)
            {
                m_endTime    = m_startTime;
                m_startTime  = newEndTime;
                return RESIZEPOINT_START;
            }

            return RESIZEPOINT_END;
        }
        break;

        default:
            AZ_Warning("EMotionFX", false, "Don't know how to handle resizing of resize point ID %d on element '%s'", id, GetName().toUtf8().data());
        }

        return MCORE_INVALIDINDEX32;
    }
}   // namespace EMStudio
