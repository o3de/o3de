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

#include "EditorDefs.h"

#include "CurveEditorCtrl.h"

// Qt
#include <QPainter>
#include <QPainterPath>

namespace CurveEditor
{
    const int kHandleSize = 6;
    const int kHandleSizeHalf = kHandleSize / 2;
    const int kDefaultPadding = 10;
    const int kInfoFontSize = 7;
    const int kGrid = 4;
    const QColor kColor_SelectCross(132, 132, 132);
    const QColor kColor_DisabledCross(90, 90, 90);
    const QColor kColor_MiddleLines(80, 80, 80);
    const QColor kColor_Background(41, 41, 41);
    const QColor kColor_Disabled(60, 60, 60);
    const QColor kColor_PaddingBorder(128, 128, 128);
    const QColor kColor_Text(128, 128, 128);
    const QColor kColor_TextCrtPos(187, 187, 187);
    const QColor kColor_Curve(255, 0, 0);
    const QColor kColor_SelHandle(200, 200, 200);
    const QColor kColor_NormalHandle(30, 30, 30);
    const QColor kColor_HandleLight(60, 60, 60);
    const QColor kColor_HandleShadow(0, 0, 0);
    const QColor kColor_MarkLines(0, 255, 0);
}


CCurveEditorCtrl::CCurveEditorCtrl(QWidget* parent)
    : QWidget(parent)
{
    m_domainMinX = 0.0f;
    m_domainMinY = 0.0f;
    m_domainMaxX = 1.0f;
    m_domainMaxY = 1.0f;
    m_bMouseDown = m_bDragging = false;
    m_bAllowMouse = true;
    m_padding = CurveEditor::kDefaultPadding;
    m_flags = eFlag_ShowVerticalRuler
        | eFlag_ShowHorizontalRuler
        | eFlag_ShowVerticalRulerText
        | eFlag_ShowHorizontalRulerText
        | eFlag_ShowPaddingBorder
        | eFlag_ShowMovingPointAxis
        | eFlag_ShowPointHandles;
    m_gridSplits.set(CurveEditor::kGrid, CurveEditor::kGrid);
    m_fntInfo.setFamily("Arial");
    m_fntInfo.setPointSize(CurveEditor::kInfoFontSize);
    m_bHovered = false;
    m_selCrossPen = QPen(CurveEditor::kColor_SelectCross);

    GenerateDefaultCurve();
}

CCurveEditorCtrl::~CCurveEditorCtrl()
{
}

void CCurveEditorCtrl::SetFlags(UINT aFlags)
{
    m_flags = aFlags;
}

UINT CCurveEditorCtrl::GetFlags() const
{
    return m_flags;
}

bool CCurveEditorCtrl::SetDomainBounds(float aMinX, float aMinY, float aMaxX, float aMaxY)
{
    assert(aMinX < aMaxX);
    assert(aMinY < aMaxY);

    if (aMinX >= aMaxX)
    {
        return false;
    }

    if (aMinY >= aMaxY)
    {
        return false;
    }

    m_domainMinX = aMinX;
    m_domainMinY = aMinY;
    m_domainMaxX = aMaxX;
    m_domainMaxY = aMaxY;

    return true;
}

void CCurveEditorCtrl::GetDomainBounds(float& rMinX, float& rMinY, float& rMaxX, float& rMaxY) const
{
    rMinX = m_domainMinX;
    rMinY = m_domainMinY;
    rMaxX = m_domainMaxX;
    rMaxY = m_domainMaxY;
}

void CCurveEditorCtrl::SetGrid(UINT aHorizontalSplits, UINT aVerticalSplits, const QStringList& labelsX, const QStringList& labelsY)
{
    assert(aHorizontalSplits);
    assert(aVerticalSplits);

    if (!aHorizontalSplits)
    {
        // defaults
        aHorizontalSplits = 2;
    }

    if (!aVerticalSplits)
    {
        // defaults
        aVerticalSplits = 2;
    }

    m_gridSplits.x = aHorizontalSplits;
    m_gridSplits.y = aVerticalSplits;

    if (!labelsX.isEmpty())
    {
        m_labelsX = labelsX;
    }

    if (!labelsY.isEmpty())
    {
        m_labelsY = labelsY;
    }
}

