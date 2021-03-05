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

#include "EditorCommon_precompiled.h"
#include "CurveEditorControl.h"

#include <QPainter>
#include <QBitmap>
#include <QMouseEvent>

#include <algorithm>

#include "CurveEditor.h"

namespace
{
    const int kDefaultControlVisualSize = 8;
    const int kDefaultControlClickableSize = 10;
    const int kDefaultTangentControlVisualSize = 6;
    const int kDefaultTangentControlClickableSize = 8;
    const int kDefaultTangentControlDistanceFromControl = 30;

    void DrawPointRect(QPainter& painter, QPointF point, const QColor& color, int size)
    {
        painter.setBrush(QBrush(color));
        painter.setPen(QColor(0, 0, 0));
        float halfSize = size / 2.0f;
        QPointF extents = QPointF(halfSize, halfSize);
        painter.drawRect(QRectF(point - extents, point + extents));
    }
}

CCurveEditorControl::CCurveEditorControl(CCurveEditor& curveEditor, SCurveEditorCurve& curve, SCurveEditorKey& key)
    : m_CurveEditor(curveEditor)
    , m_Curve(curve)
    , m_Key(key)
    , m_VisualSize(kDefaultControlVisualSize)
    , m_ClickableSize(kDefaultControlClickableSize)
    , m_pInTangent(new CCurveEditorTangentControl(*this, ETangent_In))
    , m_pOutTangent(new CCurveEditorTangentControl(*this, ETangent_Out))
    , m_filledPxr(kDefaultControlVisualSize, kDefaultControlVisualSize)
    , m_shapePxr(kDefaultControlVisualSize, kDefaultControlVisualSize)
    , m_icon(kDefaultControlVisualSize, kDefaultControlVisualSize)
    , m_originalPxr(kDefaultControlVisualSize, kDefaultControlVisualSize)
    , m_tip("")
    , m_iconsize(16)
{
}

CCurveEditorControl::~CCurveEditorControl()
{
    delete m_pInTangent;
    delete m_pOutTangent;
}

CCurveEditor& CCurveEditorControl::GetCurveEditor() const
{
    return m_CurveEditor;
}

SCurveEditorCurve& CCurveEditorControl::GetCurve() const
{
    return m_Curve;
}

SCurveEditorKey& CCurveEditorControl::GetKey() const
{
    return m_Key;
}

CCurveEditorTangentControl& CCurveEditorControl::GetInTangent()
{
    return *m_pInTangent;
}

CCurveEditorTangentControl& CCurveEditorControl::GetOutTangent()
{
    return *m_pOutTangent;
}

bool CCurveEditorControl::IsSelected() const
{
    return m_Key.m_bSelected;
}

void CCurveEditorControl::SetSelected(bool selected)
{
    m_Key.m_bSelected = selected;
    m_pInTangent->SetVisible(selected);
    m_pOutTangent->SetVisible(selected);
}

bool CCurveEditorControl::IsKeyMarkedForRemoval() const
{
    return m_Key.m_bDeleted;
}

void CCurveEditorControl::MarkKeyForRemoval()
{
    m_Key.m_bDeleted = true;
}

void CCurveEditorControl::Paint(QPainter& painter, const QPalette& palette, const bool& paintInOutTangents)
{
    const QColor pointColor = m_Key.m_bSelected ? palette.color(QPalette::Highlight) : QColor(255, 255, 255, 255);

    if (paintInOutTangents)
    {
        m_pInTangent->Paint(painter, palette);
        m_pOutTangent->Paint(painter, palette);
    }

    DrawPointRect(painter, GetScreenPosition(), pointColor, m_VisualSize);
}
void CCurveEditorControl::PaintIcon(QPainter& painter, const QPalette& palette, const bool& paintInOutTangents)
{
    if (paintInOutTangents)
    {
        m_pInTangent->Paint(painter, palette);
        m_pOutTangent->Paint(painter, palette);
    }

    QPointF pos = GetScreenPosition();
    float halfSize = m_iconsize / 2.0f;
    QPointF extents = QPointF(halfSize, halfSize);
    painter.drawPixmap(QRectF(GetScreenPosition() - extents, GetScreenPosition() + extents), m_icon, QRectF(0, 0, m_iconsize, m_iconsize));
}

bool CCurveEditorControl::IsMouseWithinControl(QPointF screenPos) const
{
    QPointF keyPosition = GetScreenPosition();
    QPointF deltaPos = screenPos - keyPosition;

    float halfClickSize = m_ClickableSize / 2.0f;

    return abs(deltaPos.x()) <= halfClickSize && abs(deltaPos.y()) <= halfClickSize;
}

QPointF CCurveEditorControl::GetScreenPosition() const
{
    Vec2 screenPosition = m_CurveEditor.TransformToScreenCoordinates(Vec2(m_Key.m_time, m_Key.m_value));
    return QPointF(screenPosition.x, screenPosition.y);
}

