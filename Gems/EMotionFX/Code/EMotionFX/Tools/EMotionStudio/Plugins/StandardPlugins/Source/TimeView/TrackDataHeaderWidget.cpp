/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "TrackDataHeaderWidget.h"
#include "TimeViewPlugin.h"
#include "TimeInfoWidget.h"
#include "TrackHeaderWidget.h"
#include "TimeViewToolBar.h"
#include <QDir>
#include <QPainter>
#include <QToolTip>
#include <QPaintEvent>
#include <QMenu>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/Algorithms.h>

#include <MysticQt/Source/MysticQtManager.h>

#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>

#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"


namespace EMStudio
{
    // the constructor
    TrackDataHeaderWidget::TrackDataHeaderWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QOpenGLWidget(parent)
        , QOpenGLFunctions()
        , m_plugin(plugin)
        , m_lastMouseX(0)
        , m_lastMouseY(0)
        , m_mouseLeftClicked(false)
        , m_mouseRightClicked(false)
        , m_mouseMidClicked(false)
        , m_isScrolling(false)
        , m_allowContextMenu(true)
    {
        setObjectName("TrackDataHeaderWidget");

        // init brushes and pens
        m_brushBackgroundOutOfRange  = QBrush(QColor(35, 35, 35), Qt::SolidPattern);

        m_headerGradientActive       = QLinearGradient(0, 0, 0, 35);
        m_headerGradientActive.setColorAt(1.0f, QColor(100, 105, 110));
        m_headerGradientActive.setColorAt(0.5f, QColor(30, 35, 40));
        m_headerGradientActive.setColorAt(0.0f, QColor(20, 20, 20));

        m_headerGradientActiveFocus  = QLinearGradient(0, 0, 0, 35);
        m_headerGradientActiveFocus.setColorAt(1.0f, QColor(100, 105, 130));
        m_headerGradientActiveFocus.setColorAt(0.5f, QColor(30, 35, 40));
        m_headerGradientActiveFocus.setColorAt(0.0f, QColor(20, 20, 20));

        m_headerGradientInactive     = QLinearGradient(0, 0, 0, 35);
        m_headerGradientInactive.setColorAt(1.0f, QColor(30, 30, 30));
        m_headerGradientInactive.setColorAt(0.0f, QColor(20, 20, 20));

        m_headerGradientInactiveFocus = QLinearGradient(0, 0, 0, 35);
        m_headerGradientInactiveFocus.setColorAt(1.0f, QColor(30, 30, 30));
        m_headerGradientInactiveFocus.setColorAt(0.0f, QColor(20, 20, 20));

        m_penMainTimeStepLinesActive     = QPen(QColor(110, 110, 110));

        m_timeLineFont.setPixelSize(12);
        m_dataFont.setPixelSize(13);

        // load the time handle top image
        QDir imageName{ QString(MysticQt::GetMysticQt()->GetDataDir().c_str()) };
        m_timeHandleTop = QPixmap(imageName.filePath("Images/Icons/TimeHandleTop.png"));

        setMouseTracking(true);
        setAcceptDrops(true);
        setAutoFillBackground(false);

        setFocusPolicy(Qt::StrongFocus);
    }


    // destructor
    TrackDataHeaderWidget::~TrackDataHeaderWidget()
    {
    }


    // init gl
    void TrackDataHeaderWidget::initializeGL()
    {
        initializeOpenGLFunctions();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }


    // resize
    void TrackDataHeaderWidget::resizeGL(int w, int h)
    {
        MCORE_UNUSED(w);
        MCORE_UNUSED(h);
        if (m_plugin)
        {
            m_plugin->SetRedrawFlag();
        }
    }

    void TrackDataHeaderWidget::paintGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // start painting
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        QRect rect(0, 0, geometry().width(), geometry().height());