QPoint CCurveEditorCtrl::ProjectPoint(float x, float y)
{
    QPoint pt;

    pt.setX(m_padding + (width() - m_padding * 2) * (x - m_domainMinX) / (m_domainMaxX - m_domainMinX));
    pt.setY(m_padding + (height() - m_padding * 2) * (1.0f - (y - m_domainMinY) / (m_domainMaxY - m_domainMinY)));

    return pt;
}

Vec2 CCurveEditorCtrl::UnprojectPoint(const QPoint& pt)
{
    Vec2 vec;
    int y = height() - pt.y();
    float dx = (width() - m_padding * 2);
    float dy = (height() - m_padding * 2);
    const float kEpsilon = 0.00000001f;

    if (fabs(dx) <= kEpsilon)
    {
        dx = 1.0f;
    }

    if (fabs(dy) <= kEpsilon)
    {
        dy = 1.0f;
    }

    vec.x = m_domainMinX + (float)(pt.x() - m_padding) / dx * (m_domainMaxX - m_domainMinX);
    vec.y = m_domainMinY + (float)(y - m_padding) / dy * (m_domainMaxY - m_domainMinY);

    return vec;
}

void CCurveEditorCtrl::SetControlPointCount(UINT aCount)
{
    m_points.resize(aCount);
    m_projectedPoints.clear();
}

UINT CCurveEditorCtrl::GetControlPointCount() const
{
    return m_points.size();
}

void CCurveEditorCtrl::AddControlPoint(const Vec2& rPosition)
{
    m_points.push_back(CurvePoint(rPosition.x, rPosition.y));
}

void CCurveEditorCtrl::ClearControlPoints()
{
    m_points.clear();
}

void CCurveEditorCtrl::SetControlPoint(UINT aIndex, const Vec2& rPosition)
{
    assert(aIndex < m_points.size());

    if (aIndex >= m_points.size())
    {
        return;
    }

    m_points[aIndex].pos = rPosition;
}

void CCurveEditorCtrl::SetControlPointTangents(UINT aIndex, const Vec2& rLeft, const Vec2& rRight)
{
    assert(aIndex < m_points.size());

    if (aIndex >= m_points.size())
    {
        return;
    }

    m_points[aIndex].tanA = rLeft;
    m_points[aIndex].tanB = rRight;
}

void CCurveEditorCtrl::GetControlPoint(UINT aIndex, Vec2& rOutPosition) const
{
    assert(aIndex < m_points.size());

    if (aIndex >= m_points.size())
    {
        return;
    }

    rOutPosition = m_points[aIndex].pos;
}

void CCurveEditorCtrl::GetControlPointTangents(UINT aIndex, Vec2& rOutLeft, Vec2& rOutRight) const
{
    assert(aIndex < m_points.size());

    if (aIndex >= m_points.size())
    {
        return;
    }

    rOutLeft = m_points[aIndex].tanA;
    rOutRight = m_points[aIndex].tanB;
}

