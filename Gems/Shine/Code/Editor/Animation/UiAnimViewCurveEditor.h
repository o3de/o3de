/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#include "UiAnimViewDopeSheetBase.h"
#include "UiAnimViewSplineCtrl.h"
#include "Animation/Controls/UiTimelineCtrl.h"
#include "UiEditorAnimationBus.h"

#include <QWidget>

namespace Ui
{
    class UiAnimViewCurveEditor;
}

/** CUiAnimViewGraph dialog.
        Placed at the same position as tracks dialog, and display spline graphs of track.
*/
class CUiAnimViewCurveEditor
    : public QWidget
    , public IUiAnimationContextListener
    , public IEditorNotifyListener
    , public IUiAnimViewSequenceListener
    , public UiEditorAnimListenerBus::Handler
{
    friend class UiAnimViewCurveEditorDialog;
public:
    CUiAnimViewCurveEditor(QWidget* parent);
    virtual ~CUiAnimViewCurveEditor();

    void SetEditLock(bool bLock);

    void SetFPS(float fps);
    float GetFPS() const;
    void SetTickDisplayMode(EUiAVTickMode mode);

    CUiAnimViewSplineCtrl& GetSplineCtrl();
    void ResetSplineCtrlZoomLevel();

    void SetPlayCallback(const std::function<void()>& callback);

    // IUiAnimationContextListener
    void OnSequenceChanged(CUiAnimViewSequence* pNewSequence) override;
    void OnTimeChanged(float newTime) override;

protected:
    void showEvent(QShowEvent* event) override;

private:

    void OnSplineChange();
    void OnSplineCmd(UINT cmd);
    void OnSplineCmdUpdateUI();
    void OnTimelineChange();
    void OnHorizontalScrollBarChange();
    void OnVerticalScrollBarChange();
    void OnSplineScrollZoom();

    // IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    //IUiAnimViewSequenceListener
    virtual void OnKeysChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnKeySelectionChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnNodeChanged(CUiAnimViewNode* pNode, ENodeChangeType type) override;
    virtual void OnNodeSelectionChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnSequenceSettingsChanged(CUiAnimViewSequence* pSequence) override;

    // UiEditorAnimListenerInterface
    void OnActiveCanvasChanged() override;
    // ~UiEditorAnimListenerInterface

    void UpdateSplines();
    void UpdateTimeRange(CUiAnimViewSequence* pSequence);

    void AddSpline(CUiAnimViewTrack* pTrack);

    void ResetScrollBarRange();

    TimelineWidget m_timelineCtrl;

    bool m_hasActiveCanvas;

    QScopedPointer<Ui::UiAnimViewCurveEditor> m_ui;
};

class UiAnimViewCurveEditorDialog
    : public QWidget
    , public IUiAnimViewSequenceListener
    , public IUiAnimationContextListener
{
public:
    UiAnimViewCurveEditorDialog(QWidget* parent);
    virtual ~UiAnimViewCurveEditorDialog() {}

    void SetPlayCallback(const std::function<void()>& callback) { m_widget->SetPlayCallback(callback); }

    void SetEditLock(bool bLock) { m_widget->SetEditLock(bLock); }

    CUiAnimViewSplineCtrl& GetSplineCtrl(){ return m_widget->GetSplineCtrl(); }

    void SetFPS(float fps) { m_widget->SetFPS(fps); }
    float GetFPS() const { return m_widget->GetFPS(); }
    void SetTickDisplayMode(EUiAVTickMode mode) { m_widget->SetTickDisplayMode(mode); }

    void OnSequenceChanged(CUiAnimViewSequence* pNewSequence) override { m_widget->OnSequenceChanged(pNewSequence); }
    void OnTimeChanged(float newTime) override { m_widget->OnTimeChanged(newTime); }

    virtual void OnKeysChanged(CUiAnimViewSequence* pSequence) override { m_widget->OnKeysChanged(pSequence); }
    virtual void OnKeySelectionChanged(CUiAnimViewSequence* pSequence) override { m_widget->OnKeySelectionChanged(pSequence); }
    virtual void OnNodeChanged(CUiAnimViewNode* pNode, ENodeChangeType type) override { m_widget->OnNodeChanged(pNode, type); }
    virtual void OnNodeSelectionChanged(CUiAnimViewSequence* pSequence) override { m_widget->OnNodeSelectionChanged(pSequence); }
    virtual void OnSequenceSettingsChanged(CUiAnimViewSequence* pSequence) override { m_widget->OnSequenceSettingsChanged(pSequence); }

private:
    CUiAnimViewCurveEditor* m_widget;
};

