/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_SPLINECTRLEX_H
#define CRYINCLUDE_EDITOR_CONTROLS_SPLINECTRLEX_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <ISplines.h>
#include "Controls/WndGridHelper.h"
#include "IKeyTimeSet.h"
#include "Undo/IUndoObject.h"
#include <QWidget>
#endif

// Custom styles for this control.
#define SPLINE_STYLE_NOGRID 0x0001
#define SPLINE_STYLE_NO_TIME_MARKER 0x0002

// Notify event sent when spline is being modified.
#define SPLN_CHANGE (0x0001)
// Notify event sent just before when spline is modified.
#define SPLN_BEFORE_CHANGE (0x0002)
// Notify when spline control is scrolled/zoomed.
#define SPLN_SCROLL_ZOOM (0x0003)
// Notify when time changed.
#define SPLN_TIME_START_CHANGE  (0x0001)
#define SPLN_TIME_END_CHANGE        (0x0002)
#define SPLN_TIME_CHANGE (0x0004)
// Notify event sent when a key selection changes
#define SPLN_KEY_SELECTION_CHANGE (0x0005)

#ifndef NM_CLICK
#define NM_CLICK (-2)
#define NM_RCLICK (-5)
#endif

#ifdef LoadCursor
#undef LoadCursor
#endif

class AbstractTimelineWidget;
class TimelineWidget;

class QRubberBand;

class ISplineSet
{
public:
    virtual ISplineInterpolator* GetSplineFromID(const AZStd::string& id) = 0;
    virtual AZStd::string GetIDFromSpline(ISplineInterpolator* pSpline) = 0;
    virtual int GetSplineCount() const = 0;
    virtual int GetKeyCountAtTime(float time, float threshold) const = 0;
};

class ISplineCtrlUndo
    : public IUndoObject
{
public:
    virtual bool IsSelectionChanged() const = 0;
};

