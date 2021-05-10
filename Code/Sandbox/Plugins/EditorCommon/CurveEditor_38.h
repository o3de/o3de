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

#include "CurveEditorContent.h"
#include <QWidget>
#include <Range.h>
#include <AnimTime.h>

class QToolBar;

enum ECurveEditorCurveType
{
    eCECT_Bezier,
    // 2D Bezier curves are used for better curve control, the editor
    // will enforce that the resulting curve is always 1D.
    eCECT_2DBezier,
};

namespace CurveEditorHelpers
{
    // Picks a nice color value for a curve. Wraps around after 4.
    EDITOR_COMMON_API ColorB GetCurveColor(const uint n);
}

class EDITOR_COMMON_API CCurveEditor
    : public QWidget
{
    Q_OBJECT
public:
    CCurveEditor(QWidget* parent);
    ~CCurveEditor();

    void SetContent(SCurveEditorContent* pContent);
    SCurveEditorContent* Content() const { return m_pContent; }

    SAnimTime Time() const { return m_time; }
    void SetTime(const SAnimTime time);

    // The background in the time and value range will be drawn a bit brighter to indicate where keys
    // should be placed. The curve editor does not enforce that the curves actually stay in those ranges.
    void SetTimeRange(const SAnimTime start, const SAnimTime end);
    void SetValueRange(const float min, const float max);

    void ZoomToTimeRange(const float start, const float end);
    void ZoomToValueRange(const float min, const float max);

    void SetCurveType(ECurveEditorCurveType curveType);
    void SetWeighted(bool bWeighted);
    void SetHandlesVisible(bool bVisible);
    void SetRulerVisible(bool bVisible);
    void SetTimeSliderVisible(bool bVisible);
    void SetGridVisible(bool bVisible);
    void SetFrameRate(SAnimTime::EFrameRate frameRate) { m_frameRate = frameRate; }
    void SetTimeSnapping(bool snapTime) { m_bSnapTime = snapTime; }
    void SetKeySnapping(bool snapKeys) { m_bSnapKeys = snapKeys; }

    // Tools added to tool bar depend on options above
    void FillWithCurveToolsAndConnect(QToolBar* pToolBar);

    void paintEvent(QPaintEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
    void mouseMoveEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void focusOutEvent(QFocusEvent* pEvent) override;
    void wheelEvent(QWheelEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;

signals:
    void SignalContentChanged();
    void SignalScrub();

public slots:
    void OnDeleteSelectedKeys();
    void OnSetSelectedKeysTangentAuto();
    void OnSetSelectedKeysInTangentZero();
    void OnSetSelectedKeysInTangentStep();
    void OnSetSelectedKeysInTangentLinear();
    void OnSetSelectedKeysOutTangentZero();
    void OnSetSelectedKeysOutTangentStep();
    void OnSetSelectedKeysOutTangentLinear();
    void OnFitCurvesHorizontally();
    void OnFitCurvesVertically();
    void OnBreakTangents();
    void OnUnifyTangents();

private:
    struct SMouseHandler;
    struct SSelectionHandler;
    struct SPanHandler;
    struct SZoomHandler;
    struct SMoveHandler;
    struct SHandleMoveHandler;
    struct SScrubHandler;
    enum ETangent;

    void DrawGrid(QPainter& painter, const QPalette& palette);

    void LeftButtonMousePressEvent(QMouseEvent* pEvent);
    void MiddleButtonMousePressEvent(QMouseEvent* pEvent);
    void RightButtonMousePressEvent(QMouseEvent* pEvent);

    void SelectInRect(const QRect& rect);

    void ContentChanged();
    void DeleteMarkedKeys();

    std::pair<SCurveEditorCurve*, Vec2> HitDetectCurve(const QPoint point);
    std::pair<SCurveEditorCurve*, SCurveEditorKey*> HitDetectKey(const QPoint point);
    std::tuple<SCurveEditorCurve*, SCurveEditorKey, SCurveEditorKey*, ETangent> HitDetectHandle(const QPoint point);
    Vec2 ClosestPointOnCurve(const Vec2 point, const SCurveEditorCurve& curve, const ECurveEditorCurveType curveType);

    void AddPointToCurve(Vec2 point, SCurveEditorCurve* pCurve);
    void SortKeys(SCurveEditorCurve& curve);

    QRect GetCurveArea();

    enum ETangent
    {
        ETangent_In,
        ETangent_Out
    };

    void SetSelectedKeysTangentType(const ETangent tangent, const SBezierControlPoint::ETangentType type);

    SCurveEditorContent* m_pContent;
    std::unique_ptr<SMouseHandler> m_pMouseHandler;

    ECurveEditorCurveType m_curveType;
    SAnimTime::EFrameRate m_frameRate;
    bool m_bWeighted : 1;
    bool m_bHandlesVisible : 1;
    bool m_bRulerVisible : 1;
    bool m_bTimeSliderVisible : 1;
    bool m_bGridVisible : 1;
    bool m_bSnapTime : 1;
    bool m_bSnapKeys : 1;

    SAnimTime m_time;
    Vec2 m_zoom;
    Vec2 m_translation;
    TRange<SAnimTime> m_timeRange;
    Range m_valueRange;
};