        // draw a background rect
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_brushBackgroundOutOfRange);
        painter.drawRect(rect);
        painter.setFont(m_dataFont);

        // draw the timeline
        painter.setRenderHint(QPainter::Antialiasing, false);
        DrawTimeLine(painter, rect);

        const uint32 height = geometry().height();
        m_plugin->RenderElementTimeHandles(painter, height, m_plugin->m_penTimeHandles);

        DrawTimeMarker(painter, rect);
    }

    // draw the time marker
    void TrackDataHeaderWidget::DrawTimeMarker(QPainter& painter, const QRect& rect)
    {
        // draw the current time marker
        float startHeight = 0.0f;
        const float curTimeX = aznumeric_cast<float>(m_plugin->TimeToPixel(m_plugin->m_curTime));
        painter.drawPixmap(aznumeric_cast<int>(curTimeX - (m_timeHandleTop.width() / 2) - 1), 0, m_timeHandleTop);

        painter.setPen(m_plugin->m_penCurTimeHandle);
        painter.drawLine(QPointF(curTimeX, startHeight), QPointF(curTimeX, rect.bottom()));
    }

    void TrackDataHeaderWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        // if double clicked in the timeline
        m_plugin->MakeTimeVisible(m_plugin->PixelToTime(event->x()), 0.5);
    }

    // when the mouse is moving, while a button is pressed
    void TrackDataHeaderWidget::mouseMoveEvent(QMouseEvent* event)
    {
        m_plugin->SetRedrawFlag();

        const int32 deltaRelX = event->x() - m_lastMouseX;
        m_lastMouseX = event->x();
        m_plugin->m_curMouseX = event->x();
        m_plugin->m_curMouseY = event->y();

        const int32 deltaRelY = event->y() - m_lastMouseY;
        m_lastMouseY = event->y();

        const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = m_mouseLeftClicked == false && m_mouseRightClicked && altPressed;
        const bool isPanning = m_mouseLeftClicked == false && isZooming == false && (m_mouseMidClicked || m_mouseRightClicked);

        if (deltaRelY != 0)
        {
            m_allowContextMenu = false;
        }

        if (m_mouseRightClicked)
        {
            m_isScrolling = true;
        }

        // if the mouse left button is pressed
        if (m_mouseLeftClicked)
        {
            // update the current time marker
            int newX = event->x();
            newX = AZ::GetClamp(newX, 0, geometry().width() - 1);
            m_plugin->m_curTime = m_plugin->PixelToTime(newX);

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if (recorder.GetRecordTime() > AZ::Constants::FloatEpsilon)
            {
                if (recorder.GetIsInPlayMode())
                {
                    recorder.SetCurrentPlayTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    recorder.SetAutoPlay(false);
                    emit m_plugin->ManualTimeChange(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                }
            }
            else
            {
                const AZStd::vector<EMotionFX::MotionInstance*>& selectedMotionInstances = CommandSystem::GetCommandManager()->GetCurrentSelection().GetSelectedMotionInstances();
                if (selectedMotionInstances.size() == 1)
                {
                    EMotionFX::MotionInstance* motionInstance = selectedMotionInstances[0];
                    motionInstance->SetCurrentTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()), false);
                    motionInstance->SetPause(true);
                    emit m_plugin->ManualTimeChange(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                }
            }

            m_isScrolling = true;
        }
        else if (isPanning)
        {
            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                m_plugin->DeltaScrollX(-deltaRelX, false);
            }
        }
        else if (isZooming)
        {
            if (deltaRelY < 0)
            {
                setCursor(*(m_plugin->GetZoomOutCursor()));
            }
            else
            {
                setCursor(*(m_plugin->GetZoomInCursor()));
            }

            DoMouseYMoveZoom(deltaRelY, m_plugin);
        }
        else // no left mouse button is pressed
        {
            UpdateMouseOverCursor();
        }
    }

    void TrackDataHeaderWidget::DoMouseYMoveZoom(int32 deltaY, TimeViewPlugin* plugin)
    {
        // keep the scaling speed in range so that we're not scaling insanely fast
        float movement = MCore::Min(aznumeric_cast<float>(deltaY), 9.0f);
        movement = MCore::Max<float>(movement, -9.0f);

        // scale relatively to the current scaling value, meaning when the range is bigger we scale faster than when viewing only a very small time range
        float timeScale = plugin->GetTimeScale();
        timeScale *= 1.0f - 0.01f * movement;

        // set the new scaling value
        plugin->SetScale(timeScale);
    }

    // update the mouse-over cursor, depending on its location
    void TrackDataHeaderWidget::UpdateMouseOverCursor()
    {
        // disable all tooltips
        m_plugin->DisableAllToolTips();
    }


    // when the mouse is pressed
    void TrackDataHeaderWidget::mousePressEvent(QMouseEvent* event)
    {
        m_plugin->SetRedrawFlag();

        const bool altPressed = event->modifiers() & Qt::AltModifier;

        // store the last clicked position
        m_allowContextMenu = true;

        if (event->button() == Qt::RightButton)
        {
            m_mouseRightClicked = true;
        }

        if (event->button() == Qt::MidButton)
        {
            m_mouseMidClicked = true;
        }

        if (event->button() == Qt::LeftButton)
        {
            m_mouseLeftClicked   = true;

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if (!m_plugin->m_nodeHistoryItem && !altPressed)
            {
                // update the current time marker
                int newX = event->x();
                newX = AZ::GetClamp(newX, 0, geometry().width() - 1);
                m_plugin->m_curTime = m_plugin->PixelToTime(newX);

                if (recorder.GetRecordTime() > AZ::Constants::FloatEpsilon)
                {
                    if (!recorder.GetIsInPlayMode())
                    {
                        recorder.StartPlayBack();
                    }

                    recorder.SetCurrentPlayTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    recorder.SetAutoPlay(false);
                    emit m_plugin->ManualTimeChangeStart(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    emit m_plugin->ManualTimeChange(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                }
                else
                {
                    const AZStd::vector<EMotionFX::MotionInstance*>& selectedMotionInstances = CommandSystem::GetCommandManager()->GetCurrentSelection().GetSelectedMotionInstances();
                    if (selectedMotionInstances.size() == 1)
                    {
                        EMotionFX::MotionInstance* motionInstance = selectedMotionInstances[0];
                        motionInstance->SetCurrentTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()), false);
                        motionInstance->SetPause(true);
                        m_plugin->GetTimeViewToolBar()->UpdateInterface();
                        emit m_plugin->ManualTimeChangeStart(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                        emit m_plugin->ManualTimeChange(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    }
                }
            }
        }

        //const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = m_mouseLeftClicked == false && m_mouseRightClicked && altPressed;
        const bool isPanning = m_mouseLeftClicked == false && isZooming == false && (m_mouseMidClicked || m_mouseRightClicked);

        if (isPanning)
        {
            setCursor(Qt::SizeHorCursor);
        }

        if (isZooming)
        {
            setCursor(*(m_plugin->GetZoomInCursor()));
        }
    }


    // when releasing the mouse button
    void TrackDataHeaderWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        m_plugin->SetRedrawFlag();

        setCursor(Qt::ArrowCursor);

        // disable overwrite mode in any case when the mouse gets released so that we display the current time from the plugin again
        if (m_plugin->GetTimeInfoWidget())
        {
            m_plugin->GetTimeInfoWidget()->SetIsOverwriteMode(false);
        }

        if (event->button() == Qt::RightButton)
        {
            m_mouseRightClicked = false;
            m_isScrolling = false;
        }

        if (event->button() == Qt::MidButton)
        {
            m_mouseMidClicked = false;
        }

        if (event->button() == Qt::LeftButton)
        {
            m_mouseLeftClicked   = false;
            m_isScrolling        = false;

            return;
        }

        UpdateMouseOverCursor();
    }

    // drag & drop support
    void TrackDataHeaderWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        m_plugin->SetRedrawFlag();

        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }

    // the mouse wheel is adjusted
    void TrackDataHeaderWidget::DoWheelEvent(QWheelEvent* event, TimeViewPlugin* plugin)
    {
        plugin->SetRedrawFlag();

        // Vertical
        {
            const int numDegrees = event->angleDelta().y() / 8;
            const int numSteps = numDegrees / 15;
            float delta = numSteps / 10.0f;

            double zoomDelta = delta * 4 * MCore::Clamp(plugin->GetTimeScale() / 2.0, 1.0, 22.0);

            plugin->SetScale(plugin->GetTimeScale() + zoomDelta);
        }

        // Horizontal
        {
            const int numDegrees = event->angleDelta().x() / 8;
            const int numSteps = numDegrees / 15;
            float delta = numSteps / 10.0f;

            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                if (delta > 0)
                {
                    delta = 1;
                }
                else
                {
                    delta = -1;
                }

                plugin->DeltaScrollX(-delta * 600);
            }
        }
    }

    // handle mouse wheel event
    void TrackDataHeaderWidget::wheelEvent(QWheelEvent* event)
    {
        DoWheelEvent(event, m_plugin);
    }

    void TrackDataHeaderWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        m_plugin->SetRedrawFlag();
        QPoint mousePos = event->pos();

        double dropTime = m_plugin->PixelToTime(mousePos.x());
        m_plugin->SetCurrentTime(dropTime);

        const AZStd::vector<EMotionFX::MotionInstance*>& selectedMotionInstances = CommandSystem::GetCommandManager()->GetCurrentSelection().GetSelectedMotionInstances();
        if (selectedMotionInstances.size() == 1)
        {
            EMotionFX::MotionInstance* motionInstance = selectedMotionInstances[0];
            motionInstance->SetCurrentTime(aznumeric_cast<float>(dropTime), false);
            motionInstance->Pause();
        }

        //event->accept();
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataHeaderWidget::keyPressEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataHeaderWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyReleaseEvent(event);
        }
    }

    // draw the time line
    void TrackDataHeaderWidget::DrawTimeLine(QPainter& painter, const QRect& rect)
    {
        // get the time values in seconds
        double animationLength  = 0.0;
        double clipStart        = 0.0;
        double clipEnd          = 0.0;
        m_plugin->GetDataTimes(&animationLength, &clipStart, &clipEnd);

        // calculate the pixel offsets
        double animEndPixel     = m_plugin->TimeToPixel(animationLength);
        double clipStartPixel   = m_plugin->TimeToPixel(clipStart);

        // fill with the background color
        QRect motionRect        = rect;
        QRect outOfRangeRect    = rect;

        motionRect.setLeft(aznumeric_cast<int>(clipStartPixel));
        motionRect.setRight(aznumeric_cast<int>(animEndPixel));
        outOfRangeRect.setLeft(aznumeric_cast<int>(animEndPixel));

        painter.setPen(Qt::NoPen);
        if (hasFocus() == false)
        {
            painter.setBrush(m_headerGradientActive);
            painter.drawRect(motionRect);
            painter.setBrush(m_headerGradientInactive);
            painter.drawRect(outOfRangeRect);
        }
        else
        {
            painter.setBrush(m_headerGradientActiveFocus);
            painter.drawRect(motionRect);
            painter.setBrush(m_headerGradientInactiveFocus);
            painter.drawRect(outOfRangeRect);
        }

        // show the recorder range
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            QRectF recorderRect = rect;
            recorderRect.setRight(m_plugin->TimeToPixel(recorder.GetRecordTime()));
            recorderRect.setTop(height() - 3);
            recorderRect.setBottom(height());

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(175, 0, 0));
            painter.drawRect(recorderRect);
        }
        else
        {
            if (animationLength > MCore::Math::epsilon)
            {
                QRectF rangeRect = rect;
                rangeRect.setRight(m_plugin->TimeToPixel(animationLength));
                rangeRect.setTop(height() - 3);
                rangeRect.setBottom(height());

                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 175, 0));
                painter.drawRect(rangeRect);
            }
        }

        QTextOption options;
        options.setAlignment(Qt::AlignCenter);
        painter.setFont(m_timeLineFont);

        const uint32 width = rect.width();
        //const uint32 height = rect.height();

        float yOffset = 19.0f;

        double timeOffset = m_plugin->PixelToTime(0.0) * 1000.0;
        timeOffset = (timeOffset - ((int32)timeOffset % 5000)) / 1000.0;

        uint32 minutes, seconds, milSecs, frameNumber;
        double pixelTime;

        double curX = 0.0;
        double curTime = timeOffset;

        //uint32 index = 0;
        while (curX <= width)
        {
            curX = m_plugin->TimeToPixel(curTime, false);
            m_plugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
            seconds += minutes * 60;
            curX *= m_plugin->m_timeScale;

            curTime += 5.0;

            painter.setPen(m_penMainTimeStepLinesActive);
            painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));

            m_timeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

            painter.setPen(QColor(175, 175, 175));
            painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), m_timeString.c_str(), options);
        }

        // draw the seconds
        curTime = timeOffset;
        if (m_plugin->m_timeScale >= 0.25)
        {
            uint32 index = 0;
            curX = 0.0;
            while (curX <= width)
            {
                curX = m_plugin->TimeToPixel(curTime, false);
                m_plugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= m_plugin->m_timeScale;

                curTime += 1.0;
                if (index % 5 == 0)
                {
                    index++;
                    continue;
                }
                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(m_penMainTimeStepLinesActive);
                    if (m_plugin->m_timeScale < 0.9)
                    {
                        painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 5.0f));
                    }
                    else
                    {
                        painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));
                    }

                    if (m_plugin->m_timeScale >= 0.48)
                    {
                        m_timeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

                        float alpha = aznumeric_cast<float>((m_plugin->m_timeScale - 0.48f) / 1.0f);
                        alpha *= 2;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        painter.setPen(QColor(200, 200, 200, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), m_timeString.c_str(), options);
                    }
                }
            }
        }

        // 500 ms
        curTime = timeOffset;
        if (m_plugin->m_timeScale >= 0.1)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = m_plugin->TimeToPixel(curTime, false);
                m_plugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= m_plugin->m_timeScale;

                curTime += 0.5;
                if (index % 2 == 0)
                {
                    index++;
                    continue;
                }
                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(m_penMainTimeStepLinesActive);
                    if (m_plugin->m_timeScale < 1.5)
                    {
                        if (m_plugin->m_timeScale < 1.0)
                        {
                            painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 1.0f));
                        }
                        else
                        {
                            painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 8.0f));
                        }
                    }
                    else
                    {
                        painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));
                    }

                    if (m_plugin->m_timeScale >= 2.0f)
                    {
                        m_timeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

                        float alpha = aznumeric_cast<float>((m_plugin->m_timeScale - 2.0f) / 2.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        painter.setPen(QColor(175, 175, 175, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), m_timeString.c_str(), options);
                    }
                }
            }
        }

        // 100 ms
        curTime = timeOffset;
        if (m_plugin->m_timeScale >= 0.95f)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                if (index == 11)
                {
                    index = 1;
                }

                curX = m_plugin->TimeToPixel(curTime, false);
                m_plugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= m_plugin->m_timeScale;

                curTime += 0.1;
                if (index == 0 || index == 5 || index == 10)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(m_penMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 3.0f));

                    if (m_plugin->m_timeScale >= 11.0f)
                    {
                        float alpha = aznumeric_cast<float>((m_plugin->m_timeScale - 11.0f) / 4.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        m_timeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        painter.setPen(QColor(110, 110, 110, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), m_timeString.c_str(), options);
                    }
                }
            }
        }

        timeOffset = m_plugin->PixelToTime(0.0) * 1000.0;
        timeOffset = (timeOffset - ((int32)timeOffset % 1000)) / 1000.0;

        // 50 ms
        curTime = timeOffset;
        if (m_plugin->m_timeScale >= 1.9)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = m_plugin->TimeToPixel(curTime, false);
                m_plugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= m_plugin->m_timeScale;

                curTime += 0.05;
                if (index % 2 == 0)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(m_penMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 1.0f));

                    if (m_plugin->m_timeScale >= 25.0f)
                    {
                        float alpha = aznumeric_cast<float>((m_plugin->m_timeScale - 25.0f) / 6.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        m_timeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        painter.setPen(QColor(80, 80, 80, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), m_timeString.c_str(), options);
                    }
                }
            }
        }


        // 10 ms
        curTime = timeOffset;
        if (m_plugin->m_timeScale >= 7.9)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = m_plugin->TimeToPixel(curTime, false);
                m_plugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                curX *= m_plugin->m_timeScale;

                curTime += 0.01;
                if (index % 5 == 0)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    //MCore::LogInfo("%f", curX);
                    painter.setPen(m_penMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 1.0f));

                    if (m_plugin->m_timeScale >= 65.0)
                    {
                        float alpha = aznumeric_cast<float>((m_plugin->m_timeScale - 65.0f) / 5.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        m_timeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        painter.setPen(QColor(60, 60, 60, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), m_timeString.c_str(), options);
                    }
                }
            }
        }
    }

    // context event when recorder has a recording
    void TrackDataHeaderWidget::DoRecorderContextMenuEvent(QContextMenuEvent* event)
    {
        // create the context menu
        QMenu menu(this);

        //---------------------
        // Timeline actions
        //---------------------
        QAction* action = menu.addAction("Zoom To Fit All");
        connect(action, &QAction::triggered, m_plugin, &TimeViewPlugin::OnZoomAll);

        action = menu.addAction("Reset Timeline");
        connect(action, &QAction::triggered, m_plugin, &TimeViewPlugin::OnResetTimeline);

        // show the menu at the given position
        menu.exec(event->globalPos());
    }

}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/moc_TrackDataHeaderWidget.cpp>