class AbstractSplineWidget
    : public IKeyTimeSet
{
    friend class CUndoSplineCtrlEx;
public:
    AbstractSplineWidget();
    virtual ~AbstractSplineWidget();

    int InsertKey(ISplineInterpolator* pSpline, ISplineInterpolator* pDetailSpline, const QPoint& point);

    virtual void update() = 0;
    virtual void update(const QRect& rect) = 0;

    virtual QPoint mapFromGlobal(const QPoint& point) const = 0;

    virtual void SetCapture() {}

    virtual QWidget* WidgetCast() = 0;

    void SetGrid(int numX, int numY) { m_gridX = numX; m_gridY = numY; };
    void SetTimeRange(const Range& range) { m_timeRange = range; }
    void SetValueRange(const Range& range) { m_valueRange = range; }
    void SetDefaultValueRange(const Range& range) { m_defaultValueRange = range; }
    void SetDefaultKeyTangentType(ESplineKeyTangentType type) { m_defaultKeyTangentType = type; }
    ESplineKeyTangentType GetDefaultKeyTangentType() const { return m_defaultKeyTangentType; }
    void SetTooltipValueScale(float x, float y) { m_fTooltipScaleX = x; m_fTooltipScaleY = y; };
    void SetSplineSet(ISplineSet* pSplineSet);

    void AddSpline(ISplineInterpolator* pSpline, ISplineInterpolator* pDetailSpline, const QColor& color);
    void AddSpline(ISplineInterpolator * pSpline, ISplineInterpolator * pDetailSpline, QColor anColorArray[4]);
    void RemoveSpline(ISplineInterpolator* pSpline);
    void RemoveAllSplines();
    int  GetSplineCount() const { return static_cast<int>(m_splines.size()); }
    ISplineInterpolator* GetSpline(int nIndex) const { return m_splines[nIndex].pSpline; }

    void SetTimeMarker(float fTime);
    float GetTimeMarker() const { return m_fTimeMarker; }
    void SetTimeScale(float timeScale) { m_fTimeScale = timeScale; }
    void SetGridTimeScale(float fGridTimeScale) { m_fGridTimeScale = fGridTimeScale; }
    float GetGridTimeScale() { return m_fGridTimeScale; }

    void  SetMinTimeEpsilon(float fMinTimeEpsilon) { m_fMinTimeEpsilon = fMinTimeEpsilon; }
    float GetMinTimeEpsilon() const { return m_fMinTimeEpsilon; }

    void SetSnapTime(bool bOn) { m_bSnapTime = bOn; }
    void SetSnapValue(bool bOn) { m_bSnapValue = bOn; }
    bool IsSnapTime() const { return m_bSnapTime; }
    bool IsSnapValue() const { return m_bSnapValue; }

    float SnapTimeToGridVertical(float time);

    void OnUserCommand(UINT cmd);
    void FitSplineToViewWidth();
    void FitSplineToViewHeight();
    void FitSplineHeightToValueRange();

    void CopyKeys();
    void PasteKeys();

    void StoreUndo();

    void ZeroAll();
    void KeyAll();
    void SelectAll();

    void RemoveSelectedKeyTimes();

    void RedrawWindowAroundMarker();

    void SplinesChanged();
    void SetControlAmplitude(bool controlAmplitude);
    bool GetControlAmplitude() const;

    //void SelectPreviousKey();
    //void SelectNextKey();
    void GotoNextKey(bool previousKey);
    void RemoveAllKeysButThis();

    //////////////////////////////////////////////////////////////////////////
    // Scrolling/Zooming.
    //////////////////////////////////////////////////////////////////////////
    Vec2   ClientToWorld(const QPoint& point);
    QPoint WorldToClient(Vec2 v);
    Vec2 GetZoom();
    void SetZoom(Vec2 zoom, const QPoint& center);
    void SetZoom(Vec2 zoom);
    void SetScrollOffset(Vec2 ofs);
    Vec2 GetScrollOffset();
    float SnapTime(float time);
    float SnapValue(float val);
    //////////////////////////////////////////////////////////////////////////

    // IKeyTimeSet Implementation
    int GetKeyTimeCount() const override;
    float GetKeyTime(int index) const override;
    void MoveKeyTimes(int numChanges, int* indices, float scale, float offset, bool copyKeys) override;
    bool GetKeyTimeSelected(int index) const override;
    void SetKeyTimeSelected(int index, bool selected) override;
    int GetKeyCount(int index) const override;
    int GetKeyCountBound() const override;
    void BeginEdittingKeyTimes() override;
    void EndEdittingKeyTimes() override;

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    int leftBorderOffset() const
    {
        return m_nLeftOffset;
    }

protected:
    enum EHitCode
    {
        HIT_NOTHING,
        HIT_KEY,
        HIT_SPLINE,
        HIT_TIMEMARKER,
        HIT_TANGENT_HANDLE
    };
    enum EEditMode
    {
        NothingMode = 0,
        SelectMode,
        TrackingMode,
        ScrollZoomMode,
        ScrollMode,
        ZoomMode,
        TimeMarkerMode,
    };

    struct SSplineInfo
    {
        QColor anColorArray[4];
        ISplineInterpolator* pSpline;
        ISplineInterpolator* pDetailSpline;
    };

    virtual bool GetTangentHandlePts(QPoint& inTangentPt, QPoint& pt, QPoint& outTangentPt, int nSpline, int nKey, int nDimension);

    EHitCode HitTest(const QPoint& point);
    ISplineInterpolator* HitSpline(const QPoint& point);

    //Tracking support helper functions
    void StartTracking(bool copyKeys);
    void StopTracking();
    void RemoveKey(ISplineInterpolator* pSpline, int nKey);
    void RemoveSelectedKeys();
    void RemoveSelectedKeyTimesImpl();
    void MoveSelectedKeys(Vec2 offset, bool copyKeys);
    void ScaleAmplitudeKeys(float time, float startValue, float offset);
    void TimeScaleKeys(float time, float startTime, float endTime);
    void ValueScaleKeys(float startValue, float endValue);
    void ModifySelectedKeysFlags(int nRemoveFlags, int nAddFlags);

    QPoint TimeToPoint(float time, ISplineInterpolator* pSpline);
    float TimeToXOfs(float x);
    void PointToTimeValue(QPoint point, float& time, float& value);
    float  XOfsToTime(int x);
    QPoint XOfsToPoint(int x, ISplineInterpolator* pSpline);

    virtual void ClearSelection();
    virtual void SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect);
    bool IsKeySelected(ISplineInterpolator* pSpline, int nKey, int nDimension) const;
    int GetNumSelected();

    void SetHorizontalExtent(int min, int max);

    virtual void SendNotifyEvent(int nEvent) = 0;

    virtual void SelectRectangle(const QRect& rc, bool bSelect);
    //////////////////////////////////////////////////////////////////////////
    void UpdateKeyTimes() const;

    void ConditionalStoreUndo();

    void ClearSelectedKeys();
    void DuplicateSelectedKeys();

    Range GetSplinesRange();

    virtual void captureMouseImpl() = 0;
    virtual void releaseMouseImpl() = 0;
    virtual void setCursorImpl(UINT cursor) = 0;

    virtual ISplineCtrlUndo* CreateSplineCtrlUndoObject(std::vector<ISplineInterpolator*>& splineContainer);

    QRect m_rcClipRect;
    QRect m_rcSpline;
    QRect m_rcClient;

    QPoint m_cMousePos;
    QPoint m_cMouseDownPos;
    QPoint m_hitPoint;
    EHitCode m_hitCode;
    int m_nHitKeyIndex;
    int m_nHitDimension;
    int m_bHitIncomingHandle;
    ISplineInterpolator* m_pHitSpline;
    ISplineInterpolator* m_pHitDetailSpline;
    QPoint m_curvePoint;

    float m_fTimeMarker;

    int m_nKeyDrawRadius;

    bool m_bSnapTime;
    bool m_bSnapValue;
    bool m_bBitmapValid;

    int m_gridX;
    int m_gridY;

    float m_fMinTime, m_fMaxTime;
    float m_fMinValue, m_fMaxValue;
    float m_fTooltipScaleX, m_fTooltipScaleY;

    float m_fMinTimeEpsilon;

    QPoint m_lastToolTipPos;
    QString m_tooltipText;

    QRect m_rcSelect;

    QRect m_TimeUpdateRect;

    float m_fTimeScale;
    float m_fValueScale;
    float m_fGridTimeScale;

    Range m_timeRange;
    Range m_valueRange;
    Range m_defaultValueRange;

    //! This is how often to place ticks.
    //! value of 10 means place ticks every 10 second.
    double m_ticksStep;

    EEditMode m_editMode;

    int m_nLeftOffset;
    CWndGridHelper m_grid;

    //////////////////////////////////////////////////////////////////////////
    std::vector<SSplineInfo> m_splines;

    mutable bool m_bKeyTimesDirty;
    class KeyTime
    {
    public:
        KeyTime(float time, int count)
            : time(time)
            , oldTime(0.0f)
            , selected(false)
            , count(count) {}
        bool operator<(const KeyTime& other) const { return this->time < other.time; }
        float time;
        float oldTime;
        bool selected;
        int count;
    };
    mutable std::vector<KeyTime> m_keyTimes;
    mutable int m_totalSplineCount;

    static const float threshold;

    bool m_copyKeys;
    bool m_startedDragging;

    bool m_controlAmplitude;
    ESplineKeyTangentType m_defaultKeyTangentType;
    // Improving mouse control...
    bool m_boLeftMouseButtonDown;

    ISplineSet* m_pSplineSet;

    bool m_bEditLock;

    ISplineCtrlUndo* m_pCurrentUndo;
    AbstractTimelineWidget* m_pTimelineCtrl;
};

