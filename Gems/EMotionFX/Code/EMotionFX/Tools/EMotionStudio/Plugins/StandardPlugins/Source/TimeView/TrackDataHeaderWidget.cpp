/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include "../MotionWindow/MotionWindowPlugin.h"
#include "../MotionEvents/MotionEventsPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"


namespace EMStudio
{
    // the constructor
    TrackDataHeaderWidget::TrackDataHeaderWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QOpenGLWidget(parent)
        , QOpenGLFunctions()
        , mPlugin(plugin)
        , mLastMouseX(0)
        , mLastMouseY(0)
        , mMouseLeftClicked(false)
        , mMouseRightClicked(false)
        , mMouseMidClicked(false)
        , mIsScrolling(false)
        , mAllowContextMenu(true)
    {
        setObjectName("TrackDataHeaderWidget");

        // init brushes and pens
        mBrushBackgroundOutOfRange  = QBrush(QColor(35, 35, 35), Qt::SolidPattern);

        mHeaderGradientActive       = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientActive.setColorAt(1.0f, QColor(100, 105, 110));
        mHeaderGradientActive.setColorAt(0.5f, QColor(30, 35, 40));
        mHeaderGradientActive.setColorAt(0.0f, QColor(20, 20, 20));

        mHeaderGradientActiveFocus  = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientActiveFocus.setColorAt(1.0f, QColor(100, 105, 130));
        mHeaderGradientActiveFocus.setColorAt(0.5f, QColor(30, 35, 40));
        mHeaderGradientActiveFocus.setColorAt(0.0f, QColor(20, 20, 20));

        mHeaderGradientInactive     = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientInactive.setColorAt(1.0f, QColor(30, 30, 30));
        mHeaderGradientInactive.setColorAt(0.0f, QColor(20, 20, 20));

        mHeaderGradientInactiveFocus = QLinearGradient(0, 0, 0, 35);
        mHeaderGradientInactiveFocus.setColorAt(1.0f, QColor(30, 30, 30));
        mHeaderGradientInactiveFocus.setColorAt(0.0f, QColor(20, 20, 20));

        mPenMainTimeStepLinesActive     = QPen(QColor(110, 110, 110));

        mTimeLineFont.setPixelSize(12);
        mDataFont.setPixelSize(13);

        // load the time handle top image
        QDir imageName{ QString(MysticQt::GetMysticQt()->GetDataDir().c_str()) };
        mTimeHandleTop = QPixmap(imageName.filePath("Images/Icons/TimeHandleTop.png"));

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
        if (mPlugin)
        {
            mPlugin->SetRedrawFlag();
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
        painter.setBrush(mBrushBackgroundOutOfRange);
        painter.drawRect(rect);
        painter.setFont(mDataFont);

        // draw the timeline
        painter.setRenderHint(QPainter::Antialiasing, false);
        DrawTimeLine(painter, rect);

        const uint32 height = geometry().height();
        mPlugin->RenderElementTimeHandles(painter, height, mPlugin->mPenTimeHandles);

        DrawTimeMarker(painter, rect);
    }

    // draw the time marker
    void TrackDataHeaderWidget::DrawTimeMarker(QPainter& painter, const QRect& rect)
    {
        // draw the current time marker
        float startHeight = 0.0f;
        const float curTimeX = aznumeric_cast<float>(mPlugin->TimeToPixel(mPlugin->mCurTime));
        painter.drawPixmap(aznumeric_cast<int>(curTimeX - (mTimeHandleTop.width() / 2) - 1), 0, mTimeHandleTop);

        painter.setPen(mPlugin->mPenCurTimeHandle);
        painter.drawLine(QPointF(curTimeX, startHeight), QPointF(curTimeX, rect.bottom()));
    }
        
    void TrackDataHeaderWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        // if double clicked in the timeline
        mPlugin->MakeTimeVisible(mPlugin->PixelToTime(event->x()), 0.5);
    }

    // when the mouse is moving, while a button is pressed
    void TrackDataHeaderWidget::mouseMoveEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        const int32 deltaRelX = event->x() - mLastMouseX;
        mLastMouseX = event->x();
        mPlugin->mCurMouseX = event->x();
        mPlugin->mCurMouseY = event->y();

