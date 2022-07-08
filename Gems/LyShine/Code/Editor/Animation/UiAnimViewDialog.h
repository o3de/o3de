/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <LyShine/Animation/IUiAnimation.h>
#include "UiEditorAnimationBus.h"
#include "UiAnimViewNodes.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewSequenceManager.h"
#include "AnimationContext.h"
#include "Undo/IUndoManagerListener.h"

#include <QMainWindow>
#endif

class QSplitter;
class QComboBox;
class QLabel;
class CustomizeKeyboardPage;
class CUiAnimViewDopeSheetBase;
class UiAnimViewCurveEditorDialog;
class CUiAnimViewKeyPropertiesDlg;

#define LIGHT_ANIMATION_SET_NAME "_LightAnimationSet"

class CUiAnimationCallback;
class CUiAnimViewFindDlg;
class CUiAnimViewExpanderWatcher;

class CUiAnimViewDialog
    : public QMainWindow
    , public IUiAnimationContextListener
    , public IEditorNotifyListener
    , public IUiAnimViewSequenceListener
    , public IUiAnimViewSequenceManagerListener
    , IUndoManagerListener
    , public UiEditorAnimationStateBus::Handler
    , public UiEditorAnimListenerBus::Handler
{
    Q_OBJECT
public:
    friend CUiAnimationCallback;

    CUiAnimViewDialog(QWidget* pParent = NULL);
    ~CUiAnimViewDialog();

    static CUiAnimViewDialog* GetCurrentInstance() { return s_pUiAnimViewDialog; }

    void InvalidateDopeSheet();
    void Update();

    void ReloadSequences();
    void InvalidateSequence();

    void UpdateSequenceLockStatus();

    // IUiAnimationContextListener
    virtual void OnSequenceChanged(CUiAnimViewSequence* pNewSequence) override;

    // IUiAnimViewSequenceListener
    virtual void OnSequenceSettingsChanged(CUiAnimViewSequence* pSequence) override;

    // UiEditorAnimationStateInterface
    UiEditorAnimationStateInterface::UiEditorAnimationEditState GetCurrentEditState() override;
    void RestoreCurrentEditState(const UiEditorAnimationStateInterface::UiEditorAnimationEditState& animEditState) override;
    // ~UiEditorAnimationStateInterface

    // UiEditorAnimListenerInterface
    void OnActiveCanvasChanged() override;
    void OnUiElementsDeletedOrReAdded() override;
    // ~UiEditorAnimListenerInterface

    void UpdateDopeSheetTime(CUiAnimViewSequence* pSequence);

    const CUiAnimViewDopeSheetBase& GetUiAnimViewDopeSheet() const { return *m_wndDopeSheet; }

public slots:
    void OnPlay();

protected slots:
    void OnGoToPrevKey();
    void OnGoToNextKey();
    void OnAddKey();
    void OnDelKey();
    void OnMoveKey();
    void OnSlideKey();
    void OnScaleKey();
    void OnSyncSelectedTracksToBase();
    void OnSyncSelectedTracksFromBase();
    void OnAddSequence();
    void OnDelSequence();
    void OnEditSequence();
    void OnSequenceComboBox();
    void OnAddSelectedNode();
    void OnAddDirectorNode();
    void OnFindNode();

    void OnRecord();
    void OnGoToStart();
    void OnGoToEnd();
    void OnPlaySetScale();
    void OnStop();
    void OnStopHardReset();
    void OnPause();
    void OnLoop();

    void OnSnapNone();
    void OnSnapMagnet();
    void OnSnapFrame();
    void OnSnapTick();
    void OnSnapFPS();

    void OnCustomizeTrackColors();

    void OnBatchRender();

    void OnModeDopeSheet();
    void OnModeCurveEditor();
    void OnOpenCurveEditor();

    void OnViewTickInSeconds();
    void OnViewTickInFrames();

    void OnToggleDisable();
    void OnToggleMute();
    void OnMuteAll();
    void OnUnmuteAll();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void ReadLayouts();

private:
    void UpdateActions();
    void ReloadSequencesComboBox();

    void SetEditLock(bool bLock);

    void InitMenu();
    void InitToolbar();
    void InitSequences();
    void OnAddEntityNodeMenu();

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    BOOL OnInitDialog();

    void SaveLayouts();
    void SaveMiscSettings() const;
    void ReadMiscSettings();
    void SaveTrackColors() const;
    void ReadTrackColors();

    void SetCursorPosText(float fTime);

    void SaveZoomScrollSettings();

    virtual void OnNodeSelectionChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnNodeRenamed(CUiAnimViewNode* pNode, const char* pOldName) override;

    void OnSequenceAdded(CUiAnimViewSequence* pSequence) override;
    void OnSequenceRemoved(CUiAnimViewSequence* pSequence) override;

    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;
    void SaveSequenceTimingToXML();

    // Instance
    static CUiAnimViewDialog* s_pUiAnimViewDialog;

    CUiAnimViewSequenceManager* m_sequenceManager;
    CUiAnimationContext* m_animationContext;
    IUiAnimationSystem* m_animationSystem;

    // GUI
    QSplitter* m_wndSplitter;
    CUiAnimViewNodesCtrl*   m_wndNodesCtrl;
    CUiAnimViewDopeSheetBase*   m_wndDopeSheet;
    QDockWidget* m_wndCurveEditorDock;
    UiAnimViewCurveEditorDialog*    m_wndCurveEditor;
    CUiAnimViewKeyPropertiesDlg* m_wndKeyProperties;
    CUiAnimViewFindDlg* m_findDlg;
    QToolBar* m_mainToolBar;
    QToolBar* m_keysToolBar;
    QToolBar* m_playToolBar;
    QToolBar* m_viewToolBar;
    CUiAnimViewExpanderWatcher* m_expanderWatcher;
    QComboBox* m_sequencesComboBox;

    QLabel* m_cursorPos;

    // UI Animation system
    CUiAnimationCallback* m_pUiAnimationCallback;

    // Current sequence
    QString m_currentSequenceName;

    // State
    bool m_bRecord;
    bool m_bPlay;
    bool m_bPause;
    bool m_bNeedReloadSequence;
    bool m_bIgnoreUpdates;
    bool m_bDoingUndoOperation;
    bool m_lazyInitDone;
    bool m_bEditLock;

    float m_fLastTime;

    int m_currentToolBarParamTypeId;
    std::vector<CUiAnimParamType> m_toolBarParamTypes;

    QHash<int, QAction*> m_actions;
};
