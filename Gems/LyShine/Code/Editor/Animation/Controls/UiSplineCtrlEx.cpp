/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorCommon.h"
#include <Animation/Controls/UiSplineCtrlEx.h>
#include <Editor/Resource.h>
//#include "MemDC.h"
#include <Animation/Controls/UiTimelineCtrl.h>
#include "Clipboard.h"
#include "GridUtils.h"
#include "../UiAnimUndo.h"

#include <QtUtil.h>

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRubberBand>
#include <QScrollBar>
#include <QStaticText>
#include <QToolTip>
#include <QWheelEvent>

#define DEFAULT_MIN_TIME_EPSILON 0.001f
#define MIN_TIME_EPSILON_FOR_SCALING 0.1f

#define ACTIVE_BKG_COLOR      QColor(190, 190, 190)
#define GRID_COLOR            QColor(110, 110, 110)
#define EDIT_SPLINE_COLOR     QColor(128, 255, 128)

#define MIN_PIXEL_PER_GRID_X 50
#define MIN_PIXEL_PER_GRID_Y 10

#define LEFT_BORDER_OFFSET 40

#ifndef NM_CLICK
#define NM_CLICK (-2)
#define NM_RCLICK (-5)
#endif

const float AbstractSplineWidget::threshold = 0.015f;

//////////////////////////////////////////////////////////////////////////

static void SetKeyTangentType(ISplineInterpolator* pSpline, int key, ESplineKeyTangentType type)
{
    int flags = (pSpline->GetKeyFlags(key) & ~SPLINE_KEY_TANGENT_IN_MASK) & ~SPLINE_KEY_TANGENT_OUT_MASK;
    pSpline->SetKeyFlags(key, flags | (type << SPLINE_KEY_TANGENT_IN_SHIFT) | (type << SPLINE_KEY_TANGENT_OUT_SHIFT));
}

//////////////////////////////////////////////////////////////////////////
class CUndoSplineCtrlEx
    : public ISplineCtrlUndo
{
public:
    CUndoSplineCtrlEx(AbstractSplineWidget* pCtrl, std::vector<ISplineInterpolator*>& splineContainer)
    {
        m_pCtrl = FindControl(pCtrl);

        for (size_t splineIndex = 0; splineIndex < splineContainer.size(); ++splineIndex)
        {
            AddSpline(splineContainer[splineIndex]);
        }

        SerializeSplines(&SplineEntry::undo, false);
    }

protected:
    void AddSpline(ISplineInterpolator* pSpline)
    {
        AbstractSplineWidget* pCtrl = FindControl(m_pCtrl);
        m_splineEntries.resize(m_splineEntries.size() + 1);
        SplineEntry& entry = m_splineEntries.back();
        ISplineSet* pSplineSet = (pCtrl ? pCtrl->m_pSplineSet : 0);
        entry.id = (pSplineSet ? pSplineSet->GetIDFromSpline(pSpline) : AZStd::string{});
        entry.pSpline = pSpline;

        const int numKeys = pSpline->GetKeyCount();
        entry.keySelectionFlags.reserve(numKeys);
        for (int i = 0; i < numKeys; ++i)
        {
            entry.keySelectionFlags.push_back(pSpline->GetKeyFlags(i) & ESPLINE_KEY_UI_SELECTED_MASK);
        }
    }

    virtual int GetSize() { return sizeof(*this); }
    virtual const char* GetDescription() { return "UndoSplineCtrlEx"; };

    virtual void Undo(bool bUndo)
    {
        AbstractSplineWidget* pCtrl = FindControl(m_pCtrl);
        if (pCtrl)
        {
            pCtrl->SendNotifyEvent(SPLN_BEFORE_CHANGE);
        }
        if (bUndo)
        {
            SerializeSplines(&SplineEntry::redo, false);
        }
        SerializeSplines(&SplineEntry::undo, true);
        if (pCtrl && bUndo)
        {
            pCtrl->m_bKeyTimesDirty = true;
            pCtrl->SendNotifyEvent(SPLN_CHANGE);
            pCtrl->update();
        }
    }

    virtual void Redo()
    {
        AbstractSplineWidget* pCtrl = FindControl(m_pCtrl);
        if (pCtrl)
        {
            pCtrl->SendNotifyEvent(SPLN_BEFORE_CHANGE);
        }
        SerializeSplines(&SplineEntry::redo, true);
        if (pCtrl)
        {
            pCtrl->m_bKeyTimesDirty = true;
            pCtrl->SendNotifyEvent(SPLN_CHANGE);
            pCtrl->update();
        }
    }

private:
    class SplineEntry
    {
    public:
        std::vector<int> keySelectionFlags;
        _smart_ptr<ISplineBackup> undo;
        _smart_ptr<ISplineBackup> redo;
        AZStd::string id;
        ISplineInterpolator* pSpline;
    };

    void SerializeSplines(_smart_ptr<ISplineBackup> SplineEntry::* backup, bool bLoading)
    {
        AbstractSplineWidget* pCtrl = FindControl(m_pCtrl);
        ISplineSet* pSplineSet = (pCtrl ? pCtrl->m_pSplineSet : 0);
        for (auto it = m_splineEntries.begin(); it != m_splineEntries.end(); ++it)
        {
            SplineEntry& entry = *it;
            ISplineInterpolator* pSpline = (pSplineSet ? pSplineSet->GetSplineFromID(entry.id) : entry.pSpline);

            if (!pSpline && pCtrl && pCtrl->GetSplineCount() > 0)
            {
                pSpline = pCtrl->GetSpline(0);
            }

            if (pSpline && bLoading)
            {
                pSpline->Restore(entry.*backup);
            }
            else if (pSpline)
            {
                (entry.*backup) = pSpline->Backup();
            }
        }
    }

public:
    typedef std::list<AbstractSplineWidget*> CSplineCtrls;

    static AbstractSplineWidget* FindControl(AbstractSplineWidget* pCtrl)
    {
        if (!pCtrl)
        {
            return 0;
        }

        auto iter = std::find(s_activeCtrls.begin(), s_activeCtrls.end(), pCtrl);
        if (iter == s_activeCtrls.end())
        {
            return 0;
        }

        return *iter;
    }

    static void RegisterControl(AbstractSplineWidget* pCtrl)
    {
        if (!FindControl(pCtrl))
        {
            s_activeCtrls.push_back(pCtrl);
        }
    }

    static void UnregisterControl(AbstractSplineWidget* pCtrl)
    {
        if (FindControl(pCtrl))
        {
            s_activeCtrls.remove(pCtrl);
        }
    }

    static CSplineCtrls s_activeCtrls;

    virtual bool IsSelectionChanged() const
    {
        AbstractSplineWidget* pCtrl = FindControl(m_pCtrl);
        ISplineSet* pSplineSet = (pCtrl ? pCtrl->m_pSplineSet : 0);

        for (auto it = m_splineEntries.begin(); it != m_splineEntries.end(); ++it)
        {
            const SplineEntry& entry = *it;
            ISplineInterpolator* pSpline = (pSplineSet ? pSplineSet->GetSplineFromID(entry.id) : entry.pSpline);

            if (!pSpline && pCtrl && pCtrl->GetSplineCount() > 0)
            {
                pSpline = pCtrl->GetSpline(0);
            }

            if (!pSpline)
            {
                return false;
            }

            if (pSpline->GetKeyCount() != entry.keySelectionFlags.size())
            {
                return true;
            }

            for (int i = 0; i < pSpline->GetKeyCount(); ++i)
            {
                if (entry.keySelectionFlags[i] != (pSpline->GetKeyFlags(i) & ESPLINE_KEY_UI_SELECTED_MASK))
                {
                    return true;
                }
            }
        }

        return false;
    }

private:
    AbstractSplineWidget* m_pCtrl;
    std::vector<SplineEntry> m_splineEntries;
    std::vector<float> m_keyTimes;
};

CUndoSplineCtrlEx::CSplineCtrls CUndoSplineCtrlEx::s_activeCtrls;

SplineWidget::SplineWidget(QWidget* parent)
    : QWidget(parent)
    , m_rubberBand(new QRubberBand(QRubberBand::Rectangle, this))
{
    m_rubberBand->setVisible(false);
    setMouseTracking(true);
}

SplineWidget::~SplineWidget()
{
}

//////////////////////////////////////////////////////////////////////////
AbstractSplineWidget::AbstractSplineWidget()
    : m_defaultKeyTangentType(SPLINE_KEY_TANGENT_NONE)
{
    m_pTimelineCtrl = 0;

    m_totalSplineCount = 0;
    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;
    m_nHitDimension = -1;
    m_bHitIncomingHandle = true;
    m_nKeyDrawRadius = 3;
    m_gridX = 10;
    m_gridY = 10;
    m_timeRange.start = 0;
    m_timeRange.end = 1;
    m_fMinValue = -1;
    m_fMaxValue = 1;
    m_valueRange.Set(-1, 1);
    m_fTooltipScaleX = 1;
    m_fTooltipScaleY = 1;

    m_cMousePos = QPoint(0, 0);
    m_fTimeScale = 1;
    m_fValueScale = 1;
    m_fGridTimeScale = 30.0f;

    m_ticksStep = 10;

    m_fTimeMarker = -10;
    m_editMode = NothingMode;

    m_bSnapTime = false;
    m_bSnapValue = false;
    m_bBitmapValid = false;

    m_nLeftOffset = LEFT_BORDER_OFFSET;

    m_grid.zoom.x = 200;
    m_grid.zoom.y = 100;

    m_bKeyTimesDirty = false;

    m_rcSelect = QRect();
    m_rcSpline = QRect(QPoint(0, 0), QPoint(0, 0));

    m_boLeftMouseButtonDown = false;

    m_pSplineSet = 0;

    m_controlAmplitude = false;

    m_fMinTimeEpsilon = DEFAULT_MIN_TIME_EPSILON;

    m_defaultValueRange.Set(-1.1f, 1.1f);

    m_bEditLock = false;

    m_pCurrentUndo = nullptr;

    CUndoSplineCtrlEx::RegisterControl(this);
}


//////////////////////////////////////////////////////////////////////////
AbstractSplineWidget::~AbstractSplineWidget()
{
    CUndoSplineCtrlEx::UnregisterControl(this);
}


//////////////////////////////////////////////////////////////////////////
Vec2 AbstractSplineWidget::GetZoom()
{
    return m_grid.zoom;
}