//////////////////////////////////////////////////////////////////////////
// Spline control.
//////////////////////////////////////////////////////////////////////////


class SplineWidget
    : public QWidget
    , public AbstractSplineWidget
{
    Q_OBJECT
public:
    SplineWidget(QWidget* parent);
    virtual ~SplineWidget();

    void update() override { QWidget::update(); }
    void update(const QRect& rect) override { QWidget::update(rect); }

    QPoint mapFromGlobal(const QPoint& point) const override { return QWidget::mapFromGlobal(point); }

    void SetTimelineCtrl(TimelineWidget* pTimelineCtrl);

    QWidget* WidgetCast() override { return this; }

Q_SIGNALS:
    void beforeChange();
    void change();
    void timeChange();
    void scrollZoomRequested();
    void clicked();
    void rightClicked();
    void keySelectionChange();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;

    void OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnLButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnRButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnMButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void OnMButtonUp(const QPoint& point, Qt::KeyboardModifiers modifiers);

    void DrawGrid(QPainter* painter);
    void DrawSpline(QPainter* pDC, SSplineInfo& splineInfo, float startTime, float endTime);
    void DrawKeys(QPainter* painter, int splineIndex, float startTime, float endTime);
    void DrawTimeMarker(QPainter* pDC);

    void DrawTangentHandle(QPainter* pDC, int nSpline, int nKey, int nDimension);

    void SendNotifyEvent(int nEvent) override;

    void captureMouseImpl() override { grabMouse(); }
    void releaseMouseImpl() override { releaseMouse(); }
    void setCursorImpl(UINT cursor) override { setCursor(CMFCUtils::LoadCursor(cursor)); }

protected:
    QRubberBand* m_rubberBand;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_SPLINECTRLEX_H
