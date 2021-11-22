/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewDopeSheetBase.h"

// Qt
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>

// AzFramework
#include <AzCore/std/sort.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

// CryCommon
#include <CryCommon/Maestro/Types/AnimValueType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>
#include <CryCommon/Maestro/Types/AssetBlendKey.h>

// Editor
#include "Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h"
#include "Clipboard.h"
#include "Util/fastlib.h"
#include "TrackView/TrackViewNodes.h"
#include "TVCustomizeTrackColorsDlg.h"
#include "TrackView/TrackViewKeyPropertiesDlg.h"


#define EDIT_DISABLE_GRAY_COLOR QColor(128, 128, 128)
#define KEY_TEXT_COLOR QColor(0, 0, 50)
#define INACTIVE_TEXT_COLOR QColor(128, 128, 128)

namespace
{
    const int kMarginForMagnetSnapping = 10;
    const unsigned int kDefaultTrackHeight = 16;
}

enum ETVMouseMode
{
    eTVMouseMode_None = 0,
    eTVMouseMode_Select = 1,
    eTVMouseMode_Move,
    eTVMouseMode_Clone,
    eTVMouseMode_DragTime,
    eTVMouseMode_DragStartMarker,
    eTVMouseMode_DragEndMarker,
    eTVMouseMode_Paste,
    eTVMouseMode_SelectWithinTime,
    eTVMouseMode_StartTimeAdjust,
    eTVMouseMode_EndTimeAdjust
};

