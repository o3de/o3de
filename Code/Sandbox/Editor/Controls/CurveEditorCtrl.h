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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_CURVEEDITORCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_CURVEEDITORCTRL_H
#pragma once
#include "Util/GdiUtil.h"
#include <QWidget>
#include <QPen>


class CCurveEditorCtrl
    : public QWidget
{
public:
    enum EFlags
    {
        eFlag_ShowVerticalRuler = (1 << 0),
        eFlag_ShowHorizontalRuler = (1 << 1),
        eFlag_ShowVerticalRulerText = (1 << 2),
        eFlag_ShowHorizontalRulerText = (1 << 3),
        eFlag_ShowPaddingBorder = (1 << 4),
        eFlag_ShowMovingPointAxis = (1 << 5),
        eFlag_ShowPointHandles = (1 << 6),
        eFlag_ShowCursorAlways = (1 << 7),
        eFlag_Disabled = (1 << 8) // special case, when disabling preview window.
    };

    CCurveEditorCtrl(QWidget* parent);
    virtual ~CCurveEditorCtrl();

    void SetFlags(UINT aFlags);
    UINT GetFlags() const;
    void SetMouseEnable(bool bEnable = true) { m_bAllowMouse = bEnable; }
    bool GetMouseEnable() const {return m_bAllowMouse; }
    bool SetDomainBounds(float aMinX, float aMinY, float aMaxX, float aMaxY);
    void GetDomainBounds(float& rMinX, float& rMinY, float& rMaxX, float& rMaxY) const;
    // labelsX/labelsY must be null (to use default labels)
    // or contain aHorizontalSplits+1/aVerticalSplits+1 items.
    void SetGrid(UINT aHorizontalSplits, UINT aVerticalSplits, const QStringList& labelsX = QStringList(), const QStringList& labelsY = QStringList());
    void SetPadding(float padding) { m_padding = padding; }
    void MarkX(float value);
    void MarkY(float value);
    void AddControlPoint(const Vec2& rPosition);
    void ClearControlPoints();
    void SetControlPointCount(UINT aCount);
    UINT GetControlPointCount() const;
    void SetControlPoint(UINT aIndex, const Vec2& rPosition);
    void SetControlPointTangents(UINT aIndex, const Vec2& rLeft, const Vec2& rRight);
    void GetControlPoint(UINT aIndex, Vec2& rOutPosition) const;
    void GetControlPointTangents(UINT aIndex, Vec2& rOutLeft, Vec2& rOutRight) const;
    QPoint ProjectPoint(float x, float y);
    Vec2 UnprojectPoint(const QPoint& pt);
    void UpdateProjectedPoints();

protected:
    struct CurvePoint
    {
        CurvePoint(float aX = 0.0f, float aY = 0.0f)
        {
            pos.x = aX;
            pos.y = aY;
        }

        Vec2 pos;
        Vec2 tanA, tanB;
    };

    void ComputeTangents();
    void ClampToDomain(Vec2& rVec);
    void GenerateDefaultCurve();

    void paintEvent(QPaintEvent* event) override;

    std::vector<CurvePoint> m_points;
    std::vector<QPoint> m_projectedPoints;
    float m_domainMinX;
    float m_domainMinY;
    float m_domainMaxX;
    float m_domainMaxY;
    Vec2 m_gridSplits;
    int m_padding;
    bool m_bMouseDown, m_bDragging, m_bAllowMouse;
    bool m_bHovered;
    QPoint m_lastMousePoint;
    std::vector<int> m_selectedIndices;
    QFont m_fntInfo;
    QPen m_pen, m_selCrossPen;
    UINT m_flags;
    QStringList m_labelsX;
    QStringList m_labelsY;
    std::vector<float> m_marksX;
    std::vector<float> m_marksY;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
};



#endif // CRYINCLUDE_EDITOR_CONTROLS_CURVEEDITORCTRL_H
