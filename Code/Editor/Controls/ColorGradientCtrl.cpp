/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ColorGradientCtrl.h"

// Qt
#include <QPainter>
#include <QToolTip>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/ColorPicker.h>


#define MIN_TIME_EPSILON 0.01f

//////////////////////////////////////////////////////////////////////////
CColorGradientCtrl::CColorGradientCtrl(QWidget* parent)
    : QWidget(parent)
{
    m_nActiveKey = -1;
    m_nHitKeyIndex = -1;
    m_nKeyDrawRadius = 3;
    m_bTracking = false;
    m_pSpline = nullptr;
    m_fMinTime = -1;
    m_fMaxTime = 1;
    m_fMinValue = -1;
    m_fMaxValue = 1;
    m_fTooltipScaleX = 1;
    m_fTooltipScaleY = 1;
    m_bNoTimeMarker = true;
    m_bLockFirstLastKey = false;
    m_bNoZoom = true;

    ClearSelection();

    m_bSelectedKeys.reserve(0);

    m_fTimeMarker = -10;

    m_grid.zoom.x = 100;

    setMouseTracking(true);
}

CColorGradientCtrl::~CColorGradientCtrl()
{
}


/////////////////////////////////////////////////////////////////////////////
// QColorGradientCtrl message handlers

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    QRect rc(QPoint(0, 0), event->size());
    m_rcGradient = rc;
    m_rcGradient.setHeight(m_rcGradient.height() - 11);
    //m_rcGradient.DeflateRect(4,4);

    m_grid.rect = m_rcGradient;
    if (m_bNoZoom)
    {
        m_grid.zoom.x = static_cast<f32>(m_grid.rect.width());
    }

    m_rcKeys = rc;
    m_rcKeys.setTop(m_rcKeys.bottom() - 10);
}


