/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Animation/Controls/UiTimelineCtrl.h>
#include "EditorDefs.h"
//#include "MemDC.h"
#include "ScopedVariableSetter.h"
#include "../GridUtils.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStaticText>

QColor InterpolateColor(const QColor& c1, const QColor& c2, float fraction)
{
    const int r = static_cast<int>(static_cast<float>(c2.red() - c1.red()) * fraction + c1.red());
    const int g = static_cast<int>(static_cast<float>(c2.green() - c1.green()) * fraction + c1.green());
    const int b = static_cast<int>(static_cast<float>(c2.blue() - c1.blue()) * fraction + c1.blue());
    return QColor(r, g, b);
}

//////////////////////////////////////////////////////////////////////////
TimelineWidget::TimelineWidget()
{
    setMouseTracking(true);

    m_timeRange.start = 0;
    m_timeRange.end = 1;

    m_timeScale = 1;
    m_fTicksTextScale = 1.0f;

    m_fTimeMarker = -10;

    m_nTicksStep = 100;

    m_trackingMode = TRACKING_MODE_NONE;

    m_leftOffset = 0;
    m_scrollOffset = 0;

    m_ticksStep = 10.0f;

    m_grid.zoom.SetX(100);

    m_bIgnoreSetTime = false;

    m_pKeyTimeSet = 0;

    m_markerStyle = MARKER_STYLE_SECONDS;
    m_fps = 30.0f;

    m_copyKeyTimes = false;
    m_bTrackingSnapToFrames = false;
}

TimelineWidget::~TimelineWidget()
{
}
/////////////////////////////////////////////////////////////////////////////
// CTimelineCtrl message handlers

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    m_rcTimeline = rect();
    m_grid.rect = m_rcTimeline;
}

//////////////////////////////////////////////////////////////////////////
int TimelineWidget::TimeToClient(float time)
{
    return m_grid.WorldToClient(Vec2(time, 0)).x();
}

//////////////////////////////////////////////////////////////////////////
float TimelineWidget::ClientToTime(int x)
{
    return m_grid.ClientToWorld(QPoint(x, 0)).x;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QRect rcClient = rect();

    {
        //////////////////////////////////////////////////////////////////////////
        // Fill keys background.
        //////////////////////////////////////////////////////////////////////////
        const QRect rc = rcClient.intersected(event->rect());
        painter.fillRect(rc, palette().brush(QPalette::Button));
        painter.drawRect(rc);
        //////////////////////////////////////////////////////////////////////////

        m_grid.CalculateGridLines();

        DrawTicks(&painter);
    }
}

