/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
#include "AnimationContext.h"
#include "TrackViewNode.h"
#include "TrackViewSequence.h"

#include <QWidget>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

class CTVTrackPropsDialog;
class CTrackViewTrack;
class CTrackViewNodesCtrl;
class CTrackViewNode;
class CTrackViewKeyPropertiesDlg;
class CTrackViewAnimNode;

class QRubberBand;
class QScrollBar;
class ReflectedPropertyControl;

namespace AZ
{
    class Color;
}

enum ETVActionMode
{
    eTVActionMode_MoveKey = 1,
    eTVActionMode_AddKeys,
    eTVActionMode_SlideKey,
    eTVActionMode_ScaleKey,
};

enum ESnappingMode
{
    eSnappingMode_SnapNone = 0,
    eSnappingMode_SnapTick,
    eSnappingMode_SnapMagnet,
    eSnappingMode_SnapFrame,
};

enum ETVTickMode
{
    eTVTickMode_InSeconds = 0,
    eTVTickMode_InFrames,
};

/** TrackView DopeSheet interface
*/
class CTrackViewDopeSheetBase
    : public QWidget
    , public IAnimationContextListener
    , public ITrackViewSequenceListener
{
public:
    CTrackViewDopeSheetBase(QWidget* parent = 0);
    virtual ~CTrackViewDopeSheetBase();

    void SetNodesCtrl(CTrackViewNodesCtrl* pNodesCtrl) { m_pNodesCtrl = pNodesCtrl; }

    void SetTimeScale(float timeScale, float fAnchorTime);
    float GetTimeScale() const { return m_timeScale; }

    void SetScrollOffset(int hpos);

    int GetScrollPos() const;

    void SetTimeRange(float start, float end);
    void SetStartMarker(float fTime);
    void SetEndMarker(float fTime);

    void SetMouseActionMode(ETVActionMode mode);

    void SetKeyPropertiesDlg(CTrackViewKeyPropertiesDlg* dlg) { m_keyPropertiesDlg = dlg; }

    void SetSnappingMode(ESnappingMode mode) { m_snappingMode = mode; }
    ESnappingMode GetSnappingMode() const { return m_snappingMode; }
    void SetSnapFPS(UINT fps);

    ETVTickMode GetTickDisplayMode() const { return m_tickDisplayMode; }
    void SetTickDisplayMode(ETVTickMode mode);

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    // IAnimationContextListener
    void OnTimeChanged(float newTime) override;

    float TickSnap(float time) const;

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void OnHScroll();
    void OnLButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnLButtonDblClk(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnRButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnLButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnMButtonDown(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnMButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    void OnRButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnCaptureChanged();

private slots:
    void OnCurrentColorChange(const AZ::Color& color);

private:
    void UpdateColorKey(const QColor& color, bool addToUndo);
    void UpdateColorKeyHelper(const ColorF& color);
    void AddKeys(const QPoint& point, const bool bTryAddKeysInGroup);

    void ShowKeyPropertyCtrlOnSpot(int x, int y, bool bMultipleKeysSelected, bool bKeyChangeInSameTrack);
    void HideKeyPropertyCtrlOnSpot();

    // Utility functions to change the selected track(s) in the given track(s)
    void ChangeSequenceTrackSelection(CTrackViewSequence* sequence, CTrackViewTrack* trackToSelect) const;
    void ChangeSequenceTrackSelection(CTrackViewSequence* sequence, CTrackViewTrackBundle trackBundle, bool multiTrackSelection) const;

    XmlNodeRef GetKeysInClickboard();
    void StartPasteKeys();

    CTrackViewKeyHandle FirstKeyFromPoint(const QPoint& point);
    CTrackViewKeyHandle DurationKeyFromPoint(const QPoint& point);
    CTrackViewKeyHandle CheckCursorOnStartEndTimeAdjustBar(const QPoint& point, bool& bStart);

    void SelectKeys(const QRect& rc, const bool bMultiSelection);

    int NumKeysFromPoint(const QPoint& point);

    //! Select all keys within time frame defined by this client rectangle.
    void SelectAllKeysWithinTimeFrame(const QRect& rc, const bool bMultiSelection);

    //! Return time snapped to time step,
    double GetTickTime() const;
    float MagnetSnap(float time, const CTrackViewAnimNode* pNode) const;
    float FrameSnap(float time) const;

    //! Return move time offset snapped with current snap settings
    float ComputeSnappedMoveOffset();

    //! Returns visible time range.
    Range GetVisibleRange() const;
    Range GetTimeRange(const QRect& rc) const;

    void SetHorizontalExtent(int min, int max);

    void SetCurrTime(float time);

    //! Return client position for given time.
    int TimeToClient(float time) const;

    float TimeFromPoint(const QPoint& point) const;
    float TimeFromPointUnsnapped(const QPoint& point) const;

    void SetLeftOffset(int ofs) { m_leftOffset = ofs; };

    void SetMouseCursor(const QCursor& cursor);

    void ShowKeyTooltip(CTrackViewKeyHandle& keyHandle, const QPoint& point);

    bool IsOkToAddKeyHere(const CTrackViewTrack* pTrack, float time) const;

    void MouseMoveSelect(const QPoint& point);
    void MouseMoveMove(const QPoint& point, Qt::KeyboardModifiers modifiers);

    void MouseMoveDragTime(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveOver(const QPoint& point);
    void MouseMoveDragEndMarker(const QPoint& point, Qt::KeyboardModifiers modifiers);

    void CancelDrag();

    void MouseMoveDragStartMarker(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveStartEndTimeAdjust(const QPoint& point, bool bStart);

    CTrackViewNode* GetNodeFromPointRec(CTrackViewNode* pCurrentNode, const QPoint& point);
    CTrackViewNode* GetNodeFromPoint(const QPoint& point);
    CTrackViewAnimNode* GetAnimNodeFromPoint(const QPoint& point);
    CTrackViewTrack* GetTrackFromPoint(const QPoint& point);

    void LButtonDownOnTimeAdjustBar(const QPoint& point, CTrackViewKeyHandle& keyHandle, bool bStart);
    void LButtonDownOnKey(const QPoint& point, CTrackViewKeyHandle& keyHandle, Qt::KeyboardModifiers modifiers);

    bool CreateColorKey(CTrackViewTrack* pTrack, float keyTime);
    void EditSelectedColorKey(CTrackViewTrack* pTrack);

    void AcceptUndo();

    // Returns the snapping mode modified active keys
    ESnappingMode GetKeyModifiedSnappingMode();

    QRect GetNodeRect(const CTrackViewNode* pNode) const;

    void StoreMementoForTracksWithSelectedKeys();

    // Drawing methods.
    void DrawControl(QPainter* pDC, const QRect& rcUpdate);
    void DrawNodesRecursive(CTrackViewNode* pNode, QPainter* pDC, const QRect& rcUpdate);
    void DrawTimeline(QPainter* pDC, const QRect& rcUpdate);
    void DrawSummary(QPainter* pDC, const QRect& rcUpdate);
    void DrawSelectedKeyIndicators(QPainter* pDC);
    void DrawTicks(QPainter* pDC, const QRect& rc, Range& timeRange);
    void DrawNodeTrack(CTrackViewAnimNode* pAnimNode, QPainter* pDC, const QRect& trackRect);
    void DrawTrack(CTrackViewTrack* pTrack, QPainter* pDC, const QRect& trackRect);
    void DrawKeys(CTrackViewTrack* pTrack, QPainter* pDC, QRect& rc, Range& timeRange);
    void DrawSequenceTrack(const Range& timeRange, QPainter* pDC, CTrackViewTrack* pTrack, const QRect& rc);
    void DrawBoolTrack(const Range& timeRange, QPainter* pDC, CTrackViewTrack* pTrack, const QRect& rc);
    void DrawSelectTrack(const Range& timeRange, QPainter* pDC, CTrackViewTrack* pTrack, const QRect& rc);
    void DrawKeyDuration(CTrackViewTrack* pTrack, QPainter* pDC, const QRect& rc, int keyIndex);
    void DrawGoToTrackArrow(CTrackViewTrack* pTrack, QPainter* pDC, const QRect& rc);
    void DrawColorGradient(QPainter* pDC, const QRect& rc, const CTrackViewTrack* pTrack);
    void DrawClipboardKeys(QPainter* pDC, const QRect& rc);
    void DrawTrackClipboardKeys(QPainter* pDC, CTrackViewTrack* pTrack, XmlNodeRef trackNode, const float timeOffset);

    CTrackViewNodesCtrl* m_pNodesCtrl;
    void ComputeFrameSteps(const Range& VisRange);
    void DrawTimeLineInFrames(QPainter* dc, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step);
    void DrawTimeLineInSeconds(QPainter* dc, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step);

    static bool CompareKeyHandleByTime(const CTrackViewKeyHandle &a, const CTrackViewKeyHandle &b);

    QBrush m_bkgrBrush;
    QBrush m_bkgrBrushEmpty;
    QBrush m_selectedBrush;
    QBrush m_timeBkgBrush;
    QBrush m_timeHighlightBrush;
    QBrush m_visibilityBrush;
    QBrush m_selectTrackBrush;

    QCursor m_currCursor;
    QCursor m_crsLeftRight;
    QCursor m_crsAddKey;
    QCursor m_crsCross;
    QCursor m_crsAdjustLR;

    QRect m_rcClient;
    QPoint m_scrollOffset;
    QRect m_rcSelect;
    QRect m_rcTimeline;
    QRect m_rcSummary;

    QPoint m_lastTooltipPos;
    QPoint m_mouseDownPos;
    QPoint m_mouseOverPos;

    QPixmap m_offscreenBitmap;

    QRubberBand* m_rubberBand;
    QScrollBar* m_scrollBar;

    // Time
    float m_timeScale;
    float m_currentTime;
    float m_storedTime;
    Range m_timeRange;
    Range m_timeMarked;

    // This is how often to place ticks.
    // value of 10 means place ticks every 10 second.
    double m_ticksStep;

    CTrackViewKeyPropertiesDlg* m_keyPropertiesDlg;
    ReflectedPropertyControl* m_wndPropsOnSpot;
    const CTrackViewTrack* m_pLastTrackSelectedOnSpot;

    QFont m_descriptionFont;

    // Mouse interaction state
    int m_mouseMode;
    int m_mouseActionMode;
    bool m_bZoomDrag;
    bool m_bMoveDrag;
    bool m_bCursorWasInKey;
    bool m_bJustSelected;
    bool m_bMouseMovedAfterRButtonDown;
    bool m_bKeysMoved;
    bool m_stashedRecordModeWhileTimeDragging;

    // Offset for keys while moving/pasting
    float m_keyTimeOffset;

    // If control is locked for editing
    bool m_bEditLock;

    // Fast redraw: Only redraw time slider. Everything else is buffered.
    bool m_bFastRedraw;

    // Scrolling
    int m_leftOffset;
    int m_scrollMin;
    int m_scrollMax;

    // Snapping
    ESnappingMode m_snappingMode;
    float m_snapFrameTime;

    // Ticks in frames or seconds
    ETVTickMode m_tickDisplayMode;
    double m_fFrameTickStep;
    double m_fFrameLabelStep;

    // Key for time adjust
    CTrackViewKeyHandle m_keyForTimeAdjust;

    // Cached clipboard XML for eTVMouseMode_Paste
    XmlNodeRef m_clipboardKeys;

    // Store current track whose color is being updated
    CTrackViewTrack* m_colorUpdateTrack;

    // Store the key time of that track
    float            m_colorUpdateKeyTime;

    // Mementos of unchanged tracks for Move/Scale/Slide etc.
    struct TrackMemento
    {
        CTrackViewTrackMemento m_memento;

        // Also need to store key selection states,
        // because RestoreMemento will destroy them
        AZStd::vector<bool> m_keySelectionStates;
    };

    AZStd::unordered_map<CTrackViewTrack*, TrackMemento> m_trackMementos;

#ifdef AZ_DEBUG_BUILD
    unsigned int m_redrawCount;
#endif
};
