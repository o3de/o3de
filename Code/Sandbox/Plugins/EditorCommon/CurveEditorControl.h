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

#include "CurveEditor.h"

#include <QWidget>

class CCurveEditor;
class CCurveEditorTangentControl;
class QMouseEvent;

class QPainter;

class EDITOR_COMMON_API CCurveEditorControl
{
public:
    CCurveEditorControl(CCurveEditor& curveEditor, SCurveEditorCurve& curve, SCurveEditorKey& key);
    ~CCurveEditorControl();

    CCurveEditor& GetCurveEditor() const;
    SCurveEditorCurve& GetCurve() const;
    SCurveEditorKey& GetKey() const;

    CCurveEditorTangentControl& GetInTangent();
    CCurveEditorTangentControl& GetOutTangent();

    void SetVisualSize(int visualSize)
    {
        m_VisualSize = visualSize;
    }
    void SetClickableSize(int clickableSize)
    {
        m_ClickableSize = clickableSize;
    }

    bool IsSelected() const;
    void SetSelected(bool selected);

    bool IsKeyMarkedForRemoval() const;
    void MarkKeyForRemoval();

    void Paint(QPainter& painter, const QPalette& palette, const bool& paintInOutTangents);
    void PaintIcon(QPainter& painter, const QPalette& palette, const bool& paintInOutTangents);

    bool IsMouseWithinControl(QPointF screenPos) const;

    QPointF GetScreenPosition() const;

    QRect GetRect() const
    {
        return QRect(aznumeric_cast<int>(GetScreenPosition().x() - (m_VisualSize / 2)), aznumeric_cast<int>(GetScreenPosition().y() - (m_VisualSize / 2)), m_VisualSize, m_VisualSize);
    }

    void SetIconShapeColor(QColor color);

    void SetIconFillColor(QColor color);

    void SetIconImage(QString str);

    void SetIconShapeMask(QColor color);

    void SetIconFillMask(QColor color);

    void SetIconToolTip(QString str)
    {
        m_tip = str;
    }

    void SetIconSize(int size)
    {
        m_iconsize = size;
        BuildIcon();
    }

    QString GetToolTip() const { return m_tip; }

protected:
    void BuildIcon();

private:
    QPixmap m_icon;
    QPixmap m_filledPxr;
    QPixmap m_shapePxr;
    QPixmap m_originalPxr;
    QColor  m_fillColor;
    QColor  m_shapeColor;
    QColor  m_fillMask;
    QColor  m_shapeMask;
    QString m_tip;
    int m_iconsize;

    CCurveEditor& m_CurveEditor;
    SCurveEditorCurve& m_Curve;
    SCurveEditorKey& m_Key;

    int m_VisualSize;
    int m_ClickableSize;

    CCurveEditorTangentControl* m_pInTangent;
    CCurveEditorTangentControl* m_pOutTangent;
};

class EDITOR_COMMON_API CCurveEditorTangentControl
{
public:
    CCurveEditorTangentControl(CCurveEditorControl& curveControl, ETangent tangentDirection);
    ~CCurveEditorTangentControl();

    CCurveEditorControl& GetControl();
    ETangent GetTangentDirection() const;

    bool IsVisible() const;
    void SetVisible(bool visible);

    bool IsSelected() const;
    void SetSelected(bool selected);

    void SetVisualSize(int visualSize);
    void SetClickableSize(int clickableSize);
    void SetDistanceFromControl(int distanceFromControl);

    void Paint(QPainter& painter, const QPalette& palette);

    bool IsMouseWithinControl(QPointF screenPos) const;

private:

    QPointF GetScreenPosition() const;

    CCurveEditorControl& m_Control;
    ETangent m_TangentDirection;

    bool m_Selected;

    int m_VisualSize;
    int m_ClickableSize;
    int m_DistanceFromControl;

    bool m_Visible;
};
