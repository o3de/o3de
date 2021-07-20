/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_SPLINECTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_SPLINECTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <ISplines.h>
#endif

// Custom styles for this control.
#define SPLINE_STYLE_NOGRID 0x0001
#define SPLINE_STYLE_NO_TIME_MARKER 0x0002

// Notify event sent when spline is being modified.
#define SPLN_CHANGE (0x0001)
// Notify event sent just before when spline is modified.
#define SPLN_BEFORE_CHANGE (0x0002)

class TimelineWidget;

//////////////////////////////////////////////////////////////////////////
// Spline control.
//////////////////////////////////////////////////////////////////////////
class CSplineCtrl
    : public QWidget
{
    Q_OBJECT
public:
    CSplineCtrl(QWidget* parent = nullptr);
    virtual ~CSplineCtrl();

    //Key functions
    int  GetActiveKey() { return m_nActiveKey; };
    void SetActiveKey(int nIndex);
    int InsertKey(const QPoint& point);
    void ToggleKeySlope(int nIndex, int nDist);

    void SetGrid(int numX, int numY) { m_gridX = numX; m_gridY = numY; };
    void SetTimeRange(float tmin, float tmax) { m_fMinTime = tmin; m_fMaxTime = tmax; }
    void SetValueRange(float tmin, float tmax)
    {
        m_fMinValue = tmin;
        m_fMaxValue = tmax;
        if (m_fMinValue == m_fMaxValue)
        {
            m_fMaxValue = m_fMinValue + 0.001f;
        }
    }
    void SetTooltipValueScale(float x, float y) { m_fTooltipScaleX = x; m_fTooltipScaleY = y; };
    // Lock value of first and last key to be the same.
    void LockFirstAndLastKeys(bool bLock) { m_bLockFirstLastKey = bLock; }

    void SetSpline(ISplineInterpolator* pSpline, BOOL bRedraw = FALSE);
    ISplineInterpolator* GetSpline();

    void SetTimeMarker(float fTime);
    void SetTimelineCtrl(TimelineWidget* pTimelineCtrl);
    void UpdateToolTip();

    typedef AZStd::function<void(CSplineCtrl*)> UpdateCallback;
    void SetUpdateCallback(UpdateCallback cb) { m_updateCallback = cb; };

Q_SIGNALS:
    void beforeChange();
    void change();

protected:
    enum EHitCode
    {
        HIT_NOTHING,
        HIT_KEY,
        HIT_SPLINE,
    };

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void mouseMoveEvent(QMouseEvent* event) override;
    void OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnSetCursor();
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // Drawing functions
    void DrawGrid(QPainter* pDC);
    void DrawSpline(QPainter* pDC);
    void DrawKeys(QPainter* pDC);
    void DrawTimeMarker(QPainter* pDC);

    EHitCode HitTest(const QPoint& point);

    //Tracking support helper functions
    void StartTracking();
    void TrackKey(const QPoint& point);
    void StopTracking();
    void RemoveKey(int nKey);

    QPoint KeyToPoint(int nKey);
    QPoint TimeToPoint(float time);
    void PointToTimeValue(const QPoint& point, float& time, float& value);
    float  XOfsToTime(int x);
    QPoint XOfsToPoint(int x);

    void ClearSelection();
    void ValidateSpline();

private:
    ISplineInterpolator* m_pSpline;

    QRect m_rcClipRect;
    QRect m_rcSpline;

    QPoint m_hitPoint;
    EHitCode m_hitCode;
    int m_nHitKeyIndex;
    int m_nHitKeyDist;

    float m_fTimeMarker;

    int m_nActiveKey;
    int m_nKeyDrawRadius;

    bool  m_bTracking;

    int m_gridX;
    int m_gridY;

    float m_fMinTime, m_fMaxTime;
    float m_fMinValue, m_fMaxValue;
    float m_fTooltipScaleX, m_fTooltipScaleY;

    bool m_bLockFirstLastKey;

    std::vector<int> m_bSelectedKeys;

    TimelineWidget* m_pTimelineCtrl;

    QRect m_TimeUpdateRect;

    UpdateCallback m_updateCallback;
};


#endif // CRYINCLUDE_EDITOR_CONTROLS_SPLINECTRL_H
