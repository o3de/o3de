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

#include "CurveEditorContent.h"
#include <QWidget>
#include <Range.h>

class QMenu;
class CCurveEditorControl;
class CCurveEditorTangentControl;
struct ISplineInterpolator;

enum ETangent
{
    ETangent_In,
    ETangent_Out
};

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
    EDITOR_COMMON_API QColor LerpColor(const QColor& a, const QColor& b, float k);
}

class EDITOR_COMMON_API CCurveEditor
    : public QWidget
{
    Q_OBJECT
public:
    enum EOptOutFlags
    {
        EOptOutFree                     = 1 << 0,
        EOptOutFlat                     = 1 << 1,
        EOptOutLinear                   = 1 << 2,
        EOptOutStep                     = 1 << 3,
        EOptOutBezier                   = 1 << 4,
        EOptOutSelectionKey             = 1 << 5,
        EOptOutSelectionInOutTangent    = 1 << 6,
        //steven@conffx added some optout options to allow easier graphical customization
        EOptOutKeyIcon                  = 1 << 7,
        EOptOutRuler                    = 1 << 8,
        EOptOutTimeSlider               = 1 << 9,
        EOptOutBackground               = 1 << 10,
        EOptOutCustomPenColor = 1 << 11,
        EOptOutControls = 1 << 12,
        eOptOutDashedPath = 1 << 13,
        EOptOutDefaultTooltip = 1 << 14,
        EOptOutFitCurvesContextMenuOptions = 1 << 15,
        EOptOutZoomingAndPanning = 1 << 16
    };

    CCurveEditor(QWidget* parent);
    ~CCurveEditor();

    void SetContent(SCurveEditorContent* pContent);
    SCurveEditorContent* Content() const { return m_pContent; }

    void SetTime(const float time);

    // The background in the time and value range will be drawn a bit brighter to indicate where keys
    // should be placed. The curve editor does not enforce that the curves actually stay in those ranges.
    void SetTimeRange(const float start, const float end);
    void SetValueRange(const float min, const float max);

    // Points cannot be added outside of the time range and the view will not move horizontally...zooming will only be done on the vertical axis
    void EnforceTimeRange(const float start, const float end);
    bool IsTimeRangeEnforced() const;

    void ZoomToTimeRange(const float start, const float end);
    void ZoomToValueRange(const float min, const float max);

    void SetCurveType(ECurveEditorCurveType curveType);
    void SetWeighted(bool bWeighted);
    void SetHandlesVisible(bool bVisible);
    void SetRulerVisible(bool bVisible);
    void SetTimeSliderVisible(bool bVisible);

    Vec2 TransformToScreenCoordinates(Vec2 graphPoint);
    Vec2 TransformFromScreenCoordinates(Vec2 screenPoint);

    // Removes parts of the popup-menu, use CCurveEditor::EMenuOptOutFlags
    void SetOptOutFlags(int flags);

    // Tools added to tool bar depend on options above
    void PopulateControlContextMenu(QMenu* pToolBar);

    virtual void paintEvent(QPaintEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
    void mouseMoveEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void focusOutEvent(QFocusEvent* pEvent) override;
    void wheelEvent(QWheelEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;

    QString static TangentTypeToString(SCurveEditorKey::ETangentType type);

    void updateCurveKeyShapeColor();    // loop through curve keys and set shape color for them
    void SetIconShapeColor(unsigned int key, QColor color);
    void SetIconFillColor(unsigned int key, QColor color);
    void SetIconImage(QString str);
    void SetIconShapeMask(QColor color);
    void SetIconFillMask(QColor color);
    void SetIconToolTip(unsigned int key, QString str);
    void SetIconSize(unsigned int key, unsigned int size);

    void setPenColor(QColor color);
    void ContentChanged();
    void SortKeys(SCurveEditorCurve& curve);

    std::pair<SCurveEditorCurve*, Vec2> HitDetectCurve(const QPoint point);
    CCurveEditorControl* HitDetectKey(const QPoint point);
    CCurveEditorTangentControl* HitDetectTangent(const QPoint point);

    CCurveEditorControl* GetSelectedCurveKey();
    QRectF GetBackgroundRect();

    void SelectKey(CCurveEditorControl* pKeyToSelect, bool addToExistingSelection);
    void SelectTangent(CCurveEditorTangentControl* pTangentToSelect);
    void SelectInRect(const QRect& rect);

signals:
    void SignalContentChanged();
    void SignalScrub();
    void SignalKeyMoved();
    void SignalKeyMoveStarted();
    void SignalKeySelected(CCurveEditorControl* selectedKey);

public slots:
    void OnDeleteSelectedKeys();
    void OnSetSelectedKeysTangentStandard();
    void OnSetSelectedKeysTangentSmooth();
    void OnSetSelectedKeysTangentFree();
    void OnSetSelectedKeysTangentBezier();
    void OnSetSelectedKeysTangentFlat();
    void OnSetSelectedKeysTangentLinear();

    void OnSetSelectedKeysInTangentFree();
    void OnSetSelectedKeysInTangentFlat();
    void OnSetSelectedKeysInTangentLinear();
    void OnSetSelectedKeysInTangentStep();
    void OnSetSelectedKeysInTangentBezier();

    void OnSetSelectedKeysOutTangentFree();
    void OnSetSelectedKeysOutTangentFlat();
    void OnSetSelectedKeysOutTangentLinear();
    void OnSetSelectedKeysOutTangentStep();
    void OnSetSelectedKeysOutTangentBezier();

    void OnFitCurvesHorizontally();
    void OnFitCurvesVertically();

protected:
    struct SMouseHandler
    {
        virtual ~SMouseHandler() = default;
        virtual void mousePressEvent([[maybe_unused]] QMouseEvent* pEvent) {}
        virtual void mouseDoubleClickEvent([[maybe_unused]] QMouseEvent* pEvent) {}
        virtual void mouseMoveEvent([[maybe_unused]] QMouseEvent* pEvent) {}
        virtual void mouseReleaseEvent([[maybe_unused]] QMouseEvent* pEvent) {}
        virtual void focusOutEvent([[maybe_unused]] QFocusEvent* pEvent) {}
        virtual void paintOver([[maybe_unused]] QPainter& painter) {}
    };
    struct SSelectionHandler;
    struct SPanHandler;
    struct SZoomHandler;
    struct SMoveKeyHandler;
    struct SRotateTangentHandler;
    struct SScrubHandler;
    QColor m_penColor;

    void LeftButtonMousePressEvent(QMouseEvent* pEvent);
    void MiddleButtonMousePressEvent(QMouseEvent* pEvent);
    void RightButtonMousePressEvent(QMouseEvent* pEvent);

    void DeleteMarkedKeys();

    void SetTimeRange(const float start, const float end, bool enforce);

    Vec2 ClosestPointOnCurve(const Vec2 point, const SCurveEditorCurve& curve, const ECurveEditorCurveType curveType);

    bool AddPointToCurve(Vec2 point, SCurveEditorCurve* pCurve);

    QRect GetCurveArea();

    void SetSelectedKeysTangentType(const ETangent tangent, const SCurveEditorKey::ETangentType type);
    void SmoothSelectedKeys();

    void UpdateTangents();
    SCurveEditorContent* m_pContent;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::unique_ptr<SMouseHandler> m_pMouseHandler;

    ECurveEditorCurveType m_curveType;
    bool m_bWeighted;
    bool m_bHandlesVisible;
    bool m_bRulerVisible;
    bool m_bTimeSliderVisible;

    float m_time;
    Vec2 m_zoom;
    Vec2 m_translation;
    Range m_timeRange;
    bool m_timeRangeEnforced;
    Range m_valueRange;

    int m_optOutFlags;
public:
    QList<CCurveEditorControl*> m_pControlKeys;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};
