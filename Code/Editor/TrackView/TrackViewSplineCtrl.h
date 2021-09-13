/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSPLINECTRL_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSPLINECTRL_H
#pragma once


#include <IMovieSystem.h>
#include "Controls/SplineCtrlEx.h"
#include <functional>

class CTrackViewTrack;

/** A customized spline control for CTrackViewGraph.
*/
class CTrackViewSplineCtrl
    : public SplineWidget
{
    friend class CUndoTrackViewSplineCtrl;
public:
    CTrackViewSplineCtrl(QWidget* parent);
    virtual ~CTrackViewSplineCtrl();

    void ClearSelection() override;

    void AddSpline(ISplineInterpolator* pSpline, CTrackViewTrack* pTrack, const QColor& color);
    void AddSpline(ISplineInterpolator * pSpline, CTrackViewTrack * pTrack, QColor anColorArray[4]);

    const std::vector<CTrackViewTrack*>& GetTracks() const { return m_tracks; }
    void RemoveAllSplines();

    void OnUserCommand(UINT cmd);
    bool IsUnifiedKeyCurrentlySelected() const;
    bool IsKeysFrozen() const { return m_bKeysFreeze; }
    bool IsTangentsFrozen() const { return m_bTangentsFreeze; }

    void SetPlayCallback(const std::function<void()>& callback);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect) override;
    void SelectRectangle(const QRect& rc, bool bSelect) override;

    std::vector<CTrackViewTrack*> m_tracks;

    bool GetTangentHandlePts(QPoint& inTangentPt, QPoint& pt, QPoint& outTangentPt,
        int nSpline, int nKey, int nDimension) override;
    void ComputeIncomingTangentAndEaseTo(float& ds, float& easeTo, QPoint inTangentPt,
        int nSpline, int nKey, int nDimension);
    void ComputeOutgoingTangentAndEaseFrom(float& dd, float& easeFrom, QPoint outTangentPt,
        int nSpline, int nKey, int nDimension);
    void AdjustTCB(float d_tension, float d_continuity, float d_bias);
    void MoveSelectedTangentHandleTo(const QPoint& point);

    ISplineCtrlUndo* CreateSplineCtrlUndoObject(std::vector<ISplineInterpolator*>& splineContainer) override;

    bool m_bKeysFreeze;
    bool m_bTangentsFreeze;
    bool m_stashedRecordModeWhenDraggingTime;

    std::function<void()> m_playCallback;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSPLINECTRL_H
