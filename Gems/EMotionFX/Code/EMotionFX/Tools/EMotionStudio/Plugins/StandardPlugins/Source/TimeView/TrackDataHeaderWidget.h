/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/Source/Recorder.h>
#include "TimeTrack.h"
#include <QOpenGLWidget>
#include <QPen>
#include <QFont>
#include <QOpenGLFunctions>
#endif

namespace EMStudio
{
    class TimeViewPlugin;

    class TrackDataHeaderWidget
        : public QOpenGLWidget
        , public QOpenGLFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(TrackDataHeaderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);
        Q_OBJECT

        friend class TimeViewPlugin;

    public:
        TrackDataHeaderWidget(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TrackDataHeaderWidget();

        static void DoWheelEvent(QWheelEvent* event, TimeViewPlugin* plugin);
        static void DoMouseYMoveZoom(int32 deltaY, TimeViewPlugin* plugin);

        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

    protected:
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

    private:
        QBrush              mBrushBackgroundOutOfRange;
        TimeViewPlugin*     mPlugin;
        bool                mMouseLeftClicked;
        bool                mMouseMidClicked;
        bool                mMouseRightClicked;
        bool                mIsScrolling;
        int32               mLastMouseX;
        int32               mLastMouseY;
        bool                mAllowContextMenu;

        QPixmap             mTimeHandleTop;

        QFont               mTimeLineFont;
        QFont               mDataFont;
        AZStd::string       mTimeString;
        QLinearGradient     mHeaderGradientActive;
        QLinearGradient     mHeaderGradientInactive;
        QLinearGradient     mHeaderGradientActiveFocus;
        QLinearGradient     mHeaderGradientInactiveFocus;
        QPen                mPenMainTimeStepLinesActive;

        void UpdateMouseOverCursor();
        void DrawTimeLine(QPainter& painter, const QRect& rect);
        void DrawTimeMarker(QPainter& painter, const QRect& rect);

        void DoRecorderContextMenuEvent(QContextMenuEvent* event);
    };
}   // namespace EMStudio