//////////////////////////////////////////////////////////////////////////
CTrackViewDopeSheetBase::CTrackViewDopeSheetBase(QWidget* parent)
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
    m_mouseMode = eTVMouseMode_None;
    m_currentTime = 0.0f;
    m_storedTime = m_currentTime;
    m_rcSelect = QRect(0, 0, 0, 0);
    m_rubberBand = nullptr;
    m_scrollBar = new QScrollBar(Qt::Horizontal, this);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, &CTrackViewDopeSheetBase::OnHScroll);
    m_keyTimeOffset = 0;
    m_currCursor = QCursor(Qt::ArrowCursor);
    m_mouseActionMode = eTVActionMode_MoveKey;

    m_scrollMin = 0;
    m_scrollMax = 1000;

    m_descriptionFont = QFont(QStringLiteral("Verdana"), 7);

    m_bCursorWasInKey = false;
    m_bJustSelected = false;
    m_snappingMode = eSnappingMode_SnapNone;
    m_snapFrameTime = 0.033333f;
    m_bMouseMovedAfterRButtonDown = false;
    m_stashedRecordModeWhileTimeDragging = false;

    m_tickDisplayMode = eTVTickMode_InSeconds;

    m_bEditLock = false;

    m_bFastRedraw = false;

    m_pLastTrackSelectedOnSpot = nullptr;

    m_wndPropsOnSpot = nullptr;

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

    m_colorUpdateTrack = nullptr;
    m_colorUpdateKeyTime = 0;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewDopeSheetBase::~CTrackViewDopeSheetBase()
{
    HideKeyPropertyCtrlOnSpot();
    GetIEditor()->GetAnimation()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::TimeToClient(float time) const
{
    return static_cast<int>(m_leftOffset - m_scrollOffset.x() + (time * m_timeScale));
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewDopeSheetBase::GetVisibleRange() const
{
    Range r;
    r.start = (m_scrollOffset.x() - m_leftOffset) / m_timeScale;
    r.end = r.start + (m_rcClient.width()) / m_timeScale;

    Range extendedTimeRange(0.0f, m_timeRange.end);
    r = extendedTimeRange & r;

    return r;
}

//////////////////////////////////////////////////////////////////////////
Range CTrackViewDopeSheetBase::GetTimeRange(const QRect& rc) const
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
void CTrackViewDopeSheetBase::SetTimeRange(float start, float end)
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
void CTrackViewDopeSheetBase::SetTimeScale(float timeScale, float fAnchorTime)
{
    const double fOldOffset = -fAnchorTime * m_timeScale;

    timeScale = std::max(timeScale, 0.001f);
    timeScale = std::min(timeScale, 100000.0f);
    m_timeScale = timeScale;

    int steps = 0;
    if (GetTickDisplayMode() == eTVTickMode_InSeconds)
    {
        m_ticksStep = 10;
    }
    else if (GetTickDisplayMode() == eTVTickMode_InFrames)
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
    m_scrollBar->setValue(m_scrollOffset.x());

    update();

    SetHorizontalExtent(-m_leftOffset, static_cast<int>(m_timeRange.end * m_timeScale));

    ComputeFrameSteps(GetVisibleRange());

    OnHScroll();
}

void CTrackViewDopeSheetBase::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    GetIEditor()->GetAnimation()->AddListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::resizeEvent(QResizeEvent* event)
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
void CTrackViewDopeSheetBase::wheelEvent(QWheelEvent* event)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        event->ignore();
        return;
    }

    float z = (event->angleDelta().y() > 0) ? (m_timeScale * 1.25f) : (m_timeScale * 0.8f);
    // Use m_mouseOverPos to get the local position in the timeline view instead of
    // event->pos() which seems to include the variable left panel of the view that
    // lists the tracks.
    float fAnchorTime = TimeFromPointUnsnapped(m_mouseOverPos);
    SetTimeScale(z, fAnchorTime);

    event->accept();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnHScroll()
{
    // Get the current position of scroll box.
    int curpos = m_scrollBar->value();
    m_scrollOffset.setX(curpos);
    update();
}

int CTrackViewDopeSheetBase::GetScrollPos() const
{
    return m_scrollBar->value();
}

//////////////////////////////////////////////////////////////////////////
double CTrackViewDopeSheetBase::GetTickTime() const
{
    if (GetTickDisplayMode() == eTVTickMode_InFrames)
    {
        return m_fFrameTickStep;
    }
    else
    {
        return 1.0f / m_ticksStep;
    }
}


//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TickSnap(float time) const
{
    double tickTime = GetTickTime();
    double t = floor(((double)time / tickTime) + 0.5);
    t *= tickTime;
    return static_cast<float>(t);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TimeFromPoint(const QPoint& point) const
{
    int x = point.x() - m_leftOffset + m_scrollOffset.x();
    float t = static_cast<float>(x) / m_timeScale;
    return TickSnap(t);
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::TimeFromPointUnsnapped(const QPoint& point) const
{
    int x = point.x() - m_leftOffset + m_scrollOffset.x();
    double t = (double)x / m_timeScale;
    return static_cast<float>(t);
}

void CTrackViewDopeSheetBase::mousePressEvent(QMouseEvent* event)
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

void CTrackViewDopeSheetBase::mouseReleaseEvent(QMouseEvent* event)
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

void CTrackViewDopeSheetBase::mouseDoubleClickEvent(QMouseEvent* event)
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
void CTrackViewDopeSheetBase::OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!sequence)
    {
        return;
    }

    HideKeyPropertyCtrlOnSpot();

    if (m_rcTimeline.contains(point))
    {
        m_mouseDownPos = point;

        // Clicked inside timeline.
        m_mouseMode = eTVMouseMode_DragTime;

        // If mouse over selected key, change cursor to left-right arrows.
        SetMouseCursor(m_crsLeftRight);

        m_stashedRecordModeWhileTimeDragging = GetIEditor()->GetAnimation()->IsRecordMode();
        GetIEditor()->GetAnimation()->SetRecording(false);  // disable recording while dragging time

        SetCurrTime(TimeFromPoint(point));
        return;
    }

    if (m_bEditLock)
    {
        m_mouseDownPos = point;
        return;
    }

    if (m_mouseMode == eTVMouseMode_Paste)
    {
        m_mouseMode = eTVMouseMode_None;

        CTrackViewAnimNode* animNode = GetAnimNodeFromPoint(m_mouseOverPos);
        CTrackViewTrack* pTrack = GetTrackFromPoint(m_mouseOverPos);

        if (animNode)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Paste Keys");
            sequence->DeselectAllKeys();
            sequence->PasteKeysFromClipboard(animNode, pTrack, ComputeSnappedMoveOffset());
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }

        SetMouseCursor(Qt::ArrowCursor);
        OnCaptureChanged();
        return;
    }

    m_mouseDownPos = point;

    // The summary region is used for moving already selected keys.
    if (m_rcSummary.contains(point))
    {
        CTrackViewKeyBundle selectedKeys = sequence->GetSelectedKeys();
        if (selectedKeys.GetKeyCount() > 0)
        {
            // Move/Clone Key Undo Begin
            GetIEditor()->BeginUndo();
            StoreMementoForTracksWithSelectedKeys();

            m_keyTimeOffset = 0;
            m_mouseMode = eTVMouseMode_Move;
            SetMouseCursor(m_crsLeftRight);
            return;
        }
    }

    bool bStart = false;
    CTrackViewKeyHandle keyHandle = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
    if (keyHandle.IsValid())
    {
        return LButtonDownOnTimeAdjustBar(point, keyHandle, bStart);
    }

    keyHandle = FirstKeyFromPoint(point);
    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }

    if (keyHandle.IsValid())
    {
        return LButtonDownOnKey(point, keyHandle, modifiers);
    }

    if (m_mouseActionMode == eTVActionMode_AddKeys)
    {
        AddKeys(point, modifiers & Qt::ShiftModifier);
        return;
    }

    if (modifiers & Qt::ShiftModifier)
    {
        m_mouseMode = eTVMouseMode_SelectWithinTime;
    }
    else
    {
        m_mouseMode = eTVMouseMode_Select;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!pSequence)
    {
        return;
    }

    if (m_mouseMode == eTVMouseMode_Select)
    {
        // Check if any key are selected.
        m_rcSelect.translate(-m_scrollOffset);
        SelectKeys(m_rcSelect, modifiers & Qt::ControlModifier);
        m_rcSelect = QRect();
        m_rubberBand->deleteLater();
        m_rubberBand = nullptr;
    }
    else if (m_mouseMode == eTVMouseMode_SelectWithinTime)
    {
        m_rcSelect.translate(-m_scrollOffset);
        SelectAllKeysWithinTimeFrame(m_rcSelect, modifiers & Qt::ControlModifier);
        m_rcSelect = QRect();
        m_rubberBand->deleteLater();
        m_rubberBand = nullptr;
    }
    else if (m_mouseMode == eTVMouseMode_DragTime)
    {
        SetMouseCursor(Qt::ArrowCursor);
        // Notify that time was explicitly set
        GetIEditor()->GetAnimation()->TimeChanged(TimeFromPoint(point));
        if (m_stashedRecordModeWhileTimeDragging)
        {
            GetIEditor()->GetAnimation()->SetRecording(true);   // re-enable recording that was disabled while dragging time
            m_stashedRecordModeWhileTimeDragging = false;       // reset stashed value
        }

    }
    else if (m_mouseMode == eTVMouseMode_Paste)
    {
        SetMouseCursor(Qt::ArrowCursor);
    }

    OnCaptureChanged();

    m_keyTimeOffset = 0;
    m_keyForTimeAdjust = CTrackViewKeyHandle();

    AcceptUndo();

    update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!sequence || m_rcTimeline.contains(point) || m_bEditLock)
    {
        return;
    }

    CTrackViewKeyHandle keyHandle = FirstKeyFromPoint(point);

    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }
    else
    {
        CTrackViewTrack* pTrack = GetTrackFromPoint(point);
        if (pTrack)
        {
            CTrackViewSequenceNotificationContext context(sequence);

            AzToolsFramework::ScopedUndoBatch undoBatch("Select key");

            const std::vector<bool> beforeKeyState = sequence->SaveKeyStates();

            sequence->DeselectAllKeys();
            keyHandle.Select(true);

            const std::vector<bool> afterKeyState = sequence->SaveKeyStates();

            if (beforeKeyState != afterKeyState)
            {
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

            m_keyTimeOffset = 0;

            if (pTrack->GetValueType() == AnimValueType::RGB)
            {
                // bring up color picker
                EditSelectedColorKey(pTrack);
            }
            else if (pTrack->GetValueType() != AnimValueType::Bool)
            {
                // Edit On Spot is blank (not useful) for boolean tracks so we disable dbl-clicking to bring it up for boolean tracks
                const QPoint p = QCursor::pos();

                bool bKeyChangeInSameTrack = m_pLastTrackSelectedOnSpot && pTrack == m_pLastTrackSelectedOnSpot;
                m_pLastTrackSelectedOnSpot = pTrack;

                ShowKeyPropertyCtrlOnSpot(p.x(), p.y(), sequence->GetSelectedKeys().GetKeyCount() > 1, bKeyChangeInSameTrack);
            }
        }

        return;
    }

    const bool bTryAddKeysInGroup = modifiers & Qt::ShiftModifier;

    AddKeys(point, bTryAddKeysInGroup);

    m_mouseMode = eTVMouseMode_None;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    OnRButtonDown(modifiers, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    OnRButtonUp(modifiers, point);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    HideKeyPropertyCtrlOnSpot();

    m_bCursorWasInKey = false;
    m_bMouseMovedAfterRButtonDown = false;

    if (m_rcTimeline.contains(point))
    {
        // Clicked inside timeline.
        // adjust markers.
        int nMarkerStart = TimeToClient(m_timeMarked.start);
        int nMarkerEnd = TimeToClient(m_timeMarked.end);
        if ((abs(point.x() - nMarkerStart)) < (abs(point.x() - nMarkerEnd)))
        {
            SetStartMarker(TimeFromPoint(point));
            m_mouseMode = eTVMouseMode_DragStartMarker;
        }
        else
        {
            SetEndMarker(TimeFromPoint(point));
            m_mouseMode = eTVMouseMode_DragEndMarker;
        }
        return;
    }

    m_mouseDownPos = point;

    if (modifiers & Qt::ShiftModifier)  // alternative zoom
    {
        m_bZoomDrag = true;
        return;
    }

    CTrackViewKeyHandle keyHandle = FirstKeyFromPoint(point);
    if (!keyHandle.IsValid())
    {
        keyHandle = DurationKeyFromPoint(point);
    }

    if (keyHandle.IsValid())
    {
        m_bCursorWasInKey = true;

        CTrackViewNode* pNode = GetNodeFromPoint(point);
        CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(pNode);

        keyHandle.Select(true);
        m_keyTimeOffset = 0;
        update();

        // Show a little pop-up menu for copy & delete.
        QMenu menu;
        CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();
        const bool bEnableEditOnSpot = ((pTrack && pTrack->GetValueType() != AnimValueType::Bool) &&
                                        (selectedKeys.GetKeyCount() > 0 && selectedKeys.AreAllKeysOfSameType()));

        QAction* actionEditOnSpot = menu.addAction(tr("Edit On Spot"));
        actionEditOnSpot->setEnabled(bEnableEditOnSpot);
        menu.addSeparator();
        QAction* actionCopy = menu.addAction(tr("Copy"));
        menu.addSeparator();
        QAction* actionDelete = menu.addAction(tr("Delete"));

        const QPoint p = QCursor::pos();
        QAction* action = menu.exec(p);
        if (action == actionEditOnSpot)
        {
            bool bKeyChangeInSameTrack
                = m_pLastTrackSelectedOnSpot
                    && selectedKeys.GetKeyCount() == 1
                    && selectedKeys.GetKey(0).GetTrack() == m_pLastTrackSelectedOnSpot;

            if (selectedKeys.GetKeyCount() == 1)
            {
                m_pLastTrackSelectedOnSpot = selectedKeys.GetKey(0).GetTrack();
            }
            else
            {
                m_pLastTrackSelectedOnSpot = nullptr;
            }

            ShowKeyPropertyCtrlOnSpot(p.x(), p.y(), selectedKeys.GetKeyCount() > 1, bKeyChangeInSameTrack);
        }
        else if (action == actionCopy)
        {
            pSequence->CopyKeysToClipboard(true, false);
        }
        else if (action == actionDelete)
        {
            CUndo undo("Delete Keys");
            pSequence->DeleteSelectedKeys();
        }
    }
    else
    {
        m_bMoveDrag = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnRButtonUp([[maybe_unused]] Qt::KeyboardModifiers modifiers, [[maybe_unused]] const QPoint& point)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    m_bZoomDrag = false;
    m_bMoveDrag = false;

    OnCaptureChanged();

    m_mouseMode = eTVMouseMode_None;

    if (!m_bCursorWasInKey)
    {
        const bool bHasCopiedKey = (GetKeysInClickboard() != nullptr);

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
void CTrackViewDopeSheetBase::CancelDrag()
{
    AcceptUndo();
    m_mouseMode = eTVMouseMode_None;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::mouseMoveEvent(QMouseEvent* event)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
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

    // For some drags, make sure the left mouse button is still down.
    // If you drag off the window, and press the right mouse button,
    // and *then* release the left mouse button, QT will never tell us
    // about the release event.
    bool leftButtonPressed = event->buttons() & Qt::LeftButton;

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

    if (m_mouseMode == eTVMouseMode_Select
        || m_mouseMode == eTVMouseMode_SelectWithinTime)
    {
        MouseMoveSelect(event->pos());
    }
    else if (m_mouseMode == eTVMouseMode_Move)
    {
        if (leftButtonPressed)
        {
            MouseMoveMove(event->pos(), event->modifiers());
        }
        else
        {
            CancelDrag();
        }
    }
    else if (m_mouseMode == eTVMouseMode_Clone)
    {
        pSequence->CloneSelectedKeys();
        m_mouseMode = eTVMouseMode_Move;
    }
    else if (m_mouseMode == eTVMouseMode_DragTime)
    {
        if (leftButtonPressed)
        {
            MouseMoveDragTime(event->pos(), event->modifiers());
        }
        else
        {
            CancelDrag();
        }
    }
    else if (m_mouseMode == eTVMouseMode_DragStartMarker)
    {
        if (leftButtonPressed)
        {
            MouseMoveDragStartMarker(event->pos(), event->modifiers());
        }
        else
        {
            CancelDrag();
        }
    }
    else if (m_mouseMode == eTVMouseMode_DragEndMarker)
    {
        if (leftButtonPressed)
        {
            MouseMoveDragEndMarker(event->pos(), event->modifiers());
        }
        else
        {
            CancelDrag();
        }
    }
    else if (m_mouseMode == eTVMouseMode_Paste)
    {
        update();
    }
    else if (m_mouseMode == eTVMouseMode_StartTimeAdjust)
    {
        if (leftButtonPressed)
        {
            MouseMoveStartEndTimeAdjust(event->pos(), true);
        }
        else
        {
            CancelDrag();
        }
    }
    else if (m_mouseMode == eTVMouseMode_EndTimeAdjust)
    {
        if (leftButtonPressed)
        {
            MouseMoveStartEndTimeAdjust(event->pos(), false);
        }
        else
        {
            CancelDrag();
        }
    }
    else
    {
        //////////////////////////////////////////////////////////////////////////
        if (m_mouseActionMode == eTVActionMode_AddKeys)
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
void CTrackViewDopeSheetBase::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

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

            if (GetIEditor()->GetAnimation()->GetSequence())
            {
                if (m_bEditLock)
                {
                    painter.fillRect(event->rect(), EDIT_DISABLE_GRAY_COLOR);
                }

                DrawControl(&painter, event->rect());
            }
        }
    }

    if (GetIEditor()->GetAnimation()->GetSequence())
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
void CTrackViewDopeSheetBase::SelectAllKeysWithinTimeFrame(const QRect& rc, const bool bMultiSelection)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!sequence)
    {
        return;
    }

    AzToolsFramework::ScopedUndoBatch undoBatch("Select keys");

    const std::vector<bool> beforeKeyState = sequence->SaveKeyStates();

    if (!bMultiSelection)
    {
        sequence->DeselectAllKeys();
    }

    // put selection rectangle from client to track space.
    QRect trackRect = rc;
    trackRect.translate(m_scrollOffset);

    Range selTime = GetTimeRange(trackRect);

    CTrackViewTrackBundle tracks = sequence->GetAllTracks();

    CTrackViewSequenceNotificationContext context(sequence);
    for (unsigned int i = 0; i < tracks.GetCount(); ++i)
    {
        CTrackViewTrack* pTrack = tracks.GetTrack(i);

        // Check which keys we intersect.
        for (unsigned int j = 0; j < pTrack->GetKeyCount(); j++)
        {
            CTrackViewKeyHandle keyHandle = pTrack->GetKey(j);
            const float time = keyHandle.GetTime();

            if (selTime.IsInside(time))
            {
                keyHandle.Select(true);
            }
        }
    }

    const std::vector<bool> afterKeyState = sequence->SaveKeyStates();

    if (beforeKeyState != afterKeyState)
    {
        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetMouseCursor(const QCursor& cursor)
{
    m_currCursor = cursor;
    setCursor(m_currCursor);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetCurrTime(float time)
{
    if (time < m_timeRange.start)
    {
        time = m_timeRange.start;
    }
    if (time > m_timeRange.end)
    {
        time = m_timeRange.end;
    }

    GetIEditor()->GetAnimation()->SetTime(time);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::OnTimeChanged(float newTime)
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
void CTrackViewDopeSheetBase::SetStartMarker(float fTime)
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

    GetIEditor()->GetAnimation()->SetMarkers(m_timeMarked);
    update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetEndMarker(float fTime)
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
    GetIEditor()->GetAnimation()->SetMarkers(m_timeMarked);
    update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetMouseActionMode(ETVActionMode mode)
{
    m_mouseActionMode = mode;
    if (mode == eTVActionMode_AddKeys)
    {
        setCursor(m_crsAddKey);
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNode* CTrackViewDopeSheetBase::GetNodeFromPointRec(CTrackViewNode* pCurrentNode, const QPoint& point)
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

    if (pCurrentNode->GetExpanded())
    {
        unsigned int childCount = pCurrentNode->GetChildCount();
        for (unsigned int i = 0; i < childCount; ++i)
        {
            CTrackViewNode* pFoundNode = GetNodeFromPointRec(pCurrentNode->GetChild(i), point);
            if (pFoundNode)
            {
                return pFoundNode;
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNode* CTrackViewDopeSheetBase::GetNodeFromPoint(const QPoint& point)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    return GetNodeFromPointRec(pSequence, point);
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode* CTrackViewDopeSheetBase::GetAnimNodeFromPoint(const QPoint& point)
{
    CTrackViewNode* pNode = GetNodeFromPoint(point);

    if (pNode)
    {
        if (pNode->GetNodeType() == eTVNT_Track)
        {
            CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(pNode);
            return static_cast<CTrackViewAnimNode*>(pTrack->GetAnimNode());
        }
        else if (pNode->GetNodeType() == eTVNT_AnimNode)
        {
            return static_cast<CTrackViewAnimNode*>(pNode);
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack* CTrackViewDopeSheetBase::GetTrackFromPoint(const QPoint& point)
{
    CTrackViewNode* pNode = GetNodeFromPoint(point);

    if (pNode && pNode->GetNodeType() == eTVNT_Track)
    {
        return static_cast<CTrackViewTrack*>(pNode);
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetHorizontalExtent(int min, int max)
{
    m_scrollMin = min;
    m_scrollMax = max;
    m_scrollBar->setPageStep(m_rcClient.width() / 2);
    m_scrollBar->setRange(min, max - m_scrollBar->pageStep() * 2 + m_leftOffset);
};

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CTrackViewDopeSheetBase::GetKeysInClickboard()
{
    CClipboard clip(this);
    if (clip.IsEmpty())
    {
        return nullptr;
    }

    if (clip.GetTitle() != "Track view keys")
    {
        return nullptr;
    }

    XmlNodeRef copyNode = clip.Get();
    if (copyNode == nullptr || strcmp(copyNode->getTag(), "CopyKeysNode"))
    {
        return nullptr;
    }

    int nNumTracksToPaste = copyNode->getChildCount();
    if (nNumTracksToPaste == 0)
    {
        return nullptr;
    }

    return copyNode;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::StartPasteKeys()
{
    m_clipboardKeys = GetKeysInClickboard();

    if (m_clipboardKeys)
    {
        m_mouseMode = eTVMouseMode_Paste;
        // If mouse over selected key, change cursor to left-right arrows.
        SetMouseCursor(m_crsLeftRight);
        m_mouseDownPos = m_mouseOverPos;
    }
}

void CTrackViewDopeSheetBase::keyPressEvent(QKeyEvent* event)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!sequence)
    {
        return;
    }

    // HAVE TO INCLUDE CASES FOR THESE IN THE ShortcutOverride handler in ::event() below
    if (event->matches(QKeySequence::Delete))
    {
        CUndo undo("Delete Keys");
        sequence->DeleteSelectedKeys();
        return;
    }

    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down || event->key() == Qt::Key_Right || event->key() == Qt::Key_Left)
    {
        CTrackViewKeyBundle keyBundle = sequence->GetSelectedKeys();
        CTrackViewKeyHandle keyHandle = keyBundle.GetSingleSelectedKey();

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
                CTrackViewSequenceNotificationContext context(sequence);

                const std::vector<bool> beforeKeyState = sequence->SaveKeyStates();

                AzToolsFramework::ScopedUndoBatch undoBatch("Select Key");

                sequence->DeselectAllKeys();
                keyHandle.Select(true);

                const std::vector<bool> afterKeyState = sequence->SaveKeyStates();

                if (beforeKeyState != afterKeyState)
                {
                    undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
                }

            }
        }

        return;
    }

    if (event->matches(QKeySequence::Copy))
    {
        sequence->CopyKeysToClipboard(true, false);
    }
    else if (event->matches(QKeySequence::Paste))
    {
        StartPasteKeys();
    }
    else if (event->matches(QKeySequence::Undo))
    {
        GetIEditor()->Undo();
    }
    else if (event->matches(QKeySequence::Redo))
    {
        GetIEditor()->Redo();
    }
    else
    {
        return QWidget::keyPressEvent(event);
    }
}

bool CTrackViewDopeSheetBase::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride)
    {
        // since we respond to the following things, let Qt know so that shortcuts don't override us
        bool respondsToEvent = false;

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        switch (keyEvent->key())
        {
            case Qt::Key_Delete:
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Left:
            case Qt::Key_Right:
                respondsToEvent = true;
            break;

            default:
                respondsToEvent =
                    keyEvent->matches(QKeySequence::Copy) ||
                    keyEvent->matches(QKeySequence::Paste) ||
                    keyEvent->matches(QKeySequence::Undo) ||
                    keyEvent->matches(QKeySequence::Redo);
            break;
        }

        if (respondsToEvent)
        {
            e->accept();
            return true;
        }
    }

    return QWidget::event(e);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ShowKeyTooltip(CTrackViewKeyHandle& keyHandle, const QPoint& point)
{
    if (m_lastTooltipPos == point)
    {
        return;
    }

    m_lastTooltipPos = point;

    const float time = keyHandle.GetTime();
    const char* desc = keyHandle.GetDescription();

    QString tipText;
    if (GetTickDisplayMode() == eTVTickMode_InSeconds)
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
void CTrackViewDopeSheetBase::OnCaptureChanged()
{
    AcceptUndo();

    m_bZoomDrag = false;
    m_bMoveDrag = false;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::IsOkToAddKeyHere(const CTrackViewTrack* pTrack, float time) const
{
    for (unsigned int i = 0; i < pTrack->GetKeyCount(); ++i)
    {
        const CTrackViewKeyConstHandle& keyHandle = pTrack->GetKey(i);

        if (keyHandle.GetTime() == time)
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MouseMoveSelect(const QPoint& point)
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
    if (m_mouseMode == eTVMouseMode_SelectWithinTime)
    {
        rc.setTop(m_rcClient.top());
        rc.setBottom(m_rcClient.bottom());
    }

    m_rcSelect = rc;
    m_rubberBand->setGeometry(m_rcSelect);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MouseMoveStartEndTimeAdjust(const QPoint& p, bool bStart)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!sequence)
    {
        return;
    }

    SetMouseCursor(m_crsAdjustLR);
    const QPoint point(qBound(m_rcClient.left(), p.x(), m_rcClient.right()), p.y());

    const QPoint ofs = point - m_mouseDownPos;

    CTrackViewKeyHandle& keyHandle = m_keyForTimeAdjust;

    // TODO: Refactor this Time Range Key stuff.
    ICharacterKey characterKey;
    AZ::IAssetBlendKey assetBlendKey;
    ITimeRangeKey* timeRangeKey = nullptr;

    if (keyHandle.GetTrack()->GetValueType() == AnimValueType::AssetBlend)
    {
        keyHandle.GetKey(&assetBlendKey);
        timeRangeKey = &assetBlendKey;
    }
    else
    {
        // This will work for both character & time range keys because
        // ICharacterKey is derived from ITimeRangeKey. Not the most beautiful code.
        keyHandle.GetKey(&characterKey);
        timeRangeKey = &characterKey;
    }

    float& timeToAdjust = bStart ? timeRangeKey->m_startTime : timeRangeKey->m_endTime;

    // Undo the last offset.
    timeToAdjust += -m_keyTimeOffset;

    // Apply a new offset.
    m_keyTimeOffset = (ofs.x() / m_timeScale) * timeRangeKey->m_speed;
    timeToAdjust += m_keyTimeOffset;

    // Check the validity.
    if (bStart)
    {
        if (timeToAdjust < 0)
        {
            timeToAdjust = 0;
        }
        else if (timeToAdjust > timeRangeKey->GetValidEndTime())
        {
            timeToAdjust = timeRangeKey->GetValidEndTime();
        }
    }
    else
    {
        float endTime = AZStd::min(timeRangeKey->GetValidEndTime(), timeRangeKey->m_duration);
        if (timeToAdjust < timeRangeKey->m_startTime)
        {
            timeToAdjust = timeRangeKey->m_startTime;
        }
        else if (timeToAdjust > endTime)
        {
            timeToAdjust = endTime;
        }
    }

    keyHandle.SetKey(timeRangeKey);

    update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::MouseMoveMove(const QPoint& p, [[maybe_unused]] Qt::KeyboardModifiers modifiers)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    CTrackViewSequenceNotificationContext context(pSequence);

    SetMouseCursor(m_crsLeftRight);
    const QPoint point(qBound(m_rcClient.left(), p.x(), m_rcClient.right()), p.y());

    // Reset tracks to their initial state before starting the move
    for (auto iter = m_trackMementos.begin(); iter != m_trackMementos.end(); ++iter)
    {
        CTrackViewTrack* pTrack = iter->first;

        const TrackMemento& trackMemento = iter->second;
        pTrack->RestoreFromMemento(trackMemento.m_memento);

        const unsigned int numKeys = static_cast<unsigned int>(trackMemento.m_keySelectionStates.size());
        for (unsigned int i = 0; i < numKeys; ++i)
        {
            pTrack->GetKey(i).Select(trackMemento.m_keySelectionStates[i]);
        }
    }

    CTrackViewKeyHandle keyHandle = FirstKeyFromPoint(m_mouseDownPos);
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

    if (m_mouseActionMode == eTVActionMode_ScaleKey)
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
        if (m_mouseActionMode == eTVActionMode_SlideKey)
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
            CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();
            CTrackViewKeyHandle selectedKey = selectedKeys.GetSingleSelectedKey();

            if (selectedKey.IsValid())
            {
                GetIEditor()->GetAnimation()->SetTime(selectedKey.GetTime());
            }
        }
        m_keyTimeOffset = timeOffset;
    }

    // The time of the selected keys has likely just changed. OnKeySelectionChanged() so the 
    // UI elements of the key properties control will update.
    pSequence->OnKeySelectionChanged();
}

void CTrackViewDopeSheetBase::MouseMoveDragTime(const QPoint& point, Qt::KeyboardModifiers modifiers)
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

void CTrackViewDopeSheetBase::MouseMoveDragStartMarker(const QPoint& point, Qt::KeyboardModifiers modifiers)
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

void CTrackViewDopeSheetBase::MouseMoveDragEndMarker(const QPoint& point, Qt::KeyboardModifiers modifiers)
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

void CTrackViewDopeSheetBase::MouseMoveOver(const QPoint& point)
{
    // No mouse mode.
    SetMouseCursor(Qt::ArrowCursor);

    bool bStart = false;
    CTrackViewKeyHandle keyHandle = CheckCursorOnStartEndTimeAdjustBar(point, bStart);
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
        CTrackViewTrack* pTrack = GetTrackFromPoint(point);

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

float CTrackViewDopeSheetBase::MagnetSnap(float newTime, const CTrackViewAnimNode* pNode) const
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return newTime;
    }

    CTrackViewKeyBundle keys = pSequence->GetKeysInTimeRange(newTime - kMarginForMagnetSnapping / m_timeScale,
            newTime + kMarginForMagnetSnapping / m_timeScale);

    if (keys.GetKeyCount() > 0)
    {
        // By default, just use the first key that belongs to the time range as a magnet.
        newTime = keys.GetKey(0).GetTime();
        // But if there is an in-range key in a sibling track, use it instead.
        // Here a 'sibling' means a track that belongs to a same node.
        for (unsigned int i = 0; i < keys.GetKeyCount(); ++i)
        {
            CTrackViewKeyHandle keyHandle = keys.GetKey(i);
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
float CTrackViewDopeSheetBase::FrameSnap(float time) const
{
    double t = floor((double)time / m_snapFrameTime + 0.5);
    t = t * m_snapFrameTime;
    return static_cast<float>(t);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ShowKeyPropertyCtrlOnSpot(int x, int y, [[maybe_unused]] bool bMultipleKeysSelected, bool bKeyChangeInSameTrack)
{
    if (m_keyPropertiesDlg == nullptr)
    {
        return;
    }


    if (m_wndPropsOnSpot == nullptr)
    {
        m_wndPropsOnSpot = new ReflectedPropertyControl(this);
        m_wndPropsOnSpot->Setup(true, 150);
        m_wndPropsOnSpot->setWindowFlags(Qt::CustomizeWindowHint | Qt::Popup | Qt::WindowStaysOnTopHint);
        m_wndPropsOnSpot->SetStoreUndoByItems(false);
        bKeyChangeInSameTrack = false;
    }

    if (bKeyChangeInSameTrack)
    {
        m_wndPropsOnSpot->ClearSelection();
        m_wndPropsOnSpot->ReloadValues();
    }
    else
    {
        m_keyPropertiesDlg->PopulateVariables(m_wndPropsOnSpot);
    }

    m_wndPropsOnSpot->show();
    m_wndPropsOnSpot->move(x, y);
    m_wndPropsOnSpot->ExpandAll();
    QTimer::singleShot(0, [this]() {
        m_wndPropsOnSpot->resize(m_wndPropsOnSpot->sizeHint());
    });
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::HideKeyPropertyCtrlOnSpot()
{
    if (m_wndPropsOnSpot)
    {
        m_wndPropsOnSpot->hide();
        m_wndPropsOnSpot->ClearSelection();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetScrollOffset(int hpos)
{
    m_scrollBar->setValue(hpos);
    m_scrollOffset.setX(hpos);
    update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::LButtonDownOnTimeAdjustBar([[maybe_unused]] const QPoint& point, CTrackViewKeyHandle& keyHandle, bool bStart)
{
    m_keyTimeOffset = 0;
    m_keyForTimeAdjust = keyHandle;

    GetIEditor()->BeginUndo();

    if (bStart)
    {
        m_mouseMode = eTVMouseMode_StartTimeAdjust;
    }
    else
    {
        // TODO: Refactor this Time Range Key stuff.
        ICharacterKey characterKey;
        AZ::IAssetBlendKey assetBlendKey;
        ITimeRangeKey* timeRangeKey = nullptr;

        if (keyHandle.GetTrack()->GetValueType() == AnimValueType::AssetBlend)
        {
            keyHandle.GetKey(&assetBlendKey);
            timeRangeKey = &assetBlendKey;
        }
        else
        {
            // This will work for both character & time range keys because
            // ICharacterKey is derived from ITimeRangeKey. Not the most beautiful code.
            keyHandle.GetKey(&characterKey);
            timeRangeKey = &characterKey;
        }

        // In case of the end time, make it have a valid (not zero)
        // end time, first.
        if (timeRangeKey->m_endTime == 0)
        {
            timeRangeKey->m_endTime = timeRangeKey->m_duration;
            keyHandle.SetKey(timeRangeKey);
        }
        m_mouseMode = eTVMouseMode_EndTimeAdjust;
    }
    SetMouseCursor(m_crsAdjustLR);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::LButtonDownOnKey([[maybe_unused]] const QPoint& point, CTrackViewKeyHandle& keyHandle, Qt::KeyboardModifiers modifiers)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    AZ_Assert(sequence, "Expected a valid sequence.");

    if (!sequence)
    {
        return;
    }

    if (!keyHandle.IsSelected() && !(modifiers & Qt::ControlModifier))
    {
        CTrackViewSequenceNotificationContext context(sequence);
        AzToolsFramework::ScopedUndoBatch undoBatch("Select keys");

        const std::vector<bool> beforeKeyState = sequence->SaveKeyStates();

        sequence->DeselectAllKeys();
        m_bJustSelected = true;
        m_keyTimeOffset = 0;
        keyHandle.Select(true);

        ChangeSequenceTrackSelection(sequence, keyHandle.GetTrack());

        const std::vector<bool> afterKeyState = sequence->SaveKeyStates();

        if (beforeKeyState != afterKeyState)
        {
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
    else
    {
        GetIEditor()->CancelUndo();
    }

    // Move/Clone Key Undo Begin
    GetIEditor()->BeginUndo();
    StoreMementoForTracksWithSelectedKeys();

    if (modifiers & Qt::ShiftModifier)
    {
        m_mouseMode = eTVMouseMode_Clone;
        SetMouseCursor(m_crsLeftRight);
    }
    else
    {
        m_mouseMode = eTVMouseMode_Move;
        SetMouseCursor(m_crsLeftRight);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ChangeSequenceTrackSelection(CTrackViewSequence* sequenceWithTrack, CTrackViewTrack* trackToSelect) const
{
    // Deselect all currently selected tracks that aren't the keyHandle's Track, then ensure the trackToSelect  is selected
    CTrackViewTrackBundle prevSelectedTracks;

    prevSelectedTracks = sequenceWithTrack->GetSelectedTracks();
    for (unsigned int i = 0; i < prevSelectedTracks.GetCount(); i++)
    {
        CTrackViewTrack* prevSelectedTrack = prevSelectedTracks.GetTrack(i);
        if (prevSelectedTrack != trackToSelect)
        {
            prevSelectedTrack->SetSelected(false);
        }
    }
    trackToSelect->SetSelected(true);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ChangeSequenceTrackSelection(CTrackViewSequence* sequence, CTrackViewTrackBundle tracksToSelect, bool multiTrackSelection) const
{
    if (!multiTrackSelection)
    {
        // Deselect any tracks not in the tracksToSelect bundle
        CTrackViewTrackBundle prevSelectedTracks;
        prevSelectedTracks = sequence->GetSelectedTracks();

        for (int i = prevSelectedTracks.GetCount(); --i >= 0; )
        {
            bool                deselectTrack = true;
            CTrackViewTrack*    prevSelectedTrack = prevSelectedTracks.GetTrack(i);

            for (int j = tracksToSelect.GetCount(); --j >= 0; )
            {
                CTrackViewTrack* selectTrackCandidate = tracksToSelect.GetTrack(j);
                if (selectTrackCandidate == prevSelectedTrack)
                {
                    // selectTrackCandidate is already selected
                    tracksToSelect.RemoveTrack(selectTrackCandidate);
                    deselectTrack = false;
                    break;
                }
            }
            if (deselectTrack)
            {
                prevSelectedTrack->SetSelected(false);
            }
        }
    }

    // Add remaining tracks in tracksToSelect bundle to track selection
    for (int j = tracksToSelect.GetCount(); --j >= 0; )
    {
        tracksToSelect.GetTrack(j)->SetSelected(true);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::CreateColorKey(CTrackViewTrack* pTrack, float keyTime)
{
    bool keyCreated = false;
    Vec3 vColor(0, 0, 0);
    pTrack->GetValue(keyTime, vColor);

    const AZ::Color defaultColor(
        clamp_tpl<AZ::u8>(static_cast<AZ::u8>(FloatToIntRet(vColor.x)), 0, 255),
        clamp_tpl<AZ::u8>(static_cast<AZ::u8>(FloatToIntRet(vColor.y)), 0, 255),
        clamp_tpl<AZ::u8>(static_cast<AZ::u8>(FloatToIntRet(vColor.z)), 0, 255),
        255);
    AzQtComponents::ColorPicker dlg(AzQtComponents::ColorPicker::Configuration::RGB, QString(), this);
    dlg.setWindowTitle(tr("Select Color"));
    dlg.setCurrentColor(defaultColor);
    dlg.setSelectedColor(defaultColor);
    if (dlg.exec() == QDialog::Accepted)
    {
        const AZ::Color col = dlg.currentColor();
        ColorF colArray(col.GetR8(), col.GetG8(), col.GetB8(), col.GetA8());

        CTrackViewSequence* sequence = pTrack->GetSequence();
        if (nullptr != sequence)
        {
            CTrackViewSequenceNotificationContext context(sequence);

            AzToolsFramework::ScopedUndoBatch undoBatch("Set Key");
            const unsigned int numChildNodes = pTrack->GetChildCount();
            for (unsigned int i = 0; i < numChildNodes; ++i)
            {
                CTrackViewTrack* subTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
                if (IsOkToAddKeyHere(subTrack, keyTime))
                {
                    CTrackViewKeyHandle newKey = subTrack->CreateKey(keyTime);

                    I2DBezierKey bezierKey;
                    newKey.GetKey(&bezierKey);
                    bezierKey.value = Vec2(keyTime, colArray[i]);
                    newKey.SetKey(&bezierKey);

                    keyCreated = true;
                }
            }
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }

    return keyCreated;
}

void CTrackViewDopeSheetBase::OnCurrentColorChange(const AZ::Color& color)
{
    // This is while the color picker is up 
    // so we want to update the property but not store an undo
    UpdateColorKey(AzQtComponents::toQColor(color), false);
}

void CTrackViewDopeSheetBase::UpdateColorKey(const QColor& color, bool addToUndo)
{
    ColorF colArray(static_cast<f32>(color.redF()), static_cast<f32>(color.greenF()), static_cast<f32>(color.blueF()), static_cast<f32>(color.alphaF()));

    CTrackViewSequence* sequence = m_colorUpdateTrack->GetSequence();
    if (nullptr != sequence)
    {
        CTrackViewSequenceNotificationContext context(sequence);

        if (addToUndo)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Set Key");
            UpdateColorKeyHelper(colArray);
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
        else
        {
            UpdateColorKeyHelper(colArray);
        }

        // We want this to take affect now
        if (!addToUndo)
        {
            GetIEditor()->GetAnimation()->ForceAnimation();
        }
    }
}

void CTrackViewDopeSheetBase::UpdateColorKeyHelper(const ColorF& color)
{
    const unsigned int numChildNodes = m_colorUpdateTrack->GetChildCount();
    for (unsigned int i = 0; i < numChildNodes; ++i)
    {
        CTrackViewTrack* subTrack = static_cast<CTrackViewTrack*>(m_colorUpdateTrack->GetChild(i));
        CTrackViewKeyHandle subTrackKey = subTrack->GetKeyByTime(m_colorUpdateKeyTime);
        I2DBezierKey bezierKey;
        if (subTrackKey.IsValid())
        {
            subTrackKey.GetKey(&bezierKey);
        }
        else
        {
            // no valid key found at this time - create Key
            subTrackKey = subTrack->CreateKey(m_colorUpdateKeyTime);
            subTrackKey.GetKey(&bezierKey);
        }

        bezierKey.value.x = m_colorUpdateKeyTime;
        bezierKey.value.y = color[i];
        subTrackKey.SetKey(&bezierKey);
    }
}

void CTrackViewDopeSheetBase::EditSelectedColorKey(CTrackViewTrack* pTrack)
{
    if (pTrack->IsCompoundTrack())
    {
        CTrackViewKeyBundle selectedKeyBundle = pTrack->GetSelectedKeys();
        if (selectedKeyBundle.GetKeyCount())
        {
            m_colorUpdateTrack = pTrack;
            // init with the first selected key color
            m_colorUpdateKeyTime = selectedKeyBundle.GetKey(0).GetTime();

            Vec3  color;
            pTrack->GetValue(m_colorUpdateKeyTime, color);

            const AZ::Color defaultColor(
                clamp_tpl(static_cast<AZ::u8>(FloatToIntRet(color.x)), AZ::u8(0), AZ::u8(255)),
                clamp_tpl(static_cast<AZ::u8>(FloatToIntRet(color.y)), AZ::u8(0), AZ::u8(255)),
                clamp_tpl(static_cast<AZ::u8>(FloatToIntRet(color.z)), AZ::u8(0), AZ::u8(255)),
                255);

            AzQtComponents::ColorPicker picker(AzQtComponents::ColorPicker::Configuration::RGB);
            picker.setWindowTitle(tr("Select Color"));
            picker.setCurrentColor(defaultColor);
            picker.setSelectedColor(defaultColor);
            QObject::connect(&picker, &AzQtComponents::ColorPicker::currentColorChanged, this, &CTrackViewDopeSheetBase::OnCurrentColorChange);
    
            if (picker.exec() == QDialog::Accepted)
            {
                const AZ::Color col = picker.currentColor();

                // Moved bulk of method into helper to handle matching logic in QT callback and undo redo cases
                UpdateColorKey(AzQtComponents::toQColor(col), true);
            }
            else
            {
                // We canceled out of the color picker, revert to color held before opening it
                UpdateColorKey(AzQtComponents::toQColor(defaultColor), false);
            }
        }      
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::AcceptUndo()
{
    if (CUndo::IsRecording())
    {
        CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

        if (m_mouseMode == eTVMouseMode_Paste)
        {
            GetIEditor()->CancelUndo();
        }
        else if (m_mouseMode == eTVMouseMode_Move || m_mouseMode == eTVMouseMode_Clone)
        {
            if (sequence && m_bKeysMoved)
            {
                GetIEditor()->CancelUndo();

                // Keys Moved, mark the sequence dirty to get an AZ undo event.
                AzToolsFramework::ScopedUndoBatch undoBatch("Move/Clone Keys");
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
            else
            {
                GetIEditor()->CancelUndo();
            }
        }
        else if (m_mouseMode == eTVMouseMode_StartTimeAdjust || m_mouseMode == eTVMouseMode_EndTimeAdjust)
        {
            if (sequence)
            {
                GetIEditor()->CancelUndo();

                AzToolsFramework::ScopedUndoBatch undoBatch("Adjust Start/End Time of an Animation Key");
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
            else
            {
                GetIEditor()->CancelUndo();
            }
        }
    }

    m_mouseMode = eTVMouseMode_None;
    m_trackMementos.clear();
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewDopeSheetBase::ComputeSnappedMoveOffset()
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
void CTrackViewDopeSheetBase::AddKeys(const QPoint& point, const bool bTryAddKeysInGroup)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!sequence)
    {
        return;
    }

    // Add keys here.
    CTrackViewTrack* pTrack = GetTrackFromPoint(point);

    if (!pTrack)
    {
        return;
    }

    CTrackViewSequenceNotificationContext context(sequence);

    CTrackViewAnimNode* pNode = pTrack->GetAnimNode();
    float keyTime = TimeFromPoint(point);
    bool inRange = m_timeRange.IsInside(keyTime);

    if (pTrack && inRange)
    {
        if (bTryAddKeysInGroup && pNode->GetParentNode())       // Add keys in group
        {
            CTrackViewTrackBundle tracksInGroup = pNode->GetTracksByParam(pTrack->GetParameterType());
            for (int i = 0; i < (int)tracksInGroup.GetCount(); ++i)
            {
                CTrackViewTrack* pCurrTrack = tracksInGroup.GetTrack(i);

                if (pCurrTrack->GetChildCount() == 0)   // A simple track
                {
                    if (IsOkToAddKeyHere(pCurrTrack, keyTime))
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Create Key");
                        pCurrTrack->CreateKey(keyTime);
                        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
                    }
                }
                else                                                                            // A compound track
                {
                    for (unsigned int k = 0; k < pCurrTrack->GetChildCount(); ++k)
                    {
                        CTrackViewTrack* pSubTrack = static_cast<CTrackViewTrack*>(pCurrTrack->GetChild(k));
                        if (IsOkToAddKeyHere(pSubTrack, keyTime))
                        {
                            AzToolsFramework::ScopedUndoBatch undoBatch("Create Key");
                            pSubTrack->CreateKey(keyTime);
                            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
                        }
                    }
                }
            }
        }
        else if (pTrack->GetChildCount() == 0)          // A simple track
        {
            if (IsOkToAddKeyHere(pTrack, keyTime))
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Create Key");
                pTrack->CreateKey(keyTime);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
        else                                                                                // A compound track
        {
            if (pTrack->GetValueType() == AnimValueType::RGB)
            {
                CreateColorKey(pTrack, keyTime);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Create Key");
                for (unsigned int i = 0; i < pTrack->GetChildCount(); ++i)
                {
                    CTrackViewTrack* pSubTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
                    if (IsOkToAddKeyHere(pSubTrack, keyTime))
                    {
                        pSubTrack->CreateKey(keyTime);
                    }
                }
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawControl(QPainter* painter, const QRect& rcUpdate)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    DrawNodesRecursive(pSequence, painter, rcUpdate);

    DrawSummary(painter, rcUpdate);

    DrawSelectedKeyIndicators(painter);

    if (m_mouseMode == eTVMouseMode_Paste)
    {
        // If in paste mode draw keys that are in clipboard
        DrawClipboardKeys(painter, QRect());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawNodesRecursive(CTrackViewNode* pNode, QPainter* painter, const QRect& rcUpdate)
{
    const QRect rect = GetNodeRect(pNode);

    if (!rect.isEmpty())
    {
        switch (pNode->GetNodeType())
        {
        case eTVNT_AnimNode:
            DrawNodeTrack(static_cast<CTrackViewAnimNode*>(pNode), painter, rect);
            break;
        case eTVNT_Track:
            DrawTrack(static_cast<CTrackViewTrack*>(pNode), painter, rect);
            break;
        }
    }

    if (pNode->GetExpanded())
    {
        unsigned int numChildren = pNode->GetChildCount();
        for (unsigned int i = 0; i < numChildren; ++i)
        {
            DrawNodesRecursive(pNode->GetChild(i), painter, rcUpdate);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawTicks(QPainter* painter, const QRect& rc, Range& timeRange)
{
    // Draw time ticks every tick step seconds.
    const QPen dkgray(QColor(90, 90, 90));
    const QPen ltgray(QColor(120, 120, 120));

    const QPen prevPen = painter->pen();
    painter->setPen(dkgray);
    Range VisRange = GetVisibleRange();
    int nNumberTicks = 10;
    if (GetTickDisplayMode() == eTVTickMode_InFrames)
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
void CTrackViewDopeSheetBase::DrawTrack(CTrackViewTrack* pTrack, QPainter* painter, const QRect& trackRect)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    const QPen prevPen = painter->pen();
    painter->setPen(QColor(120, 120, 120));
    painter->drawLine(trackRect.bottomLeft(), trackRect.bottomRight());
    painter->setPen(prevPen);

    QRect rcInner = trackRect;
    rcInner.setLeft(max(trackRect.left(), m_leftOffset - m_scrollOffset.x()));
    rcInner.setRight(min(trackRect.right(), (m_scrollMax + m_scrollMin) - m_scrollOffset.x() + m_leftOffset * 2));

    bool bLightAnimationSetActive = pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet;
    if (bLightAnimationSetActive && pTrack->GetKeyCount() > 0)
    {
        // In the case of the light animation set, the time of of the last key
        // determines the end of the track.
        float lastKeyTime = pTrack->GetKey(pTrack->GetKeyCount() - 1).GetTime();
        rcInner.setRight(min(rcInner.right(), TimeToClient(lastKeyTime)));
    }

    QRect rcInnerDraw(QPoint(rcInner.left() - 6, rcInner.top()), QPoint(rcInner.right() + 6, rcInner.bottom()));
    QColor trackColor = CTVCustomizeTrackColorsDlg::GetTrackColor(pTrack->GetParameterType());
    if (pTrack->HasCustomColor())
    {
        ColorB customColor = pTrack->GetCustomColor();
        trackColor = QColor(customColor.r, customColor.g, customColor.b);
    }
    // For the case of tracks belonging to an inactive director node,
    // changes the track color to a custom one.
    const QColor colorForDisabled = CTVCustomizeTrackColorsDlg::GetColorForDisabledTracks();
    const QColor colorForMuted = CTVCustomizeTrackColorsDlg::GetColorForMutedTracks();

    CTrackViewAnimNode* pDirectorNode = pTrack->GetDirector();
    if (!pDirectorNode->IsActiveDirector())
    {
        trackColor = colorForDisabled;
    }

    // A disabled/muted track or any track in a disabled node also uses a custom color.
    CTrackViewAnimNode* animNode = pTrack->GetAnimNode();
    bool bTrackDisabled = pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled;
    bool bTrackMuted = pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted;
    bool bTrackInvalid = !pTrack->IsSubTrack() && !animNode->IsParamValid(pTrack->GetParameterType());
    bool bTrackInDisabledNode = animNode->AreFlagsSetOnNodeOrAnyParent(eAnimNodeFlags_Disabled);
    if (bTrackDisabled || bTrackInDisabledNode || bTrackInvalid)
    {
        trackColor = colorForDisabled;
    }
    else if (bTrackMuted)
    {
        trackColor = colorForMuted;
    }
    const QRect rc = rcInnerDraw.adjusted(0, 1, 0, 0);

    const EAnimCurveType trackType = pTrack->GetCurveType();
    if (trackType == eAnimCurveType_TCBFloat || trackType == eAnimCurveType_TCBQuat || trackType == eAnimCurveType_TCBVector)
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
    else if (pTrack->GetValueType() == AnimValueType::RGB && pTrack->GetKeyCount() > 0)
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
    AnimValueType trackValueType = pTrack->GetValueType();
    CAnimParamType trackParamType = pTrack->GetParameterType();

    if (trackValueType == AnimValueType::Bool)
    {
        // If this track is bool Track draw bars where track is true
        DrawBoolTrack(timeRange, painter, pTrack, rc);
    }
    else if (trackValueType == AnimValueType::Select)
    {
        // If this track is Select Track draw bars to show where selection is active.
        DrawSelectTrack(timeRange, painter, pTrack, rc);
    }
    else if (trackParamType == AnimParamType::Sequence)
    {
        // If this track is Sequence Track draw bars to show where sequence is active.
        DrawSequenceTrack(timeRange, painter, pTrack, rc);
    }
    else if (trackParamType == AnimParamType::Goto)
    {
        // if this track is GoTo Track, draw an arrow to indicate jump position.
        DrawGoToTrackArrow(pTrack, painter, rc);
    }

    // Draw keys in time range.
    DrawKeys(pTrack, painter, rcInner, timeRange);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawSelectTrack(const Range& timeRange, QPainter* painter, CTrackViewTrack* pTrack, const QRect& rc)
{
    const QBrush prevBrush = painter->brush();
    painter->setBrush(m_selectTrackBrush);

    const int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

        ISelectKey selectKey;
        keyHandle.GetKey(&selectKey);

        if (!selectKey.szSelection.empty() || selectKey.cameraAzEntityId.IsValid())
        {
            float time = keyHandle.GetTime();
            float nextTime = timeRange.end;
            if (i < numKeys - 1)
            {
                nextTime = pTrack->GetKey(i + 1).GetTime();
            }

            time = clamp_tpl(time, timeRange.start, timeRange.end);
            nextTime = clamp_tpl(nextTime, timeRange.start, timeRange.end);

            int x0_2 = TimeToClient(time);

            float fBlendTime = selectKey.fBlendTime;
            int blendTimeEnd = 0;

            if (fBlendTime > 0.0f && fBlendTime < (nextTime - time))
            {
                blendTimeEnd = TimeToClient(nextTime);
                nextTime -= fBlendTime;
            }

            int x = TimeToClient(nextTime);

            if (x != x0_2)
            {
                QLinearGradient gradient(x0_2, rc.top() + 1, x0_2, rc.bottom());
                gradient.setColorAt(0, Qt::white);
                gradient.setColorAt(1, QColor(100, 190, 255));
                painter->fillRect(QRect(QPoint(x0_2, rc.top() + 1), QPoint(x, rc.bottom())), gradient);
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
void CTrackViewDopeSheetBase::DrawBoolTrack(const Range& timeRange, QPainter* painter, CTrackViewTrack* pTrack, const QRect& rc)
{
    int x0 = TimeToClient(timeRange.start);

    const QBrush prevBrush = painter->brush();
    painter->setBrush(m_visibilityBrush);

    const int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

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
void CTrackViewDopeSheetBase::DrawSequenceTrack(const Range& timeRange, QPainter* painter, CTrackViewTrack* pTrack, const QRect& rc)
{
    const QBrush prevBrush = painter->brush();
    painter->setBrush(m_selectTrackBrush);

    const int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys - 1; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

        ISequenceKey sequenceKey;
        keyHandle.GetKey(&sequenceKey);
        if (sequenceKey.sequenceEntityId.IsValid())
        {
            float time = keyHandle.GetTime();
            float nextTime = timeRange.end;
            if (i < numKeys - 1)
            {
                nextTime = pTrack->GetKey(i + 1).GetTime();
            }
            time = clamp_tpl(time, timeRange.start, timeRange.end);
            nextTime = clamp_tpl(nextTime, timeRange.start, timeRange.end);

            int x0_2 = TimeToClient(time);
            int x = TimeToClient(nextTime);

            if (x != x0_2)
            {
                const QColor startColour(100, 190, 255);
                const QColor endColour(250, 250, 250);
                QLinearGradient gradient(x0_2, rc.top() + 1, x0_2, rc.bottom());
                gradient.setColorAt(0, startColour);
                gradient.setColorAt(1, endColour);
                painter->fillRect(QRect(QPoint(x0_2, rc.top() + 1), QPoint(x, rc.bottom())), gradient);
            }
        }
    }
    painter->setBrush(prevBrush);
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewDopeSheetBase::CompareKeyHandleByTime(const CTrackViewKeyHandle &a, const CTrackViewKeyHandle &b)
{
    return a.GetTime() < b.GetTime();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawKeys(CTrackViewTrack* pTrack, QPainter* painter, QRect& rect, [[maybe_unused]] Range& timeRange)
{
    int numKeys = pTrack->GetKeyCount();

    const QFont prevFont = painter->font();
    painter->setFont(m_descriptionFont);

    painter->setPen(KEY_TEXT_COLOR);

    int prevKeyPixel = -10000;
    const int kDefaultWidthForDescription = 200;
    const int kSmallMargin = 10;

    AZStd::vector<CTrackViewKeyHandle> sortedKeys;
    sortedKeys.reserve(numKeys);
    for (int i = 0; i < numKeys; ++i)
    {
        sortedKeys.push_back(pTrack->GetKey(i));
    }
    AZStd::sort(sortedKeys.begin(), sortedKeys.end(), CompareKeyHandleByTime);

    // Draw keys.
    for (int i = 0; i < numKeys; ++i)
    {
        CTrackViewKeyHandle keyHandle = sortedKeys[i];

        const float time = keyHandle.GetTime();
        int x = TimeToClient(time);
        if (x - kSmallMargin > rect.right())
        {
            continue;
        }

        int x1 = x + kDefaultWidthForDescription;

        int nextKeyIndex = i + 1;

        // Skip over next keys that have the same time as the current key.
        // If they have the same time it means they are keys from sub tracks
        // in a compound track at the same time.
        while (nextKeyIndex < numKeys && AZ::IsClose(sortedKeys[nextKeyIndex].GetTime(), time, AZ::Constants::FloatEpsilon))
        {
            nextKeyIndex++;
        }

        if (nextKeyIndex < numKeys)
        {
            CTrackViewKeyHandle nextKey2 = sortedKeys[nextKeyIndex];
            x1 = TimeToClient(nextKey2.GetTime()) - kSmallMargin;
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
                bool bSelectedAndBeingMoved = m_mouseMode == eTVMouseMode_Move && keyHandle.IsSelected();
                if (bSelectedAndBeingMoved)
                {
                    // Show its time or frame number additionally.
                    if (GetTickDisplayMode() == eTVTickMode_InSeconds)
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
void CTrackViewDopeSheetBase::DrawClipboardKeys(QPainter* painter, [[maybe_unused]] const QRect& rc)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    const float timeOffset = ComputeSnappedMoveOffset();

    // Get node & track under cursor
    CTrackViewAnimNode* animNode = GetAnimNodeFromPoint(m_mouseOverPos);
    CTrackViewTrack* pTrack = GetTrackFromPoint(m_mouseOverPos);

    auto matchedLocations = pSequence->GetMatchedPasteLocations(m_clipboardKeys, animNode, pTrack);

    for (size_t i = 0; i < matchedLocations.size(); ++i)
    {
        auto& matchedLocation = matchedLocations[i];
        CTrackViewTrack* pMatchedTrack = matchedLocation.first;
        XmlNodeRef trackNode = matchedLocation.second;

        if (pMatchedTrack->IsCompoundTrack())
        {
            // Both child counts should be the same, but make sure
            const unsigned int numSubTrack = std::min(pMatchedTrack->GetChildCount(), (unsigned int)trackNode->getChildCount());

            for (unsigned int subTrackIndex = 0; subTrackIndex < numSubTrack; ++subTrackIndex)
            {
                CTrackViewTrack* pSubTrack = static_cast<CTrackViewTrack*>(pMatchedTrack->GetChild(subTrackIndex));
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
void CTrackViewDopeSheetBase::DrawTrackClipboardKeys(QPainter* painter, CTrackViewTrack* pTrack, XmlNodeRef trackNode, const float timeOffset)
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
CTrackViewKeyHandle CTrackViewDopeSheetBase::FirstKeyFromPoint(const QPoint& point)
{
    CTrackViewTrack* pTrack = GetTrackFromPoint(point);
    if (!pTrack)
    {
        return CTrackViewKeyHandle();
    }

    float t1 = TimeFromPointUnsnapped(QPoint(point.x() - 4, point.y()));
    float t2 = TimeFromPointUnsnapped(QPoint(point.x() + 4, point.y()));

    int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

        float time = keyHandle.GetTime();
        if (time >= t1 && time <= t2)
        {
            return keyHandle;
        }
    }

    return CTrackViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyHandle CTrackViewDopeSheetBase::DurationKeyFromPoint(const QPoint& point)
{
    CTrackViewTrack* pTrack = GetTrackFromPoint(point);
    if (!pTrack)
    {
        return CTrackViewKeyHandle();
    }

    float t = TimeFromPointUnsnapped(point);

    int numKeys = pTrack->GetKeyCount();
    // Iterate in a reverse order to prioritize later nodes.
    for (int i = numKeys - 1; i >= 0; --i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

        const float time = keyHandle.GetTime();
        const float duration = keyHandle.GetDuration();

        if (t >= time && t <= time + duration)
        {
            return keyHandle;
        }
    }

    return CTrackViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyHandle CTrackViewDopeSheetBase::CheckCursorOnStartEndTimeAdjustBar(const QPoint& point, bool& bStart)
{
    CTrackViewTrack* pTrack = GetTrackFromPoint(point);

    if (!pTrack || (pTrack->GetParameterType() != AnimParamType::Animation &&
                    pTrack->GetParameterType() != AnimParamType::TimeRanges && 
                    pTrack->GetValueType() != AnimValueType::CharacterAnim &&
                    pTrack->GetValueType() != AnimValueType::AssetBlend))
    {
        return CTrackViewKeyHandle();
    }

    int numKeys = pTrack->GetKeyCount();
    for (int i = 0; i < numKeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

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

        // TODO: Refactor this Time Range Key stuff.
        ICharacterKey characterKey;
        AZ::IAssetBlendKey assetBlendKey;
        ITimeRangeKey* timeRangeKey = nullptr;

        if (pTrack->GetValueType() == AnimValueType::AssetBlend)
        {
            keyHandle.GetKey(&assetBlendKey);
            timeRangeKey = &assetBlendKey;
        }
        else
        {
            // This will work for both character & time range keys because
            // ICharacterKey is derived from ITimeRangeKey. Not the most beautiful code.
            keyHandle.GetKey(&characterKey);
            timeRangeKey = &characterKey;
        }

        int stime = TimeToClient(time);
        int etime = TimeToClient(time + AZStd::min(timeRangeKey->GetValidEndTime(), timeRangeKey->m_duration));

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

    return CTrackViewKeyHandle();
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheetBase::NumKeysFromPoint(const QPoint& point)
{
    CTrackViewTrack* pTrack = GetTrackFromPoint(point);
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
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

        const float time = keyHandle.GetTime();
        if (time >= t1 && time <= t2)
        {
            ++count;
        }
    }
    return count;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SelectKeys(const QRect& rc, const bool bMultiSelection)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    AZ_Assert(sequence != nullptr, "sequence should never be nullptr here");

    if (!sequence)
    {
        return;
    }

    AzToolsFramework::ScopedUndoBatch undoBatch("Select Keys");

    const std::vector<bool> beforeKeyState = sequence->SaveKeyStates();

    CTrackViewSequenceNotificationContext context(sequence);
    if (!bMultiSelection)
    {
        sequence->DeselectAllKeys();
    }

    // put selection rectangle from client to track space.
    const QRect rci = rc.translated(m_scrollOffset);

    Range selTime = GetTimeRange(rci);

    CTrackViewTrackBundle tracks = sequence->GetAllTracks();

    // note the tracks to select for the keyHandles selected
    CTrackViewTrackBundle tracksToSelect;

    for (unsigned int i = 0; i < tracks.GetCount(); ++i)
    {
        CTrackViewTrack* pTrack = tracks.GetTrack(i);

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
                CTrackViewKeyHandle keyHandle = pTrack->GetKey(j);

                const float time = keyHandle.GetTime();
                if (selTime.IsInside(time))
                {
                    keyHandle.Select(true);
                    tracksToSelect.AppendTrack(pTrack);
                }
            }
        }
    }

    ChangeSequenceTrackSelection(sequence, tracksToSelect, bMultiSelection);

    const std::vector<bool> afterKeyState = sequence->SaveKeyStates();

    if (beforeKeyState != afterKeyState)
    {
        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetTickDisplayMode(ETVTickMode mode)
{
    m_tickDisplayMode = mode;
    SetTimeScale(GetTimeScale(), 0); // for refresh
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::SetSnapFPS(UINT fps)
{
    m_snapFrameTime = (fps == 0) ? 0.033333f : (1.0f / float(fps));
}

//////////////////////////////////////////////////////////////////////////
ESnappingMode CTrackViewDopeSheetBase::GetKeyModifiedSnappingMode()
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
void CTrackViewDopeSheetBase::DrawSelectedKeyIndicators(QPainter* painter)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    const QPen prevPen = painter->pen();
    painter->setPen(Qt::green);

    CTrackViewKeyBundle keys = pSequence->GetSelectedKeys();
    for (unsigned int i = 0; i < keys.GetKeyCount(); ++i)
    {
        const CTrackViewKeyHandle& keyHandle = keys.GetKey(i);
        int x = TimeToClient(keyHandle.GetTime());
        painter->drawLine(x, m_rcClient.top(), x, m_rcClient.bottom());
    }

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::ComputeFrameSteps(const Range& visRange)
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

    if (TimeToClient(static_cast<float>(m_fFrameLabelStep)) - TimeToClient(0.0f) > 1300)
    {
        nBIntermediateTicks = 10;
    }

    m_fFrameTickStep = m_fFrameLabelStep * double (m_snapFrameTime) / double(nBIntermediateTicks);
}


void CTrackViewDopeSheetBase::DrawTimeLineInFrames(QPainter* painter, const QRect& rc, [[maybe_unused]] const QColor& lineCol, const QColor& textCol, [[maybe_unused]] double step)
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


void CTrackViewDopeSheetBase::DrawTimeLineInSeconds(QPainter* painter, const QRect& rc, [[maybe_unused]] const QColor& lineCol, const QColor& textCol, double step)
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
void CTrackViewDopeSheetBase::DrawTimeline(QPainter* painter, const QRect& rcUpdate)
{
    bool recording = GetIEditor()->GetAnimation()->IsRecording();

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
    if (GetTickDisplayMode() == eTVTickMode_InFrames)
    {
        DrawTimeLineInFrames(painter, rc, lineCol, textCol, step);
    }
    else if (GetTickDisplayMode() == eTVTickMode_InSeconds)
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
void CTrackViewDopeSheetBase::DrawSummary(QPainter* painter, const QRect& rcUpdate)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

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
    Range timeRange = m_timeRange;

    // Draw a short thick line at each place where there is a key in any tracks.
    CTrackViewKeyBundle keys = pSequence->GetAllKeys();
    for (unsigned int i = 0; i < keys.GetKeyCount(); ++i)
    {
        const CTrackViewKeyHandle& keyHandle = keys.GetKey(i);
        int x = TimeToClient(keyHandle.GetTime());
        painter->drawLine(x, rc.bottom() - 2, x, rc.top() + 2);
    }

    painter->setPen(prevPen);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawNodeTrack(CTrackViewAnimNode* animNode, QPainter* painter, const QRect& trackRect)
{
    const QFont prevFont = painter->font();
    painter->setFont(m_descriptionFont);

    CTrackViewAnimNode* pDirectorNode = animNode->GetDirector();

    if (pDirectorNode->GetNodeType() != eTVNT_Sequence && !pDirectorNode->IsActiveDirector())
    {
        painter->setPen(INACTIVE_TEXT_COLOR);
    }
    else
    {
        painter->setPen(KEY_TEXT_COLOR);
    }

    const QRect textRect = trackRect.adjusted(4, 0, -4, 0);

    QString sAnimNodeName = QString::fromUtf8(animNode->GetName().c_str());
    const bool hasObsoleteTrack = animNode->HasObsoleteTrack();

    if (hasObsoleteTrack)
    {
        painter->setPen(QColor(245, 80, 70));
        sAnimNodeName += tr(": Some of the sub-tracks contains obsoleted TCB splines (marked in red), thus cannot be copied or pasted.");
    }

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, painter->fontMetrics().elidedText(sAnimNodeName, Qt::ElideRight, textRect.width()));

    painter->setFont(prevFont);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawGoToTrackArrow(CTrackViewTrack* pTrack, QPainter* painter, const QRect& rc)
{
    int numKeys = pTrack->GetKeyCount();
    const QColor colorLine(150, 150, 150);
    const QColor colorHeader(50, 50, 50);
    const int tickness = 2;
    const int halfMargin = (rc.height() - tickness) / 2;

    for (int i = 0; i < numKeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(i);

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
void CTrackViewDopeSheetBase::DrawKeyDuration(CTrackViewTrack* pTrack, QPainter* painter, const QRect& rc, int keyIndex)
{
    const CTrackViewKeyHandle& keyHandle = pTrack->GetKey(keyIndex);

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
    if (pTrack->GetParameterType() == AnimParamType::Sound)    // If it is a sound key
    {
        ISoundKey soundKey;
        keyHandle.GetKey(&soundKey);
        colorFrom.setRgbF(soundKey.customColor.x, soundKey.customColor.y, soundKey.customColor.z);
    }
    QLinearGradient gradient(x, rc.top() + 3, x, rc.bottom() - 3);
    gradient.setColorAt(0, colorFrom);
    gradient.setColorAt(1, QColor(250, 250, 250));
    const int width = x1 + 1 - x;
    painter->fillRect(QRect(x, rc.top() + 3, width, rc.height() - 3), gradient);

    painter->setBrush(prevBrush);
    painter->drawLine(x1, rc.top(), x1, rc.bottom());

    bool typeHasAnimBox = pTrack->GetParameterType() == AnimParamType::Animation ||
        pTrack->GetParameterType() == AnimParamType::TimeRanges ||
        pTrack->GetValueType() == AnimValueType::CharacterAnim ||
        pTrack->GetValueType() == AnimValueType::AssetBlend;

    // If it is a selected animation track, draw the whole animation box (in green)
    // and two adjust bars (in red) for start/end time each, too.
    if (keyHandle.IsSelected() && typeHasAnimBox)
    {
        // Draw the whole animation box.

        // TODO: Refactor this Time Range Key stuff.
        ICharacterKey characterKey;
        AZ::IAssetBlendKey assetBlendKey;
        ITimeRangeKey* timeRangeKey = nullptr;

        if (pTrack->GetValueType() == AnimValueType::AssetBlend)
        {
            keyHandle.GetKey(&assetBlendKey);
            timeRangeKey = &assetBlendKey;
        }
        else
        {
            // This will work for both character & time range keys because
            // ICharacterKey is derived from ITimeRangeKey. Not the most beautiful code.
            keyHandle.GetKey(&characterKey);
            timeRangeKey = &characterKey;
        }

        int startX = TimeToClient(time - timeRangeKey->m_startTime / timeRangeKey->m_speed);
        int endX = TimeToClient(time + (timeRangeKey->m_duration - timeRangeKey->m_startTime) / timeRangeKey->m_speed);
        const QPen prevPen = painter->pen();
        painter->setPen(Qt::green);
        painter->drawLine(startX, rc.top(), endX, rc.top());
        painter->drawLine(endX, rc.top(), endX, rc.bottom());
        painter->drawLine(endX, rc.bottom(), startX, rc.bottom());
        painter->drawLine(startX, rc.bottom(), startX, rc.top());

        // Draw two adjust bars.
        int durationX = TimeToClient(time + AZStd::min(timeRangeKey->GetValidEndTime(), timeRangeKey->m_duration));
        painter->setPen(QPen(Qt::red, 3));
        painter->drawLine(x - 2, rc.top(), x - 2, rc.bottom());
        painter->drawLine(durationX + 2, rc.top(), durationX + 2, rc.bottom());
        painter->setPen(prevPen);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::DrawColorGradient(QPainter* painter, const QRect& rc, const CTrackViewTrack* pTrack)
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
QRect CTrackViewDopeSheetBase::GetNodeRect(const CTrackViewNode* pNode) const
{
    CTrackViewNodesCtrl::CRecord* pRecord = m_pNodesCtrl->GetNodeRecord(pNode);

    if (pRecord && pRecord->IsVisible())
    {
        QRect recordRect = pRecord->GetRect();
        return QRect(0, recordRect.top(), m_rcClient.width(), recordRect.height());
    }

    return QRect();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheetBase::StoreMementoForTracksWithSelectedKeys()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    CTrackViewKeyBundle selectedKeys = pSequence->GetSelectedKeys();

    m_trackMementos.clear();

    // Construct the set of tracks that have selected keys
    std::set<CTrackViewTrack*> tracks;

    const unsigned int numKeys = selectedKeys.GetKeyCount();
    for (unsigned int keyIndex = 0; keyIndex < numKeys; ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
        tracks.insert(keyHandle.GetTrack());
    }

    // For each of those tracks store an undo object
    for (auto iter = tracks.begin(); iter != tracks.end(); ++iter)
    {
        CTrackViewTrack* pTrack = *iter;

        TrackMemento trackMemento;
        trackMemento.m_memento = pTrack->GetMemento();

        const unsigned int numKeys2 = pTrack->GetKeyCount();
        for (unsigned int i = 0; i < numKeys2; ++i)
        {
            trackMemento.m_keySelectionStates.push_back(pTrack->GetKey(i).IsSelected());
        }

        m_trackMementos[pTrack] = trackMemento;
    }
}
