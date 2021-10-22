/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_COLORGRADIENTCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_COLORGRADIENTCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <ISplines.h>
#include "Controls/WndGridHelper.h"
#endif

namespace AZ
{
    class Color;
}

// Notify event sent when spline is being modified.
#define CLRGRDN_CHANGE (0x0001)
// Notify event sent just before when spline is modified.
#define CLRGRDN_BEFORE_CHANGE (0x0002)
// Notify event sent when the active key changes
#define CLRGRDN_ACTIVE_KEY_CHANGE (0x0003)

//////////////////////////////////////////////////////////////////////////
// Spline control.
//////////////////////////////////////////////////////////////////////////
class CColorGradientCtrl
    : public QWidget
{
    Q_OBJECT
public:
    CColorGradientCtrl(QWidget* parent = nullptr);
    virtual ~CColorGradientCtrl();

    //Key functions
    int  GetActiveKey() { return m_nActiveKey; };
    void SetActiveKey(int nIndex);
    int InsertKey(QPoint point);

    // Turns on/off zooming and scroll support.
    void SetNoZoom([[maybe_unused]] bool bNoZoom) { m_bNoZoom = false; };

    void SetTimeRange(float tmin, float tmax) { m_fMinTime = tmin; m_fMaxTime = tmax; }
    void SetValueRange(float tmin, float tmax) { m_fMinValue = tmin; m_fMaxValue = tmax; }
    void SetTooltipValueScale(float x, float y) { m_fTooltipScaleX = x; m_fTooltipScaleY = y; };
    // Lock value of first and last key to be the same.
    void LockFirstAndLastKeys(bool bLock) { m_bLockFirstLastKey = bLock; }

    void SetSpline(ISplineInterpolator* pSpline, bool bRedraw = false);
    ISplineInterpolator* GetSpline();

    void SetTimeMarker(float fTime);

    // Zoom in pixels per time unit.
    void SetZoom(float fZoom);
    void SetOrigin(float fOffset);

    typedef AZStd::function<void(CColorGradientCtrl*)> UpdateCallback;
    void SetUpdateCallback(const UpdateCallback& cb) { m_updateCallback = cb; };

    void SetNoTimeMarker(bool noTimeMarker);

signals:
    void change();
    void beforeChange();
    void activeKeyChange();

protected:
    enum EHitCode
    {
        HIT_NOTHING,
        HIT_KEY,
        HIT_SPLINE,
    };

    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void OnLButtonDown(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event) override;
    void OnLButtonUp(QMouseEvent* event);
    void OnRButtonUp(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void OnRButtonDown(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event) override;

    // Drawing functions
    void DrawGradient(QPaintEvent* e, QPainter* painter);
    void DrawKeys(QPaintEvent* e, QPainter* painter);
    void UpdateTooltip(QPoint pos);

    EHitCode HitTest(QPoint point);

    //Tracking support helper functions
    void StartTracking();
    void TrackKey(QPoint point);
    void StopTracking(QPoint point);
    void RemoveKey(int nKey);
    void EditKey(int nKey);

    QPoint KeyToPoint(int nKey);
    QPoint TimeToPoint(float time);
    void PointToTimeValue(QPoint point, float& time, ISplineInterpolator::ValueType& val);
    float  XOfsToTime(int x);
    QPoint XOfsToPoint(int x);

    AZ::Color XOfsToColor(int x);
    AZ::Color TimeToColor(float time);

    void ClearSelection();

    void SendNotifyEvent(int nEvent);

    AZ::Color ValueToColor(ISplineInterpolator::ValueType val);
    void ColorToValue(const AZ::Color& col, ISplineInterpolator::ValueType& val);


private:
    void OnKeyColorChanged(const AZ::Color& color);

private:
    ISplineInterpolator* m_pSpline;

    bool m_bNoZoom;

    QRect m_rcClipRect;
    QRect m_rcGradient;
    QRect m_rcKeys;

    QPoint m_hitPoint;
    EHitCode m_hitCode;
    int    m_nHitKeyIndex;
    int    m_nHitKeyDist;
    QPoint m_curvePoint;

    float m_fTimeMarker;

    int m_nActiveKey;
    int m_nKeyDrawRadius;

    bool  m_bTracking;

    float m_fMinTime, m_fMaxTime;
    float m_fMinValue, m_fMaxValue;
    float m_fTooltipScaleX, m_fTooltipScaleY;

    bool m_bLockFirstLastKey;

    bool m_bNoTimeMarker;

    std::vector<int> m_bSelectedKeys;

    UpdateCallback m_updateCallback;

    CWndGridHelper m_grid;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_COLORGRADIENTCTRL_H
