/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "TimeInfoWidget.h"
#include "TimeViewPlugin.h"
#include "TrackDataWidget.h"
#include <QPainter>
#include <QPaintEvent>

#include <MCore/Source/LogManager.h>

namespace EMStudio
{
    // the constructor
    TimeInfoWidget::TimeInfoWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("TimeInfoWidget");

        mPlugin = plugin;

        // init font
        mFont.setPixelSize(mShowOverwriteStartTime ? 22 : 18);
        mOverwriteFont.setPixelSize(12);
        //mFont.setBold( true );

        mOverwriteStartTime = 0;
        mOverwriteEndTime   = 0;
        mOverwriteMode      = false;

        // init brushes and pens
        mPenText            = QPen(QColor(200, 200, 200));
        mPenTextFocus       = QPen(QColor(244, 156, 28));

        setFocusPolicy(Qt::StrongFocus);
    }


    // destructor
    TimeInfoWidget::~TimeInfoWidget()
    {
    }


    // return the preferred size
    QSize TimeInfoWidget::sizeHint() const
    {
        return QSize(150, 35);
    }


    // set the overwrite time which will be displayed when the overwrite mode is active
    void TimeInfoWidget::SetOverwriteTime(double startTime, double endTime)
    {
        mOverwriteStartTime = startTime;
        mOverwriteEndTime   = endTime;
    }


    // paint event
    void TimeInfoWidget::paintEvent(QPaintEvent* event)
    {
        // start painting
        QPainter painter(this);

        // enable anti aliassing
        //painter.setRenderHint(QPainter::Antialiasing);

        QTextOption options;
        options.setAlignment(Qt::AlignCenter);

        if (mPlugin->GetTrackDataWidget()->hasFocus())
        {
            painter.setPen(mPenTextFocus);
        }
        else
        {
            painter.setPen(mPenText);
        }

        painter.setFont(mFont);

        // use the time of the plugin in case we are not in overwrite mode, if we are use the overwrite time
        //uint32 usedTime = mPlugin->mCurTimeX;
        //if (mOverwriteMode)
        //  usedTime = mOverwriteTime;

        // calculate the time values for this pixel
        uint32 minutes;
        uint32 seconds;
        uint32 milSecs;
        uint32 frameNumber;
        //  mPlugin->CalcTime(mPlugin->mCurTimeX/mPlugin->mTimeScale, nullptr, &minutes, &seconds, &milSecs, &frameNumber, false);
        mPlugin->DecomposeTime(mPlugin->mCurTime, &minutes, &seconds, &milSecs, &frameNumber);
        mCurTimeString = AZStd::string::format("%.2d:%.2d:%.2d", minutes, seconds, milSecs);

        QRect upperTextRect = event->rect();
        if (mShowOverwriteStartTime)
        {
            upperTextRect.setTop(upperTextRect.top() + 1);
            upperTextRect.setHeight(upperTextRect.height() - 17);
        }
        else
        {
            mPlugin->DecomposeTime(mOverwriteEndTime, &minutes, &seconds, &milSecs, &frameNumber);
            mCurTimeString += AZStd::string::format(" / %.2d:%.2d:%.2d", minutes, seconds, milSecs);
        }
        painter.drawText(upperTextRect, mCurTimeString.c_str(), options);

        if (!mShowOverwriteStartTime)
        {
            return;
        }

        if (mOverwriteStartTime < 0)
        {
            mOverwriteStartTime = 0;
        }

        if (mOverwriteEndTime < 0)
        {
            mOverwriteEndTime = 0;
        }

        // calculate the time values for the overwrite time
        uint32 minutesStart, minutesEnd;
        uint32 secondsStart, secondsEnd;
        uint32 milSecsStart, milSecsEnd;
        mPlugin->DecomposeTime(mOverwriteStartTime, &minutesStart, &secondsStart, &milSecsStart, &frameNumber);
        mPlugin->DecomposeTime(mOverwriteEndTime, &minutesEnd, &secondsEnd, &milSecsEnd, &frameNumber);

        // use the duration of the motion or recording
        if (minutesStart == minutesEnd && secondsStart == secondsEnd && milSecsStart == milSecsEnd)
        {
            //mOverwriteTimeString = AZStd::string::format("%.2d:%.2d:%.2d", minutesStart, secondsStart, milSecsStart);
            uint32 dummyFrame;
            double duration;
            mPlugin->GetDataTimes(&duration, nullptr, nullptr);
            mPlugin->DecomposeTime(duration, &minutesEnd, &secondsEnd, &milSecsEnd, &dummyFrame);
        }

        mOverwriteTimeString = AZStd::string::format("%.2d:%.2d:%.2d / %.2d:%.2d:%.2d", minutesStart, secondsStart, milSecsStart, minutesEnd, secondsEnd, milSecsEnd);

        QRect lowerTextRect = event->rect();
        lowerTextRect.setTop(upperTextRect.height());
        painter.setFont(mOverwriteFont);
        painter.drawText(lowerTextRect, mOverwriteTimeString.c_str(), options);
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TimeInfoWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TimeInfoWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/moc_TimeInfoWidget.cpp>
