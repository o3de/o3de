/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#include <MCore/Source/Color.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/Algorithms.h>


namespace EMStudio
{
    // statics
    QColor TimeTrackElement::mTextColor             = QColor(30, 30, 30);
    QColor TimeTrackElement::mHighlightedTextColor  = QColor(0, 0, 0);
    QColor TimeTrackElement::mHighlightedColor      = QColor(255, 128, 0);
    int32  TimeTrackElement::mTickHalfWidth         = 7;

    // constructor
    TimeTrackElement::TimeTrackElement(const char* name, TimeTrack* timeTrack, uint32 elementNumber, QColor color)
    {
        mTrack              = timeTrack;
        mName               = name;
        mIsSelected         = false;
        mShowTimeHandles    = false;
        mIsHighlighted      = false;
        mStartTime          = 0.0;
        mEndTime            = 0.0;
        mColor              = color;
        mElementNumber      = elementNumber;
        mIsCut              = false;

        // init font
        mFont.setPixelSize(10);
    }


    // destructor
    TimeTrackElement::~TimeTrackElement()
    {
    }


    // calculate the dimensions in pixels
    void TimeTrackElement::CalcDimensions(int32* outStartX, int32* outStartY, int32* outWidth, int32* outHeight) const
    {
        TimeViewPlugin* plugin = mTrack->GetPlugin();

        *outStartX  = aznumeric_cast<int32>(plugin->TimeToPixel(mStartTime));
        int32 endX  = aznumeric_cast<int32>(plugin->TimeToPixel(mEndTime));
        *outStartY  = mTrack->GetStartY() + 1;
        *outWidth   = (endX - *outStartX);
        *outHeight  = mTrack->GetHeight() - 1;
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

        QColor color        = mColor;
        QColor borderColor(30, 30, 30);
        QColor textColor    = mTextColor;
        if (mIsSelected)
        {
            color       = mHighlightedColor;
            borderColor = mHighlightedColor;
            textColor   = mHighlightedTextColor;
        }

        // in case the track is disabled
        if (isTrackEnabled == false)
        {
            borderColor.setAlpha(25);
            color.setAlpha(25);
            textColor.setAlpha(150);
        }

        // make the colors a bit lighter so that we see some highlighting effect
        if (mIsHighlighted)
        {
            borderColor = borderColor.lighter(130);
            color       = color.lighter(130);
        }

        // in case the track is cutted
        if (mIsCut)
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
            painter.setFont(mFont);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawText(rect, mName, options);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }
        else
        {
            height--;
            mTickPoints[0] = QPoint(startX,                 startY);
            mTickPoints[1] = QPoint(startX + mTickHalfWidth,  startY + height / 2);
            mTickPoints[2] = QPoint(startX + mTickHalfWidth,  startY + height);
            mTickPoints[3] = QPoint(startX - mTickHalfWidth,  startY + height);
            mTickPoints[4] = QPoint(startX - mTickHalfWidth,  startY + height / 2);
            mTickPoints[5] = QPoint(startX,                 startY);

            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            //painter.setBrush( color );
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolygon(mTickPoints, 5, Qt::WindingFill);
            painter.setRenderHint(QPainter::Antialiasing, false);

            painter.setBrush(Qt::NoBrush);
            painter.setPen(borderColor);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolyline(mTickPoints, 6);
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
            startX -= mTickHalfWidth;
            width  += 2 * mTickHalfWidth;
        }

        // take scrolling into account
        startX = aznumeric_cast<int32>(startX + mTrack->GetPlugin()->GetScrollX());

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
        if (mStartTime + timeDelta < 0.0)
        {
            mStartTime -= mStartTime;
            mEndTime -= mStartTime;
        }
        else
        {
            mStartTime  += timeDelta;
            mEndTime    += timeDelta;
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
            startX -= mTickHalfWidth;
            width  += 2 * mTickHalfWidth;
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
            double newStartTime = mStartTime + timeDelta;
            mTrack->GetPlugin()->SnapTime(&newStartTime, this, snapThreshold);
            mStartTime = newStartTime;

            if (newStartTime > mEndTime)
            {
                mStartTime  = mEndTime;
                mEndTime    = newStartTime;
                return RESIZEPOINT_END;
            }

            return RESIZEPOINT_START;
        }
        break;

        // resize the end point
        case RESIZEPOINT_END:
        {
            double newEndTime = mEndTime + timeDelta;
            mTrack->GetPlugin()->SnapTime(&newEndTime, this, snapThreshold);
            mEndTime = newEndTime;

            if (newEndTime < mStartTime)
            {
                mEndTime    = mStartTime;
                mStartTime  = newEndTime;
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
