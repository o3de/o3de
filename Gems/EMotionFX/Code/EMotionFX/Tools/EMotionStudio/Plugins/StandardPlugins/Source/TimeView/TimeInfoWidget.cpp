/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        m_plugin = plugin;

        // init font
        m_font.setPixelSize(m_showOverwriteStartTime ? 22 : 18);
        m_overwriteFont.setPixelSize(12);

        m_overwriteStartTime = 0;
        m_overwriteEndTime   = 0;
        m_overwriteMode      = false;

        // init brushes and pens
        m_penText            = QPen(QColor(200, 200, 200));
        m_penTextFocus       = QPen(QColor(244, 156, 28));

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
        m_overwriteStartTime = startTime;
        m_overwriteEndTime   = endTime;
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

        if (m_plugin->GetTrackDataWidget()->hasFocus())
        {
            painter.setPen(m_penTextFocus);
        }
        else
        {
            painter.setPen(m_penText);
        }

        painter.setFont(m_font);

        // calculate the time values for this pixel
        uint32 minutes;
        uint32 seconds;
        uint32 milSecs;
        uint32 frameNumber;
        m_plugin->DecomposeTime(m_plugin->m_curTime, &minutes, &seconds, &milSecs, &frameNumber);
        m_curTimeString = AZStd::string::format("%.2d:%.2d:%.2d", minutes, seconds, milSecs);

        QRect upperTextRect = event->rect();
        if (m_showOverwriteStartTime)
        {
            upperTextRect.setTop(upperTextRect.top() + 1);
            upperTextRect.setHeight(upperTextRect.height() - 17);
        }
        else
        {
            m_plugin->DecomposeTime(m_overwriteEndTime, &minutes, &seconds, &milSecs, &frameNumber);
            m_curTimeString += AZStd::string::format(" / %.2d:%.2d:%.2d", minutes, seconds, milSecs);
        }
        painter.drawText(upperTextRect, m_curTimeString.c_str(), options);

        if (!m_showOverwriteStartTime)
        {
            return;
        }

        if (m_overwriteStartTime < 0)
        {
            m_overwriteStartTime = 0;
        }

        if (m_overwriteEndTime < 0)
        {
            m_overwriteEndTime = 0;
        }

        // calculate the time values for the overwrite time
        uint32 minutesStart, minutesEnd;
        uint32 secondsStart, secondsEnd;
        uint32 milSecsStart, milSecsEnd;
        m_plugin->DecomposeTime(m_overwriteStartTime, &minutesStart, &secondsStart, &milSecsStart, &frameNumber);
        m_plugin->DecomposeTime(m_overwriteEndTime, &minutesEnd, &secondsEnd, &milSecsEnd, &frameNumber);

        // use the duration of the motion or recording
        if (minutesStart == minutesEnd && secondsStart == secondsEnd && milSecsStart == milSecsEnd)
        {
            uint32 dummyFrame;
            double duration;
            m_plugin->GetDataTimes(&duration, nullptr, nullptr);
            m_plugin->DecomposeTime(duration, &minutesEnd, &secondsEnd, &milSecsEnd, &dummyFrame);
        }

        m_overwriteTimeString = AZStd::string::format("%.2d:%.2d:%.2d / %.2d:%.2d:%.2d", minutesStart, secondsStart, milSecsStart, minutesEnd, secondsEnd, milSecsEnd);

        QRect lowerTextRect = event->rect();
        lowerTextRect.setTop(upperTextRect.height());
        painter.setFont(m_overwriteFont);
        painter.drawText(lowerTextRect, m_overwriteTimeString.c_str(), options);
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TimeInfoWidget::keyPressEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TimeInfoWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyReleaseEvent(event);
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/moc_TimeInfoWidget.cpp>