//////////////////////////////////////////////////////////////////////////
float TimelineWidget::SnapTime(float time)
{
    double t = floor((double)time * m_ticksStep + 0.5);
    t = t / m_ticksStep;
    return static_cast<float>(t);
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::DrawTicks(QPainter* painter)
{
    const QRectF rc = rect();

    const QPen pOldPen = painter->pen();

    const QPen ltgray(QColor(110, 110, 110));
    const QPen black(Qt::black);
    const QPen redpen(QColor(255, 0, 255));

    // Draw time ticks every tick step seconds.
    painter->setPen(ltgray);

    switch (m_markerStyle)
    {
    case MARKER_STYLE_SECONDS:
        DrawSecondTicks(painter);
        break;

    case MARKER_STYLE_FRAMES:
        DrawFrameTicks(painter);
        break;
    }

    painter->setPen(redpen);
    int x = TimeToClient(m_fTimeMarker);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRect(QPoint(x - 3, static_cast<int>(rc.top())), QPoint(x + 4, static_cast<int>(rc.bottom()))));

    painter->setPen(redpen);
    painter->drawLine(x, static_cast<int>(rc.top()), x, static_cast<int>(rc.bottom()));
    painter->setBrush(Qt::NoBrush);

    // Draw vertical line showing current time.
    {
        int x2 = TimeToClient(m_fTimeMarker);
        if (x2 > m_rcTimeline.left() && x2 < m_rcTimeline.right())
        {
            painter->setPen(QColor(255, 0, 255));
            painter->drawLine(x2, 0, x2, m_rcTimeline.bottom());
        }
    }

    // Draw the key times.
    painter->setPen(redpen);
    painter->setBrush(Qt::NoBrush);
    const QPen keySelectedPen(QColor(100, 255, 255));
    const QBrush keySelectedBrush(QColor(100, 255, 255));
    for (int keyTimeIndex = 0; m_pKeyTimeSet && keyTimeIndex < m_pKeyTimeSet->GetKeyTimeCount(); ++keyTimeIndex)
    {
        int keyCountBound = __max(m_pKeyTimeSet->GetKeyCountBound(), 1);
        int keyCount = __min(m_pKeyTimeSet->GetKeyCount(keyTimeIndex), keyCountBound);
        float colorCodeFraction = float(keyCount) / keyCountBound;
        const QColor keyMarkerCol = InterpolateColor(Qt::green, Qt::red, colorCodeFraction);

        const QPen keyPen(keyMarkerCol);
        const QBrush keyBrush(keyMarkerCol);
        bool keyTimeSelected = m_pKeyTimeSet && m_pKeyTimeSet->GetKeyTimeSelected(keyTimeIndex);
        painter->setBrush(keyTimeSelected ? keySelectedBrush : keyBrush);
        painter->setPen(keyTimeSelected ? keySelectedPen : keyPen);

        float keyTime = (m_pKeyTimeSet ? m_pKeyTimeSet->GetKeyTime(keyTimeIndex) : 0.0f);

        int x2 = TimeToClient(keyTime);
        painter->drawRect(QRect(QPoint(x2 - 2, static_cast<int>(rc.top())), QPoint(x2 + 3, static_cast<int>(rc.bottom()))));
    }

    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
Range TimelineWidget::GetVisibleRange() const
{
    Range r;

    r.start = (m_scrollOffset - m_leftOffset) / m_timeScale;
    r.end = r.start + (m_rcTimeline.width()) / m_timeScale;
    // Intersect range with global time range.
    r = m_timeRange & r;

    return r;
}


/////////////////////////////////////////////////////////////////////////////
//Mouse Message Handlers
//////////////////////////////////////////////////////////////////////////
void TimelineWidget::mousePressEvent(QMouseEvent* event)
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

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->pos(), event->modifiers());
        break;
    }
}

namespace
{
    const float EDITOR_FPS = 30.0f;
    float SnapTimeToFrame(float time) {return int((time * EDITOR_FPS) + 0.5f) * (1.0f / EDITOR_FPS); }
}