void CCurveEditorCtrl::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter dc(this);

    QRect rc = geometry();
    QString str;
    QRect textSize;

    dc.setFont(m_fntInfo);
    QFontMetrics fntMetrics(m_fntInfo);

    if (m_flags & eFlag_Disabled)
    {
        // If disabled, just draw a blank square.
        dc.fillRect(rc, CurveEditor::kColor_Disabled);

        dc.setPen(CurveEditor::kColor_DisabledCross);
        dc.drawLine(0, 0, rc.width(), rc.height());

        dc.drawLine(rc.width(), 0, 0, rc.height());
        return;
    }

    dc.fillRect(rc, CurveEditor::kColor_Background);

    dc.setPen(CurveEditor::kColor_MiddleLines);

    if (m_flags & eFlag_ShowVerticalRuler)
    {
        float y = m_domainMinY;
        float grid = (m_domainMaxY - m_domainMinY) / m_gridSplits.y;
        QPoint p;

        for (int i = 0; i <= m_gridSplits.y; ++i)
        {
            p = ProjectPoint(0, y);
            dc.drawLine(m_padding, p.y(), rc.width() - m_padding, p.y());

            if (m_flags & eFlag_ShowVerticalRulerText)
            {
                if (m_labelsY.empty())
                {
                    str.asprintf("%0.2f", y);
                }
                else
                {
                    str = m_labelsY[i];
                }
                textSize = fntMetrics.tightBoundingRect(str);
                dc.drawText(2, p.y(), str);
            }

            y += grid;
        }
    }

    if (m_flags & eFlag_ShowHorizontalRuler)
    {
        float x = m_domainMinX;
        float grid = (m_domainMaxX - m_domainMinX) / m_gridSplits.x;
        QPoint p;

        for (int i = 0; i <= m_gridSplits.x; ++i)
        {
            p = ProjectPoint(x, 0);
            dc.drawLine(p.x(), m_padding, p.x(), rc.height() - m_padding);

            if (m_flags & eFlag_ShowHorizontalRulerText)
            {
                if (m_labelsX.empty())
                {
                    str.asprintf("%0.2f", x);
                }
                else
                {
                    str = m_labelsX[i];
                }
                textSize = fntMetrics.tightBoundingRect(str);

                p.setX(p.x() + 2);

                if (p.x() + textSize.width() > width())
                {
                    p.setX(width() - textSize.width());
                }

                dc.drawText(p.x(), height() - m_padding + textSize.height() + 2, str);
            }

            x += grid;
        }
    }

    dc.setPen(CurveEditor::kColor_MarkLines);

    if (m_flags & eFlag_ShowVerticalRuler)
    {
        QPoint p;
        for (size_t i = 0; i < m_marksY.size(); ++i)
        {
            float v = m_marksY[i];
            if (v < m_domainMinY || v > m_domainMaxY)
            {
                continue;
            }
            p = ProjectPoint(0, v);
            dc.drawLine(m_padding, p.y(), width() - m_padding, p.y());
        }
    }

    if (m_flags & eFlag_ShowHorizontalRuler)
    {
        QPoint p;
        for (size_t i = 0; i < m_marksX.size(); ++i)
        {
            float v = m_marksX[i];
            if (v < m_domainMinX || v > m_domainMaxX)
            {
                continue;
            }
            p = ProjectPoint(v, 0);
            dc.drawLine(p.x(), m_padding, p.x(), height() - m_padding);
        }
    }

    if (m_flags & eFlag_ShowPaddingBorder)
    {
        dc.setPen(CurveEditor::kColor_PaddingBorder);
        dc.drawRect(m_padding, m_padding, width() - m_padding * 2, height() - m_padding * 2);
    }

    if (m_bDragging
        && !m_selectedIndices.empty()
        && (m_flags & eFlag_ShowMovingPointAxis))
    {
        const Vec2& crtPos = m_points[m_selectedIndices[0]].pos;

        dc.setBrush(CurveEditor::kColor_TextCrtPos);
        str.asprintf("(%0.2f,%0.2f)", crtPos.x, crtPos.y);
        textSize = fntMetrics.tightBoundingRect(str);
        const int kOffsetFromPointer = 5;
        QPoint txtPos(m_lastMousePoint.x() + kOffsetFromPointer, m_lastMousePoint.y() + kOffsetFromPointer);

        if (txtPos.x() + textSize.width() > width())
        {
            txtPos.setX(width() - textSize.width());
        }

        if (txtPos.y() + textSize.height() > height())
        {
            txtPos.setY(height() - textSize.height());
        }

        dc.drawText(txtPos, str);
    }

    ComputeTangents();
    UpdateProjectedPoints();

    // for curve debug, tangents poly, don't delete
    //  dc.setPen(Qt::black);
    //  dc.drawPolyline(m_projectedPoints.data(), m_projectedPoints.size());

    dc.setPen(CurveEditor::kColor_Curve);

    // curve
    QPainterPath bezierPath;
    bezierPath.moveTo(m_projectedPoints[0]);
    for (int i = 1; i < m_projectedPoints.size(); i += 3)
    {
        bezierPath.cubicTo(m_projectedPoints[i], m_projectedPoints[i + 1], m_projectedPoints[i + 2]);
    }
    dc.drawPath(bezierPath);

    // curve control point handles
    if (m_flags & eFlag_ShowPointHandles)
    {
        for (size_t i = 0; i < m_points.size(); ++i)
        {
            QPoint ptProj = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
            QRect rcHandle(0, 0, CurveEditor::kHandleSize, CurveEditor::kHandleSize);
            rcHandle.moveCenter(ptProj);

            std::vector<int>::iterator iter =
                std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i);

            bool bSelected = (iter != m_selectedIndices.end());

            if (bSelected && m_bDragging)
            {
                dc.setPen(m_selCrossPen);
                dc.drawLine(0, ptProj.y(), width(), ptProj.y());
                dc.drawLine(ptProj.x(), 0, ptProj.x(), height());
            }

            dc.fillRect(rcHandle, bSelected
                ? CurveEditor::kColor_SelHandle
                : CurveEditor::kColor_NormalHandle);

            dc.setPen(CurveEditor::kColor_HandleLight);
            dc.drawLine(ptProj.x() - CurveEditor::kHandleSizeHalf, ptProj.y() - CurveEditor::kHandleSizeHalf,
                ptProj.x() - CurveEditor::kHandleSizeHalf, ptProj.y() + CurveEditor::kHandleSizeHalf);
            dc.drawLine(ptProj.x() - CurveEditor::kHandleSizeHalf, ptProj.y() + CurveEditor::kHandleSizeHalf,
                ptProj.x() + CurveEditor::kHandleSizeHalf, ptProj.y() + CurveEditor::kHandleSizeHalf);
            dc.setPen(CurveEditor::kColor_HandleShadow);
            dc.drawLine(ptProj.x() + CurveEditor::kHandleSizeHalf, ptProj.y() + CurveEditor::kHandleSizeHalf,
                ptProj.x() + CurveEditor::kHandleSizeHalf, ptProj.y() - CurveEditor::kHandleSizeHalf);
            dc.drawLine(ptProj.x() + CurveEditor::kHandleSizeHalf, ptProj.y() - CurveEditor::kHandleSizeHalf,
                ptProj.x() - CurveEditor::kHandleSizeHalf, ptProj.y() - CurveEditor::kHandleSizeHalf);
        }
    }
}

