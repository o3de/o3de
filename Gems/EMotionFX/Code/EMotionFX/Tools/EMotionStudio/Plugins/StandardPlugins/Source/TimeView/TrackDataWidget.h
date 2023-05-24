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

QT_FORWARD_DECLARE_CLASS(QBrush)


namespace EMStudio
{
    class TimeViewPlugin;

    class TrackDataWidget
        : public QOpenGLWidget
        , public QOpenGLFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(TrackDataWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);
        Q_OBJECT

        friend class TimeViewPlugin;
        friend class TrackHeaderWidget;
        friend class TimeViewToolBar;

    public:
        TrackDataWidget(TimeViewPlugin* plugin, QWidget* parent = nullptr);
        ~TrackDataWidget();

        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

        void RemoveTrack(size_t trackIndex);
        
    protected:
        //void paintEvent(QPaintEvent* event);
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    signals:
        void MotionEventPresetsDropped(QPoint position);
        void MotionEventChanged(TimeTrackElement* element, double startTime, double endTime);
        void TrackAdded(TimeTrack* track);
        void SelectionChanged();
        void ElementTrackChanged(size_t eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName);

    private slots:
        void OnRemoveElement()                          { RemoveMotionEvent(m_contextMenuX, m_contextMenuY); }
        void OnAddElement()                             { AddMotionEvent(m_contextMenuX, m_contextMenuY); }
        void OnAddTrack();
        void OnCreatePresetEvent();
        void RemoveSelectedMotionEventsInTrack();
        void RemoveAllMotionEventsInTrack();
        void OnRemoveEventTrack();

        void OnCutTrack();
        void OnCopyTrack();
        void OnCutElement();
        void OnCopyElement();
        void OnPaste();
        void OnPasteAtLocation();
        
        void OnRequiredHeightChanged(int newHeight);

    private:
        void RemoveMotionEvent(int32 x, int32 y);
        void AddMotionEvent(int32 x, int32 y);
        void SetPausedTime(float timeValue, bool emitTimeChangeStart=false);
        void ClearState();

        void DoWheelEvent(QWheelEvent* event, TimeViewPlugin* plugin);
        void DoMouseYMoveZoom(int32 deltaY, TimeViewPlugin* plugin);
        void wheelEvent(QWheelEvent* event) override;
        void DoPaste(bool useLocation);
        void PaintRecorder(QPainter& painter, const QRect& rect);
        void PaintRecorderNodeHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData);
        void PaintRecorderEventHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData);
        void PaintMotionTracks(QPainter& painter, const QRect& rect);
        void ShowElementTimeInfo(TimeTrackElement* element);
        void PaintRelativeGraph(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData);
        uint32 PaintSeparator(QPainter& painter, int32 heightOffset, float animationLength);

        QBrush              m_brushBackground;
        QBrush              m_brushBackgroundClipped;
        QBrush              m_brushBackgroundOutOfRange;
        TimeViewPlugin*     m_plugin;
        bool                m_mouseLeftClicked;
        bool                m_mouseMidClicked;
        bool                m_mouseRightClicked;
        bool                m_dragging;
        bool                m_resizing;
        bool                m_rectZooming;
        bool                m_isScrolling;
        int32               m_lastLeftClickedX;
        int32               m_lastMouseMoveX;
        int32               m_lastMouseX;
        int32               m_lastMouseY;
        uint32              m_nodeHistoryItemHeight;
        uint32              m_eventHistoryTotalHeight;
        bool                m_allowContextMenu;

        TimeTrackElement*   m_draggingElement;
        TimeTrack*          m_dragElementTrack;
        TimeTrackElement*   m_resizeElement;
        uint32              m_resizeId;
        int32               m_contextMenuX;
        int32               m_contextMenuY;
        uint32              m_graphStartHeight;
        uint32              m_eventsStartHeight;
        uint32              m_nodeRectsStartHeight;
        double              m_oldCurrentTime;

        AZStd::vector<EMotionFX::Recorder::ExtractedNodeHistoryItem> m_activeItems;
        AZStd::vector<size_t>                                        m_trackRemap;

        // copy and paste
        struct CopyElement
        {
            uint32 m_motionID;
            AZStd::string m_trackName;
            const EMotionFX::EventDataSet m_eventDatas;
            float m_startTime;
            float m_endTime;

            CopyElement(uint32 motionID, AZStd::string trackName, const EMotionFX::EventDataSet& eventDatas, float startTime, float endTime)
                : m_motionID(motionID)
                , m_trackName(trackName)
                , m_eventDatas(eventDatas)
                , m_startTime(startTime)
                , m_endTime(endTime)
            {
            }
        };

        bool GetIsReadyForPaste() const                                                             { return m_copyElements.empty() == false; }
        void FillCopyElements(bool selectedItemsOnly);

        AZStd::vector<CopyElement>      m_copyElements;
        bool                            m_cutMode;

        QFont           m_dataFont;
        AZStd::string   m_tempString;

        // rect selection
        QPoint          m_selectStart;
        QPoint          m_selectEnd;
        bool            m_rectSelecting;

        QRect           m_nodeHistoryRect;

        void CalcSelectRect(QRect& outRect);
        void SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode);

        void RenderTracks(QPainter& painter, uint32 width, uint32 height, double animationLength, double clipStartTime, double clipEndTime);
        void UpdateMouseOverCursor(int32 x, int32 y);
        void DrawTimeMarker(QPainter& painter, const QRect& rect);

        bool GetIsInsideNodeHistory(int32 y) const                                              { return m_nodeHistoryRect.contains(1, y); }
        void DoRecorderContextMenuEvent(QContextMenuEvent* event);

        void UpdateRects();
        EMotionFX::Recorder::NodeHistoryItem* FindNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y);
        EMotionFX::Recorder::EventHistoryItem* FindEventHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y) const;
        EMotionFX::Recorder::ActorInstanceData* FindActorInstanceData() const;
        void BuildToolTipString(EMotionFX::Recorder::NodeHistoryItem* item, AZStd::string& outString);
        void BuildToolTipString(EMotionFX::Recorder::EventHistoryItem* item, AZStd::string& outString);

        bool event(QEvent* event) override;
    };
}   // namespace EMStudio