void TimelineWidget::OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    if (m_trackingMode != TRACKING_MODE_NONE)
    {
        return;
    }

    Q_EMIT clicked();

    int hitKeyTimeIndex = HitKeyTimes(point);
    bool autoDeselect = !(modifiers& Qt::ControlModifier) && (m_pKeyTimeSet && ((hitKeyTimeIndex >= 0) ? !m_pKeyTimeSet->GetKeyTimeSelected(hitKeyTimeIndex) : true));
    for (int keyTimeIndex = 0; m_pKeyTimeSet && keyTimeIndex < m_pKeyTimeSet->GetKeyTimeCount(); ++keyTimeIndex)
    {
        bool shouldBeSelected;
        if (keyTimeIndex == hitKeyTimeIndex)
        {
            shouldBeSelected = (modifiers& Qt::ControlModifier) || !((modifiers& Qt::ShiftModifier) && m_pKeyTimeSet->GetKeyTimeSelected(keyTimeIndex));
        }
        else
        {
            shouldBeSelected = (!autoDeselect || (modifiers & Qt::ShiftModifier)) && m_pKeyTimeSet->GetKeyTimeSelected(keyTimeIndex);
        }

        m_pKeyTimeSet->SetKeyTimeSelected(keyTimeIndex, shouldBeSelected);
    }
    TrackingMode trackingMode = (hitKeyTimeIndex >= 0 ? TRACKING_MODE_MOVE_KEYS : TRACKING_MODE_NONE);
    if (trackingMode == TRACKING_MODE_NONE)
    {
        trackingMode = ((modifiers& Qt::ControlModifier) ? TRACKING_MODE_SELECTION_RANGE : TRACKING_MODE_SET_TIME);
    }
    StartTracking(trackingMode);

    switch (m_trackingMode)
    {
    case TRACKING_MODE_SET_TIME:
    {
        if (m_bTrackingSnapToFrames)
        {
            SetTimeMarker(SnapTimeToFrame(ClientToTime(point.x())));
        }
        else
        {
            SetTimeMarker(ClientToTime(point.x()));
        }
        CScopedVariableSetter<bool> ignoreSetTime(m_bIgnoreSetTime, true);
        Q_EMIT startChange();
        Q_EMIT change();
    }
    break;

    case TRACKING_MODE_MOVE_KEYS:
        m_bChangedKeyTimeSet = false;
        m_copyKeyTimes = (modifiers & Qt::ControlModifier ? true : false);
        break;

    case TRACKING_MODE_NONE:
        break;
    }

    m_lastPoint = point;

    update();
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::OnRButtonDown(const QPoint& point, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    Q_EMIT clicked();

    if (m_trackingMode != TRACKING_MODE_NONE)
    {
        return;
    }

    StartTracking(TRACKING_MODE_SET_TIME);

    if (m_bTrackingSnapToFrames)
    {
        SetTimeMarker(SnapTimeToFrame(ClientToTime(point.x())));
    }
    else
    {
        SetTimeMarker(ClientToTime(point.x()));
    }
    CScopedVariableSetter<bool> ignoreSetTime(m_bIgnoreSetTime, true);
    Q_EMIT startChange();
    Q_EMIT change();

    update();
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::OnRButtonUp([[maybe_unused]] const QPoint& point, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    switch (m_trackingMode)
    {
    case TRACKING_MODE_SET_TIME:
    {
        Q_EMIT endChange();
    }
    break;
    }

    if (m_trackingMode != TRACKING_MODE_NONE)
    {
        StopTracking();
    }
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Delete))
    {
        Q_EMIT deleteRequested();
    }
    if (event->key() == Qt::Key_Space && m_playCallback)
    {
        m_playCallback();
    }
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    switch (m_trackingMode)
    {
    case TRACKING_MODE_SET_TIME:
    {
        if (m_bTrackingSnapToFrames)
        {
            SetTimeMarker(SnapTimeToFrame(ClientToTime(event->x())));
        }
        else
        {
            SetTimeMarker(ClientToTime(event->x()));
        }
        CScopedVariableSetter<bool> ignoreSetTime(m_bIgnoreSetTime, true);
        Q_EMIT change();
    }
    break;

    case TRACKING_MODE_MOVE_KEYS:
    {
        if (m_pKeyTimeSet && !m_bChangedKeyTimeSet)
        {
            m_bChangedKeyTimeSet = true;
            m_pKeyTimeSet->BeginEdittingKeyTimes();
        }

        bool altClicked = Qt::AltModifier & QApplication::queryKeyboardModifiers();
        float scale, offset;
        float startTime = ClientToTime(m_lastPoint.x());
        float endTime = ClientToTime(event->x());
        if (altClicked)
        {
            // Alt was pressed, so we should scale the key times rather than translate.
            // Calculate the scaling parameters (ie t1 = t0 * M + C).
            scale = 1.0f;
            if (fabsf(startTime - m_fTimeMarker) > 0.1)
            {
                scale = (endTime - m_fTimeMarker) / (startTime - m_fTimeMarker);
            }
            offset = endTime - startTime * scale;
        }
        else
        {
            // Simply move the keys.
            offset = endTime - startTime;
            scale = 1.0f;
        }

        MoveSelectedKeyTimes(scale, offset);
    }
    break;

    case TRACKING_MODE_SELECTION_RANGE:
    {
        float start = min(ClientToTime(m_lastPoint.x()), ClientToTime(event->x()));
        float end = max(ClientToTime(m_lastPoint.x()), ClientToTime(event->x()));
        SelectKeysInRange(start, end, !(event->modifiers() & Qt::ShiftModifier));
        m_lastPoint = event->pos();
        update();
    }
    break;

    case TRACKING_MODE_NONE:
        break;
    }

    //m_lastPoint = point;
}