void CCurveEditorCtrl::ComputeTangents()
{
    for (size_t i = 0; i < m_points.size(); ++i)
    {
        m_points[i].tanA = m_points[i].pos;
        m_points[i].tanB = m_points[i].pos;
    }

    int maxIndex = m_points.size() - 1;

    for (size_t i = 0; i < m_points.size(); ++i)
    {
        if (i > maxIndex)
        {
            break;
        }

        Vec2& p2 = m_points[i].pos;
        Vec2& back = m_points[i].tanA;
        Vec2& forw = m_points[i].tanB;
        const float kEpsilon = 0.000001f;

        // first point
        if (i == 0)
        {
            back = p2;

            if (maxIndex == 1)
            {
                Vec2& p3 = m_points[i + 1].pos;
                forw = p2 + (p3 - p2) / 3.0f;
            }
            else if (maxIndex > 0)
            {
                Vec2& p3 = m_points[i + 1].pos;
                Vec2& pb3 = m_points[i + 1].tanA;

                float lenOsn = (pb3 - p2).GetLength();
                float lenb = (p3 - p2).GetLength();

                if (lenOsn > kEpsilon && lenb > kEpsilon)
                {
                    forw = p2 + (pb3 - p2) / (lenOsn / lenb * 3.0f);
                }
                else
                {
                    forw = p2;
                }
            }
        }

        if (i == maxIndex)
        {
            forw = p2;

            if (i > 0)
            {
                Vec2& p1 = m_points[i - 1].pos;
                Vec2& pf1 = m_points[i - 1].tanB;

                float lenOsn = (pf1 - p2).GetLength();
                float lenf = (p1 - p2).GetLength();

                if (lenOsn > kEpsilon && lenf > kEpsilon)
                {
                    back = p2 + (pf1 - p2) / (lenOsn / lenf * 3.0f);
                }
                else
                {
                    back = p2;
                }
            }
        }
        else if (i >= 1 && i <= maxIndex - 1)
        {
            Vec2& p1 = m_points[i - 1].pos;
            Vec2& p3 = m_points[i + 1].pos;

            float lenOsn = (p3 - p1).GetLength();
            float lenb = (p1 - p2).GetLength();
            float lenf = (p3 - p2).GetLength();

            if (lenOsn > kEpsilon
                && lenf > kEpsilon
                && lenb > kEpsilon)
            {
                back = p2 + (p1 - p3) * (lenb / lenOsn / 3.0f);
                forw = p2 + (p3 - p1) * (lenf / lenOsn / 3.0f);
            }
        }

        ClampToDomain(back);
        ClampToDomain(forw);
    }

    // fix tangents in relation of one to another
    for (size_t i = 0; i < m_points.size(); ++i)
    {
        Vec2& p = m_points[i].pos;
        Vec2& tanA = m_points[i].tanA;
        Vec2& tanB = m_points[i].tanB;

        if (i < m_points.size() - 1)
        {
            if (tanB.x > m_points[i + 1].tanA.x)
            {
                tanB.x = (m_points[i + 1].pos.x + p.x) * 0.5f;
            }
        }
        if (i > 0)
        {
            if (tanA.x < m_points[i - 1].tanB.x)
            {
                tanA.x = (m_points[i - 1].pos.x + p.x) * 0.5f;
            }
        }
    }
}

