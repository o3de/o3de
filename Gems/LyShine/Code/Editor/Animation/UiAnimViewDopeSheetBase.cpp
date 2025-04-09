/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "Editor/Resource.h"
#include "UiEditorAnimationBus.h"
#include "UiAnimViewDopeSheetBase.h"

#include "UiAnimViewDialog.h"
#include "AnimationContext.h"
#include "UiAnimViewUndo.h"
#include "UiAVCustomizeTrackColorsDlg.h"
#include "UiAnimViewAnimNode.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewSequence.h"

#include "Clipboard.h"
#include <Editor/Util/fastlib.h>

#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QRubberBand>
#include <QScrollBar>
#include <QStaticText>
#include <QToolTip>
#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif

#include <AzQtComponents/Components/Widgets/ColorPicker.h>

#define EDIT_DISABLE_GRAY_COLOR QColor(128, 128, 128)
#define KEY_TEXT_COLOR QColor(0, 0, 50)
#define INACTIVE_TEXT_COLOR QColor(128, 128, 128)

namespace
{
    const int kMarginForMagnetSnapping = 10;
    const unsigned int kDefaultTrackHeight = 16;
}

enum EUiAVMouseMode
{
    eUiAVMouseMode_None = 0,
    eUiAVMouseMode_Select = 1,
    eUiAVMouseMode_Move,
    eUiAVMouseMode_Clone,
    eUiAVMouseMode_DragTime,
    eUiAVMouseMode_DragStartMarker,
    eUiAVMouseMode_DragEndMarker,
    eUiAVMouseMode_Paste,
    eUiAVMouseMode_SelectWithinTime,
    eUiAVMouseMode_StartTimeAdjust,
    eUiAVMouseMode_EndTimeAdjust
};

//////////////////////////////////////////////////////////////////////////
CUiAnimViewDopeSheetBase::CUiAnimViewDopeSheetBase(QWidget* parent)
    : QWidget(parent)
{
    m_bkgrBrush = palette().color(QPalette::Window);
    m_bkgrBrushEmpty = QColor(190, 190, 190);
    m_timeBkgBrush = QColor(0xE0, 0xE0, 0xE0);
    m_timeHighlightBrush = QColor(0xFF, 0x0, 0x0);
    m_selectedBrush = QColor(200, 200, 230);
    m_visibilityBrush = QColor(120, 120, 255);
    m_selectTrackBrush = QColor(100, 190, 255);

    m_timeScale = 1.0f;
    m_ticksStep = 10;

    m_bZoomDrag = false;
    m_bMoveDrag = false;

    m_leftOffset = 30;
    m_scrollOffset = QPoint(0, 0);
    m_mouseMode = eUiAVMouseMode_None;
    m_currentTime = 0.0f;
    m_storedTime = m_currentTime;
    m_rcSelect = QRect(0, 0, 0, 0);
    m_rubberBand = 0;
    m_scrollBar = new QScrollBar(Qt::Horizontal, this);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, &CUiAnimViewDopeSheetBase::OnHScroll);
    m_keyTimeOffset = 0;
    m_currCursor = QCursor(Qt::ArrowCursor);
    m_mouseActionMode = eUiAVActionMode_MoveKey;

    m_scrollMin = 0;
    m_scrollMax = 1000;

    m_descriptionFont = QFont(QStringLiteral("Verdana"), 7);

    m_bCursorWasInKey = false;
    m_bJustSelected = false;
    m_snappingMode = eSnappingMode_SnapNone;
    m_snapFrameTime = 0.033333f;
    m_bMouseMovedAfterRButtonDown = false;

    m_tickDisplayMode = eUiAVTickMode_InSeconds;

    m_bEditLock = false;

    m_bFastRedraw = false;

    m_pLastTrackSelectedOnSpot = NULL;


#ifdef DEBUG
    m_redrawCount = 0;
