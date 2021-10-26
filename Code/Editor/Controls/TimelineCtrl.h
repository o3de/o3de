/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_TIMELINECTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_TIMELINECTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Range.h"
#include "SplineCtrlEx.h"
#include "Controls/WndGridHelper.h"
#include "Util/fastlib.h"
#endif

// Custom styles for this control.
#define TL_STYLE_AUTO_DELETE    0x0001
#define TL_STYLE_NO_TICKS       0x0002
#define TL_STYLE_NO_TIME_MARKER 0x0004
#define TL_STYLE_NO_TEXT        0x0008

// Notify event sent when current time is change on the timeline control.
#define TLN_START_CHANGE    (0x0001)
#define TLN_END_CHANGE      (0x0002)
#define TLN_CHANGE              (0x0003)
#define TLN_DELETE              (0x0004)

class AbstractTimelineWidget
{
public:
    virtual void setZoom(float zoom, float origin) = 0;
    virtual void update(const QRect& r = QRect()) = 0;
    virtual void setGeometry(const QRect& r) = 0;
    virtual void SetTimeMarker(float marker) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Timeline control.
//////////////////////////////////////////////////////////////////////////
class TimelineWidget
    : public QWidget
    , public AbstractTimelineWidget
{
    Q_OBJECT
public:
    TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget();

    void setZoom(float zoom, float origin) override { SetZoom(zoom); SetOrigin(origin); update(); }
    void update(const QRect& r = QRect()) override { QWidget::update(r); }
    void setGeometry(const QRect& r) override { QWidget::setGeometry(r); }

    void SetTimeRange(const Range& r) { m_timeRange = r; }
    void SetTimeMarker(float fTime) override;
    float GetTimeMarker() const { return m_fTimeMarker; }

    void SetZoom(float fZoom);
    void SetOrigin(float fOffset);

    void SetKeyTimeSet(IKeyTimeSet* pKeyTimeSet);

    void SetTicksTextScale(float fScale) { m_fTicksTextScale = fScale; }
    float GetTicksTextScale() const { return m_fTicksTextScale; }

    void SetTrackingSnapToFrames(bool bEnable) { m_bTrackingSnapToFrames = bEnable; }

    enum MarkerStyle
    {
        MARKER_STYLE_SECONDS,
        MARKER_STYLE_FRAMES
    };
    void SetMarkerStyle(MarkerStyle markerStyle);
    void SetFPS(float fps); // Only referred to if MarkerStyle == MARKER_STYLE_FRAMES.
    float GetFPS() const
    {
        return m_fps;
    }

    void SetPlayCallback(const std::function<void()>& callback);

Q_SIGNALS:
    void deleteRequested();
    void clicked();

    void startChange();
    void change();
    void endChange();

protected:
    enum TrackingMode
    {
        TRACKING_MODE_NONE,
        TRACKING_MODE_SET_TIME,
        TRACKING_MODE_MOVE_KEYS,
        TRACKING_MODE_SELECTION_RANGE
    };

    int HitKeyTimes(const QPoint& point);
    void MoveSelectedKeyTimes(float scale, float offset);
    void SelectKeysInRange(float start, float end, bool select);

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnRButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void keyPressEvent(QKeyEvent* event) override;

    // Drawing functions
    float  ClientToTime(int x);
    int    TimeToClient(float fTime);

    void DrawTicks(QPainter* painter);

    Range GetVisibleRange() const;

    void StartTracking(TrackingMode trackingMode);
    void StopTracking();

    QString TimeToString(float time);
    // Convert time in seconds into the milliseconds.
    int ToMillis(float time) { return RoundFloatToInt(time * 1000.0f); };
    float MillisToTime(int nMillis) { return nMillis / 1000.0f; }
    float SnapTime(float time);

    void DrawSecondTicks(QPainter* dc);
    void DrawFrameTicks(QPainter* dc);

private:
    QRect m_rcClient;
    QRect m_rcTimeline;
    float m_fTimeMarker;
    float m_fTicksTextScale;
    TrackingMode m_trackingMode;
    QPoint m_lastPoint;

    Range m_timeRange;

    float m_timeScale;

    int m_scrollOffset;
    int m_leftOffset;

    // Tick every Nth millisecond.
    int m_nTicksStep;

    double m_ticksStep;

    CWndGridHelper m_grid;

    bool m_bIgnoreSetTime;

    IKeyTimeSet* m_pKeyTimeSet;
    bool m_bChangedKeyTimeSet;

    MarkerStyle m_markerStyle;
    float m_fps;
    bool m_copyKeyTimes;
    bool m_bTrackingSnapToFrames;
    std::function<void()> m_playCallback;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_TIMELINECTRL_H