void CCurveEditorCtrl::UpdateProjectedPoints()
{
    m_projectedPoints.resize(m_points.size() * 3 - 2);

    int numPts = 0;

    for (size_t i = 0; i < m_points.size(); ++i)
    {
        if (i == 0)
        {
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanB.x, m_points[i].tanB.y);
        }
        else if (i == m_points.size() - 1)
        {
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanA.x, m_points[i].tanA.y);
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
        }
        else
        {
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanA.x, m_points[i].tanA.y);
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
            m_projectedPoints[numPts++] = ProjectPoint(m_points[i].tanB.x, m_points[i].tanB.y);
        }
    }
}

void CCurveEditorCtrl::ClampToDomain(Vec2& rVec)
{
    if (rVec.x < m_domainMinX)
    {
        rVec.x = m_domainMinX;
    }
    else if (rVec.x > m_domainMaxX)
    {
        rVec.x = m_domainMaxX;
    }

    if (rVec.y < m_domainMinY)
    {
        rVec.y = m_domainMinY;
    }
    else if (rVec.y > m_domainMaxY)
    {
        rVec.y = m_domainMaxY;
    }
}

void CCurveEditorCtrl::GenerateDefaultCurve()
{
    m_points.clear();
    m_domainMinX = 0.0f;
    m_domainMinY = 0.0f;
    m_domainMaxX = 1.0f;
    m_domainMaxY = 1.0f;
    m_points.push_back(CurvePoint(0.00f, 0.00f));
    m_points.push_back(CurvePoint(0.25f, 0.25f));
    m_points.push_back(CurvePoint(0.50f, 0.50f));
    m_points.push_back(CurvePoint(0.75f, 0.75f));
    m_points.push_back(CurvePoint(1.00f, 1.00f));
}

