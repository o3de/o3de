/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
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
        QBrush              m_brushBackgroundOutOfRange;
        TimeViewPlugin*     m_plugin;
        bool                m_mouseLeftClicked;
        bool                m_mouseMidClicked;
        bool                m_mouseRightClicked;
        bool                m_isScrolling;
        int32               m_lastMouseX;
        int32               m_lastMouseY;
        bool                m_allowContextMenu;

        QPixmap             m_timeHandleTop;

        QFont               m_timeLineFont;
        QFont               m_dataFont;
        AZStd::string       m_timeString;
        QLinearGradient     m_headerGradientActive;
        QLinearGradient     m_headerGradientInactive;
        QLinearGradient     m_headerGradientActiveFocus;
        QLinearGradient     m_headerGradientInactiveFocus;
        QPen                m_penMainTimeStepLinesActive;

        void UpdateMouseOverCursor();
        void DrawTimeLine(QPainter& painter, const QRect& rect);
        void DrawTimeMarker(QPainter& painter, const QRect& rect);

        void DoRecorderContextMenuEvent(QContextMenuEvent* event);
    };
}   // namespace EMStudio
