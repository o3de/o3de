/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CTrackViewDialog Implementation file.


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWDIALOG_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "IMovieSystem.h"

#include "TrackViewNodes.h"
#include "TrackViewDopeSheetBase.h"
#include "TrackViewCurveEditor.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TrackViewSequence.h"
#include "TrackViewSequenceManager.h"
#include "AnimationContext.h"

#include <AzCore/Component/EntityBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QMainWindow>
#endif

class QSplitter;
class QComboBox;
class QLabel;

class CMovieCallback;
class CTrackViewFindDlg;

class CTrackViewDialog
    : public QMainWindow
    , public IAnimationContextListener
    , public IEditorNotifyListener
    , public ITrackViewSequenceListener
    , public ITrackViewSequenceManagerListener
    , public AZ::EntitySystemBus::Handler
    , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    , IUndoManagerListener
{
    Q_OBJECT
public:
    friend CMovieCallback;

    CTrackViewDialog(QWidget* pParent = nullptr);
    ~CTrackViewDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID();

    static CTrackViewDialog* GetCurrentInstance() { return s_pTrackViewDialog; }

    void InvalidateDopeSheet();
    void Update();

    void ReloadSequences();
    void InvalidateSequence();

    void UpdateSequenceLockStatus();

    // IAnimationContextListener
    void OnSequenceChanged(CTrackViewSequence* pNewSequence) override;

    // ITrackViewSequenceListener
    void OnSequenceSettingsChanged(CTrackViewSequence* pSequence) override;

    void UpdateDopeSheetTime(CTrackViewSequence* pSequence);

    const CTrackViewDopeSheetBase& GetTrackViewDopeSheet() const { return *m_wndDopeSheet; }
    const AZStd::vector<AnimParamType>& GetDefaultTracksForEntityNode() const { return m_defaultTracksForEntityNode; }

    bool IsDoingUndoOperation() const { return m_bDoingUndoOperation; }

    //////////////////////////////////////////////////////////////////////////
    // AZ::EntitySystemBus
    void OnEntityDestruction(const AZ::EntityId& entityId) override;
    //~AZ::EntitySystemBus

public: // static functions

    static QString GetEntityIdAsString(const AZ::EntityId& entityId) { return QString::number(static_cast<AZ::u64>(entityId)); }

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
    void OnExportFBXSequence();
    void OnExportNodeKeysGlobalTime();
    void OnDelSequence();
    void OnEditSequence();
    void OnSequenceComboBox();
    void OnAddSelectedNode();
    void OnAddDirectorNode();
    void OnFindNode();

    void OnRecord();
    void OnAutoRecord();
    void OnAutoRecordStep();
    void OnGoToStart();
    void OnGoToEnd();
    void OnPlay();
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

    void OnTracksToolBar();
    void OnToggleDisable();
    void OnToggleMute();
    void OnMuteAll();
    void OnUnmuteAll();

protected:
    void keyPressEvent(QKeyEvent* event) override;
#if defined(AZ_PLATFORM_WINDOWS)
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif
    bool event(QEvent* event) override;

private slots:
    void ReadLayouts();
    void FillAddSelectedEntityMenu();

private:

    enum class ViewMode
    {
        TrackView = 1,
        CurveEditor = 2,
        Both = 3
    };

    void setViewMode(ViewMode);
    void UpdateActions();
    void ReloadSequencesComboBox();

    void UpdateTracksToolBar();
    void ClearTracksToolBar();
    void AddButtonToTracksToolBar(const CAnimParamType& paramId, const QIcon& hIcon, const QString& title);
    void SetNodeForTracksToolBar(CTrackViewAnimNode* pNode) { m_pNodeForTracksToolBar = pNode; }

    void SetEditLock(bool bLock);
    void OnGameOrSimModeLock(bool lock);

    void InitMenu();
    void InitToolbar();
    void InitSequences();
    void OnAddEntityNodeMenu();

    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
    bool OnInitDialog();

    void SaveLayouts();
    void SaveMiscSettings() const;
    void ReadMiscSettings();
    void SaveTrackColors() const;
    void ReadTrackColors();

    void SetCursorPosText(float fTime);

#if defined(AZ_PLATFORM_WINDOWS)
    bool processRawInput(MSG* pMsg);
#endif

    void OnNodeSelectionChanged(CTrackViewSequence* pSequence) override;
    void OnNodeRenamed(CTrackViewNode* pNode, const char* pOldName) override;

    void OnSequenceAdded(CTrackViewSequence* pSequence) override;
    void OnSequenceRemoved(CTrackViewSequence* pSequence) override;

    void AddSequenceListeners(CTrackViewSequence* sequence);
    void RemoveSequenceListeners(CTrackViewSequence* sequence);

    void AddDialogListeners();
    void RemoveDialogListeners();

    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;
    void SaveCurrentSequenceToFBX();
    void SaveSequenceTimingToXML();

    // ToolsApplicationNotificationBus ...
    void AfterEntitySelectionChanged(
        const AzToolsFramework::EntityIdList& newlySelectedEntities,
        const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;

    // Instance
    static CTrackViewDialog* s_pTrackViewDialog;

    // GUI
    QSplitter* m_wndSplitter;
    CTrackViewNodesCtrl*    m_wndNodesCtrl;
    CTrackViewDopeSheetBase*    m_wndDopeSheet;
    QDockWidget* m_wndCurveEditorDock;
    TrackViewCurveEditorDialog* m_wndCurveEditor;
    CTrackViewKeyPropertiesDlg* m_wndKeyProperties;
    CTrackViewFindDlg* m_findDlg;
    QToolBar* m_mainToolBar;
    QToolBar* m_keysToolBar;
    QToolBar* m_playToolBar;
    QToolBar* m_viewToolBar;
    QToolBar* m_tracksToolBar;
    QComboBox* m_sequencesComboBox;

    QLabel* m_cursorPos;
    QLabel* m_activeCamStatic;

    // CryMovie
    CMovieCallback* m_pMovieCallback;

    // Current sequence
    AZ::EntityId m_currentSequenceEntityId;

    // State
    bool m_bRecord;
    bool m_bAutoRecord;
    bool m_bPlay;
    bool m_bPause;
    bool m_bNeedReloadSequence;
    bool m_bIgnoreUpdates;
    bool m_bDoingUndoOperation;
    bool m_lazyInitDone;
    bool m_bEditLock;
    bool m_enteringGameOrSimModeLock = false;
    bool m_needReAddListeners = false;

    float m_fLastTime;
    float m_fAutoRecordStep;

    CTrackViewAnimNode* m_pNodeForTracksToolBar;

    int m_currentToolBarParamTypeId;
    std::vector<CAnimParamType> m_toolBarParamTypes;

    // Default tracks menu
    AZStd::vector<AnimParamType> m_defaultTracksForEntityNode;

    QHash<int, QAction*> m_actions;
    ViewMode m_lastMode = ViewMode::TrackView;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWDIALOG_H