void CCurveEditorCtrl::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    const QPoint point = event->pos();
    if (m_bAllowMouse)
    {
        bool bSimpleSelect = !(event->modifiers() & Qt::ShiftModifier) && !(event->modifiers() & Qt::ControlModifier);

        if (bSimpleSelect)
        {
            m_selectedIndices.clear();
        }

        for (size_t i = 0; i < m_points.size(); ++i)
        {
            QPoint ptProj = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);

            QRect rcHandle(0, 0, CurveEditor::kHandleSize, CurveEditor::kHandleSize);
            rcHandle.moveCenter(ptProj);

            if (rcHandle.contains(point))
            {
                if (bSimpleSelect)
                {
                    m_selectedIndices.push_back(i);
                    break;
                }

                if (event->modifiers() & Qt::ShiftModifier)
                {
                    m_selectedIndices.push_back(i);
                }
                else if (event->modifiers() & Qt::ControlModifier)
                {
                    std::vector<int>::iterator iter =
                        std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i);

                    if (iter == m_selectedIndices.end())
                    {
                        m_selectedIndices.push_back(i);
                    }
                    else
                    {
                        m_selectedIndices.erase(iter);
                    }
                }
            }
        }

        m_bMouseDown = true;
        m_lastMousePoint = point;
    }

    grabMouse();
    update();
}

void CCurveEditorCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    m_bMouseDown = false;
    m_bDragging = false;
    m_selectedIndices.clear();

    releaseMouse();
    update();
}

void CCurveEditorCtrl::mouseMoveEvent(QMouseEvent* event)
{
    if (m_bMouseDown && !m_bDragging)
    {
        m_bDragging = true;
    }

    m_bHovered = true;
    if (m_flags & eFlag_ShowCursorAlways)
    {
        m_bHovered = true;
    }
    else
    {
        m_bHovered = false;

        for (size_t i = 0; i < m_points.size(); ++i)
        {
            QPoint ptProj = ProjectPoint(m_points[i].pos.x, m_points[i].pos.y);
            QRect rcHandle(0, 0, CurveEditor::kHandleSize, CurveEditor::kHandleSize);
            rcHandle.moveCenter(ptProj);

            if (rcHandle.contains(event->pos()))
            {
                m_bHovered = true;
                break;
            }
        }
    }

    if (m_bDragging)
    {
        Vec2 v1 = UnprojectPoint(m_lastMousePoint);
        Vec2 v2 = UnprojectPoint(event->pos());
        Vec2 v = v1 - v2;

        for (size_t i = 0; i < m_selectedIndices.size(); ++i)
        {
            int index = m_selectedIndices[i];
            CurvePoint& cpt = m_points[index];

            // do not move first and last points on X
            if (index > 0 && index < m_points.size() - 1)
            {
                cpt.pos.x -= v.x;
            }

            cpt.pos.y -= v.y;

            // lets check if the point is overlapping its neighbours
            if (index > 0 && (index - 1) > 0)
            {
                if (cpt.pos.x < m_points[index - 1].pos.x)
                {
                    CurvePoint p = m_points[index];

                    // swap!
                    m_points[index] = m_points[index - 1];
                    m_points[index - 1] = p;
                    m_selectedIndices[i] = index - 1;
                }
            }

            if (index < m_points.size() - 1 && (index + 1) < m_points.size() - 1)
            {
                if (cpt.pos.x > m_points[index + 1].pos.x)
                {
                    CurvePoint p = m_points[index];

                    // swap!
                    m_points[index] = m_points[index + 1];
                    m_points[index + 1] = p;
                    m_selectedIndices[i] = index + 1;
                }
            }

            ClampToDomain(cpt.pos);
        }

        update();
        m_lastMousePoint = event->pos();
    }

    QWidget::mouseMoveEvent(event);
}

void CCurveEditorCtrl::MarkX(float value)
{
    m_marksX.push_back(value);
}

void CCurveEditorCtrl::MarkY(float value)
{
    m_marksY.push_back(value);
}
