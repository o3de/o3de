/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Controls/TimelineCtrl.h"
#include "TrackViewDopeSheetBase.h"
#include "TrackViewSplineCtrl.h"
#include "TrackViewTimeline.h"

#include <QWidget>

#include <AzCore/std/functional.h>

namespace Ui
{
    class TrackViewCurveEditor;
}

/** CTrackViewGraph dialog.
        Placed at the same position as tracks dialog, and display spline graphs of track.
*/
class CTrackViewCurveEditor
    : public QWidget
    , public IAnimationContextListener
    , public IEditorNotifyListener
    , public ITrackViewSequenceListener
{
    friend class TrackViewCurveEditorDialog;
public:
    CTrackViewCurveEditor(QWidget* parent);
    virtual ~CTrackViewCurveEditor();

    void SetEditLock(bool bLock);

    void SetFPS(float fps);
    float GetFPS() const;
    void SetTickDisplayMode(ETVTickMode mode);

    CTrackViewSplineCtrl& GetSplineCtrl();
    void ResetSplineCtrlZoomLevel();

    void SetPlayCallback(const std::function<void()>& callback);

    // IAnimationContextListener
    void OnSequenceChanged(CTrackViewSequence* pNewSequence) override;
    void OnTimeChanged(float newTime) override;

protected:
    void showEvent(QShowEvent* event) override;

private:

    void OnSplineChange();
    void OnSplineCmd(UINT cmd);
    void OnSplineCmdUpdateUI();
    void OnTimelineChange();
    void OnSplineTimeMarkerChange();

    // IEditorNotifyListener
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    //ITrackViewSequenceListener
    void OnKeysChanged(CTrackViewSequence* pSequence) override;
    void OnKeyAdded(CTrackViewKeyHandle& addedKeyHandle) override;
    void OnKeySelectionChanged(CTrackViewSequence* pSequence) override;
    void OnNodeChanged(CTrackViewNode* pNode, ENodeChangeType type) override;
    void OnNodeSelectionChanged(CTrackViewSequence* pSequence) override;
    void OnSequenceSettingsChanged(CTrackViewSequence* pSequence) override;
    //~ITrackViewSequenceListener

    void UpdateSplines();
    void UpdateTimeRange(CTrackViewSequence* pSequence);

    void AddSpline(CTrackViewTrack* pTrack);

    TrackView::CTrackViewTimelineWidget m_timelineCtrl;

    bool m_bIgnoreSelfEvents;

    bool m_bLevelClosing;

    QScopedPointer<Ui::TrackViewCurveEditor> m_ui;
};

class TrackViewCurveEditorDialog
    : public QWidget
    , public ITrackViewSequenceListener
    , public IAnimationContextListener
{
public:
    TrackViewCurveEditorDialog(QWidget* parent);
    virtual ~TrackViewCurveEditorDialog() {}

    void SetPlayCallback(const std::function<void()>& callback) { m_widget->SetPlayCallback(callback); }

    void SetEditLock(bool bLock) { m_widget->SetEditLock(bLock); }

    CTrackViewSplineCtrl& GetSplineCtrl(){ return m_widget->GetSplineCtrl(); }

    void SetFPS(float fps) { m_widget->SetFPS(fps); }
    float GetFPS() const { return m_widget->GetFPS(); }
    void SetTickDisplayMode(ETVTickMode mode) { m_widget->SetTickDisplayMode(mode); }

    void OnSequenceChanged(CTrackViewSequence* pNewSequence) override { m_widget->OnSequenceChanged(pNewSequence); }
    void OnTimeChanged(float newTime) override { m_widget->OnTimeChanged(newTime); }

    // ITrackViewSequenceListener delegation to m_widget
    void OnKeysChanged(CTrackViewSequence* pSequence) override { m_widget->OnKeysChanged(pSequence); }
    void OnKeyAdded(CTrackViewKeyHandle& addedKeyHandle) override { m_widget->OnKeyAdded(addedKeyHandle); }
    void OnKeySelectionChanged(CTrackViewSequence* pSequence) override { m_widget->OnKeySelectionChanged(pSequence); }
    void OnNodeChanged(CTrackViewNode* pNode, ENodeChangeType type) override { m_widget->OnNodeChanged(pNode, type); }
    void OnNodeSelectionChanged(CTrackViewSequence* pSequence) override { m_widget->OnNodeSelectionChanged(pSequence); }
    void OnSequenceSettingsChanged(CTrackViewSequence* pSequence) override { m_widget->OnSequenceSettingsChanged(pSequence); }
    //~ITrackViewSequenceListener

private:
    CTrackViewCurveEditor* m_widget;
};
