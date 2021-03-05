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

    virtual void ClearSelection();

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
    virtual void SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect) override;
    virtual void SelectRectangle(const QRect& rc, bool bSelect) override;

    std::vector<CTrackViewTrack*> m_tracks;

    virtual bool GetTangentHandlePts(QPoint& inTangentPt, QPoint& pt, QPoint& outTangentPt,
        int nSpline, int nKey, int nDimension) override;
    void ComputeIncomingTangentAndEaseTo(float& ds, float& easeTo, QPoint inTangentPt,
        int nSpline, int nKey, int nDimension);
    void ComputeOutgoingTangentAndEaseFrom(float& dd, float& easeFrom, QPoint outTangentPt,
        int nSpline, int nKey, int nDimension);
    void AdjustTCB(float d_tension, float d_continuity, float d_bias);
    void MoveSelectedTangentHandleTo(const QPoint& point);

    virtual ISplineCtrlUndo* CreateSplineCtrlUndoObject(std::vector<ISplineInterpolator*>& splineContainer);

    bool m_bKeysFreeze;
    bool m_bTangentsFreeze;
    bool m_stashedRecordModeWhenDraggingTime;

    std::function<void()> m_playCallback;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWSPLINECTRL_H