Vec2 AbstractSplineWidget::GetScrollOffset()
{
    return m_grid.origin;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetZoom(Vec2 zoom, const QPoint& center)
{
    m_grid.SetZoom(zoom, QPoint(center.x(), m_rcSpline.bottom() + 1 - center.y()));
    SetScrollOffset(m_grid.origin);
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->setZoom(zoom.x, m_grid.origin.x);
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetZoom(Vec2 zoom)
{
    m_grid.zoom = zoom;
    SetScrollOffset(m_grid.origin);
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->setZoom(zoom.x, m_grid.origin.x);
    }
    SendNotifyEvent(SPLN_SCROLL_ZOOM);
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetScrollOffset(Vec2 ofs)
{
    m_grid.origin = ofs;
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->setZoom(m_grid.zoom.x, m_grid.origin.x);
    }
    SendNotifyEvent(SPLN_SCROLL_ZOOM);
    update();
}

//////////////////////////////////////////////////////////////////////////
float AbstractSplineWidget::SnapTime(float time)
{
    if (m_bSnapTime)
    {
        float step = m_grid.step.x / 10.0f;
        return floor((time / step) + 0.5f) * step;
    }
    return time;
}

//////////////////////////////////////////////////////////////////////////
float AbstractSplineWidget::SnapValue(float val)
{
    if (m_bSnapValue)
    {
        float step = m_grid.step.y;
        return floor((val / step) + 0.5f) * step;
    }
    return val;
}

//////////////////////////////////////////////////////////////////////////
int AbstractSplineWidget::GetKeyTimeCount() const
{
    UpdateKeyTimes();

    return int(m_keyTimes.size());
}

//////////////////////////////////////////////////////////////////////////
float AbstractSplineWidget::GetKeyTime(int index) const
{
    UpdateKeyTimes();

    return m_keyTimes[index].time;
}

//////////////////////////////////////////////////////////////////////////
bool AbstractSplineWidget::GetKeyTimeSelected(int index) const
{
    UpdateKeyTimes();

    return m_keyTimes[index].selected;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetKeyTimeSelected(int index, bool selected)
{
    m_keyTimes[index].selected = selected;
}

//////////////////////////////////////////////////////////////////////////
int AbstractSplineWidget::GetKeyCount(int index) const
{
    UpdateKeyTimes();

    return m_keyTimes[index].count;
}

//////////////////////////////////////////////////////////////////////////
int AbstractSplineWidget::GetKeyCountBound() const
{
    UpdateKeyTimes();

    return m_totalSplineCount;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::BeginEdittingKeyTimes()
{
    if (UiAnimUndo::IsRecording())
    {
        UiAnimUndoManager::Get()->Cancel();
        m_pCurrentUndo = nullptr;
    }
    UiAnimUndoManager::Get()->Begin();

    for (int keyTimeIndex = 0; keyTimeIndex < int(m_keyTimes.size()); ++keyTimeIndex)
    {
        m_keyTimes[keyTimeIndex].oldTime = m_keyTimes[keyTimeIndex].time;
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::EndEdittingKeyTimes()
{
    if (UiAnimUndo::IsRecording())
    {
        UiAnimUndoManager::Get()->Accept("Batch key move");
        m_pCurrentUndo = nullptr;
    }

    m_bKeyTimesDirty = true;
    update();
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::MoveKeyTimes(int numChanges, int* indices, float scale, float offset, bool copyKeys)
{
    if (UiAnimUndo::IsRecording())
    {
        UiAnimUndoManager::Get()->Restore();

        std::vector<ISplineInterpolator*> splines;
        for (int splineIndex = 0; splineIndex < int(m_splines.size()); ++splineIndex)
        {
            ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
            if (pSpline)
            {
                splines.push_back(pSpline);
            }
        }
        UiAnimUndo::Record(m_pCurrentUndo = CreateSplineCtrlUndoObject(splines));

        for (int keyTimeIndex = 0; keyTimeIndex < int(m_keyTimes.size()); ++keyTimeIndex)
        {
            m_keyTimes[keyTimeIndex].time = m_keyTimes[keyTimeIndex].oldTime;
        }
    }

    class KeyChange
    {
    public:
        KeyChange(ISplineInterpolator* pSpline, int keyIndex, float oldTime, float newTime, int flags)
            : pSpline(pSpline)
            , keyIndex(keyIndex)
            , oldTime(oldTime)
            , newTime(newTime)
            , flags(flags) {}

        ISplineInterpolator* pSpline;
        int keyIndex;
        float oldTime;
        float newTime;
        ISplineInterpolator::ValueType value;
        int flags;
        ISplineInterpolator::ValueType tin, tout;
    };

    std::vector<KeyChange> individualKeyChanges;
    for (int changeIndex = 0; indices && changeIndex < numChanges; ++changeIndex)
    {
        int index = (indices ? indices[changeIndex] : 0);

        float oldTime = m_keyTimes[index].time;
        float time = __max(m_timeRange.start, __min(m_timeRange.end, scale * oldTime + offset));

        for (int splineIndex = 0; splineIndex < int(m_splines.size()); ++splineIndex)
        {
            ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

            for (int keyIndex = 0; pSpline && keyIndex < pSpline->GetKeyCount(); ++keyIndex)
            {
                float keyTime = pSpline->GetKeyTime(keyIndex);
                KeyChange change(pSpline, keyIndex, keyTime, SnapTimeToGridVertical(time), pSpline->GetKeyFlags(keyIndex));

                pSpline->GetKeyValue(keyIndex, change.value);
                pSpline->GetKeyTangents(keyIndex, change.tin, change.tout);

                if (fabsf(keyTime - oldTime) < threshold)
                {
                    individualKeyChanges.push_back(change);
                }
            }
        }

        m_keyTimes[index].time = SnapTimeToGridVertical(time);
    }

    for (std::vector<KeyChange>::iterator itChange = individualKeyChanges.begin(); itChange != individualKeyChanges.end(); ++itChange)
    {
        (*itChange).pSpline->SetKeyTime((*itChange).keyIndex, (*itChange).newTime);
    }

    if (copyKeys)
    {
        for (std::vector<KeyChange>::iterator keyToAdd = individualKeyChanges.begin(), endKeysToAdd = individualKeyChanges.end(); keyToAdd != endKeysToAdd; ++keyToAdd)
        {
            int keyIndex = (*keyToAdd).pSpline->InsertKey((*keyToAdd).oldTime, (*keyToAdd).value);
            (*keyToAdd).pSpline->SetKeyTangents(keyIndex, (*keyToAdd).tin, (*keyToAdd).tout);
            (*keyToAdd).pSpline->SetKeyFlags(keyIndex, (*keyToAdd).flags & (~ESPLINE_KEY_UI_SELECTED_MASK));
        }
    }

    // Loop through all moved keys, checking whether there are multiple keys on the same frame.
    for (int splineIndex = 0; splineIndex < int(m_splines.size()); ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        float lastKeyTime = -FLT_MAX;
        pSpline->Update();
        for (int keyIndex = 0, keys = pSpline->GetKeyCount(); keyIndex <= keys; )
        {
            float keyTime = pSpline->GetKeyTime(keyIndex);
            if (fabsf(keyTime - lastKeyTime) < m_fMinTimeEpsilon)
            {
                --keys;
                pSpline->RemoveKey(keyIndex);
            }
            else
            {
                ++keyIndex;
                lastKeyTime = keyTime;
            }
        }
    }

    SendNotifyEvent(SPLN_CHANGE);
    update();
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::resizeEvent(QResizeEvent* event)
{
    QRect oldRect = m_rcSpline;

    QWidget::resizeEvent(event);

    m_rcClient = rect();
    m_rcSpline = m_rcClient;

    if (m_pTimelineCtrl)
    {
        QRect rct = m_rcSpline;
        rct.setBottom(rct.top() + 16);
        m_rcSpline.setTop(rct.bottom() + 1);
        rct.setLeft(rct.left() + m_nLeftOffset);
        m_pTimelineCtrl->setGeometry(rct);
    }

    m_rcSpline.setLeft(m_rcSpline.left() + m_nLeftOffset);

    m_grid.rect = m_rcSpline;

    int oldW = oldRect.width();
    int oldH = oldRect.height();

    if (width() > 1 && height() > 1 && oldW > 1 && oldH > 1 && m_rcSpline.width() > 0 && m_rcSpline.height())
    {
        SetZoom(Vec2(float(m_rcSpline.width()) / oldW * GetZoom().x, float(m_rcSpline.height()) / oldH * GetZoom().y));
    }
}

//////////////////////////////////////////////////////////////////////////
QPoint AbstractSplineWidget::TimeToPoint(float time, ISplineInterpolator* pSpline)
{
    float val = 0;
    if (pSpline)
    {
        pSpline->InterpolateFloat(time, val);
    }

    return WorldToClient(Vec2(time, val));
    ;
}

//////////////////////////////////////////////////////////////////////////
float AbstractSplineWidget::TimeToXOfs(float x)
{
    return aznumeric_cast<float>(WorldToClient(Vec2(float(x), 0.0f)).x());
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::PointToTimeValue(QPoint point, float& time, float& value)
{
    Vec2 v = ClientToWorld(point);
    value = v.y;
    time = XOfsToTime(point.x());
}

//////////////////////////////////////////////////////////////////////////
float AbstractSplineWidget::XOfsToTime(int x)
{
    Vec2 v = ClientToWorld(QPoint(x, 0));
    float time = v.x;
    return time;
}

//////////////////////////////////////////////////////////////////////////
QPoint AbstractSplineWidget::XOfsToPoint(int x, ISplineInterpolator* pSpline)
{
    return TimeToPoint(XOfsToTime(x), pSpline);
}

//////////////////////////////////////////////////////////////////////////
QPoint AbstractSplineWidget::WorldToClient(Vec2 v)
{
    QPoint p = m_grid.WorldToClient(v);
    p.setY(m_rcSpline.bottom() - p.y());
    return p;
}

//////////////////////////////////////////////////////////////////////////
Vec2 AbstractSplineWidget::ClientToWorld(const QPoint& point)
{
    Vec2 v = m_grid.ClientToWorld(QPoint(point.x(), m_rcSpline.bottom() - point.y()));
    return v;
}

void SplineWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (m_TimeUpdateRect != event->rect())
    {
        painter.fillRect(event->rect(), QColor(160, 160, 160));

        m_grid.CalculateGridLines();

        //Draw Grid
        DrawGrid(&painter);

        const QRect drawSplineRect = event->rect().intersected(m_rcSpline);

        // Calculate the times corresponding to the left and right of the area to be painted -
        // we can use this to draw only the necessary parts of the splines.
        float startTime = XOfsToTime(drawSplineRect.left());
        float endTime = XOfsToTime(drawSplineRect.right());

        //Draw Keys and Curve
        for (int i = 0; i < int(m_splines.size()); ++i)
        {
            DrawSpline(&painter, m_splines[i], startTime, endTime);
            DrawKeys(&painter, i, startTime, endTime);
        }
    }
    m_TimeUpdateRect = QRect();

    DrawTimeMarker(&painter);
}

/////////////////////////////////////////////////////////////////////////
class SplineControlVerticalLineDrawer
{
public:
    SplineControlVerticalLineDrawer(QPainter* painter, const QRect& rect)
        : rect(rect)
        , painter(painter)
    {
    }

    void operator()([[maybe_unused]] int frameIndex, int x)
    {
        if (painter)
        {
            painter->drawLine(x, rect.top(), x, rect.bottom());
        }
    }

    QPainter* painter;
    QRect rect;
};

void SplineWidget::DrawGrid(QPainter* painter)
{
    QPoint ptTop = WorldToClient(Vec2(0.0f, m_valueRange.end));
    QPoint ptBottom = WorldToClient(Vec2(0.0f, m_valueRange.start));
    QPoint pt0 = WorldToClient(Vec2(m_timeRange.start, 0));
    QPoint pt1 = WorldToClient(Vec2(m_timeRange.end, 0));
    QRect timeRc = QRect(QPoint(pt0.x() - 2, ptTop.y()), QPoint(pt1.x() + 2, ptBottom.y()));
    timeRc = timeRc.intersected(m_rcSpline);
    painter->fillRect(timeRc, ACTIVE_BKG_COLOR);

    //////////////////////////////////////////////////////////////////////////
    QPen pOldPen = painter->pen();
    painter->setPen(GRID_COLOR);

    /// Draw Left Separator.
    painter->fillRect(QRect(QPoint(m_rcClient.left(), m_rcClient.top()), QPoint(m_rcClient.left() + m_nLeftOffset - 1, m_rcClient.bottom())), ACTIVE_BKG_COLOR);
    painter->drawLine(m_rcClient.left() + m_nLeftOffset, m_rcClient.bottom(), m_rcClient.left() + m_nLeftOffset, m_rcClient.top());
    //////////////////////////////////////////////////////////////////////////

    int gy;

    QPen pen(QColor(GRID_COLOR), 1, Qt::DotLine);
    pen.setCosmetic(true);
    painter->setPen(pen);

    //if (m_grid.pixelsPerGrid.y >= MIN_PIXEL_PER_GRID_Y)
    {
        // Draw horizontal grid lines.
        for (gy = m_grid.firstGridLine.y(); gy < m_grid.firstGridLine.y() + m_grid.numGridLines.y() + 1; gy++)
        {
            int y = m_grid.GetGridLineY(gy);
            if (y < 0)
            {
                continue;
            }
            int py = m_rcSpline.bottom() - (m_rcSpline.top() + y);
            if (py < m_rcSpline.top() || py > m_rcSpline.bottom())
            {
                continue;
            }
            painter->setPen(pen);
            painter->drawLine(m_rcSpline.left(), py, m_rcSpline.right(), py);

            float v = m_grid.GetGridLineYValue(gy);
            v = floor(v * 1000.0f + 0.5f) / 1000.0f;

            if ((v >= m_valueRange.start && v <= m_valueRange.end) || fabs(v - m_valueRange.start) < 0.01f || fabs(v - m_valueRange.end) < 0.01f)
            {
                painter->setPen(Qt::black);
                painter->drawText(m_rcClient.left() + 2, py - 8, QString::number(v));
            }
        }
    }

    // Draw vertical grid lines.
    SplineControlVerticalLineDrawer verticalLineDrawer(painter, m_rcSpline);
    GridUtils::IterateGrid(verticalLineDrawer, 50.0f, m_grid.zoom.x, m_grid.origin.x, m_fGridTimeScale, m_grid.rect.left(), m_grid.rect.right() + 1);

    //////////////////////////////////////////////////////////////////////////
    {
        const QPen pen0(QColor(110, 100, 100), 2);
        const QPoint p = WorldToClient(Vec2(0, 0));

        painter->setPen(pen0);

        /// Draw X axis.
        painter->drawLine(m_rcSpline.left(), p.y(), m_rcSpline.right(), p.y());

        // Draw Y Axis.
        if (p.x() > m_rcSpline.left() && p.y() < m_rcSpline.right())
        {
            painter->drawLine(p.x(), m_rcSpline.top(), p.x(), m_rcSpline.bottom());
        }
    }
    //////////////////////////////////////////////////////////////////////////

    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::DrawSpline(QPainter* painter, SSplineInfo& splineInfo, float startTime, float endTime)
{
    const QPen pOldPen = painter->pen();

    //////////////////////////////////////////////////////////////////////////
    ISplineInterpolator* pSpline = splineInfo.pSpline;
    ISplineInterpolator* pDetailSpline = splineInfo.pDetailSpline;
    if (!pSpline)
    {
        return;
    }

    int nTotalNumberOfDimensions(0);
    int nCurrentDimension(0);

    int left = aznumeric_cast<int>(TimeToXOfs(startTime));
    int right = aznumeric_cast<int>(TimeToXOfs(endTime));
    QPoint p0 = TimeToPoint(pSpline->GetKeyTime(0), pSpline);
    QPoint p1 = TimeToPoint(pSpline->GetKeyTime(pSpline->GetKeyCount() - 1), pSpline);

    nTotalNumberOfDimensions = pSpline->GetNumDimensions();
    for (nCurrentDimension = 0; nCurrentDimension < nTotalNumberOfDimensions; nCurrentDimension++)
    {
        QColor splineColor = EDIT_SPLINE_COLOR;
        splineColor = splineInfo.anColorArray[nCurrentDimension];
        const QPen pen(splineColor, 2);

        if (p0.x() > left && !pDetailSpline)
        {
            QPen alternatePen(splineColor, 1, Qt::DotLine);
            alternatePen.setCosmetic(true);
            painter->setPen(alternatePen);

            painter->drawLine(m_rcSpline.left(), p0.y(), p0.x(), p0.y());
            left = p0.x();
        }

        if (p1.x() < right && !pDetailSpline)
        {
            QPen alternatePen(splineColor, 1, Qt::DotLine);
            alternatePen.setCosmetic(true);
            painter->setPen(alternatePen);

            painter->drawLine(p1.x(), p1.y(), m_rcSpline.right(), p1.y());
            right = p1.x();
        }

        painter->setPen(pen);

        int linesDrawn = 0;
        int pixels = 0;

        float gradient = 0.0f;
        int pointsInLine = -1;
        QPoint lineStart;
        QPainterPath path;
        for (int x = left; x <= right; x++)
        {
            ++pixels;

            float time = XOfsToTime(x);
            ISplineInterpolator::ValueType value;
            ISplineInterpolator::ZeroValue(value);

            pSpline->Interpolate(time, value);

            if (pDetailSpline)
            {
                ISplineInterpolator::ValueType value2;
                ISplineInterpolator::ZeroValue(value2);

                pDetailSpline->Interpolate(time, value2);

                value[nCurrentDimension] = value[nCurrentDimension] + value2[nCurrentDimension];
            }


            QPoint pt = WorldToClient(Vec2(time, value[nCurrentDimension]));

            if ((x == right && pointsInLine >= 0) || (pointsInLine > 0 && fabs(lineStart.y() + gradient * (pt.x() - lineStart.x()) - pt.y()) > 1.0f))
            {
                lineStart = QPoint(pt.x() - 1, aznumeric_cast<int>(lineStart.y() + gradient * (pt.x() - 1 - lineStart.x())));
                path.lineTo(lineStart);
                gradient = float(pt.y() - lineStart.y()) / (pt.x() - lineStart.x());
                pointsInLine = 1;
                ++linesDrawn;
            }
            else if ((x == right && pointsInLine >= 0) || (pointsInLine > 0 && fabs(lineStart.y() + gradient * (pt.x() - lineStart.x()) - pt.y()) == 1.0f))
            {
                lineStart = pt;
                path.lineTo(lineStart);
                gradient = 0.0f;
                pointsInLine = 0;
                ++linesDrawn;
            }
            else if (pointsInLine > 0)
            {
                ++pointsInLine;
            }
            else if (pointsInLine == 0)
            {
                gradient = float(pt.y() - lineStart.y()) / (pt.x() - lineStart.x());
                ++pointsInLine;
            }
            else
            {
                path.moveTo(pt);
                lineStart = pt;
                ++pointsInLine;
                gradient = 0.0f;
            }
        }

        painter->drawPath(path);

        // Put back the old objects
        painter->setPen(pOldPen);
    }
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::DrawKeys(QPainter* painter, int splineIndex, [[maybe_unused]] float startTime, float endTime)
{
    SSplineInfo& splineInfo = m_splines[splineIndex];
    ISplineInterpolator* pSpline = splineInfo.pSpline;
    ISplineInterpolator* pDetailSpline = splineInfo.pDetailSpline;

    if (!pSpline)
    {
        return;
    }

    // create and select a white pen // KDAB: white?
    const QPen pOldPen = painter->pen();
    painter->setPen(Qt::black);

    int i;

    int nTotalNumberOfDimensions(0);
    int nCurrentDimension(0);

    nTotalNumberOfDimensions = pSpline->GetNumDimensions();
    for (nCurrentDimension = 0; nCurrentDimension < nTotalNumberOfDimensions; nCurrentDimension++)
    {
        // Why is this here? Not even god knows...
        //for (i = 0; i < pSpline->GetKeyCount() && pSpline->GetKeyTime(i) < startTime; ++i);
        int lastKeyX2 = m_rcSpline.left() - 100;

        int numKeys = pSpline->GetKeyCount();
        for (i = 0; i < numKeys; i++)
        {
            float time = pSpline->GetKeyTime(i);
            if (time >= endTime)
            {
                break;
            }

            ISplineInterpolator::ValueType value;
            ISplineInterpolator::ZeroValue(value);

            pSpline->Interpolate(time, value);

            if (pDetailSpline)
            {
                ISplineInterpolator::ValueType value2;
                ISplineInterpolator::ZeroValue(value2);

                pDetailSpline->Interpolate(time, value2);

                value[nCurrentDimension] = value[nCurrentDimension] + value2[nCurrentDimension];
            }
            QPoint pt = WorldToClient(Vec2(time, value[nCurrentDimension]));
            ;

            if (pt.x() < m_rcSpline.left())
            {
                continue;
            }

            if (abs(pt.x() - lastKeyX2) < 4)
            {
                continue;
            }

            QColor clr(220, 220, 0);
            if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
            {
                clr = Qt::red;
                DrawTangentHandle(painter, splineIndex, i, nCurrentDimension);
            }

            const QBrush brush(clr);
            const QBrush pOldBrush = painter->brush();
            painter->setBrush(brush);

            // Draw this key.
            painter->drawRect(QRect(QPoint(pt.x() - m_nKeyDrawRadius, pt.y() - m_nKeyDrawRadius), QPoint(pt.x() + m_nKeyDrawRadius - 1, pt.y() + m_nKeyDrawRadius - 1)));

            lastKeyX2 = pt.x();

            painter->setBrush(pOldBrush);
        }
    }

    painter->setPen(pOldPen);
}

bool AbstractSplineWidget::GetTangentHandlePts(QPoint&, QPoint&, QPoint&, int, int, int)
{
    return false;
}

void SplineWidget::DrawTangentHandle(QPainter* painter, int nSpline, int nKey, int nDimension)
{
    // create and select a white pen
    const QPen pOldPen = painter->pen();
    painter->setPen(QColor(96, 96, 96));

    // Draw in-tangent & out-tangent lines.
    QPoint a, b, pt;
    if (GetTangentHandlePts(a, pt, b, nSpline, nKey, nDimension))
    {
        painter->drawLine(a, pt);
        painter->drawLine(pt, b);

        // Draw end-effectors.
        const QBrush pOldBrush = painter->brush();
        painter->setBrush(QColor(0, 220, 0));

        painter->drawRect(QRect(QPoint(a.x() - m_nKeyDrawRadius, a.y() - m_nKeyDrawRadius), QPoint(a.x() + m_nKeyDrawRadius - 1, a.y() + m_nKeyDrawRadius - 1)));
        painter->drawRect(QRect(QPoint(b.x() - m_nKeyDrawRadius, b.y() - m_nKeyDrawRadius), QPoint(b.x() + m_nKeyDrawRadius - 1, b.y() + m_nKeyDrawRadius - 1)));

        painter->setBrush(pOldBrush);
    }

    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::DrawTimeMarker(QPainter* painter)
{
    const QPen pOldPen = painter->pen();
    painter->setPen(QColor(255, 0, 255));
    int x = aznumeric_cast<int>(TimeToXOfs(m_fTimeMarker));
    if (x >= m_rcSpline.left() && x <= m_rcSpline.right())
    {
        painter->drawLine(x, m_rcSpline.top(), x, m_rcSpline.bottom());
    }
    painter->setPen(pOldPen);
}

/////////////////////////////////////////////////////////////////////////////
//Mouse Message Handlers
//////////////////////////////////////////////////////////////////////////
void SplineWidget::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->pos(), event->modifiers());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->pos(), event->modifiers());
        break;
    }
}

void SplineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->pos(), event->modifiers());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->pos(), event->modifiers());
        break;
    }
}

void SplineWidget::OnLButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_pCurrentUndo = nullptr;

    if (m_bEditLock)
    {
        return;
    }

    m_boLeftMouseButtonDown = true;

    if (m_editMode == TrackingMode)
    {
        return;
    }

    SendNotifyEvent(NM_CLICK);

    m_cMouseDownPos = point;

    ISplineInterpolator* pSpline = HitSpline(m_cMouseDownPos);

    // Get control key status.
    bool bCtrlClick = modifiers & Qt::ControlModifier;

    switch (m_hitCode)
    {
    case HIT_KEY:
    {
        {
            UiAnimUndo undo("Select Spline Key");
            StoreUndo();

            SendNotifyEvent(SPLN_BEFORE_CHANGE);
            bool bHitSelection = IsKeySelected(m_pHitSpline, m_nHitKeyIndex, m_nHitDimension);
            bool bAddSelect = bCtrlClick;
            if (!bAddSelect && !bHitSelection)
            {
                ClearSelection();
            }
            SelectKey(pSpline, m_nHitKeyIndex, m_nHitDimension, true);
            SendNotifyEvent(SPLN_CHANGE);

            if (m_pCurrentUndo && !m_pCurrentUndo->IsSelectionChanged())
            {
                undo.Cancel();
            }

            m_pCurrentUndo = nullptr;
        }

        UiAnimUndoManager::Get()->Begin();
        StartTracking(bCtrlClick);
    }
    break;

    case HIT_TANGENT_HANDLE:
    {
        {
            UiAnimUndo undo("Select Tangent Handle");
            SendNotifyEvent(SPLN_BEFORE_CHANGE);
            ClearSelection();
            SelectKey(pSpline, m_nHitKeyIndex, m_nHitDimension, true);
            SendNotifyEvent(SPLN_CHANGE);
        }
        StartTracking(false);
    }
    break;

    case HIT_SPLINE:
    {
        if (GetNumSelected() > 0)
        {
            StartTracking(bCtrlClick);
        }
    }
    break;

    case HIT_TIMEMARKER:
    {
        SendNotifyEvent(SPLN_TIME_START_CHANGE);
        m_editMode = TimeMarkerMode;
        SetCapture();
    }
    break;

    case HIT_NOTHING:
    {
        if (m_rcSpline.contains(point))
        {
            UiAnimUndoManager::Get()->Begin();
            StoreUndo();
            m_rcSelect = QRect();
            m_rubberBand->setVisible(false);
            m_editMode = SelectMode;
            SetCapture();
        }
    }
    break;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::OnRButtonDown(const QPoint&, Qt::KeyboardModifiers)
{
    m_pCurrentUndo = nullptr;

    SendNotifyEvent(NM_RCLICK);
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::OnMButtonDown(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    m_pCurrentUndo = nullptr;

    bool bShiftClick = modifiers & Qt::ShiftModifier;

    if (m_editMode == NothingMode)
    {
        if (bShiftClick)
        {
            m_editMode = ZoomMode;
            setCursor(Qt::SizeAllCursor);
        }
        else
        {
            setCursor(Qt::SizeAllCursor);
            m_editMode = ScrollMode;
        }
        m_cMouseDownPos = point;
    }
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::OnMButtonUp(const QPoint&, Qt::KeyboardModifiers)
{
    if (m_editMode == ScrollMode || m_editMode == ZoomMode)
    {
        m_editMode = NothingMode;
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    const QPoint point = event->pos();
    m_pCurrentUndo = nullptr;

    if (m_bEditLock)
    {
        return;
    }

    switch (m_hitCode)
    {
    case HIT_SPLINE:
    {
        if (m_pHitSpline)
        {
            InsertKey(m_pHitSpline, m_pHitDetailSpline, point);
        }
        update();
        if (m_pTimelineCtrl)
        {
            m_pTimelineCtrl->update();
        }
    }
    break;
    case HIT_KEY:
    {
        RemoveKey(m_pHitSpline, m_nHitKeyIndex);
    }
    break;
    case HIT_TANGENT_HANDLE:
    {
        if (m_bHitIncomingHandle)
        {
            ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_IN_MASK, 0);
        }
        else
        {
            ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_OUT_MASK, 0);
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint point = event->pos();

    switch (HitTest(point))
    {
    case HIT_SPLINE:
        setCursor(CMFCUtils::LoadCursor(IDC_ARRWHITE));
        break;
    case HIT_KEY:
    case HIT_TANGENT_HANDLE:
        setCursor(CMFCUtils::LoadCursor(IDC_ARRBLCK));
        break;
    default:
        break;
    }

    if (m_pHitSpline && m_nHitKeyIndex >= 0)
    {
        float time = m_pHitSpline->GetKeyTime(m_nHitKeyIndex);
        ISplineInterpolator::ValueType  afValue;
        m_pHitSpline->GetKeyValue(m_nHitKeyIndex, afValue);
        const QString tipText = QStringLiteral("t=%1  v=%2").arg(time * m_fTooltipScaleX, 0, 'f', 3).arg(afValue[m_nHitDimension] * m_fTooltipScaleY, 2, 'f', 3);

        m_tooltipText = tipText;
        if (m_lastToolTipPos != point)
        {
            m_lastToolTipPos = point;
            QToolTip::showText(point, tipText);
        }
    }
    else if (m_editMode != TrackingMode)
    {
        if (!m_tooltipText.isEmpty())
        {
            QToolTip::hideText();
        }
    }

    if (m_bEditLock)
    {
        return;
    }

    m_cMousePos = event->pos();

    if (m_editMode == SelectMode)
    {
        setCursor(Qt::BlankCursor);
        QRect rc(QPoint(m_cMouseDownPos.x(), m_cMouseDownPos.y()), point);
        rc = rc.normalized().intersected(m_rcSpline);

        m_rcSelect = rc;
        m_rubberBand->setGeometry(m_rcSelect);
        m_rubberBand->setVisible(true);
    }

    if (m_editMode == TimeMarkerMode)
    {
        setCursor(Qt::BlankCursor);
        SetTimeMarker(XOfsToTime(event->x()));
        SendNotifyEvent(SPLN_TIME_CHANGE);
    }

    if (m_boLeftMouseButtonDown)
    {
        if (m_editMode == TrackingMode && event->pos() != m_cMouseDownPos)
        {
            m_startedDragging = true;
            UiAnimUndoManager::Get()->Restore();
            m_pCurrentUndo = nullptr;

            StoreUndo();

            bool bAltClick = event->modifiers() & Qt::AltModifier;

            Vec2 v0 = ClientToWorld(m_cMouseDownPos);
            Vec2 v1 = ClientToWorld(event->pos());

            if (bAltClick)
            {
                TimeScaleKeys(m_fTimeMarker, v0.x, v1.x);
            }
            else if (m_controlAmplitude)
            {
                ScaleAmplitudeKeys(v0.x, v0.y, v1.y - v0.y);
            }
            else
            {
                MoveSelectedKeys(v1 - v0, m_copyKeys);
            }
        }
    }

    if (m_editMode == TrackingMode && GetNumSelected() == 1)
    {
        float time = 0;
        ISplineInterpolator::ValueType  afValue;
        QString                         tipText;
        bool                            boFoundTheSelectedKey(false);

        for (int splineIndex = 0, endSpline = static_cast<int>(m_splines.size()); splineIndex < endSpline; ++splineIndex)
        {
            ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
            for (int i = 0; i < pSpline->GetKeyCount(); i++)
            {
                for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
                {
                    if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                    {
                        time = pSpline->GetKeyTime(i);
                        pSpline->GetKeyValue(i, afValue);
                        tipText = QStringLiteral("t=%1  v=%2").arg(time * m_fTooltipScaleX, 0, 'f', 3).arg(afValue[nCurrentDimension] * m_fTooltipScaleY, 2, 'f', 3);
                        boFoundTheSelectedKey = true;
                        break;
                    }
                }
                if (boFoundTheSelectedKey)
                {
                    break;
                }
            }
        }

        if (event->pos() != m_lastToolTipPos)
        {
            m_lastToolTipPos = event->pos();
            m_tooltipText = tipText;
            update();
            //m_tooltip.UpdateTipText( tipText,this,1 );
            //m_tooltip.Activate(TRUE);
        }
    }

    switch (m_editMode)
    {
    case ScrollMode:
    {
        // Set the new scrolled coordinates
        float ofsx = m_grid.origin.x - (event->x() - m_cMouseDownPos.x()) / m_grid.zoom.x;
        float ofsy = m_grid.origin.y + (event->y() - m_cMouseDownPos.y()) / m_grid.zoom.y;
        SetScrollOffset(Vec2(ofsx, ofsy));
        m_cMouseDownPos = event->pos();
    }
    break;

    case ZoomMode:
    {
        float ofsx = (event->x() - m_cMouseDownPos.x()) * 0.01f;
        float ofsy = (event->y() - m_cMouseDownPos.y()) * 0.01f;

        Vec2 z = m_grid.zoom;
        if (ofsx != 0)
        {
            z.x = max(z.x * (1.0f + ofsx), 0.001f);
        }
        if (ofsy != 0)
        {
            z.y = max(z.y * (1.0f + ofsy), 0.001f);
        }
        SetZoom(z, m_cMouseDownPos);
        m_cMouseDownPos = event->pos();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::UpdateKeyTimes() const
{
    if (!m_bKeyTimesDirty)
    {
        return;
    }

    std::vector<float> selectedKeyTimes;
    selectedKeyTimes.reserve(m_keyTimes.size());
    for (std::vector<KeyTime>::iterator it = m_keyTimes.begin(), end = m_keyTimes.end(); it != end; ++it)
    {
        if ((*it).selected)
        {
            selectedKeyTimes.push_back((*it).time);
        }
    }
    std::sort(selectedKeyTimes.begin(), selectedKeyTimes.end());

    m_keyTimes.clear();
    for (int splineIndex = 0; splineIndex < int(m_splines.size()); ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        for (int keyIndex = 0; pSpline && keyIndex < pSpline->GetKeyCount(); ++keyIndex)
        {
            float value = pSpline->GetKeyTime(keyIndex);

            int lower = 0;
            int upper = int(m_keyTimes.size());
            while (lower < upper - 1)
            {
                int mid = ((lower + upper) >> 1);
                ((m_keyTimes[mid].time >= value) ? upper : lower) = mid;
            }

            if ((lower >= int(m_keyTimes.size()) || fabsf(m_keyTimes[lower].time - value) > threshold) &&
                (upper >= int(m_keyTimes.size()) || fabsf(m_keyTimes[upper].time - value) > threshold))
            {
                m_keyTimes.insert(m_keyTimes.begin() + upper, KeyTime(value, 0));
            }
        }
    }

    for (std::vector<KeyTime>::iterator it = m_keyTimes.begin(), end = m_keyTimes.end(); it != end; ++it)
    {
        (*it).count = (m_pSplineSet ? m_pSplineSet->GetKeyCountAtTime((*it).time, threshold) : 0);
    }

    std::vector<float>::iterator itSelected = selectedKeyTimes.begin(), endSelected = selectedKeyTimes.end();
    for (std::vector<KeyTime>::iterator it = m_keyTimes.begin(), end = m_keyTimes.end(); it != end; ++it)
    {
        const float thisThreshold = 0.01f;
        for (; itSelected != endSelected && (*itSelected) < (*it).time - thisThreshold; ++itSelected)
        {
            ;
        }
        if (itSelected != endSelected && fabsf((*itSelected) - (*it).time) < thisThreshold)
        {
            (*it).selected = true;
        }
    }

    m_totalSplineCount = (m_pSplineSet ? m_pSplineSet->GetSplineCount() : 0);

    m_bKeyTimesDirty = false;
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::OnLButtonUp([[maybe_unused]] const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    if (m_bEditLock)
    {
        return;
    }

    m_boLeftMouseButtonDown = false;

    if (m_editMode == TrackingMode)
    {
        StopTracking();

        if (!m_startedDragging)
        {
            HitSpline(m_cMouseDownPos);
        }
    }

    if (m_editMode == SelectMode)
    {
        // Get control key status.
        bool bAltClick = modifiers & Qt::AltModifier;
        bool bCtrlClick = modifiers & Qt::ControlModifier;

        bool bAddSelect = bCtrlClick;
        bool bUnselect = bAltClick;

        if (!bAddSelect && !bUnselect)
        {
            ClearSelection();
        }

        SelectRectangle(m_rcSelect, !bUnselect);

        m_rcSelect = QRect();
        m_rubberBand->setVisible(false);

        StopTracking();
    }

    if (m_editMode == TimeMarkerMode)
    {
        m_editMode = NothingMode;
        releaseMouse();
        SendNotifyEvent(SPLN_TIME_END_CHANGE);
    }

    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->update();
    }

    m_tooltipText = "";
    update();
    m_editMode = NothingMode;
}

/////////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect)
{
    if (nKey >= 0)
    {
        pSpline->SelectKeyAtDimension(nKey, nDimension, bSelect);
    }
}

//////////////////////////////////////////////////////////////////////////
bool AbstractSplineWidget::IsKeySelected(ISplineInterpolator* pSpline, int nKey, int nHitDimension) const
{
    if (pSpline && nKey >= 0)
    {
        return (pSpline->IsKeySelectedAtDimension(nKey, nHitDimension));
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
int AbstractSplineWidget::GetNumSelected()
{
    int nSelected = 0;
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        if (ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline)
        {
            for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
            {
                for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
                {
                    if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                    {
                        nSelected++;
                    }
                }
            }
        }
    }
    return nSelected;
}

/////////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetSplineSet(ISplineSet* pSplineSet)
{
    m_pSplineSet = pSplineSet;
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::wheelEvent(QWheelEvent* event)
{
    int zDelta = event->angleDelta().y();
    if (zDelta == 0)
    {
        return;
    }
    Vec2 z = m_grid.zoom;
    float scale = 1.2f * fabs(zDelta / 120.0f);
    if (zDelta > 0)
    {
        z *= scale;
    }
    else
    {
        z /= scale;
    }
    SetZoom(z, m_cMousePos);

    event->accept();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetHorizontalExtent([[maybe_unused]] int min, [[maybe_unused]] int max)
{
    /*
    m_scrollMin.x = min;
    m_scrollMax.x = max;
    int width = max - min;
    int nPage = m_rcClient.Width()/2;
    int sx = width - nPage + m_rcSpline.left;

    SCROLLINFO si;
    ZeroStruct(si);
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = m_scrollMin.x;
    si.nMax = m_scrollMax.x - nPage + m_rcSpline.left;
    si.nPage = m_rcClient.Width()/2;
    si.nPos = m_scrollOffset.x;
    //si.nPage = max(0,m_rcClient.Width() - m_leftOffset*2);
    //si.nPage = 1;
    //si.nPage = 1;
    SetScrollInfo( SB_HORZ,&si,TRUE );
    */
}

//////////////////////////////////////////////////////////////////////////
ISplineInterpolator* AbstractSplineWidget::HitSpline(const QPoint& point)
{
    if (HitTest(point) != HIT_NOTHING)
    {
        return m_pHitSpline;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
AbstractSplineWidget::EHitCode AbstractSplineWidget::HitTest(const QPoint& point)
{
    float   time, val;
    int     nTotalNumberOfDimensions(0);
    int     nCurrentDimension(0);


    PointToTimeValue(point, time, val);

    m_hitCode = HIT_NOTHING;
    m_pHitSpline = NULL;
    m_pHitDetailSpline = NULL;
    m_nHitKeyIndex = -1;
    m_nHitDimension = -1;
    m_bHitIncomingHandle = true;

    if (abs(point.x() - TimeToXOfs(m_fTimeMarker)) < 4)
    {
        m_hitCode = HIT_TIMEMARKER;
    }

    // For each Spline...
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
        ISplineInterpolator* pDetailSpline = m_splines[splineIndex].pDetailSpline;

        // If there is no spline, you can't hit nor a spline nor a key... isn't that logical?
        if (!pSpline)
        {
            return m_hitCode;
        }

        ISplineInterpolator::ValueType stSplineValue;
        ISplineInterpolator::ValueType stDetailSplineValue;
        ISplineInterpolator::ZeroValue(stSplineValue);
        ISplineInterpolator::ZeroValue(stDetailSplineValue);

        pSpline->Interpolate(time, stSplineValue);

        if (pDetailSpline)
        {
            pDetailSpline->Interpolate(time, stDetailSplineValue);
        }

        // For each dimension...
        nTotalNumberOfDimensions = pSpline->GetNumDimensions();
        for (nCurrentDimension = 0; nCurrentDimension < nTotalNumberOfDimensions; nCurrentDimension++)
        {
            if (pDetailSpline)
            {
                stSplineValue[nCurrentDimension] = stSplineValue[nCurrentDimension] + stDetailSplineValue[nCurrentDimension];
            }

            for (int i = 0; i < pSpline->GetKeyCount(); i++)
            {
                if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                // Check tangent handles first.
                {
                    QPoint incomingHandlePt, outgoingHandlePt, pt;
                    if (GetTangentHandlePts(incomingHandlePt, pt, outgoingHandlePt, splineIndex, i, nCurrentDimension))
                    {
                        // For the incoming handle
                        if (abs(incomingHandlePt.x() - point.x()) < 4 && abs(incomingHandlePt.y() - point.y()) < 4)
                        {
                            m_hitCode = HIT_TANGENT_HANDLE;
                            m_pHitSpline = pSpline;
                            m_pHitDetailSpline = m_splines[splineIndex].pDetailSpline;
                            m_nHitKeyIndex = i;
                            m_nHitDimension = nCurrentDimension;
                            m_bHitIncomingHandle = true;
                            return m_hitCode;
                        }
                        // For the outgoing handle
                        else if (abs(outgoingHandlePt.x() - point.x()) < 4 && abs(outgoingHandlePt.y() - point.y()) < 4)
                        {
                            m_hitCode = HIT_TANGENT_HANDLE;
                            m_pHitSpline = pSpline;
                            m_pHitDetailSpline = m_splines[splineIndex].pDetailSpline;
                            m_nHitKeyIndex = i;
                            m_nHitDimension = nCurrentDimension;
                            m_bHitIncomingHandle = false;
                            return m_hitCode;
                        }
                    }
                }
            }

            QPoint splinePt = WorldToClient(Vec2(time, stSplineValue[nCurrentDimension]));
            bool bSplineHit = abs(splinePt.x() - point.x()) < 4 && abs(splinePt.y() - point.y()) < 4;

            if (bSplineHit)
            {
                m_hitCode = HIT_SPLINE;
                m_pHitSpline = pSpline;
                m_pHitDetailSpline = m_splines[splineIndex].pDetailSpline;
                for (int i = 0; i < pSpline->GetKeyCount(); i++)
                {
                    QPoint splinePt2 = TimeToPoint(pSpline->GetKeyTime(i), pSpline);
                    if (abs(splinePt2.x() - point.x()) < 4 /* && abs(splinePt.y()-point.y()) < 4*/)
                    {
                        m_nHitKeyIndex = i;
                        m_nHitDimension = nCurrentDimension;
                        m_hitCode = HIT_KEY;
                        return m_hitCode;
                    }
                }
            }
        }
    }

    return m_hitCode;
}

///////////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::StartTracking(bool copyKeys)
{
    m_copyKeys = copyKeys;
    m_startedDragging = false;

    m_editMode = TrackingMode;
    SetCapture();

    UiAnimUndoManager::Get()->Begin();

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    setCursorImpl(IDC_ARRBLCKCROSS);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::StopTracking()
{
    if ((m_editMode == TrackingMode) && (m_cMousePos != m_cMouseDownPos))
    {
        UiAnimUndoManager::Get()->Accept("Spline Move");
    }
    else if ((m_editMode == SelectMode) || ((m_editMode == TrackingMode) && (m_cMousePos == m_cMouseDownPos)))
    {
        if (m_pCurrentUndo && m_pCurrentUndo->IsSelectionChanged())
        {
            UiAnimUndoManager::Get()->Accept("Key Selection");
        }
    }
    else
    {
        UiAnimUndoManager::Get()->Cancel();
    }

    m_editMode = NothingMode;
    releaseMouseImpl();
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ScaleAmplitudeKeys(float time, float startValue, float offset)
{
    //TODO: Test it in the facial animation pane and fix it...
    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;
    m_nHitDimension = -1;

    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        // Find the range of keys to process.
        int keyCount = (pSpline ? pSpline->GetKeyCount() : 0);
        int firstKeyIndex = keyCount;
        int lastKeyIndex = -1;
        for (int i = 0; i < keyCount; ++i)
        {
            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                firstKeyIndex = min(firstKeyIndex, i);
                lastKeyIndex = max(lastKeyIndex, i);
            }
        }

        // Find the parameters of a line between the start and end points. This will form the centre line
        // around which the amplitude of the keys will be scaled.
        float rangeStartTime = (firstKeyIndex >= 0 && pSpline ? pSpline->GetKeyTime(firstKeyIndex) : 0.0f);
        float rangeEndTime = (lastKeyIndex >= 0 && pSpline ? pSpline->GetKeyTime(lastKeyIndex) : 0.0f);
        float rangeLength = max(0.01f, rangeEndTime - rangeStartTime);

        for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
        {
            ISplineInterpolator::ValueType afRangeStartValue;
            if (firstKeyIndex >= 0 && pSpline)
            {
                pSpline->GetKeyValue(firstKeyIndex, afRangeStartValue);
            }
            else
            {
                memset(afRangeStartValue, 0, sizeof(ISplineInterpolator::ValueType));
            }

            ISplineInterpolator::ValueType afRangeEndValue;
            if (lastKeyIndex >= 0 && pSpline)
            {
                pSpline->GetKeyValue(lastKeyIndex, afRangeEndValue);
            }
            else
            {
                memset(afRangeEndValue, 0, sizeof(ISplineInterpolator::ValueType));
            }
            float centreM = (afRangeEndValue[nCurrentDimension] - afRangeStartValue[nCurrentDimension]) / rangeLength;
            float centreC = afRangeStartValue[nCurrentDimension] - centreM * rangeStartTime;
            // Calculate the scale factor, based on how the mouse was dragged.
            float dragCentreValue = centreM * time + centreC;
            float dragCentreOffset = startValue - dragCentreValue;
            float offsetScale = (fabs(dragCentreOffset) > 0.001 ? (offset + dragCentreOffset) / dragCentreOffset : 1.0f);
            // Scale all the selected keys around this central line.
            for (int i = 0; i < keyCount; ++i)
            {
                if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                {
                    float keyTime = (pSpline ? pSpline->GetKeyTime(i) : 0.0f);
                    float centreValue = keyTime * centreM + centreC;
                    ISplineInterpolator::ValueType afKeyValue;
                    if (pSpline)
                    {
                        pSpline->GetKeyValue(i, afKeyValue);
                    }
                    else
                    {
                        memset(afKeyValue, 0, sizeof(ISplineInterpolator::ValueType));
                    }
                    float keyOffset = afKeyValue[nCurrentDimension] - centreValue;
                    float newKeyOffset = keyOffset * offsetScale;
                    if (pSpline)
                    {
                        afKeyValue[nCurrentDimension] = centreValue + newKeyOffset;
                        pSpline->SetKeyValue(i, afKeyValue);
                    }
                }
            }
        }
    }

    update();
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->update();
    }
    SendNotifyEvent(SPLN_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::TimeScaleKeys(float time, float startTime, float endTime)
{
    // Calculate the scaling parameters (ie t1 = t0 * M + C).
    float timeScaleM = 1.0f;
    if (fabsf(startTime - time) > MIN_TIME_EPSILON_FOR_SCALING)
    {
        timeScaleM = (endTime - time) / (startTime - time);
    }
    float timeScaleC = endTime - startTime * timeScaleM;

    // Loop through all keys that are selected.
    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;

    float affectedRangeMin = FLT_MAX;
    float affectedRangeMax = -FLT_MAX;
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        int keyCount = pSpline->GetKeyCount();
        float keyRangeMin = FLT_MAX;
        float keyRangeMax = -FLT_MAX;
        for (int i = 0; i < keyCount; i++)
        {
            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                float oldTime = pSpline->GetKeyTime(i);
                float t = SnapTime(oldTime * timeScaleM + timeScaleC);

                pSpline->SetKeyTime(i, SnapTimeToGridVertical(t));

                keyRangeMin = min(keyRangeMin, oldTime);
                keyRangeMin = min(keyRangeMin, t);
                keyRangeMax = max(keyRangeMax, oldTime);
                keyRangeMax = max(keyRangeMax, t);
            }
        }
        if (keyRangeMin <= keyRangeMax)
        {
            // Changes to a key's value affect spline up to two keys away.
            int lastMovedKey = 0;
            for (int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
            {
                if (pSpline->GetKeyTime(keyIndex) <= keyRangeMax)
                {
                    lastMovedKey = keyIndex + 1;
                }
            }
            int firstMovedKey = pSpline->GetKeyCount();
            for (int keyIndex = pSpline->GetKeyCount() - 1; keyIndex >= 0; --keyIndex)
            {
                if (pSpline->GetKeyTime(keyIndex) >= keyRangeMin)
                {
                    firstMovedKey = keyIndex;
                }
            }

            int firstAffectedKey = max(0, firstMovedKey - 2);
            int lastAffectedKey = min(keyCount - 1, lastMovedKey + 2);

            affectedRangeMin = min(affectedRangeMin, (firstAffectedKey <= 0 ? m_timeRange.start : pSpline->GetKeyTime(firstAffectedKey)));
            affectedRangeMax = max(affectedRangeMax, (lastAffectedKey >= keyCount - 1 ? m_timeRange.end : pSpline->GetKeyTime(lastAffectedKey)));

            // Loop through all moved keys, checking whether there are multiple keys on the same frame.
            float lastKeyTime = -FLT_MAX;
            pSpline->Update();
            for (int keyIndex = 0, keys = pSpline->GetKeyCount(); keyIndex <= keys; )
            {
                float keyTime = pSpline->GetKeyTime(keyIndex);
                if (fabsf(keyTime - lastKeyTime) < m_fMinTimeEpsilon)
                {
                    --keys;
                    pSpline->RemoveKey(keyIndex);
                }
                else
                {
                    ++keyIndex;
                    lastKeyTime = keyTime;
                }
            }
        }
    }

    int rangeMin = aznumeric_cast<int>(TimeToXOfs(affectedRangeMin));
    int rangeMax = aznumeric_cast<int>(TimeToXOfs(affectedRangeMax));

    if (m_timeRange.start == affectedRangeMin)
    {
        rangeMin = m_rcSpline.left();
    }
    if (m_timeRange.end == affectedRangeMax)
    {
        rangeMax = m_rcSpline.right();
    }

    QRect invalidRect(QPoint(rangeMin - 3, m_rcSpline.top()), QPoint(rangeMax + 3, m_rcSpline.bottom()));
    update(invalidRect);
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->update();
    }

    m_bKeyTimesDirty = true;
    SendNotifyEvent(SPLN_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ValueScaleKeys(float startValue, float endValue)
{
    // Calculate the scaling parameters.
    float valueScale = 1.0f;
    if (fabsf(startValue) > MIN_TIME_EPSILON_FOR_SCALING)
    {
        valueScale = endValue / startValue;
    }

    // Loop through all keys that are selected.
    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;
    m_nHitDimension = -1;

    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        int keyCount = pSpline->GetKeyCount();
        for (int i = 0; i < keyCount; i++)
        {
            for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
            {
                if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                {
                    ISplineInterpolator::ValueType  afValue;
                    pSpline->GetKeyValue(i, afValue);

                    afValue[nCurrentDimension] = SnapValue(afValue[nCurrentDimension] * valueScale);
                    pSpline->SetKeyValue(i, afValue);
                }
            }
        }
    }

    update();
    SendNotifyEvent(SPLN_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::MoveSelectedKeys(Vec2 offset, bool copyKeys)
{
    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;
    m_nHitDimension = -1;

    if (copyKeys)
    {
        DuplicateSelectedKeys();
    }

    float affectedRangeMin = FLT_MAX;
    float affectedRangeMax = -FLT_MAX;
    // For each spline...
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        int  keyCount = pSpline->GetKeyCount();
        float keyRangeMin = FLT_MAX;
        float keyRangeMax = -FLT_MAX;
        for (int i = 0; i < keyCount; i++)
        {
            float   oldTime = pSpline->GetKeyTime(i);
            float   t = SnapTime(oldTime + offset.x);

            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                if (pSpline->FindKey(t, m_fMinTimeEpsilon) < 0)
                {
                    pSpline->SetKeyTime(i, SnapTimeToGridVertical(t));
                }

                keyRangeMin = min(keyRangeMin, oldTime);
                keyRangeMin = min(keyRangeMin, t);
                keyRangeMax = max(keyRangeMax, oldTime);
                keyRangeMax = max(keyRangeMax, t);
            }

            for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
            {
                if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                {
                    ISplineInterpolator::ValueType  afValue;
                    pSpline->GetKeyValue(i, afValue);

                    afValue[nCurrentDimension] = SnapValue(afValue[nCurrentDimension] + offset.y);
                    pSpline->SetKeyValue(i, afValue);
                }
            }
        }
        if (keyRangeMin <= keyRangeMax)
        {
            // Changes to a key's value affect spline up to two keys away.
            int lastMovedKey = 0;
            for (int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
            {
                if (pSpline->GetKeyTime(keyIndex) <= keyRangeMax)
                {
                    lastMovedKey = keyIndex + 1;
                }
            }
            int firstMovedKey = pSpline->GetKeyCount();
            for (int keyIndex = pSpline->GetKeyCount() - 1; keyIndex >= 0; --keyIndex)
            {
                if (pSpline->GetKeyTime(keyIndex) >= keyRangeMin)
                {
                    firstMovedKey = keyIndex;
                }
            }

            int firstAffectedKey = max(0, firstMovedKey - 2);
            int lastAffectedKey = min(keyCount - 1, lastMovedKey + 2);

            affectedRangeMin = min(affectedRangeMin, (firstAffectedKey <= 0 ? m_timeRange.start : pSpline->GetKeyTime(firstAffectedKey)));
            affectedRangeMax = max(affectedRangeMax, (lastAffectedKey >= keyCount - 1 ? m_timeRange.end : pSpline->GetKeyTime(lastAffectedKey)));
        }
    }

    int rangeMin = aznumeric_cast<int>(TimeToXOfs(affectedRangeMin));
    int rangeMax = aznumeric_cast<int>(TimeToXOfs(affectedRangeMax));

    if (m_timeRange.start == affectedRangeMin)
    {
        rangeMin = m_rcSpline.left();
    }
    if (m_timeRange.end == affectedRangeMax)
    {
        rangeMax = m_rcSpline.right();
    }

    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->update();
    }

    m_bKeyTimesDirty = true;
    SendNotifyEvent(SPLN_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveKey(ISplineInterpolator* pSpline, int nKey)
{
    if (pSpline == nullptr || nKey < 0)
    {
        return;
    }

    UiAnimUndo undo("Remove Spline Key");
    ConditionalStoreUndo();

    m_bKeyTimesDirty = true;

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;
    pSpline->RemoveKey(nKey);

    SendNotifyEvent(SPLN_CHANGE);
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveSelectedKeys()
{
    UiAnimUndo undo("Remove Spline Key");
    StoreUndo();

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;

    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        for (int i = 0; i < (int)pSpline->GetKeyCount(); )
        {
            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                pSpline->RemoveKey(i);
            }
            else
            {
                i++;
            }
        }
    }

    m_bKeyTimesDirty = true;
    SendNotifyEvent(SPLN_CHANGE);
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveSelectedKeyTimesImpl()
{
    int numSelectedKeyTimes = 0;
    for (std::vector<KeyTime>::iterator it = m_keyTimes.begin(), end = m_keyTimes.end(); it != end; ++it)
    {
        if ((*it).selected)
        {
            ++numSelectedKeyTimes;
        }
    }

    if (numSelectedKeyTimes)
    {
        UiAnimUndo undo("Remove Spline Key");
        StoreUndo();
        SendNotifyEvent(SPLN_BEFORE_CHANGE);

        for (int splineIndex = 0, end = static_cast<int>(m_splines.size()); splineIndex < end; ++splineIndex)
        {
            std::vector<KeyTime>::iterator itTime = m_keyTimes.begin(), endTime = m_keyTimes.end();
            for (int keyIndex = 0, endIndex = m_splines[splineIndex].pSpline->GetKeyCount(); keyIndex < endIndex; )
            {
                const float thisThreshold = 0.01f;
                for (; itTime != endTime && (*itTime).time < m_splines[splineIndex].pSpline->GetKeyTime(keyIndex) - thisThreshold; ++itTime)
                {
                    ;
                }
                if (itTime != endTime && fabsf((*itTime).time - m_splines[splineIndex].pSpline->GetKeyTime(keyIndex)) < thisThreshold && (*itTime).selected)
                {
                    m_splines[splineIndex].pSpline->RemoveKey(keyIndex);
                }
                else
                {
                    ++keyIndex;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveSelectedKeyTimes()
{
    RemoveSelectedKeyTimesImpl();

    m_bKeyTimesDirty = true;
    SendNotifyEvent(SPLN_CHANGE);
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RedrawWindowAroundMarker()
{
    UpdateKeyTimes();
    std::vector<KeyTime>::iterator itKeyTime = std::lower_bound(m_keyTimes.begin(), m_keyTimes.end(), KeyTime(m_fTimeMarker, 0));
    int keyTimeIndex = static_cast<int>(itKeyTime != m_keyTimes.end() ? itKeyTime - m_keyTimes.begin() : m_keyTimes.size());
    int redrawRangeStart = (keyTimeIndex >= 2 ? aznumeric_cast<int>(TimeToXOfs(m_keyTimes[keyTimeIndex - 2].time)) : m_rcSpline.left());
    int redrawRangeEnd = (keyTimeIndex < int(m_keyTimes.size()) - 2 ? aznumeric_cast<int>(TimeToXOfs(m_keyTimes[keyTimeIndex + 2].time)) : m_rcSpline.right());

    QRect rc(QPoint(redrawRangeStart, m_rcSpline.top()), QPoint(redrawRangeEnd, m_rcSpline.bottom()));
    rc = rc.normalized().intersected(m_rcSpline);

    m_TimeUpdateRect = QRect(QPoint(1, 2), QPoint(3, 4));
    update(rc);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SplinesChanged()
{
    m_bKeyTimesDirty = true;
    UpdateKeyTimes();
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetControlAmplitude(bool controlAmplitude)
{
    m_controlAmplitude = controlAmplitude;
}

//////////////////////////////////////////////////////////////////////////
bool AbstractSplineWidget::GetControlAmplitude() const
{
    return m_controlAmplitude;
}

float AbstractSplineWidget::SnapTimeToGridVertical(float time)
{
    //float fSnapTime = int((time * m_fGridTimeScale) + 0.5f) * (1.0f / m_fGridTimeScale);
    float fSnapTime = time;
    return fSnapTime;
}

//////////////////////////////////////////////////////////////////////////
int AbstractSplineWidget::InsertKey(ISplineInterpolator* pSpline, ISplineInterpolator* pDetailSpline, const QPoint& point)
{
    UiAnimUndo undo("Spline Insert Key");
    StoreUndo();

    float time, val;
    PointToTimeValue(point, time, val);

    time = SnapTimeToGridVertical(time);

    int i;
    for (i = 0; i < pSpline->GetKeyCount(); i++)
    {
        // Skip if any key already have time that is very close.
        if (fabs(pSpline->GetKeyTime(i) - time) < m_fMinTimeEpsilon)
        {
            return i;
        }
    }

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    // The proper key value for a spline that has a detail spline is not what is shown in the control - we have
    // to remove the detail value to get back to the underlying spline value.
    if (pDetailSpline)
    {
        float offset = 0.0f;
        pDetailSpline->InterpolateFloat(time, offset);
        val -= offset;
    }

    ClearSelection();
    ISplineInterpolator::ValueType currValue;
    ISplineInterpolator::ZeroValue(currValue);

    pSpline->Interpolate(time, currValue);

    if (pSpline->GetNumDimensions() > 1)
    {
    }

    int nKey = pSpline->InsertKey(time, currValue);  // TODO: Don't use FE specific snapping!
    if (m_defaultKeyTangentType != SPLINE_KEY_TANGENT_NONE)
    {
        SetKeyTangentType(pSpline, nKey, m_defaultKeyTangentType);
    }

    //int nKey = pSpline->InsertKeyFloat( time,val ); // TODO: Don't use FE specific snapping!
    SelectKey(pSpline, nKey, 0, true);
    update();

    m_bKeyTimesDirty = true;

    SendNotifyEvent(SPLN_CHANGE);

    return nKey;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ClearSelection()
{
    ConditionalStoreUndo();

    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            pSpline->SelectKeyAllDimensions(i, false);
        }
    }

    ClearSelectedKeys();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SetTimeMarker(float fTime)
{
    if (m_pTimelineCtrl)
    {
        m_pTimelineCtrl->SetTimeMarker(fTime);
    }

    if (fTime == m_fTimeMarker)
    {
        return;
    }

    // Erase old first.
    int x1 = aznumeric_cast<int>(TimeToXOfs(m_fTimeMarker));
    int x2 = aznumeric_cast<int>(TimeToXOfs(fTime));
    QRect rc(QPoint(x1, m_rcSpline.top()), QPoint(x2, m_rcSpline.bottom()));
    rc = rc.normalized().adjusted(-3, 0, 3, 0).intersected(m_rcSpline);

    m_TimeUpdateRect = rc;
    update(rc);

    m_fTimeMarker = fTime;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::StoreUndo()
{
    if (UiAnimUndo::IsRecording() && !m_pCurrentUndo)
    {
        std::vector<ISplineInterpolator*> splines(m_splines.size());
        for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
        {
            splines[splineIndex] = m_splines[splineIndex].pSpline;
        }
        UiAnimUndo::Record(m_pCurrentUndo = CreateSplineCtrlUndoObject(splines));
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ConditionalStoreUndo()
{
    if (m_editMode == TrackingMode || m_editMode == SelectMode)
    {
        StoreUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ClearSelectedKeys()
{
    for (std::vector<KeyTime>::iterator it = m_keyTimes.begin(), end = m_keyTimes.end(); it != end; ++it)
    {
        (*it).selected = false;
    }
}

//////////////////////////////////////////////////////////////////////////
class CKeyCopyInfo
{
public:
    ISplineInterpolator::ValueType value;
    float time;
    int flags;
    ISplineInterpolator::ValueType tin, tout;
};
void AbstractSplineWidget::DuplicateSelectedKeys()
{
    m_pHitSpline = 0;
    m_pHitDetailSpline = 0;
    m_nHitKeyIndex = -1;

    typedef std::vector<CKeyCopyInfo> KeysToAddContainer;
    KeysToAddContainer keysToInsert;
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        keysToInsert.resize(0);
        for (int i = 0; i < pSpline->GetKeyCount(); i++)
        {
            // In this particular case, the dimension doesn't matter.
            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                keysToInsert.resize(keysToInsert.size() + 1);
                CKeyCopyInfo& copyInfo = keysToInsert.back();

                copyInfo.time = pSpline->GetKeyTime(i);
                pSpline->GetKeyValue(i, copyInfo.value);
                pSpline->GetKeyTangents(i, copyInfo.tin, copyInfo.tout);
                copyInfo.flags = pSpline->GetKeyFlags(i);
            }
        }

        for (KeysToAddContainer::iterator keyToAdd = keysToInsert.begin(), endKeysToAdd = keysToInsert.end(); keyToAdd != endKeysToAdd; ++keyToAdd)
        {
            int keyIndex = pSpline->InsertKey(SnapTimeToGridVertical((*keyToAdd).time), (*keyToAdd).value);
            pSpline->SetKeyTangents(keyIndex, (*keyToAdd).tin, (*keyToAdd).tout);
            pSpline->SetKeyFlags(keyIndex, (*keyToAdd).flags & (~ESPLINE_KEY_UI_SELECTED_MASK));
        }
    }

    m_bKeyTimesDirty = true;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ZeroAll()
{
    UiAnimUndoManager::Get()->Begin();

    typedef std::vector<ISplineInterpolator*> SplineContainer;
    SplineContainer splines;
    for (int splineIndex = 0; splineIndex < int(m_splines.size()); ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
        int keyIndex = (pSpline ? pSpline->FindKey(m_fTimeMarker, 0.015f) : -1);
        if (pSpline && keyIndex >= 0)
        {
            splines.push_back(pSpline);
        }
    }

    UiAnimUndo::Record(CreateSplineCtrlUndoObject(splines));

    for (SplineContainer::iterator itSpline = splines.begin(); itSpline != splines.end(); ++itSpline)
    {
        int keyIndex = ((*itSpline) ? (*itSpline)->FindKey(m_fTimeMarker, 0.015f) : -1);
        if ((*itSpline) && keyIndex >= 0)
        {
            (*itSpline)->SetKeyValueFloat(keyIndex, 0.0f);
        }
    }

    UiAnimUndoManager::Get()->Accept("Zero All");
    m_pCurrentUndo = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::KeyAll()
{
    UiAnimUndoManager::Get()->Begin();

    typedef std::vector<ISplineInterpolator*> SplineContainer;
    SplineContainer splines;
    for (int splineIndex = 0; splineIndex < int(m_splines.size()); ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
        int keyIndex = (pSpline ? pSpline->FindKey(m_fTimeMarker, 0.015f) : -1);
        if (pSpline && keyIndex == -1)
        {
            splines.push_back(pSpline);
        }
    }

    UiAnimUndo::Record(CreateSplineCtrlUndoObject(splines));

    for (SplineContainer::iterator itSpline = splines.begin(); itSpline != splines.end(); ++itSpline)
    {
        float value = 0.0f;
        (*itSpline)->InterpolateFloat(m_fTimeMarker, value);
        int keyIndex = (*itSpline)->InsertKeyFloat(SnapTimeToGridVertical(m_fTimeMarker), value);
        if (m_defaultKeyTangentType != SPLINE_KEY_TANGENT_NONE)
        {
            SetKeyTangentType(*itSpline, keyIndex, m_defaultKeyTangentType);
        }
    }

    UiAnimUndoManager::Get()->Accept("Key All");
    m_pCurrentUndo = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SelectAll()
{
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            pSpline->SelectKeyAllDimensions(i, true);
        }
    }
    update();
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::SendNotifyEvent(int nEvent)
{
    if (nEvent == SPLN_BEFORE_CHANGE)
    {
        ConditionalStoreUndo();
    }

    switch (nEvent)
    {
    case SPLN_BEFORE_CHANGE:
        Q_EMIT beforeChange();
        break;
    case SPLN_CHANGE:
        Q_EMIT change();
        break;
    case SPLN_TIME_CHANGE:
        Q_EMIT timeChange();
        break;
    case SPLN_SCROLL_ZOOM:
        Q_EMIT scrollZoomRequested();
        break;
    case NM_CLICK:
        Q_EMIT clicked();
        break;
    case NM_RCLICK:
        Q_EMIT rightClicked();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void SplineWidget::SetTimelineCtrl(TimelineWidget* pTimelineCtrl)
{
    m_pTimelineCtrl = pTimelineCtrl;
    if (m_pTimelineCtrl)
    {
        pTimelineCtrl->setParent(this);
        pTimelineCtrl->SetZoom(m_grid.zoom.x);
        pTimelineCtrl->SetOrigin(m_grid.origin.x);
        pTimelineCtrl->SetKeyTimeSet(this);
        pTimelineCtrl->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::AddSpline(ISplineInterpolator* pSpline, ISplineInterpolator* pDetailSpline, COLORREF color)
{
    for (int i = 0; i < (int)m_splines.size(); i++)
    {
        if (m_splines[i].pSpline == pSpline)
        {
            return;
        }
    }
    SSplineInfo si;

    for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
    {
        si.anColorArray[nCurrentDimension] = color;
    }

    si.pSpline = pSpline;
    si.pDetailSpline = pDetailSpline;
    m_splines.push_back(si);
    m_bKeyTimesDirty = true;
    update();
}

void AbstractSplineWidget::AddSpline(ISplineInterpolator* pSpline, ISplineInterpolator* pDetailSpline, COLORREF anColorArray[4])
{
    for (int i = 0; i < (int)m_splines.size(); i++)
    {
        if (m_splines[i].pSpline == pSpline)
        {
            return;
        }
    }
    SSplineInfo si;

    for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
    {
        si.anColorArray[nCurrentDimension] = anColorArray[nCurrentDimension];
    }

    si.pSpline = pSpline;
    si.pDetailSpline = pDetailSpline;
    m_splines.push_back(si);
    m_bKeyTimesDirty = true;
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveSpline(ISplineInterpolator* pSpline)
{
    for (int i = 0; i < (int)m_splines.size(); i++)
    {
        if (m_splines[i].pSpline == pSpline)
        {
            m_splines.erase(m_splines.begin() + i);
            return;
        }
    }
    m_bKeyTimesDirty = true;
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveAllSplines()
{
    m_splines.clear();
    m_bKeyTimesDirty = true;
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::SelectRectangle(const QRect& rc, bool bSelect)
{
    ConditionalStoreUndo();

    ClearSelectedKeys();

    Vec2 vec0 = ClientToWorld(rc.topLeft());
    Vec2 vec1 = ClientToWorld(rc.bottomRight());
    float t0 = vec0.x;
    float t1 = vec1.x;
    float v0 = vec0.y;
    float v1 = vec1.y;
    if (v0 > v1)
    {
        std::swap(v0, v1);
    }
    if (t0 > t1)
    {
        std::swap(t0, t1);
    }
    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
        ISplineInterpolator* pDetailSpline = m_splines[splineIndex].pDetailSpline;

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            float t = pSpline->GetKeyTime(i);
            ISplineInterpolator::ValueType  afValue;
            pSpline->GetKeyValue(i, afValue);

            int nTotalNumberOfDimensions(pSpline->GetNumDimensions());

            ISplineInterpolator::ValueType afDetailValue;
            if (pDetailSpline)
            {
                ISplineInterpolator::ZeroValue(afDetailValue);
                pDetailSpline->Interpolate(t, afDetailValue);
            }

            for (int nCurrentDimension = 0; nCurrentDimension < nTotalNumberOfDimensions; nCurrentDimension++)
            {
                if (pDetailSpline)
                {
                    afValue[nCurrentDimension] = afValue[nCurrentDimension] + afDetailValue[nCurrentDimension];
                }
                if (t >= t0 && t <= t1 && afValue[nCurrentDimension] >= v0 && afValue[nCurrentDimension] <= v1)
                {
                    pSpline->SelectKeyAtDimension(i, nCurrentDimension, bSelect);
                }
            }
        }
    }
    SendNotifyEvent(SPLN_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::CopyKeys()
{
    // Copy selected keys.
    if (m_splines.empty() || GetNumSelected() == 0)
    {
        return;
    }

    XmlNodeRef rootNode = XmlHelpers::CreateXmlNode("SplineKeys");

    int i;
    float minTime = FLT_MAX;
    float maxTime = -FLT_MAX;

    ISplineInterpolator* pSpline = m_splines[0].pSpline;

    for (i = 0; i < (int)pSpline->GetKeyCount(); i++)
    {
        if (!pSpline->IsKeySelectedAtAnyDimension(i))
        {
            continue;
        }
        float t = pSpline->GetKeyTime(i);
        if (t < minTime)
        {
            minTime = t;
        }
        if (t > maxTime)
        {
            maxTime = t;
        }
    }

    rootNode->setAttr("start", minTime);
    rootNode->setAttr("end", maxTime);

    for (i = 0; i < (int)pSpline->GetKeyCount(); i++)
    {
        if (!pSpline->IsKeySelectedAtAnyDimension(i))
        {
            continue;
        }

        float t = pSpline->GetKeyTime(i); // Store offset time from copy/paste range.
        ISplineInterpolator::ValueType afValue;
        pSpline->GetKeyValue(i, afValue);

        float tin, tout;
        ISplineInterpolator::ValueType vtin, vtout;
        pSpline->GetKeyTangents(i, vtin, vtout);
        tin = vtin[0];
        tout = vtout[0];

        XmlNodeRef keyNode = rootNode->newChild("Key");
        keyNode->setAttr("time", t);
        keyNode->setAttr("flags", (int)pSpline->GetKeyFlags(i));
        keyNode->setAttr("in", tin);
        keyNode->setAttr("out", tout);


        for (int ii = 0; ii < pSpline->GetNumDimensions(); ++ii)
        {
            XmlNodeRef dimensionNode = keyNode->newChild("values");
            dimensionNode->setAttr("value", afValue[ii]);
        }
    }

    CClipboard clipboard(WidgetCast());
    clipboard.Put(rootNode);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::PasteKeys()
{
    if (m_splines.empty() || GetNumSelected() == 0)
    {
        return;
    }

    ISplineInterpolator* pSpline = m_splines[0].pSpline;

    CClipboard clipboard(WidgetCast());
    if (clipboard.IsEmpty())
    {
        return;
    }

    XmlNodeRef rootNode = clipboard.Get();
    if (!rootNode)
    {
        return;
    }
    if (!rootNode->isTag("SplineKeys"))
    {
        return;
    }

    float minTime = 0;
    float maxTime = 0;
    rootNode->getAttr("start", minTime);
    rootNode->getAttr("end", maxTime);

    const QPoint point = mapFromGlobal(QCursor::pos());
    float fTime = XOfsToTime(point.x());
    float fTimeRange = (maxTime - minTime);

    UiAnimUndo undo("Paste Spline Keys");

    ConditionalStoreUndo();

    ClearSelection();

    int i;
    // Delete keys in range min to max time.
    for (i = 0; i < pSpline->GetKeyCount(); )
    {
        float t = pSpline->GetKeyTime(i);
        if (t >= fTime && t <= fTime + fTimeRange)
        {
            pSpline->RemoveKey(i);
        }
        else
        {
            i++;
        }
    }

    for (i = 0; i < rootNode->getChildCount(); i++)
    {
        XmlNodeRef keyNode = rootNode->getChild(i);
        float t = 0;
        float tin = 0;
        float tout = 0;
        int flags = 0;

        keyNode->getAttr("time", t);
        keyNode->getAttr("flags", flags);
        keyNode->getAttr("in", tin);
        keyNode->getAttr("out", tout);

        int nNumberOfChildXMLNodes(0);
        int nCurrentChildXMLNode(0);
        int nCurrentValue(0);

        ISplineInterpolator::ValueType  afValue;

        nNumberOfChildXMLNodes = keyNode->getChildCount();
        for (nCurrentChildXMLNode = 0; nCurrentChildXMLNode < nNumberOfChildXMLNodes; ++nCurrentChildXMLNode)
        {
            XmlNodeRef rSubKeyNode = keyNode->getChild(nCurrentChildXMLNode);
            if (rSubKeyNode->isTag("values"))
            {
                rSubKeyNode->getAttr("value", afValue[nCurrentValue]);
                nCurrentValue++;
            }
        }

        int key = pSpline->InsertKey(SnapTimeToGridVertical(t - minTime + fTime), afValue);
        if (key >= 0)
        {
            pSpline->SelectKeyAllDimensions(key, true);
            ISplineInterpolator::ValueType vtin, vtout;
            vtin[0] = tin;
            vtout[0] = tout;
            pSpline->SetKeyTangents(key, vtin, vtout);
        }
    }
    m_bKeyTimesDirty = true;
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::ModifySelectedKeysFlags(int nRemoveFlags, int nAddFlags)
{
    UiAnimUndo undo("Modify Spline Keys");
    StoreUndo();

    SendNotifyEvent(SPLN_BEFORE_CHANGE);

    for (int splineIndex = 0, splineCount = static_cast<int>(m_splines.size()); splineIndex < splineCount; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        for (int i = 0; i < (int)pSpline->GetKeyCount(); i++)
        {
            // If the key is selected in any dimension...
            for (
                int nCurrentDimension = 0;
                nCurrentDimension < pSpline->GetNumDimensions();
                nCurrentDimension++
                )
            {
                if (IsKeySelected(pSpline, i, nCurrentDimension))
                {
                    int flags = pSpline->GetKeyFlags(i);
                    flags &= ~nRemoveFlags;
                    flags |= nAddFlags;

                    pSpline->SetKeyFlags(i, flags);
                    break;
                }
            }
        }
    }

    SendNotifyEvent(SPLN_CHANGE);
    update();
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::FitSplineToViewWidth()
{
    // Calculate time zoom so that whole time range fits.
    float t0 = FLT_MAX;
    float t1 = -FLT_MAX;

    bool bAnyKey = false;
    for (int i = 0; i < int(m_splines.size()); ++i)
    {
        //////////////////////////////////////////////////////////////////////////
        ISplineInterpolator* pSpline = m_splines[i].pSpline;
        if (!pSpline)
        {
            continue;
        }

        for (int keyIndex = 0; pSpline && keyIndex < pSpline->GetKeyCount(); ++keyIndex)
        {
            float keyTime = pSpline->GetKeyTime(keyIndex);
            t0 = std::min(t0, keyTime);
            t1 = std::max(t1, keyTime);
            bAnyKey = true;
        }
    }
    if (!bAnyKey)
    {
        t0 = m_timeRange.start;
        t1 = m_timeRange.end;
    }

    float zoom = abs(m_rcSpline.width() - 20) / max(1.0f, fabs(t1 - t0));
    SetZoom(Vec2(zoom, m_grid.zoom.y));
    SetScrollOffset(Vec2(t0, m_grid.origin.y));
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::FitSplineToViewHeight()
{
    // Calculate time zoom so that whole value range fits.
    float vmin = FLT_MAX;
    float vmax = -FLT_MAX;

    bool bAnyKey = false;
    for (int i = 0; i < int(m_splines.size()); ++i)
    {
        //////////////////////////////////////////////////////////////////////////
        ISplineInterpolator* pSpline = m_splines[i].pSpline;
        if (!pSpline)
        {
            continue;
        }

        ISplineInterpolator::ValueType value;
        for (int keyIndex = 0; pSpline && keyIndex < pSpline->GetKeyCount(); ++keyIndex)
        {
            pSpline->GetKeyValue(keyIndex, value);
            for (int d = 0, numDim = pSpline->GetNumDimensions(); d < numDim; d++)
            {
                vmin = std::min(vmin, value[d]);
                vmax = std::max(vmax, value[d]);
                bAnyKey = true;
            }
        }
    }
    if (!bAnyKey)
    {
        vmin = m_defaultValueRange.start;
        vmax = m_defaultValueRange.end;
    }

    float zoom = abs(m_rcSpline.height() - 40) / max(1.0f, fabs(vmax - vmin));
    SetZoom(Vec2(m_grid.zoom.x, zoom));
    SetScrollOffset(Vec2(m_grid.origin.x, vmin));
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::OnUserCommand(UINT cmd)
{
    switch (cmd)
    {
    case ID_TANGENT_IN_ZERO:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_IN_MASK, SPLINE_KEY_TANGENT_ZERO << SPLINE_KEY_TANGENT_IN_SHIFT);
        break;
    case ID_TANGENT_IN_STEP:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_IN_MASK, SPLINE_KEY_TANGENT_STEP << SPLINE_KEY_TANGENT_IN_SHIFT);
        break;
    case ID_TANGENT_IN_LINEAR:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_IN_MASK, SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_IN_SHIFT);
        break;

    case ID_TANGENT_OUT_ZERO:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_OUT_MASK, SPLINE_KEY_TANGENT_ZERO << SPLINE_KEY_TANGENT_OUT_SHIFT);
        break;
    case ID_TANGENT_OUT_STEP:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_OUT_MASK, SPLINE_KEY_TANGENT_STEP << SPLINE_KEY_TANGENT_OUT_SHIFT);
        break;
    case ID_TANGENT_OUT_LINEAR:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_OUT_MASK, SPLINE_KEY_TANGENT_LINEAR << SPLINE_KEY_TANGENT_OUT_SHIFT);
        break;

    case ID_TANGENT_AUTO:
        ModifySelectedKeysFlags(SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK, 0);
        break;

    case ID_SPLINE_FIT_X:
        FitSplineToViewWidth();
        break;
    case ID_SPLINE_FIT_Y:
        FitSplineToViewHeight();
        break;
    case ID_SPLINE_SNAP_GRID_X:
        SetSnapTime(!m_bSnapTime);
        break;
    case ID_SPLINE_SNAP_GRID_Y:
        SetSnapValue(!m_bSnapValue);
        break;
    case ID_SPLINE_PREVIOUS_KEY:
        GotoNextKey(true);
        break;
    case ID_SPLINE_NEXT_KEY:
        GotoNextKey(false);
        break;
    case ID_SPLINE_FLATTEN_ALL:
        RemoveAllKeysButThis();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::GotoNextKey(bool previousKey)
{
    if (GetNumSelected() == 1)
    {
        bool boFoundTheSelectedKey(false);

        for (int splineIndex = 0, endSpline = static_cast<int>(m_splines.size()); splineIndex < endSpline; ++splineIndex)
        {
            ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;
            for (int i = 0; i < pSpline->GetKeyCount(); i++)
            {
                for (int nCurrentDimension = 0; nCurrentDimension < pSpline->GetNumDimensions(); nCurrentDimension++)
                {
                    if (pSpline->IsKeySelectedAtDimension(i, nCurrentDimension))
                    {
                        boFoundTheSelectedKey = true;

                        if ((previousKey && i > 0) || (!previousKey && i + 1 < pSpline->GetKeyCount()))
                        {
                            int nextKey = previousKey ? i - 1 : i + 1;
                            float keyTime = pSpline->GetKeyTime(nextKey);

                            SetTimeMarker(keyTime);

                            ISplineInterpolator::ValueType afValue;
                            pSpline->GetKeyValue(nextKey, afValue);

                            pSpline->SelectKeyAtDimension(i, nCurrentDimension, false);
                            pSpline->SelectKeyAtDimension(nextKey, nCurrentDimension, true);

                            // Set the new scrolled coordinates
                            float ofsx = keyTime - ((m_grid.rect.right() + 1) / 2) / m_grid.zoom.x;
                            float ofsy = afValue[nCurrentDimension] - ((m_grid.rect.bottom() + 1) / 2) / m_grid.zoom.y;

                            SetScrollOffset(Vec2(ofsx, ofsy));
                        }
                        break;
                    }
                }
                if (boFoundTheSelectedKey)
                {
                    break;
                }
            }
        }
    }
    else
    {
        for (int splineIndex = 0, endSpline = static_cast<int>(m_splines.size()); splineIndex < endSpline; ++splineIndex)
        {
            ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

            float fClosestKeyTime = -1.0f;
            float fClosestDist = 1E8;

            for (int i = 0; i < pSpline->GetKeyCount(); i++)
            {
                float fKeyTime = pSpline->GetKeyTime(i);
                float fKeyDist = previousKey ? m_fTimeMarker - fKeyTime : fKeyTime - m_fTimeMarker;

                if ((fKeyDist > 0.0f) && (fKeyDist < fClosestDist))
                {
                    fClosestDist = fKeyDist;
                    fClosestKeyTime = pSpline->GetKeyTime(i);
                }
            }

            if (fClosestKeyTime >= 0.0f)
            {
                SetTimeMarker(fClosestKeyTime);

                float averageValue = 0.f;
                int dimensions = pSpline->GetNumDimensions();

                for (int i = 0; i < dimensions; i++)
                {
                    float keyValue;
                    int keyNum = pSpline->FindKey(fClosestKeyTime);

                    pSpline->GetKeyValueFloat(keyNum, keyValue);
                    averageValue += keyValue;
                }

                // Set the new scrolled coordinates
                float ofsx = fClosestKeyTime - ((m_grid.rect.right() + 1) / 2) / m_grid.zoom.x;
                float ofsy = averageValue / dimensions - ((m_grid.rect.bottom() + 1) / 2) / m_grid.zoom.y;

                SetScrollOffset(Vec2(ofsx, ofsy));
            }
        }
    }
    SendNotifyEvent(SPLN_TIME_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void AbstractSplineWidget::RemoveAllKeysButThis()
{
    std::vector<int> keys;

    for (int splineIndex = 0, endSpline = static_cast<int>(m_splines.size()); splineIndex < endSpline; ++splineIndex)
    {
        ISplineInterpolator* pSpline = m_splines[splineIndex].pSpline;

        for (int i = 0; i < pSpline->GetKeyCount(); i++)
        {
            if (pSpline->IsKeySelectedAtAnyDimension(i))
            {
                keys.push_back(i);
            }
        }

        for (int i = pSpline->GetKeyCount(); i >= 0; i--)
        {
            bool saveKey = false;

            for (int nIndex = 0; nIndex < keys.size(); nIndex++)
            {
                if (keys[nIndex] == i)
                {
                    saveKey = true;
                }
            }

            if (!saveKey)
            {
                RemoveKey(pSpline, i);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
ISplineCtrlUndo* AbstractSplineWidget::CreateSplineCtrlUndoObject(std::vector<ISplineInterpolator*>& splineContainer)
{
    return new CUndoSplineCtrlEx(this, splineContainer);
}

#include <Animation/Controls/moc_UiSplineCtrlEx.cpp>
