/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "SplineCtrl.h"

// Qt
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

// Editor
#include "TimelineCtrl.h"


#define MIN_TIME_EPSILON 0.01f

//////////////////////////////////////////////////////////////////////////
CSplineCtrl::CSplineCtrl(QWidget* parent)
    : QWidget(parent)
{
    m_nActiveKey = -1;
    m_nHitKeyIndex = -1;
    m_nKeyDrawRadius = 3;
    m_bTracking = false;
    m_pSpline = 0;
    m_gridX = 10;
    m_gridY = 10;
    m_fMinTime = -1;
    m_fMaxTime = 1;
    m_fMinValue = -1;
    m_fMaxValue = 1;
    m_fTooltipScaleX = 1;
    m_fTooltipScaleY = 1;
    m_bLockFirstLastKey = false;
    m_pTimelineCtrl = 0;

    m_bSelectedKeys.reserve(0);

    m_fTimeMarker = -10;

    setMouseTracking(true);
}

CSplineCtrl::~CSplineCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSplineCtrl message handlers

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::resizeEvent([[maybe_unused]] QResizeEvent* event)
{
    m_rcSpline = rect();

    if (m_pTimelineCtrl)
    {
        QRect rct = m_rcSpline;
        rct.setHeight(20);
        m_rcSpline.setTop(rct.bottom() + 1);
        m_pTimelineCtrl->setGeometry(rct);
    }

    m_rcSpline.adjust(2, 2, -2, -2);
}

//////////////////////////////////////////////////////////////////////////
QPoint CSplineCtrl::KeyToPoint(int nKey)
{
    if (nKey >= 0)
    {
        return TimeToPoint(m_pSpline->GetKeyTime(nKey));
    }
    return QPoint(0, 0);
}