void CCurveEditorControl::BuildIcon()
{
    m_filledPxr = QPixmap(m_originalPxr.size());
    m_shapePxr = QPixmap(m_originalPxr.size());
    m_shapePxr.fill(m_shapeColor);
    m_filledPxr.fill(m_fillColor);
    m_shapePxr.setMask(m_originalPxr.createMaskFromColor(m_shapeMask).createMaskFromColor(m_fillMask));
    m_filledPxr.setMask(m_originalPxr.createMaskFromColor(m_fillMask, Qt::MaskOutColor));
    QPainter painter(&m_shapePxr);
    painter.drawPixmap(0, 0, m_filledPxr);
    m_icon = m_shapePxr.scaled(m_iconsize, m_iconsize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void CCurveEditorControl::SetIconFillMask(QColor color)
{
    m_fillMask = color;
    BuildIcon();
}

void CCurveEditorControl::SetIconShapeMask(QColor color)
{
    m_shapeMask = color;
    BuildIcon();
}

void CCurveEditorControl::SetIconImage(QString str)
{
    m_originalPxr.load(str);
    BuildIcon();
}

void CCurveEditorControl::SetIconFillColor(QColor color)
{
    m_fillColor = color;
    BuildIcon();
}

void CCurveEditorControl::SetIconShapeColor(QColor color)
{
    m_shapeColor = color;
    BuildIcon();
}


CCurveEditorTangentControl::CCurveEditorTangentControl(CCurveEditorControl& control, ETangent tangentDirection)
    : m_Control(control)
    , m_TangentDirection(tangentDirection)
    , m_Visible(false)
    , m_Selected(false)
    , m_VisualSize(kDefaultTangentControlVisualSize)
    , m_ClickableSize(kDefaultTangentControlClickableSize)
    , m_DistanceFromControl(kDefaultTangentControlDistanceFromControl)
{
}

CCurveEditorTangentControl::~CCurveEditorTangentControl()
{
}

CCurveEditorControl& CCurveEditorTangentControl::GetControl()
{
    return m_Control;
}

ETangent CCurveEditorTangentControl::GetTangentDirection() const
{
    return m_TangentDirection;
}

bool CCurveEditorTangentControl::IsVisible() const
{
    if (!m_Visible)
    {
        return false;
    }
    SCurveEditorKey::ETangentType tangentType;

    if (m_TangentDirection == ETangent_In)
    {
        tangentType = m_Control.GetKey().m_inTangentType;
    }
    else
    {
        tangentType = m_Control.GetKey().m_outTangentType;
    }

    if (tangentType == SCurveEditorKey::eTangentType_Step)
    {
        return false;
    }

    // first in
    if ((*m_Control.GetCurve().m_keys.begin()).m_time == m_Control.GetKey().m_time && m_TangentDirection == ETangent_In)
    {
        return false;
    }

    // last out
    if ((*(m_Control.GetCurve().m_keys.end() - 1)).m_time == m_Control.GetKey().m_time && m_TangentDirection == ETangent_Out)
    {
        return false;
    }

    return true;
}

void CCurveEditorTangentControl::SetVisible(bool visible)
{
    m_Visible = visible;
}

bool CCurveEditorTangentControl::IsSelected() const
{
    return m_Selected;
}

void CCurveEditorTangentControl::SetSelected(bool selected)
{
    m_Selected = selected;
}

void CCurveEditorTangentControl::SetVisualSize(int visualSize)
{
    m_VisualSize = visualSize;
}

void CCurveEditorTangentControl::SetClickableSize(int clickableSize)
{
    m_ClickableSize = clickableSize;
}

void CCurveEditorTangentControl::SetDistanceFromControl(int distanceFromControl)
{
    m_DistanceFromControl = distanceFromControl;
}

void CCurveEditorTangentControl::Paint(QPainter& painter, const QPalette& palette)
{
    if (!IsVisible())
    {
        return;
    }

    QPointF controlPosition = m_Control.GetScreenPosition();
    QPointF tangentPosition = GetScreenPosition();

    float highlightPercent;
    if (m_Selected)
    {
        highlightPercent = 0.0f;
    }
    else
    {
        highlightPercent = 0.5f;
    }
    const QColor tangentColor = CurveEditorHelpers::LerpColor(palette.color(QPalette::Highlight), palette.color(QPalette::Window), highlightPercent);
    const QPen tangentPen = QPen(tangentColor);

    painter.setPen(tangentPen);
    painter.drawLine(controlPosition, tangentPosition);

    const QColor tangentControlColor = m_Selected ? palette.color(QPalette::Highlight) : palette.color(QPalette::Dark);
    DrawPointRect(painter, tangentPosition, tangentControlColor, m_VisualSize);
}

bool CCurveEditorTangentControl::IsMouseWithinControl(QPointF screenPos) const
{
    if (!IsVisible())
    {
        return false;
    }

    QPointF keyPosition = GetScreenPosition();
    QPointF deltaPos = screenPos - keyPosition;

    float halfClickSize = m_ClickableSize / 2.0f;

    return abs(deltaPos.x()) <= halfClickSize && abs(deltaPos.y()) <= halfClickSize;
}

QPointF CCurveEditorTangentControl::GetScreenPosition() const
{
    QPointF controlPosition = m_Control.GetScreenPosition();

    Vec2 tangent;
    if (m_TangentDirection == ETangent_In)
    {
        tangent = m_Control.GetKey().m_inTangent;
    }
    else
    {
        tangent = m_Control.GetKey().m_outTangent;
    }

    Vec2 tangentScreenPosition = m_Control.GetCurveEditor().TransformToScreenCoordinates(Vec2(m_Control.GetKey().m_time + tangent.x, m_Control.GetKey().m_value + tangent.y));
    Vec2 transformedTangentDelta = (tangentScreenPosition - Vec2(aznumeric_cast<float>(controlPosition.x()), aznumeric_cast<float>(controlPosition.y()))).Normalize() * aznumeric_cast<float>(m_DistanceFromControl);

    return controlPosition + QPointF(transformedTangentDelta.x, transformedTangentDelta.y);
}