//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::SetZoom(float fZoom)
{
    m_grid.zoom.x = fZoom;
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::SetOrigin(float fOffset)
{
    m_grid.origin.x = fOffset;
}

//////////////////////////////////////////////////////////////////////////
QPoint CColorGradientCtrl::KeyToPoint(int nKey)
{
    if (nKey >= 0)
    {
        return TimeToPoint(m_pSpline->GetKeyTime(nKey));
    }
    return QPoint(0, 0);
}

//////////////////////////////////////////////////////////////////////////
QPoint CColorGradientCtrl::TimeToPoint(float time)
{
    return QPoint(m_grid.WorldToClient(Vec2(time, 0)).x(), m_rcGradient.height() / 2);
}

//////////////////////////////////////////////////////////////////////////
AZ::Color CColorGradientCtrl::TimeToColor(float time)
{
    ISplineInterpolator::ValueType val;
    m_pSpline->Interpolate(time, val);
    const AZ::Color col = ValueToColor(val);
    return col;
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::PointToTimeValue(QPoint point, float& time, ISplineInterpolator::ValueType& val)
{
    time = XOfsToTime(point.x());
    ColorToValue(TimeToColor(time), val);
}

//////////////////////////////////////////////////////////////////////////
float CColorGradientCtrl::XOfsToTime(int x)
{
    return m_grid.ClientToWorld(QPoint(x, 0)).x;
}

//////////////////////////////////////////////////////////////////////////
QPoint CColorGradientCtrl::XOfsToPoint(int x)
{
    return TimeToPoint(XOfsToTime(x));
}

//////////////////////////////////////////////////////////////////////////
AZ::Color CColorGradientCtrl::XOfsToColor(int x)
{
    return TimeToColor(XOfsToTime(x));
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    QRect rcClient = rect();

    if (m_pSpline)
    {
        m_bSelectedKeys.resize(m_pSpline->GetKeyCount());
    }
    {
        if (!isEnabled())
        {
            painter.setBrush(palette().button());
            painter.drawRect(rcClient);
            return;
        }

        //////////////////////////////////////////////////////////////////////////
        // Fill keys backgound.
        //////////////////////////////////////////////////////////////////////////
        QRect rcKeys = m_rcKeys.intersected(e->rect());
        painter.setBrush(palette().button());
        painter.drawRect(rcKeys);
        //////////////////////////////////////////////////////////////////////////

        //Draw Keys and Curve
        if (m_pSpline)
        {
            DrawGradient(e, &painter);
            DrawKeys(e, &painter);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::DrawGradient(QPaintEvent* e, QPainter* painter)
{
    //Draw Curve
    // create and select a thick, white pen
    painter->setPen(QPen(QColor(128, 255, 128), 1, Qt::SolidLine));

    const QRect rcClip = e->rect().intersected(m_rcGradient);
    const int right = rcClip.left() + rcClip.width();
    for (int x = rcClip.left(); x < right; x++)
    {
        const AZ::Color col = XOfsToColor(x);
        QPen pen(QColor(col.GetR8(), col.GetG8(), col.GetR8(), col.GetA8()), 1, Qt::SolidLine);
        painter->setPen(pen);
        painter->drawLine(x, m_rcGradient.top(), x, m_rcGradient.top() + m_rcGradient.height());
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::DrawKeys(QPaintEvent* e, QPainter* painter)
{
    if (!m_pSpline)
    {
        return;
    }

    // create and select a white pen
    painter->setPen(QPen(QColor(0, 0, 0), 1, Qt::SolidLine));

    QRect rcClip = e->rect();

    m_bSelectedKeys.resize(m_pSpline->GetKeyCount());

    for (int i = 0; i < m_pSpline->GetKeyCount(); i++)
    {
        float time = m_pSpline->GetKeyTime(i);
        QPoint pt = TimeToPoint(time);

        if (pt.x() < rcClip.left() - 8 || pt.x() > rcClip.left() + rcClip.width() + 8)
        {
            continue;
        }

        const AZ::Color clr = TimeToColor(time);
        QBrush brush(QColor(clr.GetR8(), clr.GetG8(), clr.GetB8(), clr.GetA8()));
        painter->setBrush(brush);

        // Find the midpoints of the top, right, left, and bottom
        // of the client area. They will be the vertices of our polygon.
        QPoint pts[3];
        pts[0].rx() = pt.x();
        pts[0].ry() = m_rcKeys.top() + 1;
        pts[1].rx() = pt.x() - 5;
        pts[1].ry() = m_rcKeys.top() + 8;
        pts[2].rx() = pt.x() + 5;
        pts[2].ry() = m_rcKeys.top() + 8;
        painter->drawPolygon(pts, 3);

        if (m_bSelectedKeys[i])
        {
            QPen pen(QColor(200, 0, 0), 1, Qt::SolidLine);
            QPen oldPen = painter->pen();
            painter->setPen(pen);
            painter->drawPolygon(pts, 3);
            painter->setPen(oldPen);
        }
    }

    if (!m_bNoTimeMarker)
    {
        QPen timePen(QColor(255, 0, 255), 1, Qt::SolidLine);
        painter->setPen(timePen);
        QPoint pt = TimeToPoint(m_fTimeMarker);
        painter->drawLine(pt.x(), m_rcGradient.top() + 1, pt.x(), m_rcGradient.bottom() - 1);
    }
}

void CColorGradientCtrl::UpdateTooltip(QPoint pos)
{
    if (m_nHitKeyIndex >= 0)
    {
        float time = m_pSpline->GetKeyTime(m_nHitKeyIndex);
        ISplineInterpolator::ValueType val;
        m_pSpline->GetKeyValue(m_nHitKeyIndex, val);

        AZ::Color col = TimeToColor(time);
        int cont_s = (m_pSpline->GetKeyFlags(m_nHitKeyIndex) >> SPLINE_KEY_TANGENT_IN_SHIFT) & SPLINE_KEY_TANGENT_LINEAR ? 1 : 2;
        int cont_d = (m_pSpline->GetKeyFlags(m_nHitKeyIndex) >> SPLINE_KEY_TANGENT_OUT_SHIFT) & SPLINE_KEY_TANGENT_LINEAR ? 1 : 2;

        QString tipText(tr("%1 : %2,%3,%4 [%5,%6]").arg(time * m_fTooltipScaleX, 0, 'f', 2).arg(col.GetR8()).arg(col.GetG8()).arg(col.GetB8()).arg(cont_s).arg(cont_d));
        const QPoint globalPos = mapToGlobal(pos);
        QToolTip::showText(mapToGlobal(pos), tipText, this, QRect(globalPos, QSize(1, 1)));
    }
}

/////////////////////////////////////////////////////////////////////////////
//Mouse Message Handlers
//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        OnLButtonDown(event);
    }
    else if (event->button() == Qt::RightButton)
    {
        OnRButtonDown(event);
    }
}

void CColorGradientCtrl::OnLButtonDown([[maybe_unused]] QMouseEvent* event)
{
    if (m_bTracking)
    {
        return;
    }
    if (!m_pSpline)
    {
        return;
    }

    setFocus();

    switch (m_hitCode)
    {
    case HIT_KEY:
        StartTracking();
        SetActiveKey(m_nHitKeyIndex);
        break;

    /*
    case HIT_SPLINE:
    {
        // Cycle the spline slope of the nearest key.
        int flags = m_pSpline->GetKeyFlags(m_nHitKeyIndex);
        if (m_nHitKeyDist < 0)
            // Toggle left side.
            flags ^= SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT;
        if (m_nHitKeyDist > 0)
            // Toggle right side.
            flags ^= SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_OUT_SHIFT;
        m_pSpline->SetKeyFlags(m_nHitKeyIndex, flags);
        m_pSpline->Update();

        SetActiveKey(-1);
        SendNotifyEvent( CLRGRDN_CHANGE );
        if (m_updateCallback)
            m_updateCallback(this);
        break;
    }
    */

    case HIT_NOTHING:
        SetActiveKey(-1);
        break;
    }
    update();
}



//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::OnRButtonDown([[maybe_unused]] QMouseEvent* event)
{
}


//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_pSpline)
    {
        return;
    }

    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    switch (m_hitCode)
    {
    case HIT_SPLINE:
    {
        int iIndex = InsertKey(event->pos());
        SetActiveKey(iIndex);
        EditKey(iIndex);

        update();
    }
    break;
    case HIT_KEY:
    {
        EditKey(m_nHitKeyIndex);
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_pSpline)
    {
        return;
    }

    if (!m_bTracking)
    {
        switch (HitTest(event->pos()))
        {
        case HIT_SPLINE:
        {
            setCursor(CMFCUtils::LoadCursor(IDC_ARRWHITE));
        } break;
        case HIT_KEY:
        {
            setCursor(CMFCUtils::LoadCursor(IDC_ARRBLCK));
        } break;
        default:
            break;
        }
    }

    if (m_bTracking)
    {
        TrackKey(event->pos());
    }

    if (m_bTracking || m_nHitKeyIndex >= 0)
    {
        UpdateTooltip(event->pos());
    }
    else
    {
        QToolTip::hideText();
    }
}

void CColorGradientCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        OnLButtonUp(event);
    }
    else if (event->button() == Qt::RightButton)
    {
        OnRButtonUp(event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::OnLButtonUp(QMouseEvent* event)
{
    if (!m_pSpline)
    {
        return;
    }

    if (m_bTracking)
    {
        StopTracking(event->pos());
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::OnRButtonUp([[maybe_unused]] QMouseEvent* event)
{
    if (!m_pSpline)
    {
        return;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::SetActiveKey(int nIndex)
{
    ClearSelection();

    //Activate New Key
    if (nIndex >= 0)
    {
        m_bSelectedKeys[nIndex] = true;
    }
    m_nActiveKey = nIndex;
    update();

    SendNotifyEvent(CLRGRDN_ACTIVE_KEY_CHANGE);
}

/////////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::SetSpline(ISplineInterpolator* pSpline, bool bRedraw)
{
    if (pSpline != m_pSpline)
    {
        //if (pSpline && pSpline->GetNumDimensions() != 3)
        //return;
        m_pSpline = pSpline;
        m_nActiveKey = -1;
    }

    ClearSelection();

    if (bRedraw)
    {
        update();
    }
}

//////////////////////////////////////////////////////////////////////////
ISplineInterpolator* CColorGradientCtrl::GetSpline()
{
    return m_pSpline;
}

/////////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::keyPressEvent(QKeyEvent* event)
{
    bool bProcessed = false;

    if (m_nActiveKey != -1 && m_pSpline)
    {
        switch (event->key())
        {
        case Qt::Key_Delete:
        {
            RemoveKey(m_nActiveKey);
            bProcessed = true;
        } break;
        case Qt::Key_Up:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.rx() -= 1;
            SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);
            TrackKey(point);
            bProcessed = true;
        } break;
        case Qt::Key_Down:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.rx() += 1;
            SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);
            TrackKey(point);
            bProcessed = true;
        } break;
        case Qt::Key_Left:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.rx() -= 1;
            SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);
            TrackKey(point);
            bProcessed = true;
        } break;
        case Qt::Key_Right:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.rx() += 1;
            SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);
            TrackKey(point);
            bProcessed = true;
        } break;

        default:
            break; //do nothing
        }

        update();
    }

    event->setAccepted(bProcessed);
}

//////////////////////////////////////////////////////////////////////////////
CColorGradientCtrl::EHitCode CColorGradientCtrl::HitTest(QPoint point)
{
    if (!m_pSpline)
    {
        return HIT_NOTHING;
    }

    ISplineInterpolator::ValueType val;
    float time;
    PointToTimeValue(point, time, val);

    QRect rc = rect();

    m_nHitKeyIndex = -1;

    if (rc.contains(point))
    {
        m_nHitKeyDist = 0xFFFF;
        m_hitCode = HIT_SPLINE;

        for (int i = 0; i < m_pSpline->GetKeyCount(); i++)
        {
            QPoint splinePt = TimeToPoint(m_pSpline->GetKeyTime(i));
            if (abs(point.x() - splinePt.x()) < abs(m_nHitKeyDist))
            {
                m_nHitKeyIndex = i;
                m_nHitKeyDist = point.x() - splinePt.x();
            }
        }
        if (abs(m_nHitKeyDist) < 4)
        {
            m_hitCode = HIT_KEY;
        }
    }
    else
    {
        m_hitCode = HIT_NOTHING;
    }

    return m_hitCode;
}

///////////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::StartTracking()
{
    m_bTracking = true;

    GetIEditor()->BeginUndo();
    SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);

    setCursor(CMFCUtils::LoadCursor(IDC_ARRBLCKCROSS));
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::TrackKey(QPoint point)
{
    if (point.x() < m_rcGradient.left() || point.y() > m_rcGradient.right())
    {
        return;
    }

    int nKey = m_nHitKeyIndex;

    if (nKey >= 0)
    {
        ISplineInterpolator::ValueType val;
        float time;
        PointToTimeValue(point, time, val);

        // Clamp to min/max time.
        if (time < m_fMinTime || time > m_fMaxTime)
        {
            return;
        }

        int i;
        for (i = 0; i < m_pSpline->GetKeyCount(); i++)
        {
            // Switch to next key.
            if ((m_pSpline->GetKeyTime(i) < time && i > nKey) ||
                (m_pSpline->GetKeyTime(i) > time && i < nKey))
            {
                m_pSpline->SetKeyTime(nKey, time);
                m_pSpline->Update();
                SetActiveKey(i);
                m_nHitKeyIndex = i;
                return;
            }
        }

        if (!m_bLockFirstLastKey || (nKey != 0 && nKey != m_pSpline->GetKeyCount() - 1))
        {
            m_pSpline->SetKeyTime(nKey, time);
            m_pSpline->Update();
        }

        SendNotifyEvent(CLRGRDN_CHANGE);
        if (m_updateCallback)
        {
            m_updateCallback(this);
        }

        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::StopTracking(QPoint point)
{
    if (!m_bTracking)
    {
        return;
    }

    GetIEditor()->AcceptUndo("Spline Move");

    if (m_nHitKeyIndex >= 0)
    {
        QRect rc = rect();
        rc = rc.marginsAdded(QMargins(100, 100, 100, 100));
        if (!rc.contains(point))
        {
            RemoveKey(m_nHitKeyIndex);
        }
    }

    m_bTracking = false;
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::EditKey(int nKey)
{
    if (!m_pSpline)
    {
        return;
    }

    if (nKey < 0 || nKey >= m_pSpline->GetKeyCount())
    {
        return;
    }

    SetActiveKey(nKey);

    ISplineInterpolator::ValueType val;
    m_pSpline->GetKeyValue(nKey, val);

    SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);

    AzQtComponents::ColorPicker dlg(AzQtComponents::ColorPicker::Configuration::RGB);
    dlg.setCurrentColor(ValueToColor(val));
    dlg.setSelectedColor(ValueToColor(val));
    connect(&dlg, &AzQtComponents::ColorPicker::currentColorChanged, this, &CColorGradientCtrl::OnKeyColorChanged);
    if (dlg.exec() == QDialog::Accepted)
    {
        CUndo undo("Modify Gradient Color");
        OnKeyColorChanged(dlg.selectedColor());
    }
    else
    {
        OnKeyColorChanged(ValueToColor(val));
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::OnKeyColorChanged(const AZ::Color& color)
{
    int nKey = m_nActiveKey;
    if (!m_pSpline)
    {
        return;
    }
    if (nKey < 0 || nKey >= m_pSpline->GetKeyCount())
    {
        return;
    }

    ISplineInterpolator::ValueType val;
    ColorToValue(color, val);
    m_pSpline->SetKeyValue(nKey, val);
    update();

    if (m_bLockFirstLastKey)
    {
        if (nKey == 0)
        {
            m_pSpline->SetKeyValue(m_pSpline->GetKeyCount() - 1, val);
        }
        else if (nKey == m_pSpline->GetKeyCount() - 1)
        {
            m_pSpline->SetKeyValue(0, val);
        }
    }
    m_pSpline->Update();
    SendNotifyEvent(CLRGRDN_CHANGE);
    if (m_updateCallback)
    {
        m_updateCallback(this);
    }

    GetIEditor()->UpdateViews(eRedrawViewports);
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::RemoveKey(int nKey)
{
    if (!m_pSpline)
    {
        return;
    }
    if (m_bLockFirstLastKey)
    {
        if (nKey == 0 || nKey == m_pSpline->GetKeyCount() - 1)
        {
            return;
        }
    }

    CUndo undo("Remove Spline Key");

    SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);
    m_nActiveKey = -1;
    m_nHitKeyIndex = -1;
    if (m_pSpline)
    {
        m_pSpline->RemoveKey(nKey);
        m_pSpline->Update();
    }
    SendNotifyEvent(CLRGRDN_CHANGE);
    if (m_updateCallback)
    {
        m_updateCallback(this);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////
int CColorGradientCtrl::InsertKey(QPoint point)
{
    CUndo undo("Spline Insert Key");

    ISplineInterpolator::ValueType val;

    float time;
    PointToTimeValue(point, time, val);

    if (time < m_fMinTime || time > m_fMaxTime)
    {
        return -1;
    }

    int i;
    for (i = 0; i < m_pSpline->GetKeyCount(); i++)
    {
        // Skip if any key already have time that is very close.
        if (fabs(m_pSpline->GetKeyTime(i) - time) < MIN_TIME_EPSILON)
        {
            return i;
        }
    }

    SendNotifyEvent(CLRGRDN_BEFORE_CHANGE);

    m_pSpline->InsertKey(time, val);
    m_pSpline->Interpolate(time, val);
    ClearSelection();
    update();

    SendNotifyEvent(CLRGRDN_CHANGE);
    if (m_updateCallback)
    {
        m_updateCallback(this);
    }

    for (i = 0; i < m_pSpline->GetKeyCount(); i++)
    {
        // Find key with added time.
        if (m_pSpline->GetKeyTime(i) == time)
        {
            return i;
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::ClearSelection()
{
    m_nActiveKey = -1;
    if (m_pSpline)
    {
        m_bSelectedKeys.resize(m_pSpline->GetKeyCount());
    }
    for (int i = 0; i < (int)m_bSelectedKeys.size(); i++)
    {
        m_bSelectedKeys[i] = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::SetTimeMarker(float fTime)
{
    if (!m_pSpline)
    {
        return;
    }

    {
        QPoint pt = TimeToPoint(m_fTimeMarker);
        QRect rc = QRect(pt.x(), m_rcGradient.top(), 0, m_rcGradient.bottom() - m_rcGradient.top()).normalized();
        rc += QMargins(1, 0, 1, 0);
        update(rc);
    }
    {
        QPoint pt = TimeToPoint(fTime);
        QRect rc = QRect(pt.x(), m_rcGradient.top(), 0, m_rcGradient.bottom() - m_rcGradient.top()).normalized();
        rc += QMargins(1, 0, 1, 0);
        update(rc);
    }
    m_fTimeMarker = fTime;
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::SendNotifyEvent(int nEvent)
{
    switch (nEvent)
    {
    case CLRGRDN_BEFORE_CHANGE:
        emit beforeChange();
        break;
    case CLRGRDN_CHANGE:
        emit change();
        break;
    case CLRGRDN_ACTIVE_KEY_CHANGE:
        emit activeKeyChange();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
AZ::Color CColorGradientCtrl::ValueToColor(ISplineInterpolator::ValueType val)
{
    const AZ::Color color(val[0], val[1], val[2], 1.0);
    return color.LinearToGamma();
}

//////////////////////////////////////////////////////////////////////////
void CColorGradientCtrl::ColorToValue(const AZ::Color& col, ISplineInterpolator::ValueType& val)
{
    const AZ::Color colLin = col.GammaToLinear();
    val[0] = colLin.GetR();
    val[1] = colLin.GetG();
    val[2] = colLin.GetB();
    val[3] = 0;
}

void CColorGradientCtrl::SetNoTimeMarker(bool noTimeMarker)
{
    m_bNoTimeMarker = noTimeMarker;
    update();
}


#include <Controls/moc_ColorGradientCtrl.cpp>