//////////////////////////////////////////////////////////////////////////
QPoint CSplineCtrl::TimeToPoint(float time)
{
    QPoint point;
    point.setX((time - m_fMinTime) * (m_rcSpline.width() / (m_fMaxTime - m_fMinTime)) + m_rcSpline.left());
    float val = 0;
    if (m_pSpline)
    {
        m_pSpline->InterpolateFloat(time, val);
    }
    point.setY((floor((m_fMaxValue - val) * (m_rcSpline.height() / (m_fMaxValue - m_fMinValue)) + 0.5f) + m_rcSpline.top()));
    return point;
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::PointToTimeValue(const QPoint& point, float& time, float& value)
{
    time = XOfsToTime(point.x());
    float t = float(m_rcSpline.bottom() - point.y()) / m_rcSpline.height();
    value = LERP(m_fMinValue, m_fMaxValue, t);
}

//////////////////////////////////////////////////////////////////////////
float CSplineCtrl::XOfsToTime(int x)
{
    // m_fMinTime to m_fMaxTime time range.
    float t = float(x - m_rcSpline.left()) / m_rcSpline.width();
    return LERP(m_fMinTime, m_fMaxTime, t);
}

//////////////////////////////////////////////////////////////////////////
QPoint CSplineCtrl::XOfsToPoint(int x)
{
    return TimeToPoint(XOfsToTime(x));
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QRect rcClient = rect();

    if (m_pSpline)
    {
        m_bSelectedKeys.resize(m_pSpline->GetKeyCount());
    }

    {
        if (m_TimeUpdateRect != event->rect())
        {
            painter.fillRect(event->rect(), QColor(140, 140, 140));

            //Draw Grid
            DrawGrid(&painter);

            //Draw Keys and Curve
            if (m_pSpline)
            {
                DrawSpline(&painter);
                DrawKeys(&painter);
            }
        }
        m_TimeUpdateRect = QRect();
    }
    DrawTimeMarker(&painter);
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::DrawGrid(QPainter* painter)
{
    QPen pOldPen = painter->pen();

    int cx = m_rcSpline.width();
    int cy = m_rcSpline.height();

    QPen pen(QColor(90, 90, 90), 1, Qt::DotLine);
    pen.setCosmetic(true);
    painter->setPen(pen);

    //Draw Vertical Grid Lines
    for (int y = 1; y < m_gridX; y++)
    {
        painter->drawLine(m_rcSpline.left() + y * cx / m_gridX, m_rcSpline.top() + cy, m_rcSpline.left() + y * cx / m_gridX, m_rcSpline.top());
    }

    //Draw Horizontal Grid Lines
    for (int x = 1; x < m_gridY; x++)
    {
        painter->drawLine(m_rcSpline.left(), m_rcSpline.top() + x * cy / m_gridY, m_rcSpline.left() + cx, m_rcSpline.top() + x * cy / m_gridY);
    }

    painter->setPen(QColor(75, 75, 75));

    painter->drawLine(m_rcSpline.left() + (m_gridX / 2) * cx / m_gridX, m_rcSpline.top() + cy, m_rcSpline.left() + (m_gridX / 2) * cx / m_gridX, m_rcSpline.top() + 0);
    painter->drawLine(m_rcSpline.left() + 0, m_rcSpline.left() + (m_gridY / 2) * cy / m_gridY, m_rcSpline.left() + cx, m_rcSpline.left() + (m_gridY / 2) * cy / m_gridY);

    painter->drawRect(m_rcSpline);

    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::DrawSpline(QPainter* painter)
{
    //Draw Curve
    // create and select a thick, white pen
    const QPen pOldPen = painter->pen();
    painter->setPen(QColor(128, 255, 128));

    const QRect rcClip = painter->hasClipping() ? painter->clipBoundingRect().intersected(m_rcSpline).toRect() : m_rcSpline;

    bool bFirst = true;
    QPainterPath path;
    for (int x = rcClip.left(); x < rcClip.right(); x++)
    {
        QPoint pt = XOfsToPoint(x);
        if (!bFirst)
        {
            path.lineTo(pt);
        }
        else
        {
            path.moveTo(pt);
            bFirst = false;
        }
    }
    painter->drawPath(path);

    // Put back the old objects
    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::DrawKeys(QPainter* painter)
{
    if (!m_pSpline)
    {
        return;
    }

    // create and select a white pen
    const QPen pOldPen = painter->pen();
    painter->setPen(Qt::black);

    m_bSelectedKeys.resize(m_pSpline->GetKeyCount());

    for (int i = 0; i < m_pSpline->GetKeyCount(); i++)
    {
        float time = m_pSpline->GetKeyTime(i);
        const QPoint pt = TimeToPoint(time);

        QColor clr(220, 220, 0);
        if (m_bSelectedKeys[i])
        {
            clr = QColor(255, 0, 0);
        }
        const QBrush pOldBrush = painter->brush();
        painter->setBrush(clr);

        // Draw this key.
        painter->drawRect(QRect(QPoint(pt.x() - m_nKeyDrawRadius, pt.y() - m_nKeyDrawRadius), QPoint(pt.x() + m_nKeyDrawRadius - 1, pt.y() + m_nKeyDrawRadius - 1)));

        painter->setBrush(pOldBrush);
    }

    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::DrawTimeMarker(QPainter* painter)
{
    painter->setPen(QColor(255, 0, 255));
    const QPoint pt = TimeToPoint(m_fTimeMarker);
    painter->drawLine(pt.x(), m_rcSpline.top() + 1, pt.x(), m_rcSpline.bottom() - 1);
}

void CSplineCtrl::UpdateToolTip()
{
    if (m_nHitKeyIndex >= 0 && m_pSpline)
    {
        float time = m_pSpline->GetKeyTime(m_nHitKeyIndex);
        float val;
        m_pSpline->GetKeyValueFloat(m_nHitKeyIndex, val);
        int cont_s = (m_pSpline->GetKeyFlags(m_nHitKeyIndex) >> SPLINE_KEY_TANGENT_IN_SHIFT) & SPLINE_KEY_TANGENT_LINEAR ? 1 : 2;
        int cont_d = (m_pSpline->GetKeyFlags(m_nHitKeyIndex) >> SPLINE_KEY_TANGENT_OUT_SHIFT) & SPLINE_KEY_TANGENT_LINEAR ? 1 : 2;

        const QString tipText = tr("%1, %2, [%3|%4").arg(time * m_fTooltipScaleX, 3, 'f').arg(val * m_fTooltipScaleY, 3, 'f').arg(cont_s).arg(cont_d);
        QToolTip::showText(QCursor::pos(), tipText, this);
    }
}

/////////////////////////////////////////////////////////////////////////////
//Mouse Message Handlers
//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->pos(), event->modifiers());
        break;
    }
}

void CSplineCtrl::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->pos(), event->modifiers());
        break;
    }
}