#endif
    m_bKeysMoved = false;
    ComputeFrameSteps(GetVisibleRange());

    m_crsLeftRight = Qt::SizeHorCursor;

    m_crsAddKey = CMFCUtils::LoadCursor(IDC_ARROW_ADDKEY);
    m_crsCross = CMFCUtils::LoadCursor(IDC_POINTER_OBJHIT);
    m_crsAdjustLR = CMFCUtils::LoadCursor(IDC_LEFTRIGHT);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewDopeSheetBase::~CUiAnimViewDopeSheetBase()
{
    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    pAnimationContext->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewDopeSheetBase::TimeToClient(float time) const
{
    return static_cast<int>(m_leftOffset - m_scrollOffset.x() + (time * m_timeScale));
}

//////////////////////////////////////////////////////////////////////////
Range CUiAnimViewDopeSheetBase::GetVisibleRange() const
{
    Range r;
    r.start = (m_scrollOffset.x() - m_leftOffset) / m_timeScale;
    r.end = r.start + (m_rcClient.width()) / m_timeScale;

    Range extendedTimeRange(0.0f, m_timeRange.end);
    r = extendedTimeRange & r;

    return r;
}

//////////////////////////////////////////////////////////////////////////
Range CUiAnimViewDopeSheetBase::GetTimeRange(const QRect& rc) const
{
    Range r;
    r.start = (rc.left() - m_leftOffset + m_scrollOffset.x()) / m_timeScale;
    r.end = r.start + (rc.width()) / m_timeScale;

    r.start = TickSnap(r.start);
    r.end = TickSnap(r.end);

    // Intersect range with global time range.
    r = m_timeRange & r;

    return r;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetTimeRange(float start, float end)
{
    if (m_timeMarked.start < start)
    {
        m_timeMarked.start = start;
    }
    if (m_timeMarked.end > end)
    {
        m_timeMarked.end = end;
    }

    m_timeRange.Set(start, end);

    SetHorizontalExtent(-m_leftOffset, static_cast<int>(m_timeRange.end * m_timeScale - m_leftOffset));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetTimeScale(float timeScale, float fAnchorTime)
{
    const double fOldOffset = -fAnchorTime * m_timeScale;

    timeScale = std::max(timeScale, 0.001f);
    timeScale = std::min(timeScale, 100000.0f);
    m_timeScale = timeScale;

    int steps = 0;
    if (GetTickDisplayMode() == eUiAVTickMode_InSeconds)
    {
        m_ticksStep = 10;
    }
    else if (GetTickDisplayMode() == eUiAVTickMode_InFrames)
    {
        m_ticksStep = 1 / m_snapFrameTime;
    }
    else
    {
        assert(0);
    }

    double fPixelsPerTick;
    do
    {
        fPixelsPerTick = (1.0 / m_ticksStep) * (double)m_timeScale;

        if (fPixelsPerTick < 6.0)
        {
            m_ticksStep /= 2;
        }

        if (m_ticksStep <= 0)
        {
            m_ticksStep = 1;
            break;
        }
        steps++;
    }
    while (fPixelsPerTick < 6.0 && steps < 100);

    steps = 0;

    do
    {
        fPixelsPerTick = (1.0 / m_ticksStep) * (double)m_timeScale;
        if (fPixelsPerTick >= 12.0)
        {
            m_ticksStep *= 2;
        }
        if (m_ticksStep <= 0)
        {
            m_ticksStep = 1;
            break;
        }
        steps++;
    }
    while (fPixelsPerTick >= 12.0 && steps < 100);

    float fCurrentOffset = -fAnchorTime * m_timeScale;
    m_scrollOffset.rx() += static_cast<int>(fOldOffset - fCurrentOffset);

    update();

    SetHorizontalExtent(-m_leftOffset, static_cast<int>(m_timeRange.end * m_timeScale));

    ComputeFrameSteps(GetVisibleRange());
}

void CUiAnimViewDopeSheetBase::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    pAnimationContext->AddListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    m_rcClient = rect();

    m_offscreenBitmap = QPixmap(m_rcClient.width(), m_rcClient.height());
    m_offscreenBitmap.fill(Qt::transparent);

    m_rcTimeline = rect();
    m_rcTimeline.setHeight(kDefaultTrackHeight);
    m_rcSummary = m_rcTimeline;
    m_rcSummary.setTop(m_rcTimeline.bottom());
    m_rcSummary.setBottom(m_rcSummary.top() + 8);

    SetHorizontalExtent(m_scrollMin, m_scrollMax);

    m_scrollBar->setGeometry(0, height() - m_scrollBar->sizeHint().height(), width(), m_scrollBar->sizeHint().height());

    QToolTip::hideText();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::wheelEvent(QWheelEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        event->ignore();
        return;
    }

    float z = (event->angleDelta().y() > 0) ? (m_timeScale * 1.25f) : (m_timeScale * 0.8f);

    const QPoint pt = event->position().toPoint();

    float fAnchorTime = TimeFromPointUnsnapped(pt);
    SetTimeScale(z, fAnchorTime);

    event->accept();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnHScroll()
{
    // Get the current position of scroll box.
    int curpos = m_scrollBar->value();
    m_scrollOffset.setX(curpos);
    update();
}

int CUiAnimViewDopeSheetBase::GetScrollPos() const
{
    return m_scrollBar->value();
}

//////////////////////////////////////////////////////////////////////////
double CUiAnimViewDopeSheetBase::GetTickTime() const
{
    if (GetTickDisplayMode() == eUiAVTickMode_InFrames)
    {
        return m_fFrameTickStep;
    }
    else
    {
        return 1.0f / m_ticksStep;
    }
}


//////////////////////////////////////////////////////////////////////////
float CUiAnimViewDopeSheetBase::TickSnap(float time) const
{
    double tickTime = GetTickTime();
    double t = floor(((double)time / tickTime) + 0.5);
    t *= tickTime;
    return static_cast<float>(t);
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewDopeSheetBase::TimeFromPoint(const QPoint& point) const
{
    int x = point.x() - m_leftOffset + m_scrollOffset.x();
    float t = static_cast<float>(x) / m_timeScale;
    return TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewDopeSheetBase::TimeFromPointUnsnapped(const QPoint& point) const
{
    int x = point.x() - m_leftOffset + m_scrollOffset.x();
    double t = (double)x / m_timeScale;
    return static_cast<float>(t);
}

void CUiAnimViewDopeSheetBase::mousePressEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDown(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonDown(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonDown(event->modifiers(), event->pos());
        break;
    default:
        break;
    }
}

void CUiAnimViewDopeSheetBase::mouseReleaseEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonUp(event->modifiers(), event->pos());
        break;
    case Qt::RightButton:
        OnRButtonUp(event->modifiers(), event->pos());
        break;
    case Qt::MiddleButton:
        OnMButtonUp(event->modifiers(), event->pos());
        break;
    default:
        break;
    }
}

void CUiAnimViewDopeSheetBase::mouseDoubleClickEvent(QMouseEvent* event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        OnLButtonDblClk(event->modifiers(), event->pos());
        break;
    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    // KDAB: workaround until the Key Properties is fully ported to Qt
    clearFocus();
    setFocus(Qt::MouseFocusReason);

    if (m_rcTimeline.contains(point))
    {
        m_mouseDownPos = point;

        // Clicked inside timeline.
        m_mouseMode = eUiAVMouseMode_DragTime;
        // If mouse over selected key, change cursor to left-right arrows.
        SetMouseCursor(m_crsLeftRight);

        SetCurrTime(TimeFromPoint(point));
        return;
    }

    if (m_bEditLock)
    {
        m_mouseDownPos = point;
        return;
    }

    if (m_mouseMode == eUiAVMouseMode_Paste)
    {
        m_mouseMode = eUiAVMouseMode_None;

        CUiAnimViewAnimNode* pAnimNode = GetAnimNodeFromPoint(m_mouseOverPos);
        CUiAnimViewTrack* pTrack = GetTrackFromPoint(m_mouseOverPos);

        if (pAnimNode)
        {
            UiAnimUndo undo("Paste Keys");
            UiAnimUndo::Record(new CUndoAnimKeySelection(pSequence));
            pSequence->DeselectAllKeys();
            pSequence->PasteKeysFromClipboard(pAnimNode, pTrack, ComputeSnappedMoveOffset());
        }

        SetMouseCursor(Qt::ArrowCursor);
        OnCaptureChanged();
        return;
    }

    m_mouseDownPos = point;

    // The summary region is used for moving already selected keys.
    if (m_rcSummary.contains(point))
    {
        CUiAnimViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();
        if (selectedKeys.GetKeyCount() > 0)
        {
            /// Move/Clone Key Undo Begin
            UiAnimUndoManager::Get()->Begin();
            pSequence->StoreUndoForTracksWithSelectedKeys();
            StoreMementoForTracksWithSelectedKeys();

            m_keyTimeOffset = 0;
            m_mouseMode = eUiAVMouseMode_Move;
            SetMouseCursor(m_crsLeftRight);
            return;
        }
    }

    bool bStart = false;
    CUiAnimViewKeyHandle keyHandle = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
    if (keyHandle.IsValid())
    {
        return LButtonDownOnTimeAdjustBar(point, keyHandle, bStart);
    }

    keyHandle = FirstKeyFromPoint(point);
    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }
    else
    {
        return LButtonDownOnKey(point, keyHandle, modifiers);
    }

    if (m_mouseActionMode == eUiAVActionMode_AddKeys)
    {
        AddKeys(point, modifiers & Qt::ShiftModifier);
        return;
    }

    if (modifiers & Qt::ShiftModifier)
    {
        m_mouseMode = eUiAVMouseMode_SelectWithinTime;
    }
    else
    {
        m_mouseMode = eUiAVMouseMode_Select;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnLButtonUp(Qt::KeyboardModifiers modifiers, [[maybe_unused]] const QPoint& point)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (!pSequence)
    {
        return;
    }

    if (m_mouseMode == eUiAVMouseMode_Select)
    {
        // Check if any key are selected.
        m_rcSelect.translate(-m_scrollOffset);
        SelectKeys(m_rcSelect, modifiers & Qt::ControlModifier);
        m_rcSelect = QRect();
        m_rubberBand->deleteLater();
        m_rubberBand = 0;
    }
    else if (m_mouseMode == eUiAVMouseMode_SelectWithinTime)
    {
        m_rcSelect.translate(-m_scrollOffset);
        SelectAllKeysWithinTimeFrame(m_rcSelect, modifiers & Qt::ControlModifier);
        m_rcSelect = QRect();
        m_rubberBand->deleteLater();
        m_rubberBand = 0;
    }
    else if (m_mouseMode == eUiAVMouseMode_DragTime)
    {
        SetMouseCursor(Qt::ArrowCursor);
    }
    else if (m_mouseMode == eUiAVMouseMode_Paste)
    {
        SetMouseCursor(Qt::ArrowCursor);
    }

    OnCaptureChanged();

    m_keyTimeOffset = 0;
    m_keyForTimeAdjust = CUiAnimViewKeyHandle();

    AcceptUndo();

    update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence || m_rcTimeline.contains(point) || m_bEditLock)
    {
        return;
    }

    CUiAnimViewKeyHandle keyHandle = FirstKeyFromPoint(point);

    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }
    else
    {
        UiAnimUndoManager::Get()->Begin();
        CUndoAnimKeySelection* pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
        UiAnimUndo::Record(pUndoKeySelection);

        CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);
        if (pTrack)
        {
            CUiAnimViewSequenceNotificationContext context(pSequence);
            pSequence->DeselectAllKeys();
            keyHandle.Select(true);

            m_keyTimeOffset = 0;

            if (pUndoKeySelection->IsSelectionChanged())
            {
                UiAnimUndoManager::Get()->Accept("Select Key");
            }
            else
            {
                UiAnimUndoManager::Get()->Cancel();
            }
        }

        return;
    }

    const bool bTryAddKeysInGroup = modifiers & Qt::ShiftModifier;

    AddKeys(point, bTryAddKeysInGroup);

    m_mouseMode = eUiAVMouseMode_None;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    OnRButtonDown(modifiers, point);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    OnRButtonUp(modifiers, point);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    m_bCursorWasInKey = false;
    m_bMouseMovedAfterRButtonDown = false;

    // KDAB: workaround until the Key Properties is fully ported to Qt
    clearFocus();
    setFocus(Qt::MouseFocusReason);

    if (m_rcTimeline.contains(point))
    {
        // Clicked inside timeline.
        // adjust markers.
        int nMarkerStart = TimeToClient(m_timeMarked.start);
        int nMarkerEnd = TimeToClient(m_timeMarked.end);
        if ((abs(point.x() - nMarkerStart)) < (abs(point.x() - nMarkerEnd)))
        {
            SetStartMarker(TimeFromPoint(point));
            m_mouseMode = eUiAVMouseMode_DragStartMarker;
        }
        else
        {
            SetEndMarker(TimeFromPoint(point));
            m_mouseMode = eUiAVMouseMode_DragEndMarker;
        }
        return;
    }

    m_mouseDownPos = point;

    if (modifiers & Qt::ShiftModifier)  // alternative zoom
    {
        m_bZoomDrag = true;
        return;
    }

    CUiAnimViewKeyHandle keyHandle = FirstKeyFromPoint(point);
    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }

    if (keyHandle.IsValid())
    {
        m_bCursorWasInKey = true;

        keyHandle.Select(true);
        m_keyTimeOffset = 0;
        update();

        // Show a little pop-up menu for copy & delete.
        QMenu menu;

        QAction* actionCopy = menu.addAction(tr("Copy"));
        QAction* actionDelete = menu.addAction(tr("Delete"));

        const QPoint p = QCursor::pos();
        QAction* action = menu.exec(p);
        if (action == actionCopy)
        {
            pSequence->CopyKeysToClipboard(true, false);
        }
        else if (action == actionDelete)
        {
            UiAnimUndo undo("Delete Keys");
            pSequence->DeleteSelectedKeys();
        }
    }
    else
    {
        m_bMoveDrag = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnRButtonUp([[maybe_unused]] Qt::KeyboardModifiers modifiers, [[maybe_unused]] const QPoint& point)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    m_bZoomDrag = false;
    m_bMoveDrag = false;

    OnCaptureChanged();

    m_mouseMode = eUiAVMouseMode_None;

    if (!m_bCursorWasInKey)
    {
        const bool bHasCopiedKey = (GetKeysInClickboard() != NULL);

        if (bHasCopiedKey && m_bMouseMovedAfterRButtonDown == false)    // Once moved, it means the user wanted to scroll, so no paste pop-up.
        {
            // Show a little pop-up menu for paste.
            QMenu menu;
            QAction* actionPaste = menu.addAction(tr("Paste"));

            QAction* action = menu.exec(QCursor::pos());
            if (action == actionPaste)
            {
                StartPasteKeys();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::mouseMoveEvent(QMouseEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    // To prevent the key moving while selecting
    if (m_bJustSelected)
    {
        m_bJustSelected = false;
        return;
    }

    m_bMouseMovedAfterRButtonDown = true;
    m_mouseOverPos = event->pos();

    if (m_bZoomDrag && (event->modifiers() & Qt::ShiftModifier))
    {
        float fAnchorTime = TimeFromPointUnsnapped(m_mouseDownPos);
        SetTimeScale(m_timeScale * (1.0f + (event->pos().x() - m_mouseDownPos.x()) * 0.0025f), fAnchorTime);
        m_mouseDownPos = event->pos();
        return;
    }
    else
    {
        m_bZoomDrag = false;
    }

    if (m_bMoveDrag)
    {
        m_scrollOffset.setX(qBound(m_scrollMin, m_scrollOffset.x() + m_mouseDownPos.x() - event->pos().x(), m_scrollMax));
        m_mouseDownPos = event->pos();
        // Set the new position of the thumb (scroll box).
        m_scrollBar->setValue(m_scrollOffset.x());
        update();
        SetMouseCursor(m_crsLeftRight);
        return;
    }

    if (m_mouseMode == eUiAVMouseMode_Select
        || m_mouseMode == eUiAVMouseMode_SelectWithinTime)
    {
        MouseMoveSelect(event->pos());
    }
    else if (m_mouseMode == eUiAVMouseMode_Move)
    {
        MouseMoveMove(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == eUiAVMouseMode_Clone)
    {
        pSequence->CloneSelectedKeys();
        m_mouseMode = eUiAVMouseMode_Move;
    }
    else if (m_mouseMode == eUiAVMouseMode_DragTime)
    {
        MouseMoveDragTime(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == eUiAVMouseMode_DragStartMarker)
    {
        MouseMoveDragStartMarker(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == eUiAVMouseMode_DragEndMarker)
    {
        MouseMoveDragEndMarker(event->pos(), event->modifiers());
    }
    else if (m_mouseMode == eUiAVMouseMode_Paste)
    {
        update();
    }
    else if (m_mouseMode == eUiAVMouseMode_StartTimeAdjust)
    {
        MouseMoveStartEndTimeAdjust(event->pos(), true);
    }
    else if (m_mouseMode == eUiAVMouseMode_EndTimeAdjust)
    {
        MouseMoveStartEndTimeAdjust(event->pos(), false);
    }
    else
    {
        //////////////////////////////////////////////////////////////////////////
        if (m_mouseActionMode == eUiAVActionMode_AddKeys)
        {
            SetMouseCursor(m_crsAddKey);
        }
        else
        {
            MouseMoveOver(event->pos());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    {
        // In case of the fast-redraw mode, just draw the saved bitmap.
        // Otherwise, actually redraw all things.
        // This mode is helpful when playing a sequence if the sequence has a lot of keys.
        if (!m_bFastRedraw)
        {
            QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
            gradient.setColorAt(0, QColor(250, 250, 250));
            gradient.setColorAt(1, QColor(220, 220, 220));
            painter.fillRect(rect(), gradient);

            if (pSequence)
            {
                if (m_bEditLock)
                {
                    painter.fillRect(event->rect(), EDIT_DISABLE_GRAY_COLOR);
                }

                DrawControl(&painter, event->rect());
            }
        }
    }

    if (pSequence)
    {
        // Drawing the timeline is handled separately. In other words, it's not saved to the 'm_offscreenBitmap'.
        // This is for the fast-redraw mode mentioned above.
        DrawTimeline(&painter, event->rect());
    }

#ifdef DEBUG
    painter.setFont(m_descriptionFont);
    painter.setPen(QColor(255, 255, 255));
    painter.setBrush(QColor(0, 0, 0));

    const QString redrawCountStr = QString::fromLatin1("Redraw Count: %1").arg(m_redrawCount);
    QRect redrawCountRect(0, 0, 150, 20);

    QRect bounds;
    painter.drawText(redrawCountRect, Qt::AlignLeft | Qt::TextSingleLine, redrawCountStr, &bounds);
    painter.fillRect(bounds, Qt::black);
    painter.drawText(redrawCountRect, Qt::AlignLeft | Qt::TextSingleLine, redrawCountStr);

    ++m_redrawCount;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SelectAllKeysWithinTimeFrame(const QRect& rc, const bool bMultiSelection)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    UiAnimUndoManager::Get()->Begin();
    CUndoAnimKeySelection* pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
    UiAnimUndo::Record(pUndoKeySelection);

    if (!bMultiSelection)
    {
        pSequence->DeselectAllKeys();
    }

    // put selection rectangle from client to track space.
    QRect trackRect = rc;
    trackRect.translate(m_scrollOffset);

    Range selTime = GetTimeRange(trackRect);

    CUiAnimViewTrackBundle tracks = pSequence->GetAllTracks();

    CUiAnimViewSequenceNotificationContext context(pSequence);
    for (unsigned int i = 0; i < tracks.GetCount(); ++i)
    {
        CUiAnimViewTrack* pTrack = tracks.GetTrack(i);

        // Check which keys we intersect.
        for (unsigned int j = 0; j < pTrack->GetKeyCount(); j++)
        {
            CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(j);
            const float time = keyHandle.GetTime();

            if (selTime.IsInside(time))
            {
                keyHandle.Select(true);
            }
        }
    }

    if (pUndoKeySelection->IsSelectionChanged())
    {
        UiAnimUndoManager::Get()->Accept("Select keys");
    }
    else
    {
        UiAnimUndoManager::Get()->Cancel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetMouseCursor(const QCursor& cursor)
{
    m_currCursor = cursor;
    setCursor(m_currCursor);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetCurrTime(float time)
{
    if (time < m_timeRange.start)
    {
        time = m_timeRange.start;
    }
    if (time > m_timeRange.end)
    {
        time = m_timeRange.end;
    }

    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    pAnimationContext->SetTime(time);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnTimeChanged(float newTime)
{
    int x1 = TimeToClient(m_currentTime);
    int x2 = TimeToClient(newTime);

    m_currentTime = newTime;

    m_bFastRedraw = true;
    const QRect rc(QPoint(x1 - 3, m_rcClient.top()), QPoint(x1 + 4, m_rcClient.bottom()));
    update(rc);
    const QRect rc1(QPoint(x2 - 3, m_rcClient.top()), QPoint(x2 + 4, m_rcClient.bottom()));
    update(rc1);
    m_bFastRedraw = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetStartMarker(float fTime)
{
    m_timeMarked.start = fTime;

    if (m_timeMarked.start < m_timeRange.start)
    {
        m_timeMarked.start = m_timeRange.start;
    }
    if (m_timeMarked.start > m_timeRange.end)
    {
        m_timeMarked.start = m_timeRange.end;
    }
    if (m_timeMarked.start > m_timeMarked.end)
    {
        m_timeMarked.end = m_timeMarked.start;
    }

    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    pAnimationContext->SetMarkers(m_timeMarked);
    update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetEndMarker(float fTime)
{
    m_timeMarked.end = fTime;
    if (m_timeMarked.end < m_timeRange.start)
    {
        m_timeMarked.end = m_timeRange.start;
    }
    if (m_timeMarked.end > m_timeRange.end)
    {
        m_timeMarked.end = m_timeRange.end;
    }
    if (m_timeMarked.start > m_timeMarked.end)
    {
        m_timeMarked.start = m_timeMarked.end;
    }
    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    pAnimationContext->SetMarkers(m_timeMarked);
    update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetMouseActionMode(EUiAVActionMode mode)
{
    m_mouseActionMode = mode;
    if (mode == eUiAVActionMode_AddKeys)
    {
        setCursor(m_crsAddKey);
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewDopeSheetBase::GetNodeFromPointRec(CUiAnimViewNode* pCurrentNode, const QPoint& point)
{
    QRect currentNodeRect = GetNodeRect(pCurrentNode);

    if (currentNodeRect.top() > point.y())
    {
        return nullptr;
    }

    if (currentNodeRect.bottom() >= point.y())
    {
        return pCurrentNode;
    }

    if (pCurrentNode->IsExpanded())
    {
        unsigned int childCount = pCurrentNode->GetChildCount();
        for (unsigned int i = 0; i < childCount; ++i)
        {
            CUiAnimViewNode* pFoundNode = GetNodeFromPointRec(pCurrentNode->GetChild(i), point);
            if (pFoundNode)
            {
                return pFoundNode;
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNode* CUiAnimViewDopeSheetBase::GetNodeFromPoint(const QPoint& point)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    return GetNodeFromPointRec(pSequence, point);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewAnimNode* CUiAnimViewDopeSheetBase::GetAnimNodeFromPoint(const QPoint& point)
{
    CUiAnimViewNode* pNode = GetNodeFromPoint(point);

    if (pNode)
    {
        if (pNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pTrack = static_cast<CUiAnimViewTrack*>(pNode);
            return static_cast<CUiAnimViewAnimNode*>(pTrack->GetAnimNode());
        }
        else if (pNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            return static_cast<CUiAnimViewAnimNode*>(pNode);
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewTrack* CUiAnimViewDopeSheetBase::GetTrackFromPoint(const QPoint& point)
{
    CUiAnimViewNode* pNode = GetNodeFromPoint(point);

    if (pNode && pNode->GetNodeType() == eUiAVNT_Track)
    {
        return static_cast<CUiAnimViewTrack*>(pNode);
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetHorizontalExtent(int min, int max)
{
    m_scrollMin = min;
    m_scrollMax = max;
    m_scrollBar->setPageStep(m_rcClient.width() / 2);
    m_scrollBar->setRange(min, max - m_scrollBar->pageStep() * 2 + m_leftOffset);
};

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CUiAnimViewDopeSheetBase::GetKeysInClickboard()
{
    CClipboard clip(this);
    if (clip.IsEmpty())
    {
        return NULL;
    }

    if (clip.GetTitle() != "Track view keys")
    {
        return NULL;
    }

    XmlNodeRef copyNode = clip.Get();
    if (copyNode == NULL || strcmp(copyNode->getTag(), "CopyKeysNode"))
    {
        return NULL;
    }

    int nNumTracksToPaste = copyNode->getChildCount();
    if (nNumTracksToPaste == 0)
    {
        return NULL;
    }

    return copyNode;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::StartPasteKeys()
{
    m_clipboardKeys = GetKeysInClickboard();

    if (m_clipboardKeys)
    {
        m_mouseMode = eUiAVMouseMode_Paste;
        // If mouse over selected key, change cursor to left-right arrows.
        SetMouseCursor(m_crsLeftRight);
        m_mouseDownPos = m_mouseOverPos;
    }
}

void CUiAnimViewDopeSheetBase::keyPressEvent(QKeyEvent* event)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    if (event->matches(QKeySequence::Delete))
    {
        UiAnimUndo undo("Delete Keys");
        pSequence->DeleteSelectedKeys();
        return;
    }

    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down || event->key() == Qt::Key_Right || event->key() == Qt::Key_Left)
    {
        CUiAnimViewKeyBundle keyBundle = pSequence->GetSelectedKeys();
        CUiAnimViewKeyHandle keyHandle = keyBundle.GetSingleSelectedKey();

        if (keyHandle.IsValid())
        {
            switch (event->key())
            {
            case Qt::Key_Up:
                keyHandle = keyHandle.GetAboveKey();
                break;
            case Qt::Key_Down:
                keyHandle = keyHandle.GetBelowKey();
                break;
            case Qt::Key_Right:
                keyHandle = keyHandle.GetNextKey();
                break;
            case Qt::Key_Left:
                keyHandle = keyHandle.GetPrevKey();
                break;
            }

            if (keyHandle.IsValid())
            {
                UiAnimUndoManager::Get()->Begin();
                CUndoAnimKeySelection* pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
                UiAnimUndo::Record(pUndoKeySelection);

                CUiAnimViewSequenceNotificationContext context(pSequence);
                pSequence->DeselectAllKeys();
                keyHandle.Select(true);

                if (pUndoKeySelection->IsSelectionChanged())
                {
                    UiAnimUndoManager::Get()->Accept("Select Key");
                }
                else
                {
                    UiAnimUndoManager::Get()->Cancel();
                }
            }
        }

        return;
    }

    if (event->matches(QKeySequence::Copy))
    {
        pSequence->CopyKeysToClipboard(true, false);
    }
    else if (event->matches(QKeySequence::Paste))
    {
        StartPasteKeys();
    }
    else if (event->matches(QKeySequence::Undo))
    {
        UiAnimUndoManager::Get()->Undo();
    }
    else if (event->matches(QKeySequence::Redo))
    {
        UiAnimUndoManager::Get()->Redo();
    }
    else
    {
        return QWidget::keyPressEvent(event);
    }
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::RecordTrackUndo(CUiAnimViewTrack* pTrack)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pTrack && pSequence)
    {
        UiAnimUndo undo("Track Modify");
        UiAnimUndo::Record(new CUndoTrackObject(pTrack, pSequence));
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::ShowKeyTooltip(const CUiAnimViewKeyHandle& keyHandle, const QPoint& point)
{
    if (m_lastTooltipPos == point)
    {
        return;
    }

    m_lastTooltipPos = point;

    const float time = keyHandle.GetTime();
    const char* desc = keyHandle.GetDescription();

    QString tipText;
    if (GetTickDisplayMode() == eUiAVTickMode_InSeconds)
    {
        tipText = tr("%1, {%2}").arg(time, 0, 'f', 3).arg(desc);
    }
    else
    {
        tipText = tr("%1, {%2}").arg(ftoi(time / m_snapFrameTime)).arg(desc);
    }

    QToolTip::showText(point, tipText);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::OnCaptureChanged()
{
    AcceptUndo();

    m_bZoomDrag = false;
    m_bMoveDrag = false;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewDopeSheetBase::IsOkToAddKeyHere(const CUiAnimViewTrack* pTrack, float time) const
{
    for (unsigned int i = 0; i < pTrack->GetKeyCount(); ++i)
    {
        CUiAnimViewKeyHandle keyHandle = const_cast<CUiAnimViewTrack*>(pTrack)->GetKey(i);

        if (keyHandle.GetTime() == time)
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::MouseMoveSelect(const QPoint& point)
{
    SetMouseCursor(Qt::ArrowCursor);
    QRect rc(m_mouseDownPos, point);
    rc = rc.normalized();
    QRect rcClient = rect();
    rc = rc.intersected(rcClient);

    if (m_rubberBand == nullptr)
    {
        m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    }
    m_rubberBand->show();
    if (m_mouseMode == eUiAVMouseMode_SelectWithinTime)
    {
        rc.setTop(m_rcClient.top());
        rc.setBottom(m_rcClient.bottom());
    }

    m_rcSelect = rc;
    m_rubberBand->setGeometry(m_rcSelect);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::MouseMoveStartEndTimeAdjust(const QPoint& p, bool bStart)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    SetMouseCursor(m_crsAdjustLR);
    const QPoint point(qBound(m_rcClient.left(), p.x(), m_rcClient.right()), p.y());

    const QPoint ofs = point - m_mouseDownPos;

    CUiAnimViewKeyHandle& keyHandle = m_keyForTimeAdjust;

    ICharacterKey characterKey;
    keyHandle.GetKey(&characterKey);

    float& timeToAdjust = bStart ? characterKey.m_startTime : characterKey.m_endTime;

    // Undo the last offset.
    timeToAdjust += -m_keyTimeOffset;

    // Apply a new offset.
    m_keyTimeOffset = (ofs.x() / m_timeScale) * characterKey.m_speed;
    timeToAdjust += m_keyTimeOffset;

    // Check the validity.
    if (bStart)
    {
        if (timeToAdjust < 0)
        {
            timeToAdjust = 0;
        }
        else if (timeToAdjust > characterKey.GetValidEndTime())
        {
            timeToAdjust = characterKey.GetValidEndTime();
        }
    }
    else
    {
        if (timeToAdjust < characterKey.m_startTime)
        {
            timeToAdjust = characterKey.m_startTime;
        }
        else if (timeToAdjust > characterKey.GetValidEndTime())
        {
            timeToAdjust = characterKey.GetValidEndTime();
        }
    }

    UiAnimUndo::Record(new CUndoTrackObject(m_keyForTimeAdjust.GetTrack(), pSequence));
    keyHandle.SetKey(&characterKey);

    update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::MouseMoveMove(const QPoint& p, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    CUiAnimViewSequenceNotificationContext context(pSequence);

    SetMouseCursor(m_crsLeftRight);
    const QPoint point(qBound(m_rcClient.left(), p.x(), m_rcClient.right()), p.y());

    // Reset tracks to their initial state before starting the move
    for (auto iter = m_trackMementos.begin(); iter != m_trackMementos.end(); ++iter)
    {
        CUiAnimViewTrack* pTrack = iter->first;

        const TrackMemento& trackMemento = iter->second;
        pTrack->RestoreFromMemento(trackMemento.m_memento);

        const size_t numKeys = trackMemento.m_keySelectionStates.size();
        for (size_t i = 0; i < numKeys; ++i)
        {
            pTrack->GetKey(static_cast<unsigned int>(i)).Select(trackMemento.m_keySelectionStates[i]);
        }
    }

    CUiAnimViewKeyHandle keyHandle = FirstKeyFromPoint(m_mouseDownPos);
    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(m_mouseDownPos);
    }

    float oldTime;
    if (keyHandle.IsValid())
    {
        oldTime = keyHandle.GetTime();
    }
    else
    {
        oldTime = TimeFromPointUnsnapped(m_mouseDownPos);
    }

    QPoint ofs = point - m_mouseDownPos;
    float timeOffset = ofs.x() / m_timeScale;
    float newTime = oldTime + timeOffset;

    // Snap it, if necessary.
    ESnappingMode snappingMode = GetKeyModifiedSnappingMode();
    if (snappingMode == eSnappingMode_SnapFrame)
    {
        snappingMode = m_snappingMode;
    }

    if (snappingMode == eSnappingMode_SnapMagnet)
    {
        newTime = MagnetSnap(newTime, GetAnimNodeFromPoint(m_mouseOverPos));
    }

    else if (snappingMode == eSnappingMode_SnapTick)
    {
        newTime = TickSnap(newTime);
    }

    else if (snappingMode == eSnappingMode_SnapFrame)
    {
        newTime = FrameSnap(newTime);
    }

    Range extendedTimeRange(0.0f, m_timeRange.end);
    extendedTimeRange.ClipValue(newTime);

    timeOffset = newTime - oldTime; // Re-compute the time offset using snapped & clipped 'newTime'.
    if (timeOffset == 0.0f)
    {
        return;
    }

    m_bKeysMoved = true;

    if (m_mouseActionMode == eUiAVActionMode_ScaleKey)
    {
        float tscale = 0.005f;
        float tofs = ofs.x() * tscale;
        tofs = pSequence->ClipTimeOffsetForScaling(1 + tofs) - 1;
        // Offset all selected keys by this offset.
        pSequence->ScaleSelectedKeys(1 + tofs);
        m_keyTimeOffset = tofs;
    }
    else
    {
        // Offset all selected keys by this offset.
        if (m_mouseActionMode == eUiAVActionMode_SlideKey)
        {
            timeOffset = pSequence->ClipTimeOffsetForSliding(timeOffset);
            pSequence->SlideKeys(timeOffset);
        }
        else
        {
            timeOffset = pSequence->ClipTimeOffsetForOffsetting(timeOffset);
            pSequence->OffsetSelectedKeys(timeOffset);
        }

        if (CheckVirtualKey(Qt::Key_Menu))
        {
            CUiAnimViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();
            CUiAnimViewKeyHandle selectedKey = selectedKeys.GetSingleSelectedKey();

            if (selectedKey.IsValid())
            {
                CUiAnimationContext* pAnimationContext = nullptr;
                UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
                pAnimationContext->SetTime(selectedKey.GetTime());
            }
        }
        m_keyTimeOffset = timeOffset;
    }
}

void CUiAnimViewDopeSheetBase::MouseMoveDragTime(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    const QPoint p(qBound(m_rcClient.left(), point.x(), m_rcClient.right()),
        qBound(m_rcClient.top(), point.y(), m_rcClient.bottom()));

    float time = TimeFromPointUnsnapped(p);
    m_timeRange.ClipValue(time);

    bool bSnap = (modifiers & Qt::ControlModifier);
    if (bSnap)
    {
        time = TickSnap(time);
    }
    SetCurrTime(time);
}

void CUiAnimViewDopeSheetBase::MouseMoveDragStartMarker(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    const QPoint p(qBound(m_rcClient.left(), point.x(), m_rcClient.right()),
        qBound(m_rcClient.top(), point.y(), m_rcClient.bottom()));

    bool bNoSnap = (modifiers & Qt::ControlModifier);
    float time = TimeFromPointUnsnapped(p);
    m_timeRange.ClipValue(time);
    if (!bNoSnap)
    {
        time = TickSnap(time);
    }
    SetStartMarker(time);
}

void CUiAnimViewDopeSheetBase::MouseMoveDragEndMarker(const QPoint& point, Qt::KeyboardModifiers modifiers)
{
    const QPoint p(qBound(m_rcClient.left(), point.x(), m_rcClient.right()),
        qBound(m_rcClient.top(), point.y(), m_rcClient.bottom()));

    bool bNoSnap = (modifiers & Qt::ControlModifier);
    float time = TimeFromPointUnsnapped(p);
    m_timeRange.ClipValue(time);
    if (!bNoSnap)
    {
        time = TickSnap(time);
    }
    SetEndMarker(time);
}

void CUiAnimViewDopeSheetBase::MouseMoveOver(const QPoint& point)
{
    // No mouse mode.
    SetMouseCursor(Qt::ArrowCursor);

    bool bStart = false;
    CUiAnimViewKeyHandle keyHandle = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
    if (keyHandle.IsValid())
    {
        SetMouseCursor(m_crsAdjustLR);
        return;
    }

    keyHandle = FirstKeyFromPoint(point);
    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }

    if (keyHandle.IsValid())
    {
        CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);

        if (pTrack && keyHandle.IsSelected())
        {
            // If mouse over selected key, change cursor to left-right arrows.
            SetMouseCursor(m_crsLeftRight);
        }
        else
        {
            SetMouseCursor(m_crsCross);
        }

        if (pTrack)
        {
            ShowKeyTooltip(keyHandle, mapToGlobal(point));
        }
    }
    else
    {
        QToolTip::hideText();
    }
}

float CUiAnimViewDopeSheetBase::MagnetSnap(float newTime, const CUiAnimViewAnimNode* pNode) const
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return newTime;
    }

    CUiAnimViewKeyBundle keys = pSequence->GetKeysInTimeRange(newTime - kMarginForMagnetSnapping / m_timeScale,
            newTime + kMarginForMagnetSnapping / m_timeScale);

    if (keys.GetKeyCount() > 0)
    {
        // By default, just use the first key that belongs to the time range as a magnet.
        newTime = keys.GetKey(0).GetTime();
        // But if there is an in-range key in a sibling track, use it instead.
        // Here a 'sibling' means a track that belongs to a same node.
        for (unsigned int i = 0; i < keys.GetKeyCount(); ++i)
        {
            CUiAnimViewKeyHandle keyHandle = keys.GetKey(i);
            if (keyHandle.GetTrack()->GetAnimNode() == pNode)
            {
                newTime = keyHandle.GetTime();
                break;
            }
        }
    }

    return newTime;
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewDopeSheetBase::FrameSnap(float time) const
{
    double t = floor((double)time / m_snapFrameTime + 0.5);
    t = t * m_snapFrameTime;
    return static_cast<float>(t);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetScrollOffset(int hpos)
{
    m_scrollBar->setValue(hpos);
    m_scrollOffset.setX(hpos);
    update();
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewDopeSheetBase::GetScrollOffset()
{
    return m_scrollOffset.x();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::LButtonDownOnTimeAdjustBar([[maybe_unused]] const QPoint& point, CUiAnimViewKeyHandle& keyHandle, bool bStart)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    m_keyTimeOffset = 0;
    m_keyForTimeAdjust = keyHandle;

    UiAnimUndoManager::Get()->Begin();

    if (bStart)
    {
        m_mouseMode = eUiAVMouseMode_StartTimeAdjust;
    }
    else
    {
        // In case of the end time, make it have a valid (not zero)
        // end time, first.
        ICharacterKey animKey;
        keyHandle.GetKey(&animKey);

        if (animKey.m_endTime == 0)
        {
            animKey.m_endTime = animKey.m_duration;
            UiAnimUndo::Record(new CUndoTrackObject(keyHandle.GetTrack(), pSequence));
            keyHandle.SetKey(&animKey);
        }
        m_mouseMode = eUiAVMouseMode_EndTimeAdjust;
    }
    SetMouseCursor(m_crsAdjustLR);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::LButtonDownOnKey([[maybe_unused]] const QPoint& point, CUiAnimViewKeyHandle& keyHandle, Qt::KeyboardModifiers modifiers)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (!keyHandle.IsSelected() && !(modifiers & Qt::ControlModifier))
    {
        UiAnimUndo undo("Select Keys");
        CUndoAnimKeySelection* pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
        UiAnimUndo::Record(pUndoKeySelection);

        CUiAnimViewSequenceNotificationContext context(pSequence);
        pSequence->DeselectAllKeys();
        m_bJustSelected = true;
        m_keyTimeOffset = 0;
        keyHandle.Select(true);

        if (!pUndoKeySelection->IsSelectionChanged())
        {
            undo.Cancel();
        }
    }
    else
    {
        UiAnimUndoManager::Get()->Cancel();
    }

    /// Move/Clone Key Undo Begin
    UiAnimUndoManager::Get()->Begin();
    pSequence->StoreUndoForTracksWithSelectedKeys();
    StoreMementoForTracksWithSelectedKeys();

    if (modifiers & Qt::ShiftModifier)
    {
        m_mouseMode = eUiAVMouseMode_Clone;
        SetMouseCursor(m_crsLeftRight);
    }
    else
    {
        m_mouseMode = eUiAVMouseMode_Move;
        SetMouseCursor(m_crsLeftRight);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimViewDopeSheetBase::CreateColorKey(CUiAnimViewTrack* pTrack, float keyTime)
{
    bool keyCreated = false;
    Vec3 vColor(0, 0, 0);
    pTrack->GetValue(keyTime, vColor);

    const AZ::Color defaultColor = AZ::Color::CreateFromRgba(
        clamp_tpl(static_cast<AZ::u8>(FloatToIntRet(vColor.x)), AZ::u8(0), AZ::u8(255)),
        clamp_tpl(static_cast<AZ::u8>(FloatToIntRet(vColor.y)), AZ::u8(0), AZ::u8(255)),
        clamp_tpl(static_cast<AZ::u8>(FloatToIntRet(vColor.z)), AZ::u8(0), AZ::u8(255)), 255);
    AzQtComponents::ColorPicker dlg(AzQtComponents::ColorPicker::Configuration::RGB, tr("Select Color"), this);
    dlg.setCurrentColor(defaultColor);
    dlg.setSelectedColor(defaultColor);
    if (dlg.exec() == QDialog::Accepted)
    {
        const AZ::Color col = dlg.selectedColor().GammaToLinear();
        const ColorF colArray(col.GetR(), col.GetG(), col.GetB(), col.GetA());

        RecordTrackUndo(pTrack);
        CUiAnimViewSequenceNotificationContext context(pTrack->GetSequence());

        const unsigned int numChildNodes = pTrack->GetChildCount();
        for (unsigned int i = 0; i < numChildNodes; ++i)
        {
            CUiAnimViewTrack* subTrack = static_cast<CUiAnimViewTrack*>(pTrack->GetChild(i));
            if (IsOkToAddKeyHere(subTrack, keyTime))
            {
                CUiAnimViewKeyHandle newKey = subTrack->CreateKey(keyTime);

                I2DBezierKey bezierKey;
                newKey.GetKey(&bezierKey);
                bezierKey.value = Vec2(keyTime, colArray[i]);
                newKey.SetKey(&bezierKey);

                keyCreated = true;
            }
        }
    }

    return keyCreated;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::AcceptUndo()
{
    if (UiAnimUndo::IsRecording())
    {
        if (m_mouseMode == eUiAVMouseMode_Paste)
        {
            UiAnimUndoManager::Get()->Cancel();
        }
        else if (m_mouseMode == eUiAVMouseMode_Move || m_mouseMode == eUiAVMouseMode_Clone)
        {
            CUiAnimViewSequence* pSequence = nullptr;
            UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

            if (pSequence && m_bKeysMoved)
            {
                UiAnimUndo::Record(new CUndoAnimKeySelection(pSequence));
                UiAnimUndoManager::Get()->Accept("Move/Clone Keys");
            }
            else
            {
                UiAnimUndoManager::Get()->Cancel();
            }
        }
        else if (m_mouseMode == eUiAVMouseMode_StartTimeAdjust
                 || m_mouseMode == eUiAVMouseMode_EndTimeAdjust)
        {
            UiAnimUndoManager::Get()->Accept("Adjust Start/End Time of an Animation Key");
        }
    }

    m_mouseMode = eUiAVMouseMode_None;
    m_trackMementos.clear();
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewDopeSheetBase::ComputeSnappedMoveOffset()
{
    // Compute time offset
    const QPoint currentMousePos(qBound(m_rcClient.left(), m_mouseOverPos.x(), m_rcClient.right()), m_mouseOverPos.y());

    float time0 = TimeFromPointUnsnapped(m_mouseDownPos);
    float time = TimeFromPointUnsnapped(currentMousePos);

    if (GetKeyModifiedSnappingMode() == eSnappingMode_SnapTick)
    {
        time0 = TickSnap(time0);
        time = TickSnap(time);
    }

    return time - time0;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::AddKeys(const QPoint& point, const bool bTryAddKeysInGroup)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    // Add keys here.
    CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);

    if (!pTrack)
    {
        return;
    }

    CUiAnimViewSequenceNotificationContext context(pSequence);

    CUiAnimViewAnimNode* pNode = pTrack->GetAnimNode();
    float keyTime = TimeFromPoint(point);
    bool inRange = m_timeRange.IsInside(keyTime);

    if (pTrack && inRange)
    {
        if (bTryAddKeysInGroup && pNode->GetParentNode()) // Add keys in group
        {
            CUiAnimViewTrackBundle tracksInGroup = pNode->GetTracksByParam(pTrack->GetParameterType());
            for (int i = 0; i < (int)tracksInGroup.GetCount(); ++i)
            {
                CUiAnimViewTrack* pCurrTrack = tracksInGroup.GetTrack(i);

                if (pCurrTrack->GetChildCount() == 0) // A simple track
                {
                    if (IsOkToAddKeyHere(pCurrTrack, keyTime))
                    {
                        RecordTrackUndo(pCurrTrack);
                        pCurrTrack->CreateKey(keyTime);
                    }
                }
                else // A compound track
                {
                    for (unsigned int k = 0; k < pCurrTrack->GetChildCount(); ++k)
                    {
                        CUiAnimViewTrack* pSubTrack = static_cast<CUiAnimViewTrack*>(pCurrTrack->GetChild(k));
                        if (IsOkToAddKeyHere(pSubTrack, keyTime))
                        {
                            RecordTrackUndo(pSubTrack);
                            pSubTrack->CreateKey(keyTime);
                        }
                    }
                }
            }
        }
        else if (pTrack->GetChildCount() == 0) // A simple track
        {
            if (IsOkToAddKeyHere(pTrack, keyTime))
            {
                RecordTrackUndo(pTrack);
                pTrack->CreateKey(keyTime);
            }
        }
        else // A compound track
        {
            if (pTrack->GetValueType() == eUiAnimValue_RGB)
            {
                CreateColorKey(pTrack, keyTime);
            }
            else
            {
                RecordTrackUndo(pTrack);
                for (unsigned int i = 0; i < pTrack->GetChildCount(); ++i)
                {
                    CUiAnimViewTrack* pSubTrack = static_cast<CUiAnimViewTrack*>(pTrack->GetChild(i));
                    if (IsOkToAddKeyHere(pSubTrack, keyTime))
                    {
                        pSubTrack->CreateKey(keyTime);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawControl(QPainter* painter, const QRect& rcUpdate)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    DrawNodesRecursive(pSequence, painter, rcUpdate);

    DrawSummary(painter, rcUpdate);

    DrawSelectedKeyIndicators(painter);

    if (m_mouseMode == eUiAVMouseMode_Paste)
    {
        // If in paste mode draw keys that are in clipboard
        DrawClipboardKeys(painter, QRect());
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawNodesRecursive(CUiAnimViewNode* pNode, QPainter* painter, const QRect& rcUpdate)
{
    const QRect rect = GetNodeRect(pNode);

    if (!rect.isEmpty())
    {
        switch (pNode->GetNodeType())
        {
        case eUiAVNT_AnimNode:
            DrawNodeTrack(static_cast<CUiAnimViewAnimNode*>(pNode), painter, rect);
            break;
        case eUiAVNT_Track:
            DrawTrack(static_cast<CUiAnimViewTrack*>(pNode), painter, rect);
            break;
        }
    }

    if (pNode->IsExpanded())
    {
        unsigned int numChildren = pNode->GetChildCount();
        for (unsigned int i = 0; i < numChildren; ++i)
        {
            DrawNodesRecursive(pNode->GetChild(i), painter, rcUpdate);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawTicks(QPainter* painter, const QRect& rc, Range& timeRange)
{
    // Draw time ticks every tick step seconds.
    const QPen dkgray(QColor(90, 90, 90));
    const QPen ltgray(QColor(120, 120, 120));

    const QPen prevPen = painter->pen();
    painter->setPen(dkgray);
    Range VisRange = GetVisibleRange();
    int nNumberTicks = 10;
    if (GetTickDisplayMode() == eUiAVTickMode_InFrames)
    {
        nNumberTicks = 8;
    }

    float start = TickSnap(timeRange.start);
    float step = 1.0f / static_cast<float>(m_ticksStep);

    for (float t = 0.0f; t <= timeRange.end + step; t += step)
    {
        float st = TickSnap(t);
        if (st > timeRange.end)
        {
            st = timeRange.end;
        }
        if (st < VisRange.start)
        {
            continue;
        }
        if (st > VisRange.end)
        {
            break;
        }
        int x = TimeToClient(st);
        if (x < 0)
        {
            continue;
        }

        int k = RoundFloatToInt(st * static_cast<float>(m_ticksStep));
        if (k % nNumberTicks == 0)
        {
            if (st >= start)
            {
                painter->setPen(Qt::black);
            }
            else
            {
                painter->setPen(dkgray);
            }

            painter->drawLine(x, rc.bottom() - 1, x, rc.bottom() - 5);
            painter->setPen(dkgray);
        }
        else
        {
            if (st >= start)
            {
                painter->setPen(dkgray);
            }
            else
            {
                painter->setPen(ltgray);
            }
            painter->drawLine(x, rc.bottom() - 1, x, rc.bottom() - 3);
        }
    }
    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawTrack(CUiAnimViewTrack* pTrack, QPainter* painter, const QRect& trackRect)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    const QPen prevPen = painter->pen();
    painter->setPen(QColor(120, 120, 120));
    painter->drawLine(trackRect.bottomLeft(), trackRect.bottomRight());
    painter->setPen(prevPen);

    QRect rcInner = trackRect;
    rcInner.setLeft(max(trackRect.left(), m_leftOffset - m_scrollOffset.x()));
    rcInner.setRight(min(trackRect.right(), (m_scrollMax + m_scrollMin) - m_scrollOffset.x() + m_leftOffset * 2));

    bool bLightAnimationSetActive = pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_LightAnimationSet;
    if (bLightAnimationSetActive && pTrack->GetKeyCount() > 0)
    {
        // In the case of the light animation set, the time of of the last key
        // determines the end of the track.
        float lastKeyTime = pTrack->GetKey(pTrack->GetKeyCount() - 1).GetTime();
        rcInner.setRight(min(rcInner.right(), TimeToClient(lastKeyTime)));
    }

    QRect rcInnerDraw(QPoint(rcInner.left() - 6, rcInner.top()), QPoint(rcInner.right() + 6, rcInner.bottom()));

    QColor trackColor = CUiAVCustomizeTrackColorsDlg::GetTrackColor(pTrack->GetParameterType());
    if (pTrack->HasCustomColor())
    {
        ColorB customColor = pTrack->GetCustomColor();
        trackColor = QColor(customColor.r, customColor.g, customColor.b);
    }
    // For the case of tracks belonging to an inactive director node,
    // changes the track color to a custom one.
    const QColor colorForDisabled = CUiAVCustomizeTrackColorsDlg::GetColorForDisabledTracks();
    const QColor colorForMuted = CUiAVCustomizeTrackColorsDlg::GetColorForMutedTracks();

    CUiAnimViewAnimNode* pDirectorNode = pTrack->GetDirector();
    if (!pDirectorNode->IsActiveDirector())
    {
        trackColor = colorForDisabled;
    }

    // A disabled/muted track or any track in a disabled node also uses a custom color.
    CUiAnimViewAnimNode* pAnimNode = pTrack->GetAnimNode();
    bool bTrackDisabled = pTrack->GetFlags() & IUiAnimTrack::eUiAnimTrackFlags_Disabled;
    bool bTrackMuted = pTrack->GetFlags() & IUiAnimTrack::eUiAnimTrackFlags_Muted;
    bool bTrackInvalid = !pTrack->IsSubTrack() && !pAnimNode->IsParamValid(pTrack->GetParameterType());
    bool bTrackInDisabledNode = pAnimNode->GetFlags() & eUiAnimNodeFlags_Disabled;
    if (bTrackDisabled || bTrackInDisabledNode || bTrackInvalid)
    {
        trackColor = colorForDisabled;
    }
    else if (bTrackMuted)
    {
        trackColor = colorForMuted;
    }
    const QRect rc = rcInnerDraw.adjusted(0, 1, 0, 0);

    const EUiAnimCurveType trackType = pTrack->GetCurveType();
    if (trackType == eUiAnimCurveType_TCBFloat || trackType == eUiAnimCurveType_TCBQuat || trackType == eUiAnimCurveType_TCBVector)
    {
        trackColor = QColor(245, 80, 70);
    }

    if (pTrack->IsSelected())
    {
        QLinearGradient gradient(rc.topLeft(), rc.bottomLeft());
        gradient.setColorAt(0, trackColor);
        gradient.setColorAt(1, QColor(trackColor.red() / 2, trackColor.green() / 2, trackColor.blue() / 2));
        painter->fillRect(rc, gradient);
    }
    else if (pTrack->GetValueType() == eUiAnimValue_RGB && pTrack->GetKeyCount() > 0)
    {
        DrawColorGradient(painter, rc, pTrack);
    }
    else
    {
        painter->fillRect(rc, trackColor);
    }

    // Left outside
    QRect rcOutside = trackRect;
    rcOutside.setRight(rcInnerDraw.left() - 1);
    rcOutside.adjust(1, 1, -1, 0);

    QLinearGradient gradient(rcOutside.topLeft(), rcOutside.bottomLeft());
    gradient.setColorAt(0, QColor(210, 210, 210));
    gradient.setColorAt(1, QColor(180, 180, 180));
    painter->fillRect(rcOutside, gradient);

    // Right outside.
    rcOutside = trackRect;
    rcOutside.setLeft(rcInnerDraw.right() + 1);
    rcOutside.adjust(1, 1, -1, 0);

    gradient = QLinearGradient(rcOutside.topLeft(), rcOutside.bottomLeft());
    gradient.setColorAt(0, QColor(210, 210, 210));
    gradient.setColorAt(1, QColor(180, 180, 180));
    painter->fillRect(rcOutside, gradient);

    // Get time range of update rectangle.
    Range timeRange = GetTimeRange(trackRect);

    // Draw tick marks in time range.
    DrawTicks(painter, rcInner, timeRange);

    // Draw special track features
    EUiAnimValue trackValueType = pTrack->GetValueType();
    CUiAnimParamType trackParamType = pTrack->GetParameterType();

    if (trackValueType == eUiAnimValue_Bool)
    {
        // If this track is bool Track draw bars where track is true
        DrawBoolTrack(timeRange, painter, pTrack, rc);
    }
    else if (trackValueType == eUiAnimValue_Select)
    {
        // If this track is Select Track draw bars to show where selection is active.
        DrawSelectTrack(timeRange, painter, pTrack, rc);
    }

    // Draw keys in time range.
    DrawKeys(pTrack, painter, rcInner, timeRange);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawSelectTrack(const Range& timeRange, QPainter* painter, CUiAnimViewTrack* pTrack, const QRect& rc)
{
    const QBrush prevBrush = painter->brush();
    painter->setBrush(m_selectTrackBrush);

    const int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        ISelectKey selectKey;
        keyHandle.GetKey(&selectKey);

        if (!selectKey.szSelection.empty())
        {
            float time = keyHandle.GetTime();
            float nextTime = timeRange.end;
            if (i < numKeys - 1)
            {
                nextTime = pTrack->GetKey(i + 1).GetTime();
            }

            time = clamp_tpl(time, timeRange.start, timeRange.end);
            nextTime = clamp_tpl(nextTime, timeRange.start, timeRange.end);

            int x0 = TimeToClient(time);

            float fBlendTime = selectKey.fBlendTime;
            int blendTimeEnd = 0;

            if (fBlendTime > 0.0f && fBlendTime < (nextTime - time))
            {
                blendTimeEnd = TimeToClient(nextTime);
                nextTime -= fBlendTime;
            }

            int x = TimeToClient(nextTime);

            if (x != x0)
            {
                QLinearGradient gradient(x0, rc.top() + 1, x0, rc.bottom());
                gradient.setColorAt(0, Qt::white);
                gradient.setColorAt(1, QColor(100, 190, 255));
                painter->fillRect(QRect(QPoint(x0, rc.top() + 1), QPoint(x, rc.bottom())), gradient);
            }

            if (fBlendTime > 0.0f)
            {
                QLinearGradient gradient(x, rc.top() + 1, x, rc.bottom());
                gradient.setColorAt(0, Qt::white);
                gradient.setColorAt(1, QColor(0, 115, 230));
                painter->fillRect(QRect(QPoint(x, rc.top() + 1), QPoint(blendTimeEnd, rc.bottom())), gradient);
            }
        }
    }
    painter->setBrush(prevBrush);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawBoolTrack(const Range& timeRange, QPainter* painter, CUiAnimViewTrack* pTrack, const QRect& rc)
{
    int x0 = TimeToClient(timeRange.start);

    const QBrush prevBrush = painter->brush();
    painter->setBrush(m_visibilityBrush);

    const int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        const float time = keyHandle.GetTime();
        if (time < timeRange.start)
        {
            continue;
        }
        if (time > timeRange.end)
        {
            break;
        }

        int x = TimeToClient(time);
        bool val = false;
        pTrack->GetValue(time - 0.001f, val);
        if (val)
        {
            QLinearGradient gradient(x0, rc.top() + 4, x0, rc.bottom() - 4);
            gradient.setColorAt(0, QColor(250, 250, 250));
            gradient.setColorAt(1, QColor(0, 80, 255));
            painter->fillRect(QRect(QPoint(x0, rc.top() + 4), QPoint(x, rc.bottom() - 4)), gradient);
        }

        x0 = x;
    }
    int x = TimeToClient(timeRange.end);
    bool val = false;
    pTrack->GetValue(timeRange.end - 0.001f, val);
    if (val)
    {
        QLinearGradient gradient(x0, rc.top() + 4, x0, rc.bottom() - 4);
        gradient.setColorAt(0, QColor(250, 250, 250));
        gradient.setColorAt(1, QColor(0, 80, 255));
        painter->fillRect(QRect(QPoint(x0, rc.top() + 4), QPoint(x, rc.bottom() - 4)), gradient);
    }
    painter->setBrush(prevBrush);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawKeys(CUiAnimViewTrack* pTrack, QPainter* painter, QRect& rect, [[maybe_unused]] Range& timeRange)
{
    int numKeys = pTrack->GetKeyCount();

    const QFont prevFont = painter->font();
    painter->setFont(m_descriptionFont);

    painter->setPen(KEY_TEXT_COLOR);

    int prevKeyPixel = -10000;
    const int kDefaultWidthForDescription = 200;
    const int kSmallMargin = 10;

    // Draw keys.
    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        const float time = keyHandle.GetTime();
        int x = TimeToClient(time);
        if (x - kSmallMargin > rect.right())
        {
            continue;
        }

        int x1 = x + kDefaultWidthForDescription;
        CUiAnimViewKeyHandle nextKey = keyHandle.GetNextKey();

        if (nextKey.IsValid())
        {
            x1 = TimeToClient(nextKey.GetTime()) - kSmallMargin;
        }

        if (x1 > x + kSmallMargin)  // Enough space for description text or duration bar
        {
            // Get info about that key.
            const char* pDescription = keyHandle.GetDescription();
            const float duration = keyHandle.GetDuration();

            int xlast = x;
            if (duration > 0)
            {
                xlast = TimeToClient(time + duration);
            }
            if (xlast + kSmallMargin < rect.left())
            {
                continue;
            }

            if (duration > 0)
            {
                DrawKeyDuration(pTrack, painter, rect, i);
            }

            if (pDescription && pDescription[0] != 0)
            {
                char keydesc[1024];
                bool bSelectedAndBeingMoved = m_mouseMode == eUiAVMouseMode_Move && keyHandle.IsSelected();
                if (bSelectedAndBeingMoved)
                {
                    // Show its time or frame number additionally.
                    if (GetTickDisplayMode() == eUiAVTickMode_InSeconds)
                    {
                        sprintf_s(keydesc, "%.3f, {", time);
                    }
                    else
                    {
                        sprintf_s(keydesc, "%d, {", ftoi(time / m_snapFrameTime));
                    }
                }
                else
                {
                    azstrcpy(keydesc, AZ_ARRAY_SIZE(keydesc), "{");
                }
                azstrcat(keydesc, AZ_ARRAY_SIZE(keydesc), pDescription);
                azstrcat(keydesc, AZ_ARRAY_SIZE(keydesc), "}");
                // Draw key description text.
                // Find next key.
                const QRect textRect(QPoint(x + 10, rect.top()), QPoint(x1, rect.bottom()));
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, painter->fontMetrics().elidedText(keydesc, Qt::ElideRight, textRect.width()));
            }
        }

        if (x < 0)
        {
            continue;
        }

        if (pTrack->GetChildCount() == 0 // At compound tracks, keys are all green.
            && abs(x - prevKeyPixel) < 2)
        {
            // If multiple keys on the same time.
            painter->drawPixmap(QPoint(x - 6, rect.top() + 2), QPixmap(":/Trackview/trackview_keys_02.png"));
        }
        else
        {
            if (keyHandle.IsSelected())
            {
                painter->drawPixmap(QPoint(x - 6, rect.top() + 2), QPixmap(":/Trackview/trackview_keys_01.png"));
            }
            else
            {
                painter->drawPixmap(QPoint(x - 6, rect.top() + 2), QPixmap(":/Trackview/trackview_keys_00.png"));
            }
        }

        prevKeyPixel = x;
    }
    painter->setFont(prevFont);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawClipboardKeys(QPainter* painter, [[maybe_unused]] const QRect& rc)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    const float timeOffset = ComputeSnappedMoveOffset();

    // Get node & track under cursor
    CUiAnimViewAnimNode* pAnimNode = GetAnimNodeFromPoint(m_mouseOverPos);
    CUiAnimViewTrack* pTrack = GetTrackFromPoint(m_mouseOverPos);

    auto matchedLocations = pSequence->GetMatchedPasteLocations(m_clipboardKeys, pAnimNode, pTrack);

    for (size_t i = 0; i < matchedLocations.size(); ++i)
    {
        auto& matchedLocation = matchedLocations[i];
        CUiAnimViewTrack* pMatchedTrack = matchedLocation.first;
        XmlNodeRef trackNode = matchedLocation.second;

        if (pMatchedTrack->IsCompoundTrack())
        {
            // Both child counts should be the same, but make sure
            const unsigned int numSubTrack = std::min(pMatchedTrack->GetChildCount(), (unsigned int)trackNode->getChildCount());

            for (unsigned int subTrackIndex = 0; subTrackIndex < numSubTrack; ++subTrackIndex)
            {
                CUiAnimViewTrack* pSubTrack = static_cast<CUiAnimViewTrack*>(pMatchedTrack->GetChild(subTrackIndex));
                XmlNodeRef subTrackNode = trackNode->getChild(subTrackIndex);
                DrawTrackClipboardKeys(painter, pSubTrack, subTrackNode, timeOffset);

                // Also draw to parent track. This is intentional
                DrawTrackClipboardKeys(painter, pMatchedTrack, subTrackNode, timeOffset);
            }
        }
        else
        {
            DrawTrackClipboardKeys(painter, pMatchedTrack, trackNode, timeOffset);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawTrackClipboardKeys(QPainter* painter, CUiAnimViewTrack* pTrack, XmlNodeRef trackNode, const float timeOffset)
{
    const QPen prevPen = painter->pen();
    painter->setPen(Qt::green);

    const QRect trackRect = GetNodeRect(pTrack);
    const int numKeysToPaste = trackNode->getChildCount();

    for (int i = 0; i < numKeysToPaste; ++i)
    {
        XmlNodeRef keyNode = trackNode->getChild(i);

        float time;
        if (keyNode->getAttr("time", time))
        {
            int x = TimeToClient(time + timeOffset);
            painter->drawPixmap(QPoint(x - 6, trackRect.top() + 2), QPixmap(":/Trackview/trackview_keys_03.png"));
            painter->drawLine(x, m_rcClient.top(), x, m_rcClient.bottom());
        }
    }

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewDopeSheetBase::FirstKeyFromPoint(const QPoint& point)
{
    CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);
    if (!pTrack)
    {
        return CUiAnimViewKeyHandle();
    }

    float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
    float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

    int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        float time = keyHandle.GetTime();
        if (time >= t1 && time <= t2)
        {
            return keyHandle;
        }
    }

    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewDopeSheetBase::DurationKeyFromPoint(const QPoint& point)
{
    CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);
    if (!pTrack)
    {
        return CUiAnimViewKeyHandle();
    }

    float t = TimeFromPointUnsnapped(point);

    int numKeys = pTrack->GetKeyCount();
    // Iterate in a reverse order to prioritize later nodes.
    for (int i = numKeys - 1; i >= 0; --i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        const float time = keyHandle.GetTime();
        const float duration = keyHandle.GetDuration();

        if (t >= time && t <= time + duration)
        {
            return keyHandle;
        }
    }

    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyHandle CUiAnimViewDopeSheetBase::CheckCursorOnStartEndTimeAdjustBar(const QPoint& point, bool& bStart)
{
    CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);

    if (!pTrack)
    {
        return CUiAnimViewKeyHandle();
    }

    int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        if (!keyHandle.IsSelected())
        {
            continue;
        }

        const float time = keyHandle.GetTime();
        const float duration = keyHandle.GetDuration();

        if (duration == 0)
        {
            continue;
        }

        int stime = TimeToClient(time);
        int etime = TimeToClient(time + duration);
        if (point.x() >= stime - 3 && point.x() <= stime)
        {
            bStart = true;
            return keyHandle;
        }
        else if (point.x() >= etime && point.x() <= etime + 3)
        {
            bStart = false;
            return keyHandle;
        }
    }

    return CUiAnimViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewDopeSheetBase::NumKeysFromPoint(const QPoint& point)
{
    CUiAnimViewTrack* pTrack = GetTrackFromPoint(point);
    if (!pTrack)
    {
        return -1;
    }

    float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
    float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

    int count = 0;
    int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        const float time = keyHandle.GetTime();
        if (time >= t1 && time <= t2)
        {
            ++count;
        }
    }
    return count;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SelectKeys(const QRect& rc, const bool bMultiSelection)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    UiAnimUndoManager::Get()->Begin();
    CUndoAnimKeySelection* pUndoKeySelection = new CUndoAnimKeySelection(pSequence);
    UiAnimUndo::Record(pUndoKeySelection);

    CUiAnimViewSequenceNotificationContext context(pSequence);
    if (!bMultiSelection)
    {
        pSequence->DeselectAllKeys();
    }

    // put selection rectangle from client to track space.
    const QRect rci = rc.translated(m_scrollOffset);

    Range selTime = GetTimeRange(rci);

    CUiAnimViewTrackBundle tracks = pSequence->GetAllTracks();

    for (unsigned int i = 0; i < tracks.GetCount(); ++i)
    {
        CUiAnimViewTrack* pTrack = tracks.GetTrack(i);

        QRect trackRect = GetNodeRect(pTrack);
        // Decrease item rectangle a bit.
        trackRect.adjust(4, 4, -4, -4);
        // Check if item rectangle intersects with selection rectangle in y axis.
        if ((trackRect.top() >= rc.top() && trackRect.top() <= rc.bottom()) ||
            (trackRect.bottom() >= rc.top() && trackRect.bottom() <= rc.bottom()) ||
            (rc.top() >= trackRect.top() && rc.top() <= trackRect.bottom()) ||
            (rc.bottom() >= trackRect.top() && rc.bottom() <= trackRect.bottom()))
        {
            // Check which keys we intersect.
            for (unsigned int j = 0; j < pTrack->GetKeyCount(); j++)
            {
                CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(j);

                const float time = keyHandle.GetTime();
                if (selTime.IsInside(time))
                {
                    keyHandle.Select(true);
                }
            }
        }
    }

    if (pUndoKeySelection->IsSelectionChanged())
    {
        UiAnimUndoManager::Get()->Accept("Select keys");
    }
    else
    {
        UiAnimUndoManager::Get()->Cancel();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetTickDisplayMode(EUiAVTickMode mode)
{
    m_tickDisplayMode = mode;
    SetTimeScale(GetTimeScale(), 0); // for refresh
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::SetSnapFPS(UINT fps)
{
    m_snapFrameTime = (fps == 0) ? 0.033333f : (1.0f / float(fps));
}

//////////////////////////////////////////////////////////////////////////
ESnappingMode CUiAnimViewDopeSheetBase::GetKeyModifiedSnappingMode()
{
    ESnappingMode snappingMode = m_snappingMode;

    if (qApp->keyboardModifiers() & Qt::ControlModifier)
    {
        snappingMode = eSnappingMode_SnapNone;
    }
    else if (qApp->keyboardModifiers() & Qt::ShiftModifier)
    {
        snappingMode = eSnappingMode_SnapMagnet;
    }
    else if (qApp->keyboardModifiers() & Qt::AltModifier)
    {
        snappingMode = eSnappingMode_SnapFrame;
    }

    return snappingMode;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawSelectedKeyIndicators(QPainter* painter)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    const QPen prevPen = painter->pen();
    painter->setPen(Qt::green);

    CUiAnimViewKeyBundle keys = pSequence->GetSelectedKeys();
    for (unsigned int i = 0; i < keys.GetKeyCount(); ++i)
    {
        CUiAnimViewKeyHandle keyHandle = keys.GetKey(i);
        int x = TimeToClient(keyHandle.GetTime());
        painter->drawLine(x, m_rcClient.top(), x, m_rcClient.bottom());
    }

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::ComputeFrameSteps(const Range& visRange)
{
    float fNbFrames = fabsf ((visRange.end - visRange.start) / m_snapFrameTime);
    float afStepTable [4] = { 1.0f, 0.5f, 0.2f, 0.1f };
    bool bDone = false;
    float fFact = 1.0f;
    unsigned int nStepIdx = 0;
    for (unsigned int nAttempts = 0; nAttempts < 10 && !bDone; ++nAttempts)
    {
        bool bLess = true;
        for (nStepIdx = 0; nStepIdx < 4; ++nStepIdx)
        {
            float fFactNbFrames = fNbFrames / (afStepTable[nStepIdx] * fFact);
            if (fFactNbFrames >= 3 && fFactNbFrames <= 9)
            {
                bDone = true;
                break;
            }
            else
            {
                bLess = (fFactNbFrames < 3);
            }
        }
        if (!bDone)
        {
            fFact *= (bLess) ? 0.1f : 10.0f;
        }
    }

    float nBIntermediateTicks = 5;
    m_fFrameLabelStep = fFact * afStepTable[nStepIdx];

    if (TimeToClient(static_cast<float>(m_fFrameLabelStep)) - TimeToClient(0) > 1300)
    {
        nBIntermediateTicks = 10;
    }

    m_fFrameTickStep = m_fFrameLabelStep * double (m_snapFrameTime) / double(nBIntermediateTicks);
}


void CUiAnimViewDopeSheetBase::DrawTimeLineInFrames(QPainter* painter, const QRect& rc, [[maybe_unused]] const QColor& lineCol, const QColor& textCol, [[maybe_unused]] double step)
{
    float fFramesPerSec = 1.0f / m_snapFrameTime;
    float fInvFrameLabelStep = 1.0f / static_cast<float>(m_fFrameLabelStep);
    Range VisRange = GetVisibleRange();

    const Range& timeRange = m_timeRange;

    const QPen ltgray(QColor(90, 90, 90));
    const QPen black(textCol);

    for (float t = TickSnap(timeRange.start); t <= timeRange.end + static_cast<float>(m_fFrameTickStep); t += static_cast<float>(m_fFrameTickStep))
    {
        float st = t;
        if (st > timeRange.end)
        {
            st = timeRange.end;
        }
        if (st < VisRange.start)
        {
            continue;
        }
        if (st > VisRange.end)
        {
            break;
        }
        if (st < m_timeRange.start || st > m_timeRange.end)
        {
            continue;
        }
        const int x = TimeToClient(st);

        float fFrame = st * fFramesPerSec;
        float fFrameScaled = fFrame * fInvFrameLabelStep;
        if (fabsf(fFrameScaled - RoundFloatToInt(fFrameScaled)) < 0.001f)
        {
            painter->setPen(black);
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 14);
            painter->drawText(x + 2, rc.top(), QString::number(fFrame));
            painter->setPen(ltgray);
        }
        else
        {
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 6);
        }
    }
}


void CUiAnimViewDopeSheetBase::DrawTimeLineInSeconds(QPainter* painter, const QRect& rc, [[maybe_unused]] const QColor& lineCol, const QColor& textCol, double step)
{
    Range VisRange = GetVisibleRange();
    const Range& timeRange = m_timeRange;
    int nNumberTicks = 10;

    const QPen ltgray(QColor(90, 90, 90));
    const QPen black(textCol);

    for (float t = TickSnap(timeRange.start); t <= timeRange.end + static_cast<float>(step); t += static_cast<float>(step))
    {
        float st = TickSnap(t);
        if (st > timeRange.end)
        {
            st = timeRange.end;
        }
        if (st < VisRange.start)
        {
            continue;
        }
        if (st > VisRange.end)
        {
            break;
        }
        if (st < m_timeRange.start || st > m_timeRange.end)
        {
            continue;
        }
        int x = TimeToClient(st);

        int k = RoundFloatToInt(st * static_cast<float>(m_ticksStep));
        if (k % nNumberTicks == 0)
        {
            painter->setPen(black);
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 14);
            painter->drawText(x + 2, rc.top(), QString::number(st));
            painter->setPen(ltgray);
        }
        else
        {
            painter->drawLine(x, rc.bottom() - 2, x, rc.bottom() - 6);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawTimeline(QPainter* painter, const QRect& rcUpdate)
{
    CUiAnimationContext* pAnimationContext = nullptr;
    UiEditorAnimationBus::BroadcastResult(pAnimationContext, &UiEditorAnimationBus::Events::GetAnimationContext);
    bool recording = pAnimationContext->IsRecording();

    QColor lineCol(255, 0, 255);
    const QColor textCol(Qt::black);
    const QColor dkgrayCol(90, 90, 90);
    const QColor ltgrayCol(150, 150, 150);

    if (recording)
    {
        lineCol = Qt::red;
    }

    // Draw vertical line showing current time.
    {
        int x = TimeToClient(m_currentTime);
        if (x > m_rcClient.left() && x < m_rcClient.right())
        {
            const QPen prevPen = painter->pen();
            painter->setPen(lineCol);
            painter->drawLine(x, 0, x, m_rcClient.bottom());
            painter->setPen(prevPen);
        }
    }

    const QRect rc = m_rcTimeline;
    if (!rc.intersects(rcUpdate))
    {
        return;
    }

    QLinearGradient gradient(rc.topLeft(), rc.bottomLeft());
    gradient.setColorAt(0, QColor(250, 250, 250));
    gradient.setColorAt(1, QColor(180, 180, 180));
    painter->fillRect(rc, gradient);

    const QPen prevPen = painter->pen();
    const QPen dkgray(dkgrayCol);
    const QPen ltgray(ltgrayCol);
    const QPen black(textCol);
    const QPen redpen(lineCol);
    // Draw time ticks every tick step seconds.

    painter->setPen(dkgray);

    double step = 1.0 / double(m_ticksStep);
    if (GetTickDisplayMode() == eUiAVTickMode_InFrames)
    {
        DrawTimeLineInFrames(painter, rc, lineCol, textCol, step);
    }
    else if (GetTickDisplayMode() == eUiAVTickMode_InSeconds)
    {
        DrawTimeLineInSeconds(painter, rc, lineCol, textCol, step);
    }
    else
    {
        assert (0);
    }

    // Draw time markers.
    int x;

    x = TimeToClient(m_timeMarked.start);
    painter->drawPixmap(QPoint(x, m_rcTimeline.bottom() - 9), QPixmap(":/Trackview/marker/bmp00016_01.png"));
    x = TimeToClient(m_timeMarked.end);
    painter->drawPixmap(QPoint(x - 7, m_rcTimeline.bottom() - 9), QPixmap(":/Trackview/marker/bmp00016_00.png"));

    painter->setPen(redpen);
    x = TimeToClient(m_currentTime);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRect(QPoint(x - 3, rc.top()), QPoint(x + 3, rc.bottom())));

    painter->setPen(redpen);
    painter->drawLine(x, rc.top(), x, rc.bottom());

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawSummary(QPainter* painter, const QRect& rcUpdate)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    const QColor lineCol = Qt::black;
    const QColor fillCol(150, 100, 220);

    const QRect rc = m_rcSummary;
    if (!rc.intersects(rcUpdate))
    {
        return;
    }

    painter->fillRect(rc, fillCol);

    const QPen prevPen = painter->pen();
    painter->setPen(QPen(lineCol, 3));

    // Draw a short thick line at each place where there is a key in any tracks.
    CUiAnimViewKeyBundle keys = pSequence->GetAllKeys();
    for (unsigned int i = 0; i < keys.GetKeyCount(); ++i)
    {
        CUiAnimViewKeyHandle keyHandle = keys.GetKey(i);
        int x = TimeToClient(keyHandle.GetTime());
        painter->drawLine(x, rc.bottom() - 2, x, rc.top() + 2);
    }

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawNodeTrack(CUiAnimViewAnimNode* pAnimNode, QPainter* painter, const QRect& trackRect)
{
    const QFont prevFont = painter->font();
    painter->setFont(m_descriptionFont);

    CUiAnimViewAnimNode* pDirectorNode = pAnimNode->GetDirector();

    if (pDirectorNode->GetNodeType() != eUiAVNT_Sequence && !pDirectorNode->IsActiveDirector())
    {
        painter->setPen(INACTIVE_TEXT_COLOR);
    }
    else
    {
        painter->setPen(KEY_TEXT_COLOR);
    }

    const QRect textRect = trackRect.adjusted(4, 0, -4, 0);

    QString sAnimNodeName = QString::fromUtf8(pAnimNode->GetName().c_str());
    const bool hasObsoleteTrack = pAnimNode->HasObsoleteTrack();

    if (hasObsoleteTrack)
    {
        painter->setPen(QColor(245, 80, 70));
        sAnimNodeName += tr(": Some of the sub-tracks contains obsoleted TCB splines (marked in red), thus cannot be copied or pasted.");
    }

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, painter->fontMetrics().elidedText(sAnimNodeName, Qt::ElideRight, textRect.width()));

    painter->setFont(prevFont);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawGoToTrackArrow(CUiAnimViewTrack* pTrack, QPainter* painter, const QRect& rc)
{
    int numKeys = pTrack->GetKeyCount();
    const QColor colorLine(150, 150, 150);
    const QColor colorHeader(50, 50, 50);
    const int tickness = 2;
    const int halfMargin = (rc.height() - tickness) / 2;

    for (int i = 0; i < numKeys; ++i)
    {
        CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(i);

        IDiscreteFloatKey discreteFloatKey;
        keyHandle.GetKey(&discreteFloatKey);

        int arrowStart = TimeToClient(discreteFloatKey.time);
        int arrowEnd = TimeToClient(discreteFloatKey.m_fValue);

        if (discreteFloatKey.m_fValue < 0.f)
        {
            continue;
        }

        // draw arrow body line
        if (arrowStart < arrowEnd)
        {
            painter->fillRect(QRect(QPoint(arrowStart, rc.top() + halfMargin), QPoint(arrowEnd, rc.bottom() - halfMargin)), colorLine);
        }
        else if (arrowStart > arrowEnd)
        {
            painter->fillRect(QRect(QPoint(arrowEnd, rc.top() + halfMargin), QPoint(arrowStart, rc.bottom() - halfMargin)), colorLine);
        }

        // draw arrow head
        if (arrowStart != arrowEnd)
        {
            painter->fillRect(QRect(QPoint(arrowEnd, rc.top() + 2), QPoint(arrowEnd + 1, rc.bottom() - 2)), colorHeader);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawKeyDuration(CUiAnimViewTrack* pTrack, QPainter* painter, const QRect& rc, int keyIndex)
{
    CUiAnimViewKeyHandle keyHandle = pTrack->GetKey(keyIndex);

    const float time = keyHandle.GetTime();
    const float duration = keyHandle.GetDuration();

    int x = TimeToClient(time);

    // Draw key duration.
    float endt = min(time + duration, m_timeRange.end);
    int x1 = TimeToClient(endt);
    if (x1 < 0)
    {
        if (x > 0)
        {
            x1 = rc.right();
        }
    }
    const QBrush prevBrush = painter->brush();
    painter->setBrush(m_visibilityBrush);
    QColor colorFrom(120, 120, 255);
    QLinearGradient gradient(x, rc.top() + 3, x, rc.bottom() - 3);
    gradient.setColorAt(0, colorFrom);
    gradient.setColorAt(1, QColor(250, 250, 250));
    const int width = x1 + 1 - x;
    painter->fillRect(QRect(x, rc.top() + 3, width, rc.height() - 3), gradient);

    painter->setBrush(prevBrush);
    painter->drawLine(x1, rc.top(), x1, rc.bottom());
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::DrawColorGradient(QPainter* painter, const QRect& rc, const CUiAnimViewTrack* pTrack)
{
    const QPen pOldPen = painter->pen();
    for (int x = rc.left(); x < rc.right(); ++x)
    {
        // This is really slow. Is there a better way?
        Vec3 vColor(0, 0, 0);
        pTrack->GetValue(TimeFromPointUnsnapped(QPoint(x, rc.top())), vColor);

        painter->setPen(ColorLinearToGamma(vColor / 255.0f));
        painter->drawLine(x, rc.top(), x, rc.bottom());
    }
    painter->setPen(pOldPen);
}

//////////////////////////////////////////////////////////////////////////
QRect CUiAnimViewDopeSheetBase::GetNodeRect(const CUiAnimViewNode* pNode) const
{
    CUiAnimViewNodesCtrl::CRecord* pRecord = m_pNodesCtrl->GetNodeRecord(pNode);

    if (pRecord && pRecord->IsVisible())
    {
        QRect recordRect = pRecord->GetRect();
        return QRect(0, recordRect.top(), m_rcClient.width(), recordRect.height());
    }

    return QRect();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDopeSheetBase::StoreMementoForTracksWithSelectedKeys()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    CUiAnimViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

    m_trackMementos.clear();

    // Construct the set of tracks that have selected keys
    std::set<CUiAnimViewTrack*> tracks;

    const unsigned int numKeys = selectedKeys.GetKeyCount();
    for (unsigned int keyIndex = 0; keyIndex < numKeys; ++keyIndex)
    {
        CUiAnimViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
        tracks.insert(keyHandle.GetTrack());
    }

    // For each of those tracks store an undo object
    for (auto iter = tracks.begin(); iter != tracks.end(); ++iter)
    {
        CUiAnimViewTrack* pTrack = *iter;

        TrackMemento trackMemento;
        trackMemento.m_memento = pTrack->GetMemento();

        const unsigned int trackNumKeys = pTrack->GetKeyCount();
        for (unsigned int i = 0; i < trackNumKeys; ++i)
        {
            trackMemento.m_keySelectionStates.push_back(pTrack->GetKey(i).IsSelected());
        }

        m_trackMementos[pTrack] = trackMemento;
    }
}
