/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include <LyShine/Animation/IUiAnimation.h>
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>
#include "UiAnimViewNode.h"
#include "UiAnimViewSequence.h"
#include "AnimationContext.h"

class CUiAVTrackPropsDialog;
class CUiAnimViewNodesCtrl;
class CUiAnimViewKeyPropertiesDlg;
class CUiAnimViewNode;
class CUiAnimViewTrack;
class CUiAnimViewAnimNode;

class QRubberBand;
class QScrollBar;

enum EUiAVActionMode
{
    eUiAVActionMode_MoveKey = 1,
    eUiAVActionMode_AddKeys,
    eUiAVActionMode_SlideKey,
    eUiAVActionMode_ScaleKey,
};

enum ESnappingMode
{
    eSnappingMode_SnapNone = 0,
    eSnappingMode_SnapTick,
    eSnappingMode_SnapMagnet,
    eSnappingMode_SnapFrame,
};

enum EUiAVTickMode
{
    eUiAVTickMode_InSeconds = 0,
    eUiAVTickMode_InFrames,
};

/** UiAnimView DopeSheet interface
*/
class CUiAnimViewDopeSheetBase
    : public QWidget
    , public IUiAnimationContextListener
    , public IUiAnimViewSequenceListener
{
public:
    CUiAnimViewDopeSheetBase(QWidget* parent = 0);
    virtual ~CUiAnimViewDopeSheetBase();

    void SetNodesCtrl(CUiAnimViewNodesCtrl* pNodesCtrl) { m_pNodesCtrl = pNodesCtrl; }

    void SetTimeScale(float timeScale, float fAnchorTime);
    float GetTimeScale() { return m_timeScale; }

    void SetScrollOffset(int hpos);
    int GetScrollOffset();

    int GetScrollPos() const;

    void SetTimeRange(float start, float end);
    void SetStartMarker(float fTime);
    void SetEndMarker(float fTime);

    void SetMouseActionMode(EUiAVActionMode mode);

    void SetKeyPropertiesDlg(CUiAnimViewKeyPropertiesDlg* dlg) { m_keyPropertiesDlg = dlg; }

    void SetSnappingMode(ESnappingMode mode) { m_snappingMode = mode; }
    ESnappingMode GetSnappingMode() const { return m_snappingMode; }
    void SetSnapFPS(UINT fps);

    EUiAVTickMode GetTickDisplayMode() const { return m_tickDisplayMode; }
    void SetTickDisplayMode(EUiAVTickMode mode);

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    // IUiAnimationContextListener
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
    void OnRButtonUp(Qt::KeyboardModifiers modifiers, const QPoint& point);
    void OnCaptureChanged();

private:
    void AddKeys(const QPoint& point, const bool bTryAddKeysInGroup);

    XmlNodeRef GetKeysInClickboard();
    void StartPasteKeys();

    CUiAnimViewKeyHandle FirstKeyFromPoint(const QPoint& point);
    CUiAnimViewKeyHandle DurationKeyFromPoint(const QPoint& point);
    CUiAnimViewKeyHandle CheckCursorOnStartEndTimeAdjustBar(const QPoint& point, bool& bStart);

    void SelectKeys(const QRect& rc, const bool bMultiSelection);

    int NumKeysFromPoint(const QPoint& point);

    //! Select all keys within time frame defined by this client rectangle.
    void SelectAllKeysWithinTimeFrame(const QRect& rc, const bool bMultiSelection);

    //! Return time snapped to time step,
    double GetTickTime() const;
    float MagnetSnap(float time, const CUiAnimViewAnimNode* pNode) const;
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

    void ShowKeyTooltip(const CUiAnimViewKeyHandle& keyHandle, const QPoint& point);

    bool IsOkToAddKeyHere(const CUiAnimViewTrack* pTrack, float time) const;

    void MouseMoveSelect(const QPoint& point);
    void MouseMoveMove(const QPoint& point, Qt::KeyboardModifiers modifiers);

    void MouseMoveDragTime(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveOver(const QPoint& point);
    void MouseMoveDragEndMarker(const QPoint& point, Qt::KeyboardModifiers modifiers);

    float SnapTime(Qt::KeyboardModifiers modifiers, const QPoint& p);

    void MouseMoveDragStartMarker(const QPoint& point, Qt::KeyboardModifiers modifiers);
    void MouseMoveStartEndTimeAdjust(const QPoint& point, bool bStart);

    CUiAnimViewNode* GetNodeFromPointRec(CUiAnimViewNode* pCurrentNode, const QPoint& point);
    CUiAnimViewNode* GetNodeFromPoint(const QPoint& point);
    CUiAnimViewAnimNode* GetAnimNodeFromPoint(const QPoint& point);
    CUiAnimViewTrack* GetTrackFromPoint(const QPoint& point);

    void LButtonDownOnTimeAdjustBar(const QPoint& point, CUiAnimViewKeyHandle& keyHandle, bool bStart);
    void LButtonDownOnKey(const QPoint& point, CUiAnimViewKeyHandle& keyHandle, Qt::KeyboardModifiers modifiers);

    bool CreateColorKey(CUiAnimViewTrack* pTrack, float keyTime);

    void RecordTrackUndo(CUiAnimViewTrack* pTrack);
    void AcceptUndo();

    // Returns the snapping mode modified active keys
    ESnappingMode GetKeyModifiedSnappingMode();

    QRect GetNodeRect(const CUiAnimViewNode* pNode) const;

    void StoreMementoForTracksWithSelectedKeys();

    // Drawing methods.
    void DrawControl(QPainter* pDC, const QRect& rcUpdate);
    void DrawNodesRecursive(CUiAnimViewNode* pNode, QPainter* pDC, const QRect& rcUpdate);
    void DrawTimeline(QPainter* pDC, const QRect& rcUpdate);
    void DrawSummary(QPainter* pDC, const QRect& rcUpdate);
    void DrawSelectedKeyIndicators(QPainter* pDC);
    void DrawTicks(QPainter* pDC, const QRect& rc, Range& timeRange);
    void DrawNodeTrack(CUiAnimViewAnimNode* pAnimNode, QPainter* pDC, const QRect& trackRect);
    void DrawTrack(CUiAnimViewTrack* pTrack, QPainter* pDC, const QRect& trackRect);
    void DrawKeys(CUiAnimViewTrack* pTrack, QPainter* pDC, QRect& rc, Range& timeRange);
    void DrawBoolTrack(const Range& timeRange, QPainter* pDC, CUiAnimViewTrack* pTrack, const QRect& rc);
    void DrawSelectTrack(const Range& timeRange, QPainter* pDC, CUiAnimViewTrack* pTrack, const QRect& rc);
    void DrawKeyDuration(CUiAnimViewTrack* pTrack, QPainter* pDC, const QRect& rc, int keyIndex);
    void DrawGoToTrackArrow(CUiAnimViewTrack* pTrack, QPainter* pDC, const QRect& rc);
    void DrawColorGradient(QPainter* pDC, const QRect& rc, const CUiAnimViewTrack* pTrack);
    void DrawClipboardKeys(QPainter* pDC, const QRect& rc);
    void DrawTrackClipboardKeys(QPainter* pDC, CUiAnimViewTrack* pTrack, XmlNodeRef trackNode, const float timeOffset);

    CUiAnimViewNodesCtrl* m_pNodesCtrl;
    void ComputeFrameSteps(const Range& VisRange);
    void DrawTimeLineInFrames(QPainter* dc, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step);
    void DrawTimeLineInSeconds(QPainter* dc, const QRect& rc, const QColor& lineCol, const QColor& textCol, double step);

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

    CUiAnimViewKeyPropertiesDlg* m_keyPropertiesDlg;
#if UI_ANIMATION_REMOVED    // UI_ANIMATION_REVISIT - not sure what PropsOnSpot is for
    ReflectedPropertyControl m_wndPropsOnSpot;
#endif
    const CUiAnimViewTrack* m_pLastTrackSelectedOnSpot;

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
    EUiAVTickMode m_tickDisplayMode;
    double m_fFrameTickStep;
    double m_fFrameLabelStep;

    // Key for time adjust
    CUiAnimViewKeyHandle m_keyForTimeAdjust;

    // Cached clipboard XML for eUiAVMouseMode_Paste
    XmlNodeRef m_clipboardKeys;

    // Mementos of unchanged tracks for Move/Scale/Slide etc.
    struct TrackMemento
    {
        CUiAnimViewTrackMemento m_memento;

        // Also need to store key selection states,
        // because RestoreMemento will destroy them
        std::vector<bool> m_keySelectionStates;
    };

    std::unordered_map<CUiAnimViewTrack*, TrackMemento> m_trackMementos;

#ifdef DEBUG
    unsigned int m_redrawCount;
#endif
};
