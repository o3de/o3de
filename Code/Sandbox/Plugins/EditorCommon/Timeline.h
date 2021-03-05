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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_TIMELINE_H
#define CRYINCLUDE_EDITORCOMMON_TIMELINE_H

#include <vector>
#include <QWidget>

#include "TimelineContent.h"

class QPainter;
class QPaintEvent;
class QLineEdit;
class QScrollBar;

struct STimelineLayout;

struct STimelineViewState
{
    float viewOrigin;
    float visibleDistance;
    float clampedViewOrigin;
    int widthPixels;
    QPoint scrollPixels;
    int maxScrollX;
    int treeWidth;
    int treeLastOpenedWidth;

    STimelineViewState()
        : viewOrigin(0.0f)
        , clampedViewOrigin(0.0f)
        , visibleDistance(1.0f)
        , scrollPixels(0, 0)
        , maxScrollX(0)
        , treeWidth(0)
        , widthPixels(1)
    {
    }

    QPoint LocalToLayout(const QPoint& p) const;
    QPoint LayoutToLocal(const QPoint& p) const;

    int ScrollOffset(float origin) const;
    int TimeToLayout(float time) const;
    float LocalToTime(int x) const;
    int TimeToLocal(float time) const;
    float LayoutToTime(int x) const;
};

struct STrackLayout;

class EDITOR_COMMON_API CTimeline
    : public QWidget
{
    Q_OBJECT
public:
    CTimeline(QWidget* parent);
    ~CTimeline();

    void SetContent(STimelineContent* pContent);
    STimelineContent* Content() const { return m_pContent; }

    void ContentUpdated() { UpdateLayout(); update(); }

    bool IsDragged() const { return m_mouseHandler.get() != 0; }

    // make it possible to have actual time in normalized units, but different display units
    void SetTimeUnitScale(float timeUnitScale, float step);
    void SetTime(SAnimTime time);
    void SetCycled(bool cycled);
    void SetSizeToContent(bool sizeToContent);
    void SetFrameRate(SAnimTime::EFrameRate frameRate) { m_frameRate = frameRate; }
    void SetTimeSnapping(bool snapTime) { m_snapTime = snapTime; }
    void SetKeySnapping(bool snapKeys) { m_snapKeys = snapKeys; }
    void SetKeyWidth(uint width) { m_keyWidth = width; UpdateLayout(); update(); }
    void SetKeyRadius(float radius) { m_keyRadius = radius; UpdateLayout(); update(); }
    void SetTreeVisible(bool visible);
    void SetDrawSelectionIndicators(bool visible) { m_selIndicators = visible; update(); }
    void SetCustomTreeCornerWidget(QWidget* pWidget, uint width);
    void SetVerticalScrollbarVisible(bool bVisible);
    void SetDrawTrackTimeMarkers(bool bDrawMarkers);
    void SetVisibleDistance(float distance);

    SAnimTime Time() const { return m_time; }

    bool HandleKeyEvent(int key);
    bool ProcessesKey(const QKeySequence& key);

    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void focusOutEvent(QFocusEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;

    void AddKeyToTrack(STimelineTrack& subTrack, SAnimTime time);

    void keyPressEvent(QKeyEvent* ev) override;
    void keyReleaseEvent(QKeyEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;
    QSize sizeHint() const override;

signals:
    void SignalScrub(bool scrubThrough);
    void SignalContentChanged(bool continuous);
    void SignalSelectionChanged(bool continuous);
    void SignalTrackSelectionChanged();
    void SignalPlay();
    void SignalNumberHotkey(int number);
    void SignalTreeContextMenu(const QPoint& point);

    void SignalUndo();
    void SignalRedo();

protected slots:
    void OnMenuSelectionToCursor();
    void OnMenuDuplicate();
    void OnMenuCopy();
    void OnMenuPaste();
    void OnMenuDelete();
    void OnMenuPlay();
    void OnMenuNextKey();
    void OnMenuPreviousKey();
    void OnMenuNextFrame();
    void OnMenuPreviousFrame();
    void OnFilterChanged();
    void OnVerticalScroll(int value);

protected:

    bool event(QEvent* e) override;

private:
    struct SMouseHandler;
    struct SSelectionHandler;
    struct SMoveHandler;
    struct SPanHandler;
    struct SScrubHandler;
    struct SSplitterHandler;
    struct STreeMouseHandler;

    void ContentChanged(bool continuous);
    void UpdateLayout();
    void UpdateCursor(QMouseEvent* ev);
    void DrawMarkers(QPainter& painter, int offsetY);
    SAnimTime ClampAndSnapTime(SAnimTime time, bool snapToFrames) const;
    void ClampAndSetTime(SAnimTime time, bool scrubThrough);
    STrackLayout* GetTrackLayoutFromPos(const QPoint& pos) const;

    // Exposed parameters
    float m_timeUnitScale;
    int m_timeStepNum;
    int m_timeStepIndex;
    SAnimTime::EFrameRate m_frameRate;
    bool m_cycled : 1;
    bool m_sizeToContent : 1;
    bool m_snapTime : 1;
    bool m_snapKeys : 1;
    bool m_treeVisible : 1;
    bool m_selIndicators : 1;
    bool m_verticalScrollbarVisible : 1;
    bool m_drawMarkers : 1;
    uint m_keyWidth;
    float m_keyRadius;
    uint m_cornerWidgetWidth;

    // Widgets
    QScrollBar* m_scrollBar;
    QWidget* m_cornerWidget;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    STimelineViewState m_viewState;
    STimelineContent* m_pContent;
    SAnimTime m_time;
    std::unique_ptr<STimelineLayout> m_layout;
    std::unique_ptr<SMouseHandler> m_mouseHandler;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    // Filtering
    QLineEdit* m_pFilterLineEdit;

    // Track selection
    STrackLayout* m_pLastSelectedTrack;

    friend class CTimelineTracks;
};

class CTimelineTracks
    : public QWidget
{
    Q_OBJECT
public:
    CTimelineTracks(QWidget* widget)
        : QWidget(widget) {}
    void ConnectToTimeline(CTimeline* timeline) { m_timeline = timeline; }

private:
    CTimeline* m_timeline;
};



#endif // CRYINCLUDE_EDITORCOMMON_TIMELINE_H