        const int32 deltaRelY = event->y() - mLastMouseY;
        mLastMouseY = event->y();

        const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = mMouseLeftClicked == false && mMouseRightClicked && altPressed;
        const bool isPanning = mMouseLeftClicked == false && isZooming == false && (mMouseMidClicked || mMouseRightClicked);

        if (deltaRelY != 0)
        {
            mAllowContextMenu = false;
        }

        if (mMouseRightClicked)
        {
            mIsScrolling = true;
        }

        // if the mouse left button is pressed
        if (mMouseLeftClicked)
        {
            // update the current time marker
            int newX = event->x();
            newX = AZ::GetClamp(newX, 0, geometry().width() - 1);
            mPlugin->mCurTime = mPlugin->PixelToTime(newX);

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if (recorder.GetRecordTime() > AZ::Constants::FloatEpsilon)
            {
                if (recorder.GetIsInPlayMode())
                {
                    recorder.SetCurrentPlayTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    recorder.SetAutoPlay(false);
                    emit mPlugin->ManualTimeChange(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                }
            }
            else
            {
                const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                if (motionInstances.size() == 1)
                {
                    EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                    motionInstance->SetCurrentTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()), false);
                    motionInstance->SetPause(true);
                    emit mPlugin->ManualTimeChange(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                }
            }

            mIsScrolling = true;
        }
        else if (isPanning)
        {
            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                mPlugin->DeltaScrollX(-deltaRelX, false);
            }
        }
        else if (isZooming)
        {
            if (deltaRelY < 0)
            {
                setCursor(*(mPlugin->GetZoomOutCursor()));
            }
            else
            {
                setCursor(*(mPlugin->GetZoomInCursor()));
            }

            DoMouseYMoveZoom(deltaRelY, mPlugin);
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
        mPlugin->DisableAllToolTips();
    }


    // when the mouse is pressed
    void TrackDataHeaderWidget::mousePressEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
        const bool altPressed = event->modifiers() & Qt::AltModifier;

        // store the last clicked position
        mAllowContextMenu = true;

        if (event->button() == Qt::RightButton)
        {
            mMouseRightClicked = true;
        }

        if (event->button() == Qt::MidButton)
        {
            mMouseMidClicked = true;
        }

        if (event->button() == Qt::LeftButton)
        {
            mMouseLeftClicked   = true;

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if (!mPlugin->mNodeHistoryItem && !altPressed)
            {
                // update the current time marker
                int newX = event->x();
                newX = AZ::GetClamp(newX, 0, geometry().width() - 1);
                mPlugin->mCurTime = mPlugin->PixelToTime(newX);

                if (recorder.GetRecordTime() > AZ::Constants::FloatEpsilon)
                {
                    if (!recorder.GetIsInPlayMode())
                    {
                        recorder.StartPlayBack();
                    }

                    recorder.SetCurrentPlayTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    recorder.SetAutoPlay(false);
                    emit mPlugin->ManualTimeChangeStart(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    emit mPlugin->ManualTimeChange(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                }
                else
                {
                    const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
                    if (motionInstances.size() == 1)
                    {
                        EMotionFX::MotionInstance* motionInstance = motionInstances[0];
                        motionInstance->SetCurrentTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()), false);
                        motionInstance->SetPause(true);
                        mPlugin->GetTimeViewToolBar()->UpdateInterface();
                        emit mPlugin->ManualTimeChangeStart(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                        emit mPlugin->ManualTimeChange(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    }
                }
            }
        }

        //const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = mMouseLeftClicked == false && mMouseRightClicked && altPressed;
        const bool isPanning = mMouseLeftClicked == false && isZooming == false && (mMouseMidClicked || mMouseRightClicked);

        if (isPanning)
        {
            setCursor(Qt::SizeHorCursor);
        }

        if (isZooming)
        {
            setCursor(*(mPlugin->GetZoomInCursor()));
        }
    }


    // when releasing the mouse button
    void TrackDataHeaderWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        setCursor(Qt::ArrowCursor);

        // disable overwrite mode in any case when the mouse gets released so that we display the current time from the plugin again
        if (mPlugin->GetTimeInfoWidget())
        {
            mPlugin->GetTimeInfoWidget()->SetIsOverwriteMode(false);
        }

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;

        if (event->button() == Qt::RightButton)
        {
            mMouseRightClicked = false;
            mIsScrolling = false;
        }

        if (event->button() == Qt::MidButton)
        {
            mMouseMidClicked = false;
        }

        if (event->button() == Qt::LeftButton)
        {
            mMouseLeftClicked   = false;
            mIsScrolling        = false;

            return;
        }

        UpdateMouseOverCursor();
    }

    // drag & drop support
    void TrackDataHeaderWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        mPlugin->SetRedrawFlag();

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
        DoWheelEvent(event, mPlugin);
    }

    void TrackDataHeaderWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        mPlugin->SetRedrawFlag();
        QPoint mousePos = event->pos();

        double dropTime = mPlugin->PixelToTime(mousePos.x());
        mPlugin->SetCurrentTime(dropTime);

        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        if (motionInstances.size() == 1)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[0];
            motionInstance->SetCurrentTime(aznumeric_cast<float>(dropTime), false);
            motionInstance->Pause();
        }

        //event->accept();
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataHeaderWidget::keyPressEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataHeaderWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }
    
    // draw the time line
    void TrackDataHeaderWidget::DrawTimeLine(QPainter& painter, const QRect& rect)
    {
        // get the time values in seconds
        double animationLength  = 0.0;
        double clipStart        = 0.0;
        double clipEnd          = 0.0;
        mPlugin->GetDataTimes(&animationLength, &clipStart, &clipEnd);

        // calculate the pixel offsets
        double animEndPixel     = mPlugin->TimeToPixel(animationLength);
        double clipStartPixel   = mPlugin->TimeToPixel(clipStart);
        //double clipEndPixel     = mPlugin->TimeToPixel(clipEnd);

        // fill with the background color
        QRect motionRect        = rect;
        QRect outOfRangeRect    = rect;

        motionRect.setLeft(aznumeric_cast<int>(clipStartPixel));
        motionRect.setRight(aznumeric_cast<int>(animEndPixel));
        outOfRangeRect.setLeft(aznumeric_cast<int>(animEndPixel));

        painter.setPen(Qt::NoPen);
        if (hasFocus() == false)
        {
            painter.setBrush(mHeaderGradientActive);
            painter.drawRect(motionRect);
            painter.setBrush(mHeaderGradientInactive);
            painter.drawRect(outOfRangeRect);
        }
        else
        {
            painter.setBrush(mHeaderGradientActiveFocus);
            painter.drawRect(motionRect);
            painter.setBrush(mHeaderGradientInactiveFocus);
            painter.drawRect(outOfRangeRect);
        }

        // show the recorder range
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            QRectF recorderRect = rect;
            recorderRect.setRight(mPlugin->TimeToPixel(recorder.GetRecordTime()));
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
                rangeRect.setRight(mPlugin->TimeToPixel(animationLength));
                rangeRect.setTop(height() - 3);
                rangeRect.setBottom(height());

                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 175, 0));
                painter.drawRect(rangeRect);
            }
        }

        QTextOption options;
        options.setAlignment(Qt::AlignCenter);
        painter.setFont(mTimeLineFont);

        const uint32 width = rect.width();
        //const uint32 height = rect.height();

        float yOffset = 19.0f;

        //const double pixelsPerSecond = mPlugin->mPixelsPerSecond;

        double timeOffset = mPlugin->PixelToTime(0.0) * 1000.0;
        timeOffset = (timeOffset - ((int32)timeOffset % 5000)) / 1000.0;

        //if (rand() % 10 == 0)
        //MCore::LogInfo("%f", mPlugin->mTimeScale);

        uint32 minutes, seconds, milSecs, frameNumber;
        double pixelTime;

        double curX = 0.0;
        double curTime = timeOffset;

        //uint32 index = 0;
        while (curX <= width)
        {
            curX = mPlugin->TimeToPixel(curTime, false);
            mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
            seconds += minutes * 60;
            curX *= mPlugin->mTimeScale;

            curTime += 5.0;

            painter.setPen(mPenMainTimeStepLinesActive);
            painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));

            mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

            //painter.setPen( mPenText );
            painter.setPen(QColor(175, 175, 175));
            painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), mTimeString.c_str(), options);
        }

        // draw the seconds
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 0.25)
        {
            uint32 index = 0;
            curX = 0.0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 1.0;
                if (index % 5 == 0)
                {
                    index++;
                    continue;
                }
                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    if (mPlugin->mTimeScale < 0.9)
                    {
                        painter.drawLine(QPointF(curX, yOffset - 1.0f), QPointF(curX, yOffset + 5.0f));
                    }
                    else
                    {
                        painter.drawLine(QPointF(curX, yOffset - 3.0f), QPointF(curX, yOffset + 10.0f));
                    }

                    if (mPlugin->mTimeScale >= 0.48)
                    {
                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

                        float alpha = aznumeric_cast<float>((mPlugin->mTimeScale - 0.48f) / 1.0f);
                        alpha *= 2;
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        painter.setPen(QColor(200, 200, 200, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }

        // 500 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 0.1)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 0.5;
                if (index % 2 == 0)
                {
                    index++;
                    continue;
                }
                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    if (mPlugin->mTimeScale < 1.5)
                    {
                        if (mPlugin->mTimeScale < 1.0)
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

                    if (mPlugin->mTimeScale >= 2.0f)
                    {
                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory

                        float alpha = aznumeric_cast<float>((mPlugin->mTimeScale - 2.0f) / 2.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        //painter.setPen( mPenText );
                        painter.setPen(QColor(175, 175, 175, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }

        // 100 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 0.95f)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                if (index == 11)
                {
                    index = 1;
                }

                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 0.1;
                if (index == 0 || index == 5 || index == 10)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 3.0f));

                    if (mPlugin->mTimeScale >= 11.0f)
                    {
                        float alpha = aznumeric_cast<float>((mPlugin->mTimeScale - 11.0f) / 4.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        painter.setPen(QColor(110, 110, 110, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }

        timeOffset = mPlugin->PixelToTime(0.0) * 1000.0;
        timeOffset = (timeOffset - ((int32)timeOffset % 1000)) / 1000.0;

        // 50 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 1.9)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                seconds += minutes * 60;
                curX *= mPlugin->mTimeScale;

                curTime += 0.05;
                if (index % 2 == 0)
                {
                    index++;
                    continue;
                }

                index++;

                if (curX > -100 && curX < width + 100)
                {
                    painter.setPen(mPenMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 1.0f));

                    if (mPlugin->mTimeScale >= 25.0f)
                    {
                        float alpha = aznumeric_cast<float>((mPlugin->mTimeScale - 25.0f) / 6.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        //painter.setPen( mPenText );
                        painter.setPen(QColor(80, 80, 80, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), mTimeString.c_str(), options);
                    }
                }
            }
        }


        // 10 ms
        curTime = timeOffset;
        if (mPlugin->mTimeScale >= 7.9)
        {
            uint32 index = 0;
            curX = 0;
            while (curX <= width)
            {
                curX = mPlugin->TimeToPixel(curTime, false);
                mPlugin->CalcTime(curX, &pixelTime, &minutes, &seconds, &milSecs, &frameNumber, false);
                curX *= mPlugin->mTimeScale;

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
                    painter.setPen(mPenMainTimeStepLinesActive);
                    painter.drawLine(QPointF(curX, yOffset), QPointF(curX, yOffset + 1.0f));

                    if (mPlugin->mTimeScale >= 65.0)
                    {
                        float alpha = aznumeric_cast<float>((mPlugin->mTimeScale - 65.0f) / 5.0f);
                        if (alpha > 1.0f)
                        {
                            alpha = 1.0;
                        }

                        mTimeString = AZStd::string::format("%.2d:%.2d", seconds, milSecs); // will only do an allocation once, reuses the memory
                        //painter.setPen( mPenText );
                        painter.setPen(QColor(60, 60, 60, aznumeric_cast<int>(alpha * 255)));
                        painter.drawText(QRect(aznumeric_cast<int>(curX - 25), aznumeric_cast<int>(yOffset - 23), 52, 20), mTimeString.c_str(), options);
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
        connect(action, &QAction::triggered, mPlugin, &TimeViewPlugin::OnZoomAll);

        action = menu.addAction("Reset Timeline");
        connect(action, &QAction::triggered, mPlugin, &TimeViewPlugin::OnResetTimeline);

        // show the menu at the given position
        menu.exec(event->globalPos());
    }

}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/moc_TrackDataHeaderWidget.cpp>
