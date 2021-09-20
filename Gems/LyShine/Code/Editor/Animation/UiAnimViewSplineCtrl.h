/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include <LyShine/Animation/IUiAnimation.h>
#include "Controls/UiSplineCtrlEx.h"
#include <functional>

class CUiAnimViewTrack;

/** A customized spline control for CUiAnimViewGraph.
*/
class CUiAnimViewSplineCtrl
    : public SplineWidget
{
    friend class CUndoUiAnimViewSplineCtrl;
public:
    CUiAnimViewSplineCtrl(QWidget* parent);
    virtual ~CUiAnimViewSplineCtrl();

    void ClearSelection() override;

    void AddSpline(ISplineInterpolator* pSpline, CUiAnimViewTrack* pTrack, const QColor& color);
    void AddSpline(ISplineInterpolator * pSpline, CUiAnimViewTrack * pTrack, QColor anColorArray[4]);

    const std::vector<CUiAnimViewTrack*>& GetTracks() const { return m_tracks; }
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
    void wheelEvent(QWheelEvent* event) override;

private:
    virtual void SelectKey(ISplineInterpolator* pSpline, int nKey, int nDimension, bool bSelect) override;
    virtual void SelectRectangle(const QRect& rc, bool bSelect) override;

    std::vector<CUiAnimViewTrack*> m_tracks;

    virtual bool GetTangentHandlePts(QPoint& inTangentPt, QPoint& pt, QPoint& outTangentPt,
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

    std::function<void()> m_playCallback;
};