//////////////////////////////////////////////////////////////////////////
QString TimelineWidget::TimeToString(float time)
{
    return QString::number(time, 'f', 3);
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::OnLButtonUp([[maybe_unused]] const QPoint& point, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    switch (m_trackingMode)
    {
    case TRACKING_MODE_MOVE_KEYS:
    {
        if (m_pKeyTimeSet && m_bChangedKeyTimeSet)
        {
            m_pKeyTimeSet->EndEdittingKeyTimes();
        }
    }
    break;

    case TRACKING_MODE_SET_TIME:
    {
        Q_EMIT endChange();
    }
    break;

    case TRACKING_MODE_NONE:
        break;
    }

    if (m_trackingMode != TRACKING_MODE_NONE)
    {
        StopTracking();
    }
}

///////////////////////////////////////////////////////////////////////////////
void TimelineWidget::StartTracking(TrackingMode trackingMode)
{
    m_trackingMode = trackingMode;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::StopTracking()
{
    if (!m_trackingMode)
    {
        return;
    }

    m_trackingMode = TRACKING_MODE_NONE;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SetTimeMarker(float fTime)
{
    if (fTime < m_timeRange.start)
    {
        fTime = m_timeRange.start;
    }
    else if (fTime > m_timeRange.end)
    {
        fTime = m_timeRange.end;
    }

    if (fTime == m_fTimeMarker || m_bIgnoreSetTime)
    {
        return;
    }

    int x0 = TimeToClient(m_fTimeMarker);
    int x1 = TimeToClient(fTime);
    QRect rc(QPoint(x0, m_rcClient.top()), QPoint(x1, m_rcClient.bottom()));
    rc = rc.normalized();
    rc.adjust(-5, 0, 5, 0);
    update(rc);

    m_fTimeMarker = fTime;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SetZoom(float fZoom)
{
    m_grid.zoom.SetX(fZoom);
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SetOrigin(float fOffset)
{
    m_grid.origin.SetX(fOffset);
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SetKeyTimeSet(IKeyTimeSet* pKeyTimeSet)
{
    m_pKeyTimeSet = pKeyTimeSet;
}

//////////////////////////////////////////////////////////////////////////
int TimelineWidget::HitKeyTimes(const QPoint& point)
{
    const int threshold = 3;

    int hitKeyTimeIndex = -1;
    for (int keyTimeIndex = 0; m_pKeyTimeSet && keyTimeIndex < m_pKeyTimeSet->GetKeyTimeCount(); ++keyTimeIndex)
    {
        float keyTime = (m_pKeyTimeSet ? m_pKeyTimeSet->GetKeyTime(keyTimeIndex) : 0.0f);
        int x = TimeToClient(keyTime);
        if (abs(point.x() - x) <= threshold)
        {
            hitKeyTimeIndex = keyTimeIndex;
        }
    }

    return hitKeyTimeIndex;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::MoveSelectedKeyTimes(float scale, float offset)
{
    std::vector<int> indices;
    for (int keyTimeIndex = 0; m_pKeyTimeSet && keyTimeIndex < m_pKeyTimeSet->GetKeyTimeCount(); ++keyTimeIndex)
    {
        if (m_pKeyTimeSet && m_pKeyTimeSet->GetKeyTimeSelected(keyTimeIndex))
        {
            indices.push_back(keyTimeIndex);
        }
    }

    if (m_pKeyTimeSet)
    {
        m_pKeyTimeSet->MoveKeyTimes(int(indices.size()), &indices[0], scale, offset, m_copyKeyTimes);
    }
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SelectKeysInRange(float start, float end, bool select)
{
    for (int keyTimeIndex = 0; m_pKeyTimeSet && keyTimeIndex < m_pKeyTimeSet->GetKeyTimeCount(); ++keyTimeIndex)
    {
        float time = (m_pKeyTimeSet ? m_pKeyTimeSet->GetKeyTime(keyTimeIndex) : 0.0f);
        if (m_pKeyTimeSet && time >= start && time <= end)
        {
            m_pKeyTimeSet->SetKeyTimeSelected(keyTimeIndex, select);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SetMarkerStyle(MarkerStyle markerStyle)
{
    m_markerStyle = markerStyle;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::SetFPS(float fps)
{
    m_fps = fps;
}

//////////////////////////////////////////////////////////////////////////
void TimelineWidget::DrawSecondTicks(QPainter* painter)
{
    const QPen ltgray(QColor(110, 110, 110));
    const QPen black(Qt::black);
    const QPen redpen(QColor(255, 0, 255));

    for (int gx = m_grid.firstGridLine.x(); gx < m_grid.firstGridLine.x() + m_grid.numGridLines.x() + 1; gx++)
    {
        painter->setPen(ltgray);

        int x = m_grid.GetGridLineX(gx);
        if (x < 0)
        {
            continue;
        }

        painter->drawLine(m_rcTimeline.left() + x, m_rcTimeline.bottom() - 2, m_rcTimeline.left() + x, m_rcTimeline.bottom() - 4);

        //if (gx % 10 == 0)
        {
            float t = m_grid.GetGridLineXValue(gx);
            t = floor(t * 1000.0f + 0.5f) / 1000.0f;

            const QString str = QString::number(t * m_fTicksTextScale, 'g');

            //t = t / pow(10,precision);
            painter->setPen(black);
            painter->drawLine(m_rcTimeline.left() + x, m_rcTimeline.bottom() - 2, m_rcTimeline.left() + x, m_rcTimeline.bottom() - 14);
            painter->drawText(m_rcTimeline.left() + x + 2, m_rcTimeline.top(), str);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
class TickDrawer
{
public:
    TickDrawer(QPainter* painter, const QRect& rect)
        : rect(rect)
        , painter(painter)
    {
    }

    void operator()(int frameIndex, int x)
    {
        if (painter)
        {
            painter->setPen(QColor(110, 110, 110));
            painter->drawLine(rect.left() + x, rect.bottom() - 2, rect.left() + x, rect.bottom() - 4);

            {
                const QString str = QString::number(frameIndex);

                painter->setPen(Qt::black);
                painter->drawLine(rect.left() + x, rect.bottom() - 2, rect.left() + x, rect.bottom() - 14);
                painter->drawText(rect.left() + x + 2, rect.top(), str);
            }
        }
    }

    QRect rect;
    QPainter* painter;
};

void TimelineWidget::DrawFrameTicks(QPainter* painter)
{
    TickDrawer tickDrawer(painter, m_rcTimeline);
    GridUtils::IterateGrid(tickDrawer, 50.0f, m_grid.zoom.GetX(), m_grid.origin.GetX(), m_fps, m_grid.rect.left(), m_grid.rect.right() + 1);
}

void TimelineWidget::SetPlayCallback(const std::function<void()>& callback)
{
    m_playCallback = callback;
}


#include <Animation/Controls/moc_UiTimelineCtrl.cpp>