void CSplineCtrl::OnLButtonDown([[maybe_unused]] const QPoint& point, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
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

    case HIT_SPLINE:
        // Cycle the spline slope of the nearest key.
        ToggleKeySlope(m_nHitKeyIndex, m_nHitKeyDist);
        SetActiveKey(-1);
        break;

    case HIT_NOTHING:
        SetActiveKey(-1);
        break;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::OnRButtonDown([[maybe_unused]] const QPoint& point, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    setFocus();

    if (!m_pSpline)
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_pSpline || event->button() != Qt::LeftButton)
    {
        return;
    }

    switch (m_hitCode)
    {
    case HIT_NOTHING:
    {
        int iIndex = InsertKey(event->pos());
        SetActiveKey(iIndex);

        update();
    }
    break;
    case HIT_KEY:
    {
        RemoveKey(m_nHitKeyIndex);
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::mouseMoveEvent(QMouseEvent* event)
{
    OnSetCursor();

    if (!m_pSpline)
    {
        return;
    }

    if (m_bTracking)
    {
        TrackKey(event->pos());
        UpdateToolTip();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::OnLButtonUp([[maybe_unused]] const QPoint& point, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    if (!m_pSpline)
    {
        return;
    }

    if (m_bTracking)
    {
        StopTracking();
    }
}


/////////////////////////////////////////////////////////////////////////////
void CSplineCtrl::SetActiveKey(int nIndex)
{
    ClearSelection();

    // Activate New Key
    if (nIndex >= 0)
    {
        m_bSelectedKeys[nIndex] = true;
    }
    m_nActiveKey = nIndex;
    update();
}

/////////////////////////////////////////////////////////////////////////////
void CSplineCtrl::SetSpline(ISplineInterpolator* pSpline, BOOL bRedraw)
{
    if (pSpline != m_pSpline)
    {
        m_pSpline = pSpline;
    }

    ValidateSpline();
    ClearSelection();

    if (bRedraw)
    {
        update();
    }
}

void CSplineCtrl::ValidateSpline()
{
    // Add initial control points (will be  serialised only if edited).
    if (m_pSpline->GetKeyCount() == 0)
    {
        m_pSpline->InsertKeyFloat(0.f, 1.f);
        m_pSpline->InsertKeyFloat(1.f, 1.f);
        m_pSpline->Update();
    }
}

//////////////////////////////////////////////////////////////////////////
ISplineInterpolator* CSplineCtrl::GetSpline()
{
    return m_pSpline;
}

/////////////////////////////////////////////////////////////////////////////
void CSplineCtrl::OnSetCursor()
{
    const QPoint point = mapFromGlobal(QCursor::pos());

    const int hitKey = m_nHitKeyIndex;

    switch (HitTest(point))
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
        unsetCursor();
        break;
    }

    if (m_bTracking)
    {
        m_nHitKeyIndex = hitKey;
    }

    if (m_pSpline)
    {
        if (m_nHitKeyIndex >= 0)
        {
            UpdateToolTip();
        }
        else if (!m_bTracking)
        {
            QToolTip::hideText();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CSplineCtrl::keyPressEvent(QKeyEvent* event)
{
    bool bProcessed = false;

    if (m_nActiveKey != -1 && m_pSpline)
    {
        switch (event->key())
        {
        case Qt::Key_Space:
        {
            ToggleKeySlope(m_nActiveKey, 0);
            bProcessed = true;
        } break;
        case Qt::Key_Delete:
        {
            RemoveKey(m_nActiveKey);
            bProcessed = true;
        } break;
        case Qt::Key_Up:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.ry() -= 1;
            emit beforeChange();
            TrackKey(point);
            bProcessed = true;
        } break;
        case Qt::Key_Down:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.ry() += 1;
            emit beforeChange();
            TrackKey(point);
            bProcessed = true;
        } break;
        case Qt::Key_Left:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.rx() -= 1;
            emit beforeChange();
            TrackKey(point);
            bProcessed = true;
        } break;
        case Qt::Key_Right:
        {
            CUndo undo("Move Spline Key");
            QPoint point = KeyToPoint(m_nActiveKey);
            point.ry() += 1;
            emit beforeChange();
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
CSplineCtrl::EHitCode CSplineCtrl::HitTest(const QPoint& point)
{
    if (!m_pSpline)
    {
        return HIT_NOTHING;
    }

    float time, val;
    PointToTimeValue(point, time, val);

    m_nHitKeyIndex = -1;
    m_nHitKeyDist = 0xFFFF;
    m_hitCode = HIT_NOTHING;

    QPoint splinePt = TimeToPoint(time);
    if (abs(splinePt.y() - point.y()) < 4)
    {
        m_hitCode = HIT_SPLINE;
        for (int i = 0; i < m_pSpline->GetKeyCount(); i++)
        {
            const QPoint splinePt2 = TimeToPoint(m_pSpline->GetKeyTime(i));
            if (abs(point.x() - splinePt2.x()) < abs(m_nHitKeyDist))
            {
                m_nHitKeyIndex = i;
                m_nHitKeyDist = point.x() - splinePt2.x();
            }
        }
        if (abs(m_nHitKeyDist) < 4)
        {
            m_hitCode = HIT_KEY;
        }
    }

    return m_hitCode;
}

///////////////////////////////////////////////////////////////////////////////
void CSplineCtrl::StartTracking()
{
    m_bTracking = TRUE;

    GetIEditor()->BeginUndo();

    emit beforeChange();

    setCursor(CMFCUtils::LoadCursor(IDC_ARRBLCKCROSS));
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::TrackKey(const QPoint& p)
{
    int nKey = m_nHitKeyIndex;

    QPoint point = p;

    if (nKey >= 0)
    {
        float time, val;

        // Editing time & value.
        Limit(point.rx(), m_rcSpline.left(), m_rcSpline.right());
        Limit(point.ry(), m_rcSpline.top(), m_rcSpline.bottom());
        PointToTimeValue(point, time, val);

        int i;
        for (i = 0; i < m_pSpline->GetKeyCount(); i++)
        {
            // Switch to next key.
            if (fabs(m_pSpline->GetKeyTime(i) - time) < MIN_TIME_EPSILON)
            {
                if (i != nKey)
                {
                    return;
                }
            }
        }

        m_pSpline->SetKeyValueFloat(nKey, val);
        if ((nKey != 0 && nKey != m_pSpline->GetKeyCount() - 1) || !m_bLockFirstLastKey)
        {
            m_pSpline->SetKeyTime(nKey, time);
        }
        else if (m_bLockFirstLastKey)
        {
            int first = 0;
            int last = m_pSpline->GetKeyCount() - 1;
            if (nKey == first)
            {
                m_pSpline->SetKeyValueFloat(last, val);
            }
            else if (nKey == last)
            {
                m_pSpline->SetKeyValueFloat(first, val);
            }
        }

        m_pSpline->Update();
        emit change();
        if (m_updateCallback)
        {
            m_updateCallback(this);
        }

        update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::StopTracking()
{
    if (!m_bTracking)
    {
        return;
    }

    GetIEditor()->AcceptUndo("Spline Move");

    m_bTracking = FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::RemoveKey(int nKey)
{
    if (!m_pSpline)
    {
        return;
    }
    if (nKey)
    {
        if (m_bLockFirstLastKey)
        {
            if (nKey == 0 || nKey == m_pSpline->GetKeyCount() - 1)
            {
                return;
            }
        }
    }

    CUndo undo("Remove Spline Key");

    emit beforeChange();
    m_nActiveKey = -1;
    m_nHitKeyIndex = -1;
    if (m_pSpline)
    {
        m_pSpline->RemoveKey(nKey);
        m_pSpline->Update();
        ValidateSpline();
    }
    emit change();
    if (m_updateCallback)
    {
        m_updateCallback(this);
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
int CSplineCtrl::InsertKey(const QPoint& point)
{
    CUndo undo("Spline Insert Key");

    float time, val;
    PointToTimeValue(point, time, val);

    int i;
    for (i = 0; i < m_pSpline->GetKeyCount(); i++)
    {
        // Skip if any key already have time that is very close.
        if (fabs(m_pSpline->GetKeyTime(i) - time) < MIN_TIME_EPSILON)
        {
            return i;
        }
    }

    emit beforeChange();

    m_pSpline->InsertKeyFloat(time, val);
    m_pSpline->Update();
    ClearSelection();
    update();

    emit change();
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

void CSplineCtrl::ToggleKeySlope(int nIndex, int nDir)
{
    if (nIndex >= 0)
    {
        int flags = m_pSpline->GetKeyFlags(nIndex);
        if (nDir <= 0)
        {
            // Toggle left side.
            flags ^= SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT;
        }
        if (nDir >= 0)
        {
            // Toggle right side.
            flags ^= SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_OUT_SHIFT;
        }
        m_pSpline->SetKeyFlags(nIndex, flags);
        m_pSpline->Update();
        emit change();
        if (m_updateCallback)
        {
            m_updateCallback(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::ClearSelection()
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
void CSplineCtrl::SetTimeMarker(float fTime)
{
    if (!m_pSpline)
    {
        return;
    }

    if (fTime == m_fTimeMarker)
    {
        return;
    }

    // Erase old first.
    QPoint pt0 = TimeToPoint(m_fTimeMarker);
    QPoint pt1 = TimeToPoint(fTime);
    QRect rc(QPoint(pt0.x(), m_rcSpline.top()), QPoint(pt1.x(), m_rcSpline.bottom()));
    rc = rc.normalized();
    rc.adjust(-5, 0, 5, 0);
    rc = rc.intersected(m_rcSpline);

    m_TimeUpdateRect = rc;
    update(rc);

    m_fTimeMarker = fTime;
}

//////////////////////////////////////////////////////////////////////////
void CSplineCtrl::SetTimelineCtrl(TimelineWidget* pTimelineCtrl)
{
    m_pTimelineCtrl = pTimelineCtrl;
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->setParent(this);
    }
}

#include <Controls/moc_SplineCtrl.cpp>
