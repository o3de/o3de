/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : CTrackViewDialog Implementation file.


#include "EditorDefs.h"

#include "TrackViewDialog.h"

// Qt
#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

// AzFramework
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

// AzQtComponents
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>

// CryCommon
#include <CryCommon/Maestro/Bus/EditorSequenceComponentBus.h>
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "Settings.h"
#include "Util/fastlib.h"
#include "TVSequenceProps.h"
#include "TrackViewFindDlg.h"
#include "SequenceBatchRenderDialog.h"
#include "TVCustomizeTrackColorsDlg.h"
#include "PluginManager.h"
#include "Util/3DConnexionDriver.h"
#include "TrackViewNewSequenceDialog.h"
#include "FBXExporterDialog.h"
#include "CryEditDoc.h"
#include "LyViewPaneNames.h"

inline namespace TrackViewInternal
{
    const char* s_kTrackViewLayoutSection = "TrackViewLayout";
    const char* s_kTrackViewSection = "DockingPaneLayouts\\TrackView";
    const char* s_kSplitterEntry = "Splitter";
    const char* s_kVersionEntry = "TrackViewLayoutVersion";

    const char* s_kTrackViewSettingsSection = "TrackView";
    const char* s_kSnappingModeEntry = "SnappingMode";
    const char* s_kFrameSnappingFPSEntry = "FrameSnappingFPS";
    const char* s_kTickDisplayModeEntry = "TickDisplayMode";
    const char* s_kDefaultTracksEntry = "DefaultTracks2";

    const char* s_kRebarVersionEntry = "TrackViewReBarVersion";
    const char* s_kRebarBandEntryPrefix = "ReBarBand";

    const char* s_kNoSequenceComboBoxEntry = "--- No Sequence ---";

    const int s_kMinimumFrameSnappingFPS = 1;
    const int s_kMaximumFrameSnappingFPS = 120;

    CTrackViewSequence* GetSequenceByEntityIdOrName(const CTrackViewSequenceManager* pSequenceManager, const char* entityIdOrName)
    {
        // the "name" string will be an AZ::EntityId in string form if this was called from
        // TrackView code. But for backward compatibility we also support a sequence name.
        bool isNameAValidU64 = false;
        QString entityIdString = entityIdOrName;
        AZ::u64 nameAsU64 = entityIdString.toULongLong(&isNameAValidU64);

        CTrackViewSequence* pSequence = nullptr;
        if (isNameAValidU64)
        {
            // "name" string was a valid u64 represented as a string. Use as an entity Id to search for sequence.
            pSequence = pSequenceManager->GetSequenceByEntityId(AZ::EntityId(nameAsU64));
        }

        if (!pSequence)
        {
            // name passed in could not find a sequence by using it as an EntityId. Use it as a
            // sequence name for backward compatibility
            pSequence = pSequenceManager->GetSequenceByName(entityIdOrName);
        }

        return pSequence;
    }
}

void CTrackViewDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions opts;
    opts.shortcut = QKeySequence(Qt::Key_T);
    opts.isDisabledInSimMode = true;
    opts.showOnToolsToolbar = true;
    opts.toolbarIcon = ":/Menu/trackview_editor.svg";

    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        AzToolsFramework::RegisterViewPane<CTrackViewDialog>(LyViewPane::TrackView, LyViewPane::CategoryTools, opts);
        GetIEditor()->GetSettingsManager()->AddToolName(s_kTrackViewLayoutSection, LyViewPane::TrackView);
    }
}

const GUID& CTrackViewDialog::GetClassID()
{
    static const GUID guid =
    {
        0xd21c9fe5, 0x22d3, 0x41e3, { 0xb8, 0x4b, 0xa3, 0x77, 0xaf, 0xa0, 0xa0, 0x5c }
    };
    return guid;
}


CTrackViewDialog* CTrackViewDialog::s_pTrackViewDialog = nullptr;

CTrackViewDialog::CTrackViewDialog(QWidget* pParent /*=nullptr*/)
    : QMainWindow(pParent)
{
    s_pTrackViewDialog = this;
    m_bRecord = false;
    m_bAutoRecord = false;
    m_bPause = false;
    m_bPlay = false;
    m_fLastTime = -1.0f;
    m_fAutoRecordStep = 0.5f;
    m_bNeedReloadSequence = false;
    m_bIgnoreUpdates = false;
    m_bDoingUndoOperation = false;

    m_findDlg = nullptr;

    m_lazyInitDone = false;
    m_bEditLock = false;

    m_pNodeForTracksToolBar = nullptr;

    m_currentToolBarParamTypeId = 0;

    // Default Tracks created for AZ Entities
    m_defaultTracksForEntityNode.push_back(AnimParamType::Position);
    m_defaultTracksForEntityNode.push_back(AnimParamType::Rotation);

    AddDialogListeners();
    OnInitDialog();

    AZ::EntitySystemBus::Handler::BusConnect();
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
}

CTrackViewDialog::~CTrackViewDialog()
{
    AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
    AZ::EntitySystemBus::Handler::BusDisconnect();

    SaveLayouts();
    SaveMiscSettings();
    SaveTrackColors();

    if (m_findDlg)
    {
        m_findDlg->deleteLater();
        m_findDlg = nullptr;
    }
    s_pTrackViewDialog = nullptr;

    const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
    CTrackViewSequence* sequence = pSequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);
    RemoveSequenceListeners(sequence);
    RemoveDialogListeners();
}

void CTrackViewDialog::OnAddEntityNodeMenu()
{
    // Toggle the selection
    QAction* action = static_cast<QAction*>(sender());
    if (action)
    {
        AnimParamType paramTime = static_cast<AnimParamType>(action->data().toInt());
        auto it = AZStd::find(m_defaultTracksForEntityNode.begin(), m_defaultTracksForEntityNode.end(), paramTime);
        if (it == m_defaultTracksForEntityNode.end())
        {
            m_defaultTracksForEntityNode.push_back(paramTime);
        }
        else
        {
            m_defaultTracksForEntityNode.erase(it);
        }
    }
}

bool CTrackViewDialog::OnInitDialog()
{
    InitToolbar();
    InitMenu();

    QWidget* w = new QWidget();
    QVBoxLayout* l = new QVBoxLayout;
    l->setMargin(0);

    m_wndSplitter = new QSplitter(w);
    m_wndSplitter->setOrientation(Qt::Horizontal);

    m_wndNodesCtrl = new CTrackViewNodesCtrl(this, this);
    m_wndNodesCtrl->SetTrackViewDialog(this);

    m_wndDopeSheet = new CTrackViewDopeSheetBase(this);
    m_wndDopeSheet->SetTimeRange(0, 20);
    m_wndDopeSheet->SetTimeScale(100, 0);

    m_wndDopeSheet->SetNodesCtrl(m_wndNodesCtrl);
    m_wndNodesCtrl->SetDopeSheet(m_wndDopeSheet);

    m_wndSplitter->addWidget(m_wndNodesCtrl);
    m_wndSplitter->addWidget(m_wndDopeSheet);
    m_wndSplitter->setStretchFactor(0, 1);
    m_wndSplitter->setStretchFactor(1, 10);
    l->addWidget(m_wndSplitter);
    w->setLayout(l);
    setCentralWidget(w);

    m_wndKeyProperties = new CTrackViewKeyPropertiesDlg(this);
    QDockWidget* dw = new AzQtComponents::StyledDockWidget(this);
    dw->setObjectName("m_wndKeyProperties");
    dw->setWindowTitle("Key");
    dw->setWidget(m_wndKeyProperties);
    addDockWidget(Qt::RightDockWidgetArea, dw);
    m_wndKeyProperties->PopulateVariables();
    m_wndKeyProperties->SetKeysCtrl(m_wndDopeSheet);

    m_wndCurveEditorDock = new AzQtComponents::StyledDockWidget(this);
    m_wndCurveEditorDock->setObjectName("m_wndCurveEditorDock");
    m_wndCurveEditorDock->setWindowTitle("Curve Editor");
    m_wndCurveEditor = new TrackViewCurveEditorDialog(this);
    m_wndCurveEditorDock->setWidget(m_wndCurveEditor);
    addDockWidget(Qt::BottomDockWidgetArea, m_wndCurveEditorDock);
    m_wndCurveEditor->SetPlayCallback([this] { OnPlay(); });

    InitSequences();

    m_lazyInitDone = false;

    QTimer::singleShot(0, this, SLOT(ReadLayouts()));
    //  ReadLayouts();
    ReadMiscSettings();
    ReadTrackColors();

    QString cursorPosText = QString("0.000(%1fps)").arg(FloatToIntRet(m_wndCurveEditor->GetFPS()));
    m_cursorPos->setText(cursorPosText);

    return true;  // return true unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTrackViewDialog::FillAddSelectedEntityMenu()
{
    QMenu* menu = qobject_cast<QMenu*>(sender());
    menu->clear();

    AZStd::vector<AnimParamType> allTracks = {
        AnimParamType::Position,
        AnimParamType::Rotation,
        AnimParamType::Scale
    };

    AZStd::map<AnimParamType, QString> paramNames;
    paramNames[AnimParamType::Position] = "Position";
    paramNames[AnimParamType::Rotation] = "Rotation",
    paramNames[AnimParamType::Scale] = "Scale";

    for (AnimParamType track : allTracks)
    {
        auto it = AZStd::find(m_defaultTracksForEntityNode.begin(), m_defaultTracksForEntityNode.end(), track);
        bool checked = (it != m_defaultTracksForEntityNode.end());

        QAction* action = menu->addAction(paramNames[track]);
        action->setCheckable(true);
        action->setChecked(checked);
        action->setData(static_cast<int>(track));
        action->setEnabled(true);
        connect(action, &QAction::triggered, this, &CTrackViewDialog::OnAddEntityNodeMenu);
    }
}

void CTrackViewDialog::InitToolbar()
{
    m_mainToolBar = addToolBar("Sequence/Node Toolbar");
    m_mainToolBar->setObjectName("m_mainToolBar");
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->addWidget(new QLabel("Sequence/Node:"));
    QAction* qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-00.png"), "Add Sequence");
    qaction->setData(ID_TV_ADD_SEQUENCE);
    m_actions[ID_TV_ADD_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddSequence);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-01.png"), "Delete Sequence");
    qaction->setData(ID_TV_DEL_SEQUENCE);
    m_actions[ID_TV_DEL_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnDelSequence);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-02.png"), "Edit Sequence Properties");
    qaction->setData(ID_TV_EDIT_SEQUENCE);
    m_actions[ID_TV_EDIT_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnEditSequence);
    m_sequencesComboBox = new QComboBox(this);
    m_sequencesComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_sequencesComboBox->setToolTip("Select the sequence");
    connect(m_sequencesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSequenceComboBox()));
    m_mainToolBar->addWidget(m_sequencesComboBox);
    m_mainToolBar->addSeparator();

    QToolButton* toolButton = new QToolButton(m_mainToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/main/tvmain-03.png"), "Add Selected Node", this);
    qaction->setData(ID_ADDNODE);
    m_actions[ID_ADDNODE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddSelectedNode);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        connect(buttonMenu, &QMenu::aboutToShow, this, &CTrackViewDialog::FillAddSelectedEntityMenu);
    }
    m_mainToolBar->addWidget(toolButton);

    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-04.png"), "Add Director Node");
    qaction->setData(ID_ADDSCENETRACK);
    m_actions[ID_ADDSCENETRACK] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddDirectorNode);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-05.png"), "Find");
    qaction->setData(ID_FIND);
    m_actions[ID_FIND] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnFindNode);
    m_mainToolBar->addSeparator();
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-06.png"), "Toggle Disable");
    qaction->setCheckable(true);
    qaction->setData(ID_TRACKVIEW_TOGGLE_DISABLE);
    m_actions[ID_TRACKVIEW_TOGGLE_DISABLE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnToggleDisable);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-07.png"), "Toggle Mute");
    qaction->setCheckable(true);
    qaction->setData(ID_TRACKVIEW_TOGGLE_MUTE);
    m_actions[ID_TRACKVIEW_TOGGLE_MUTE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnToggleMute);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-08.png"), "Mute Selected Tracks");
    qaction->setData(ID_TRACKVIEW_MUTE_ALL);
    m_actions[ID_TRACKVIEW_MUTE_ALL] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnMuteAll);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-09.png"), "Unmute Selected Tracks");
    qaction->setData(ID_TRACKVIEW_UNMUTE_ALL);
    m_actions[ID_TRACKVIEW_UNMUTE_ALL] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnUnmuteAll);

    m_viewToolBar = addToolBar("View Toolbar");
    m_viewToolBar->setObjectName("m_viewToolBar");
    m_viewToolBar->setFloatable(false);
    m_viewToolBar->addWidget(new QLabel("View:"));
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-00.png"), "Track Editor");
    qaction->setData(ID_TV_MODE_DOPESHEET);
    qaction->setShortcut(QKeySequence("Ctrl+D"));
    qaction->setCheckable(true);
    qaction->setChecked(true);
    m_actions[ID_TV_MODE_DOPESHEET] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnModeDopeSheet);
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-01.png"), "Curve Editor");
    qaction->setData(ID_TV_MODE_CURVEEDITOR);
    qaction->setShortcut(QKeySequence("Ctrl+R"));
    qaction->setCheckable(true);
    m_actions[ID_TV_MODE_CURVEEDITOR] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnModeCurveEditor);
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-02.png"), "Both");
    qaction->setData(ID_TV_MODE_OPENCURVEEDITOR);
    qaction->setShortcut(QKeySequence("Ctrl+B"));
    m_actions[ID_TV_MODE_OPENCURVEEDITOR] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnOpenCurveEditor);

    m_playToolBar = addToolBar("Play Toolbar");
    m_playToolBar->setObjectName("m_playToolBar");
    m_playToolBar->setFloatable(false);
    m_playToolBar->addWidget(new QLabel("Play:"));
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/SequenceStart.svg"), "Go to start of sequence");
    qaction->setData(ID_TV_JUMPSTART);
    m_actions[ID_TV_JUMPSTART] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToStart);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/PlayForward.svg"), "Play Animation", this);
    qaction->setData(ID_TV_PLAY);
    qaction->setCheckable(true);
    m_actions[ID_TV_PLAY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPlay);
    qaction->setShortcut(QKeySequence(Qt::Key_Space));
    qaction->setShortcutContext(Qt::WindowShortcut);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        QActionGroup* ag = new QActionGroup(buttonMenu);
        for (auto i : { .5, 1., 2., 4., 8. })
        {
            if (i == .5)
            {
                qaction = buttonMenu->addAction(" 2 ");
            }
            else if (i == 1.)
            {
                qaction = buttonMenu->addAction(" 1 ");
            }
            else
            {
                qaction = buttonMenu->addAction(QString("1/%1").arg((int)i));
            }
            qaction->setData(i);
            connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPlaySetScale);
            qaction->setCheckable(true);
            qaction->setChecked(i == 1.);
            ag->addAction(qaction);
        }
    }
    m_playToolBar->addWidget(toolButton);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/Stop.svg"), "Stop", this);
    qaction->setData(ID_TV_STOP);
    m_actions[ID_TV_STOP] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnStop);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);

        buttonMenu->addAction(qaction);
        qaction = buttonMenu->addAction("Stop with Hard Reset");
        qaction->setData(ID_TV_STOP_HARD_RESET);
        m_actions[ID_TV_STOP_HARD_RESET] = qaction;
        connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnStopHardReset);
    }
    m_playToolBar->addWidget(toolButton);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/Pause.svg"), "Pause");
    qaction->setData(ID_TV_PAUSE);
    qaction->setCheckable(true);
    m_actions[ID_TV_PAUSE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnPause);
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/SequenceEnd.svg"), "Go to end of sequence");
    qaction->setData(ID_TV_JUMPEND);
    m_actions[ID_TV_JUMPEND] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToEnd);

    qaction = m_playToolBar->addAction(QIcon(":/Trackview/RecordButton.svg"), "Start Animation Recording");
    qaction->setData(ID_TV_RECORD);
    qaction->setCheckable(true);
    m_actions[ID_TV_RECORD] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnRecord);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/AutoRecord.svg"), "Start Auto Recording", this);
    toolButton->addAction(qaction);
    toolButton->setDefaultAction(qaction);
    qaction->setData(ID_TV_RECORD_AUTO);
    qaction->setCheckable(true);
    m_actions[ID_TV_RECORD_AUTO] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAutoRecord);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        QActionGroup* ag = new QActionGroup(buttonMenu);
        for (auto i : { 1, 2, 5, 10, 25, 50, 100 })
        {
            if (i == 1)
            {
                qaction = buttonMenu->addAction(" 1 sec");
            }
            else
            {
                qaction = buttonMenu->addAction(QString("1/%1 sec").arg(i));
            }
            qaction->setData(i);
            connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAutoRecordStep);
            qaction->setCheckable(true);
            qaction->setChecked(i == 1);
            m_fAutoRecordStep = 1;
            ag->addAction(qaction);
        }
    }
    m_playToolBar->addWidget(toolButton);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/Loop.svg"), "Loop");
    qaction->setData(ID_PLAY_LOOP);
    qaction->setCheckable(true);
    m_actions[ID_PLAY_LOOP] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnLoop);
    m_playToolBar->addSeparator();
    m_cursorPos = new QLabel(this);
    m_playToolBar->addWidget(m_cursorPos);
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-08.png"), "Frame Rate");
    qaction->setData(ID_TV_SNAP_FPS);
    qaction->setCheckable(true);
    m_actions[ID_TV_SNAP_FPS] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapFPS);
    m_activeCamStatic = new QLabel(this);
    m_playToolBar->addWidget(m_activeCamStatic);
    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-09.png"), "Undo");
    qaction->setData(ID_UNDO);
    m_actions[ID_UNDO] = qaction;
    connect(qaction, &QAction::triggered, this, []()
    {
        GetIEditor()->Undo();
    });
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-10.png"), "Redo");
    qaction->setData(ID_REDO);
    m_actions[ID_REDO] = qaction;
    connect(qaction, &QAction::triggered, this, []()
    {
        GetIEditor()->Redo();
    });

    addToolBarBreak(Qt::TopToolBarArea);

    m_keysToolBar = addToolBar("Keys Toolbar");
    m_keysToolBar->setObjectName("m_keysToolBar");
    m_keysToolBar->setFloatable(false);
    m_keysToolBar->addWidget(new QLabel("Keys:"));
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-00.png"), "Go to previous key");
    qaction->setData(ID_TV_PREVKEY);
    m_actions[ID_TV_PREVKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToPrevKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-01.png"), "Go to next key");
    qaction->setData(ID_TV_NEXTKEY);
    m_actions[ID_TV_NEXTKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnGoToNextKey);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-02.png"), "Move Keys");
    qaction->setData(ID_TV_MOVEKEY);
    m_actions[ID_TV_MOVEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnMoveKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-03.png"), "Slide Keys");
    qaction->setData(ID_TV_SLIDEKEY);
    m_actions[ID_TV_SLIDEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSlideKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-04.png"), "Scale Keys");
    qaction->setData(ID_TV_SCALEKEY);
    m_actions[ID_TV_SCALEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnScaleKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-05.png"), "Add Keys");
    qaction->setData(ID_TV_ADDKEY);
    m_actions[ID_TV_ADDKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnAddKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-06.png"), "Delete Keys");
    qaction->setData(ID_TV_DELKEY);
    m_actions[ID_TV_DELKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnDelKey);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-07.png"), "No Snapping");
    qaction->setData(ID_TV_SNAP_NONE);
    m_actions[ID_TV_SNAP_NONE] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapNone);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-08.png"), "Magnet Snapping");
    qaction->setData(ID_TV_SNAP_MAGNET);
    m_actions[ID_TV_SNAP_MAGNET] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapMagnet);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-09.png"), "Frame Snapping");
    qaction->setData(ID_TV_SNAP_FRAME);
    m_actions[ID_TV_SNAP_FRAME] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapFrame);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-10.png"), "Tick Snapping");
    qaction->setData(ID_TV_SNAP_TICK);
    m_actions[ID_TV_SNAP_TICK] = qaction;
    connect(qaction, &QAction::triggered, this, &CTrackViewDialog::OnSnapTick);
    m_keysToolBar->addSeparator();
    QActionGroup* ag = new QActionGroup(this);
    ag->addAction(m_actions[ID_TV_ADDKEY]);
    ag->addAction(m_actions[ID_TV_MOVEKEY]);
    ag->addAction(m_actions[ID_TV_SLIDEKEY]);
    ag->addAction(m_actions[ID_TV_SCALEKEY]);
    for (QAction* qaction2 : ag->actions())
    {
        qaction2->setCheckable(true);
    }
    m_actions[ID_TV_MOVEKEY]->setChecked(true);
    ag = new QActionGroup(this);
    ag->addAction(m_actions[ID_TV_SNAP_NONE]);
    ag->addAction(m_actions[ID_TV_SNAP_MAGNET]);
    ag->addAction(m_actions[ID_TV_SNAP_FRAME]);
    ag->addAction(m_actions[ID_TV_SNAP_TICK]);
    for (QAction* qaction2 : ag->actions())
    {
        qaction2->setCheckable(true);
    }

    m_actions[ID_TV_SNAP_NONE]->setChecked(true);

    m_tracksToolBar = addToolBar("Tracks Toolbar");
    m_tracksToolBar->setObjectName("m_tracksToolBar");
    m_tracksToolBar->setFloatable(false);
    ClearTracksToolBar();

    m_bRecord = false;
    m_bPause = false;
    m_bPlay = false;
}

void CTrackViewDialog::InitMenu()
{
    QMenuBar* mb = this->menuBar();

    QMenu* m = mb->addMenu("&Sequence");
    QAction* a = m->addAction("New Sequence...");
    a->setData(ID_TV_SEQUENCE_NEW);
    m_actions[ID_TV_SEQUENCE_NEW] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnAddSequence);

    m = mb->addMenu("&View");
    m->addAction(m_actions[ID_TV_MODE_DOPESHEET]);
    m->addAction(m_actions[ID_TV_MODE_CURVEEDITOR]);
    m->addAction(m_actions[ID_TV_MODE_OPENCURVEEDITOR]);
    m->addSeparator();
    a = m->addAction("Tick in Seconds");
    a->setData(ID_VIEW_TICKINSECONDS);
    a->setCheckable(true);
    m_actions[ID_VIEW_TICKINSECONDS] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnViewTickInSeconds);
    a = m->addAction("Tick in Frames");
    a->setData(ID_VIEW_TICKINFRAMES);
    a->setCheckable(true);
    m_actions[ID_VIEW_TICKINFRAMES] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnViewTickInFrames);

    m = mb->addMenu("T&ools");
    a = m->addAction("Render Output...");
    a->setData(ID_TOOLS_BATCH_RENDER);
    m_actions[ID_TOOLS_BATCH_RENDER] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnBatchRender);
    a = m->addAction("Customize &Track Colors...");
    a->setData(ID_TV_TOOLS_CUSTOMIZETRACKCOLORS);
    m_actions[ID_TV_TOOLS_CUSTOMIZETRACKCOLORS] = a;
    connect(a, &QAction::triggered, this, &CTrackViewDialog::OnCustomizeTrackColors);
}

void CTrackViewDialog::UpdateActions()
{
    if (m_bIgnoreUpdates || m_actions.empty())
    {
        return;
    }

    if (GetIEditor()->GetAnimation()->IsRecordMode())
    {
        m_actions[ID_TV_RECORD]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_RECORD]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsAutoRecording())
    {
        m_actions[ID_TV_RECORD_AUTO]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_RECORD_AUTO]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsPlayMode())
    {
        m_actions[ID_TV_PLAY]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_PLAY]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsPaused())
    {
        m_actions[ID_TV_PAUSE]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_PAUSE]->setChecked(false);
    }
    if (GetIEditor()->GetAnimation()->IsLoopMode())
    {
        m_actions[ID_PLAY_LOOP]->setChecked(true);
    }
    else
    {
        m_actions[ID_PLAY_LOOP]->setChecked(false);
    }
    if (m_wndDopeSheet->GetTickDisplayMode() == eTVTickMode_InSeconds)
    {
        m_actions[ID_VIEW_TICKINSECONDS]->setChecked(true);
    }
    else
    {
        m_actions[ID_VIEW_TICKINSECONDS]->setChecked(false);
    }
    if (m_wndDopeSheet->GetTickDisplayMode() == eTVTickMode_InFrames)
    {
        m_actions[ID_VIEW_TICKINFRAMES]->setChecked(true);
    }
    else
    {
        m_actions[ID_VIEW_TICKINFRAMES]->setChecked(false);
    }

    m_actions[ID_TV_DEL_SEQUENCE]->setEnabled(m_bEditLock ? false : true);

    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        if (m_bEditLock)
        {
            m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(false);
        }
        else
        {
            m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(true);
        }

        CTrackViewAnimNodeBundle selectedNodes = sequence->GetSelectedAnimNodes();
        CTrackViewTrackBundle selectedTracks = sequence->GetSelectedTracks();

        const unsigned int selectedNodeCount = selectedNodes.GetCount();
        const unsigned int selectedTrackCount = selectedTracks.GetCount();

        bool updated_ID_TRACKVIEW_TOGGLE_DISABLE = false;
        bool updated_ID_TRACKVIEW_TOGGLE_MUTE = false;
        if (selectedNodeCount + selectedTrackCount == 1)
        {
            if (selectedNodeCount == 1)
            {
                const CTrackViewAnimNode* animNode = selectedNodes.GetNode(0);
                // The root sequence node doesn't have an internal anim node and cannot be disabled.
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(animNode->GetNodeType() != eTVNT_Sequence);
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setChecked(animNode->IsDisabled() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_DISABLE = true;
            }

            if (selectedTrackCount == 1)
            {
                CTrackViewTrack* pTrack = selectedTracks.GetTrack(0);

                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setChecked(pTrack->IsDisabled() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_DISABLE = true;
                m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setChecked(pTrack->IsMuted() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_MUTE = true;
            }
        }

        bool allSelectedTracksUseMute = true;
        for (unsigned int i = 0; i < selectedTrackCount; i++)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            if (pTrack && !pTrack->UsesMute())
            {
                allSelectedTracksUseMute = false;
                break;
            }
        }

        if (!updated_ID_TRACKVIEW_TOGGLE_DISABLE)
        {
            m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(false);
        }
        // disable toggle mute if don't have a single track selected or the all selected tracks does not use Mute
        if (!updated_ID_TRACKVIEW_TOGGLE_MUTE || !allSelectedTracksUseMute)
        {
            m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setEnabled(false);
        }

        m_actions[ID_TRACKVIEW_MUTE_ALL]->setEnabled(true);
        m_actions[ID_ADDSCENETRACK]->setEnabled(true);

        bool areAnyEntitiesSelected = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            areAnyEntitiesSelected, &AzToolsFramework::ToolsApplicationRequests::AreAnyEntitiesSelected);

        m_actions[ID_ADDNODE]->setEnabled(areAnyEntitiesSelected);
    }
    else
    {
        m_actions[ID_TV_DEL_SEQUENCE]->setEnabled(false);
        m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_TOGGLE_MUTE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_MUTE_ALL]->setEnabled(false);
        m_actions[ID_ADDSCENETRACK]->setEnabled(false);
        m_actions[ID_ADDNODE]->setEnabled(false);
    }

    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();

    if (movieSystem)
    {
        m_actions[ID_TOOLS_BATCH_RENDER]->setEnabled(movieSystem->GetNumSequences() > 0 && !m_enteringGameOrSimModeLock);
    }
    else
    {
        m_actions[ID_TOOLS_BATCH_RENDER]->setEnabled(false);
    }

    m_actions[ID_TV_ADD_SEQUENCE]->setEnabled(GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady() && !m_enteringGameOrSimModeLock);
    m_actions[ID_TV_SEQUENCE_NEW]->setEnabled(GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady() && !m_enteringGameOrSimModeLock);
}

void CTrackViewDialog::InitSequences()
{
    ReloadSequences();
}

void CTrackViewDialog::InvalidateSequence()
{
    m_bNeedReloadSequence = true;
}

void CTrackViewDialog::InvalidateDopeSheet()
{
    if (m_wndDopeSheet)
    {
        m_wndDopeSheet->update();
    }
}

void CTrackViewDialog::Update()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    bool wasReloading = m_bNeedReloadSequence;

    if (m_bNeedReloadSequence || m_needReAddListeners)
    {
        const CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = pSequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);

        if (m_bNeedReloadSequence)
        {
            m_bNeedReloadSequence = false;
            pAnimationContext->SetSequence(sequence, true, false);
        }
        if (m_needReAddListeners)
        {
            m_needReAddListeners = false;
            AddSequenceListeners(sequence);
        }
    }

    constexpr const auto noMovieCameraName = "Active Camera";
    const auto sequence = pAnimationContext->GetSequence();
    if (!sequence)  // Nothing to update ?
    {
        m_activeCamStatic->setText(noMovieCameraName);
        SetCursorPosText(-1.0f);
        return;
    }

    float fTime = pAnimationContext->GetTime();

    if (fTime != m_fLastTime)
    {
        m_fLastTime = fTime;
        SetCursorPosText(fTime);
    }

    // Display the name of the active camera in the static control, if any.
    // Having an active camera means at least the following conditions:
    // 1. The movie system has the Animation Sequence corresponding to the active TrackView Sequence.
    // 2. The Animation Sequence has an active Director node.
    // 3. The current Camera parameters in the movie system are valid.
    // TODO: invalidate the Camera parameters in the movie system when conditions 1 and 2 are not valid.

    bool cameraNameSet = false;
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (movieSystem)
    {
        const auto animSequence = movieSystem->FindSequenceById(sequence->GetCryMovieId());
        IAnimNode* activeDirector = animSequence ? animSequence->GetActiveDirector() : nullptr;

        AZ::EntityId camId = movieSystem->GetCameraParams().cameraEntityId;
        if (camId.IsValid() && activeDirector)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, camId);
            if (entity)
            {
                m_activeCamStatic->setText(entity->GetName().c_str());
                cameraNameSet = true;

                // Evaluate the corner case when the sequence is reloaded and the "Autostart" flag is set for the sequence:
                // prepare to manually scrub this sequence if the "Autostart" flag is set.
                if (wasReloading && ((animSequence->GetFlags() & IAnimSequence::eSeqFlags_PlayOnReset) != 0))
                {
                    // Try to switch camera in the Editor Viewport Widget to the CameraComponent with the EntityId from this key,
                    // works only in Editing mode (checked in the called method).
                    pAnimationContext->SwitchEditorViewportCamera(camId);
                }
            }
        }
    }
    if (!cameraNameSet)
    {
        m_activeCamStatic->setText(noMovieCameraName);
    }

    if (m_wndNodesCtrl)
    {
        m_wndNodesCtrl->Update();
    }
}


void CTrackViewDialog::OnGoToPrevKey()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* sequence = pAnimationContext->GetSequence();

    if (sequence)
    {
        float time = pAnimationContext->GetTime();

        CTrackViewNode* pNode = sequence->GetFirstSelectedNode();
        pNode = pNode ? pNode : sequence;

        if (pNode->SnapTimeToPrevKey(time))
        {
            pAnimationContext->SetTime(time);
        }
    }
}

void CTrackViewDialog::OnGoToNextKey()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* sequence = pAnimationContext->GetSequence();

    if (sequence)
    {
        float time = pAnimationContext->GetTime();

        CTrackViewNode* pNode = sequence->GetFirstSelectedNode();
        pNode = pNode ? pNode : sequence;

        if (pNode->SnapTimeToNextKey(time))
        {
            pAnimationContext->SetTime(time);
        }
    }
}

void CTrackViewDialog::OnAddKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_AddKeys);
}

void CTrackViewDialog::OnDelKey()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* sequence = pAnimationContext->GetSequence();

    if (sequence)
    {
        CUndo undo("Delete Keys");
        sequence->DeleteSelectedKeys();
    }
}

void CTrackViewDialog::OnMoveKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_MoveKey);
}

void CTrackViewDialog::OnSlideKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_SlideKey);
}

void CTrackViewDialog::OnScaleKey()
{
    m_wndDopeSheet->SetMouseActionMode(eTVActionMode_ScaleKey);
}
void CTrackViewDialog::OnAddSequence()
{
    CTVNewSequenceDialog dlg(this);

    if (dlg.exec() == QDialog::Accepted)
    {
        QString sequenceName = dlg.GetSequenceName();

        if (sequenceName != s_kNoSequenceComboBoxEntry)
        {
            SequenceType sequenceType = dlg.GetSequenceType();

            CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
            AZ_Assert(sequenceManager, "Expected valid sequenceManager.");

            CTrackViewSequence* pSequence = sequenceManager->GetSequenceByName(sequenceName);
            if (pSequence)
            {
                throw std::runtime_error("A sequence with this name already exists");
            }

            AzToolsFramework::ScopedUndoBatch undoBatch("Create TrackView Director Node");
            sequenceManager->CreateSequence(sequenceName, sequenceType);
            CTrackViewSequence* newSequence = sequenceManager->GetSequenceByName(sequenceName);

            if (!newSequence)
            {
                return;
            }
            
            undoBatch.MarkEntityDirty(newSequence->GetSequenceComponentEntityId());

            // make it the currently selected sequence
            CAnimationContext* animationContext = GetIEditor()->GetAnimation();
            animationContext->SetSequence(newSequence, true, false);
        }
    }
}

void CTrackViewDialog::ReloadSequences()
{
    IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
    if (!movieSystem || m_bIgnoreUpdates || m_bDoingUndoOperation)
    {
        return;
    }

    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* sequence = pAnimationContext->GetSequence();
    CTrackViewSequenceNoNotificationContext context(sequence);

    if (sequence)
    {
        // In case a sequence was previously selected in this Editor session, - restore the selection, selecting this in ReloadSequencesComboBox().
        m_currentSequenceEntityId = sequence->GetSequenceComponentEntityId();
        sequence->UnBindFromEditorObjects();
    }

    ClearTracksToolBar();

    if (pAnimationContext->IsPlaying())
    {
        pAnimationContext->SetPlaying(false);
    }

    ReloadSequencesComboBox();

    if (m_currentSequenceEntityId.IsValid()) // A sequence force-selected when reloading the combo box ?
    {
        OnSequenceComboBox(); // Emulate sequence selection to load it into the dialog
        sequence = pAnimationContext->GetSequence(); // In case a latest sequence created was selected, actualize the pointer.
    }
    else // No sequences yet
    {
        pAnimationContext->SetSequence(nullptr, true, false);
        m_sequencesComboBox->setCurrentIndex(0);
    }

    if (sequence && !sequence->IsBoundToEditorObjects())
    {
        sequence->BindToEditorObjects();
    }

    pAnimationContext->ForceAnimation();

    UpdateSequenceLockStatus();
    UpdateActions();
}

void CTrackViewDialog::ReloadSequencesComboBox()
{
    m_sequencesComboBox->blockSignals(true);
    m_sequencesComboBox->clear();
    m_sequencesComboBox->addItem(QString(s_kNoSequenceComboBoxEntry));

    AZ::EntityId lastSequenceComponentEntityId;
    int lastIndex = -1;
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        const unsigned int numSequences = pSequenceManager->GetCount();

        for (unsigned int k = 0; k < numSequences; ++k)
        {
            CTrackViewSequence* sequence = pSequenceManager->GetSequenceByIndex(k);
            const auto sequenceComponentEntityId = sequence->GetSequenceComponentEntityId();
            if (!sequenceComponentEntityId.IsValid())
            {
                continue;
            }
            lastIndex = static_cast<int>(k);
            lastSequenceComponentEntityId = sequenceComponentEntityId;
            QString entityIdString = GetEntityIdAsString(sequence->GetSequenceComponentEntityId());
            m_sequencesComboBox->addItem(QString::fromUtf8(sequence->GetName().c_str()), entityIdString);
        }
    }

    if (m_currentSequenceEntityId.IsValid())
    {
        QString entityIdString = GetEntityIdAsString(m_currentSequenceEntityId);
        m_sequencesComboBox->setCurrentIndex(m_sequencesComboBox->findData(entityIdString));
    }
    else if (lastSequenceComponentEntityId.IsValid())
    {
        // Make opening the dialog more user friendly: selecting a sequence probably worked on lately,
        // as sequences, when created, are pushed to back into corresponding container. 
        m_currentSequenceEntityId = lastSequenceComponentEntityId;
        m_sequencesComboBox->setCurrentIndex(lastIndex + 1);
    }
    else
    {
        m_sequencesComboBox->setCurrentIndex(0);
    }
    m_sequencesComboBox->blockSignals(false);
    InvalidateSequence();
}

void CTrackViewDialog::UpdateSequenceLockStatus()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence)
    {
        SetEditLock(true);
    }
    else
    {
        SetEditLock(false);
    }
}

void CTrackViewDialog::SetEditLock(bool bLock)
{
    m_bEditLock = bLock;

    m_wndDopeSheet->SetEditLock(bLock);
    m_wndNodesCtrl->SetEditLock(bLock);
    m_wndNodesCtrl->update();

    m_wndCurveEditor->SetEditLock(bLock);
    m_wndCurveEditor->update();
}

void CTrackViewDialog::OnGameOrSimModeLock(bool lock)
{
    if (lock)
    {
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
        CTrackViewSequence* sequence = sequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);

        // Remove sequence listeners when switching modes to ensure they get removed
        // They will fail to be removed if dialog is closed in sim mode
        RemoveSequenceListeners(sequence);
    }
    else
    {
        // Mark to re-add listeners next frame after the mode switch
        m_needReAddListeners = true;
    }

    SetEditLock(lock);
    m_enteringGameOrSimModeLock = lock;
    m_sequencesComboBox->setDisabled(lock);
    UpdateActions();
}

void CTrackViewDialog::OnDelSequence()
{
    if (m_sequencesComboBox->currentIndex() == 0)
    {
        return;
    }

    if (QMessageBox::question(this, LyViewPane::TrackView, "Delete current sequence?") == QMessageBox::Yes)
    {
        int sel = m_sequencesComboBox->currentIndex();
        if (sel != -1)
        {
            QString entityIdString = m_sequencesComboBox->currentData().toString();
            m_sequencesComboBox->removeItem(sel);
            m_sequencesComboBox->setCurrentIndex(0);

            OnSequenceComboBox();

            if (!entityIdString.isEmpty())
            {
                AZ::EntityId entityId = AZ::EntityId(entityIdString.toULongLong());
                if (entityId.IsValid())
                {
                    CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
                    CTrackViewSequence* pSequence = GetSequenceByEntityIdOrName(pSequenceManager, entityIdString.toUtf8().constData());
                    if (pSequence)
                    {
                        pSequenceManager->DeleteSequence(pSequence);
                    }
                }
            }

            UpdateActions();
        }
    }
}

void CTrackViewDialog::OnEditSequence()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (sequence)
    {
        float fps = m_wndCurveEditor->GetFPS();
        CTVSequenceProps dlg(sequence, fps, this);
        if (dlg.exec() == QDialog::Accepted)
        {
            // Sequence updated.
            ReloadSequences();
        }
        m_wndDopeSheet->update();
        UpdateActions();
    }
}

void CTrackViewDialog::OnSequenceComboBox()
{
    int sel = m_sequencesComboBox->currentIndex();
    if (sel == -1)
    {
        GetIEditor()->GetAnimation()->SetSequence(nullptr, false, false);
        return;
    }
    if (sel == 0)
    {
        GetIEditor()->GetAnimation()->SetSequence(nullptr, false, false, true);
        return;
    }

    // Display current sequence.
    QString entityIdString = m_sequencesComboBox->currentData().toString();
    const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
    CTrackViewSequence* sequence = GetSequenceByEntityIdOrName(sequenceManager, entityIdString.toUtf8().constData());
    CAnimationContext* animationContext = GetIEditor()->GetAnimation();
    if (sequence && animationContext)
    {
        const bool force = false;
        const bool noNotify = false;
        const bool user = true;
        animationContext->SetSequence(sequence, force, noNotify, user);
        InvalidateSequence(); // Force later update.
    }
}

void CTrackViewDialog::OnSequenceChanged(CTrackViewSequence* sequence)
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    // Remove listeners from previous sequence
    CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
    CTrackViewSequence* prevSequence = sequenceManager->GetSequenceByEntityId(m_currentSequenceEntityId);
    RemoveSequenceListeners(prevSequence);

    if (sequence)
    {
        m_currentSequenceEntityId = sequence->GetSequenceComponentEntityId();

        sequence->Reset(true);

        UpdateDopeSheetTime(sequence);

        m_sequencesComboBox->blockSignals(true);
        QString entityIdString = GetEntityIdAsString(m_currentSequenceEntityId);
        int sequenceIndex = m_sequencesComboBox->findData(entityIdString);
        m_sequencesComboBox->setCurrentIndex(sequenceIndex);
        m_sequencesComboBox->blockSignals(false);

        sequence->ClearSelection();

        AddSequenceListeners(sequence);
    }
    else
    {
        m_currentSequenceEntityId.SetInvalid();
        m_sequencesComboBox->setCurrentIndex(0);
        m_wndCurveEditor->GetSplineCtrl().SetEditLock(true);
    }

    m_wndNodesCtrl->OnSequenceChanged();
    m_wndKeyProperties->OnSequenceChanged(sequence);

    ClearTracksToolBar();

    GetIEditor()->GetAnimation()->ForceAnimation();

    m_wndNodesCtrl->update();
    m_wndDopeSheet->update();

    UpdateSequenceLockStatus();
    UpdateTracksToolBar();
    UpdateActions();
}

void CTrackViewDialog::OnRecord()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetRecording(!pAnimationContext->IsRecording());
    m_wndDopeSheet->update();
    UpdateActions();
}

void CTrackViewDialog::OnAutoRecord()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetAutoRecording(!pAnimationContext->IsRecording(), m_fAutoRecordStep);
    m_wndDopeSheet->update();
    UpdateActions();
}

void CTrackViewDialog::OnAutoRecordStep()
{
    QAction* action = static_cast<QAction*>(sender());
    int factor = action->data().toInt();
    m_fAutoRecordStep = 1.f / factor;
}

void CTrackViewDialog::OnGoToStart()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    const float startTime = pAnimationContext->GetMarkers().start;

    pAnimationContext->SetTime(startTime);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);

    CTrackViewSequence* sequence = pAnimationContext->GetSequence();
    if (sequence)
    {
        // Reset sequence to the beginning.
        sequence->Reset(true);
    }

    // notify explicit time changed and return to playback controls *after* the sequence is reset.
    pAnimationContext->TimeChanged(startTime);
}

void CTrackViewDialog::OnGoToEnd()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().end);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);
}

void CTrackViewDialog::OnPlay()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    bool wasRecordMode = pAnimationContext->IsRecordMode();
    if (!pAnimationContext->IsPlaying())
    {
        CTrackViewSequence* sequence = pAnimationContext->GetSequence();
        if (sequence)
        {
            if (!pAnimationContext->IsAutoRecording())
            {
                if (wasRecordMode)
                {
                    pAnimationContext->SetRecording(false);
                }
            }
            pAnimationContext->SetPlaying(true);
        }
    }
    else
    {
        pAnimationContext->SetPlaying(false);
    }
    UpdateActions();
}

void CTrackViewDialog::OnPlaySetScale()
{
    QAction* action = static_cast<QAction*>(sender());
    float v = action->data().toFloat();
    if (v > 0.f)
    {
        GetIEditor()->GetAnimation()->SetTimeScale(1.f / v);
    }
}

void CTrackViewDialog::OnStop()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();

    if (pAnimationContext->IsPlaying())
    {
        pAnimationContext->SetPlaying(false);
    }
    else
    {
        OnGoToStart();
    }
    pAnimationContext->SetRecording(false);
    UpdateActions();
}

void CTrackViewDialog::OnStopHardReset()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().start);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);

    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        sequence->ResetHard();
    }
    UpdateActions();
}

void CTrackViewDialog::OnPause()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    if (pAnimationContext->IsPaused())
    {
        pAnimationContext->Resume();
    }
    else
    {
        pAnimationContext->Pause();
    }
    UpdateActions();
}

void CTrackViewDialog::OnLoop()
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    pAnimationContext->SetLoopMode(!pAnimationContext->IsLoopMode());
}

void CTrackViewDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginLoad:
    case eNotify_OnBeginSceneSave:
        m_bIgnoreUpdates = true;
        break;
    case eNotify_OnBeginGameMode:
        OnGameOrSimModeLock(true);
        m_bIgnoreUpdates = true;
        break;
    case eNotify_OnEndNewScene:
    case eNotify_OnEndLoad:
        m_bIgnoreUpdates = false;
        ReloadSequences();
        break;
    case eNotify_OnEndSceneSave:
        m_bIgnoreUpdates = false;
        break;
    case eNotify_OnEndGameMode:
        m_bIgnoreUpdates = false;
        OnGameOrSimModeLock(false);
        break;
    case eNotify_OnReloadTrackView:
        if (!m_bIgnoreUpdates)
        {
            ReloadSequences();
        }
        break;
    case eNotify_OnIdleUpdate:
        if (!m_bIgnoreUpdates)
        {
            Update();
        }
        break;
    case eNotify_OnBeginSimulationMode:
        OnGameOrSimModeLock(true);
        break;
    case eNotify_OnEndSimulationMode:
        OnGameOrSimModeLock(false);
        break;
    case eNotify_OnSelectionChange:
        UpdateActions();
        break;
    case eNotify_OnQuit:
        SaveLayouts();
        SaveMiscSettings();
        SaveTrackColors();
        break;
    }
}

void CTrackViewDialog::OnAddSelectedNode()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (sequence)
    {
        // Try to paste to a selected group node, otherwise to sequence
        CTrackViewAnimNodeBundle selectedNodes = sequence->GetSelectedAnimNodes();
        CTrackViewAnimNode* animNode = (selectedNodes.GetCount() == 1) ? selectedNodes.GetNode(0) : sequence;
        animNode = (animNode->IsGroupNode() && animNode->GetType() != AnimNodeType::AzEntity) ? animNode : sequence;

        AzToolsFramework::ScopedUndoBatch undoBatch("Add Entities to Track View");
        CTrackViewAnimNodeBundle addedNodes = animNode->AddSelectedEntities(m_defaultTracksForEntityNode);
        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());

        if (addedNodes.GetCount() > 0)
        {
            // mark layer containing sequence as dirty
            sequence->MarkAsModified();
        }

        int selectedEntitiesCount = 0;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntitiesCount, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntitiesCount);

        // check to make sure all nodes were added and notify user if they weren't
        if (addedNodes.GetCount() != static_cast<unsigned int>(selectedEntitiesCount))
        {
            IMovieSystem* movieSystem = AZ::Interface<IMovieSystem>::Get();
            if (movieSystem)
            {
                QMessageBox::information(this, tr("Track View Warning"), tr(movieSystem->GetUserNotificationMsgs().c_str()));

                // clear the notification log now that we've consumed and presented them.
                movieSystem->ClearUserNotificationMsgs();
            }

        }

        UpdateActions();
    }
}

void CTrackViewDialog::OnAddDirectorNode()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (sequence)
    {
        QString name = sequence->GetAvailableNodeNameStartingWith("Director");
        AzToolsFramework::ScopedUndoBatch undoBatch("Create Track View Director Node");
        sequence->CreateSubNode(name, AnimNodeType::Director);
        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());

        UpdateActions();
    }
}

void CTrackViewDialog::OnFindNode()
{
    if (!m_findDlg)
    {
        m_findDlg = new CTrackViewFindDlg("Find Node in Track View", this);
        m_findDlg->Init(this);
        connect(m_findDlg, SIGNAL(finished(int)), m_wndNodesCtrl->findChild<QTreeView*>(), SLOT(setFocus()), Qt::QueuedConnection);
    }
    m_findDlg->FillData();
    m_findDlg->show();
    m_findDlg->raise();
}

void CTrackViewDialog::keyPressEvent(QKeyEvent* event)
{
    // HAVE TO INCLUDE CASES FOR THESE IN THE ShortcutOverride handler in ::event() below
    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::NoModifier)
    {
        event->accept();
        GetIEditor()->GetAnimation()->TogglePlay();
    }
    return QMainWindow::keyPressEvent(event);
}

bool CTrackViewDialog::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride)
    {
        // since we respond to the following things, let Qt know so that shortcuts don't override us
        bool respondsToEvent = false;

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Space && keyEvent->modifiers() == Qt::NoModifier)
        {
            respondsToEvent = true;
        }

        if (respondsToEvent)
        {
            e->accept();
            return true;
        }
    }

    return QMainWindow::event(e);
}

#if defined(AZ_PLATFORM_WINDOWS)
bool CTrackViewDialog::nativeEvent(const QByteArray &eventType, void *message, [[maybe_unused]] long *result)
{
    /* On Windows, eventType is set to "windows_generic_MSG" for messages sent to toplevel windows, and "windows_dispatcher_MSG" for
    system - wide messages such as messages from a registered hot key.In both cases, the message can be casted to a MSG pointer.
    The result pointer is only used on Windows, and corresponds to the LRESULT pointer.*/

    if (eventType == "windows_generic_MSG") {
        MSG* pMsg = static_cast<MSG*>(message);
        return processRawInput(pMsg);
    }

    return false;
}

bool CTrackViewDialog::processRawInput(MSG* pMsg)
{
    if (pMsg->message == WM_INPUT)
    {
        static C3DConnexionDriver* p3DConnexionDriver = 0;

        if (!p3DConnexionDriver)
        {
            p3DConnexionDriver = (C3DConnexionDriver*)GetIEditor()->GetPluginManager()->GetPluginByGUID("{AD109901-9128-4ffd-8E67-137CB2B1C41B}");
        }
        if (p3DConnexionDriver)
        {
            S3DConnexionMessage msg;
            if (p3DConnexionDriver->GetInputMessageData(pMsg->lParam, msg))
            {
                if (msg.bGotRotation)
                {
                    float fTime = GetIEditor()->GetAnimation()->GetTime();
                    float fDelta2 = msg.vRotate[2] * 0.1f;
                    fTime += fDelta2;

                    GetIEditor()->GetAnimation()->SetTime(fTime);
                    return true;
                }
            }
        }
    }
    return false;
}
#endif

void CTrackViewDialog::OnModeDopeSheet()
{
    auto sizes = m_wndSplitter->sizes();
    m_wndCurveEditorDock->setVisible(false);
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(false);
    if (m_wndCurveEditorDock->widget() != m_wndCurveEditor)
    {
        m_wndCurveEditorDock->setWidget(m_wndCurveEditor);
    }
    m_wndDopeSheet->show();
    m_wndSplitter->setSizes(sizes);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(true);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(false);
    m_wndCurveEditor->OnSequenceChanged(GetIEditor()->GetAnimation()->GetSequence());
    m_lastMode = ViewMode::TrackView;
}

void CTrackViewDialog::OnModeCurveEditor()
{
    auto sizes = m_wndSplitter->sizes();
    m_wndCurveEditorDock->setVisible(false);
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(false);
    if (m_wndCurveEditorDock->widget() == m_wndCurveEditor)
    {
        m_wndSplitter->insertWidget(1, m_wndCurveEditor);
    }
    m_wndDopeSheet->hide();
    m_wndSplitter->setSizes(sizes);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(false);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(true);
    m_wndCurveEditor->OnSequenceChanged(GetIEditor()->GetAnimation()->GetSequence());
    m_lastMode = ViewMode::CurveEditor;
}

void CTrackViewDialog::OnOpenCurveEditor()
{
    OnModeDopeSheet();
    m_wndCurveEditorDock->show();
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(true);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(true);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(true);
    m_wndCurveEditor->OnSequenceChanged(GetIEditor()->GetAnimation()->GetSequence());
    m_lastMode = ViewMode::Both;
}

void CTrackViewDialog::OnSnapNone()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapNone);
}

void CTrackViewDialog::OnSnapMagnet()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapMagnet);
}

void CTrackViewDialog::OnSnapFrame()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapFrame);
}

void CTrackViewDialog::OnSnapTick()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapTick);
}

void CTrackViewDialog::OnSnapFPS()
{
    int fps = FloatToIntRet(m_wndCurveEditor->GetFPS());
    bool ok = false;
    fps = QInputDialog::getInt(this, tr("Frame rate for frame snapping"), QStringLiteral(""), fps, s_kMinimumFrameSnappingFPS, s_kMaximumFrameSnappingFPS, 1, &ok);
    if (ok)
    {
        m_wndDopeSheet->SetSnapFPS(fps);
        m_wndCurveEditor->SetFPS(static_cast<float>(fps));

        SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
    }
}

void CTrackViewDialog::OnViewTickInSeconds()
{
    m_wndDopeSheet->SetTickDisplayMode(eTVTickMode_InSeconds);
    m_wndCurveEditor->SetTickDisplayMode(eTVTickMode_InSeconds);
    SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
    UpdateActions();
}

void CTrackViewDialog::OnViewTickInFrames()
{
    m_wndDopeSheet->SetTickDisplayMode(eTVTickMode_InFrames);
    m_wndCurveEditor->SetTickDisplayMode(eTVTickMode_InFrames);
    SetCursorPosText(GetIEditor()->GetAnimation()->GetTime());
    UpdateActions();
}

void CTrackViewDialog::SaveMiscSettings() const
{
    QSettings settings;
    settings.beginGroup(s_kTrackViewSettingsSection);
    settings.setValue(s_kSnappingModeEntry, static_cast<int>(m_wndDopeSheet->GetSnappingMode()));
    float fps = m_wndCurveEditor->GetFPS();
    settings.setValue(s_kFrameSnappingFPSEntry, fps);
    settings.setValue(s_kTickDisplayModeEntry, static_cast<int>(m_wndDopeSheet->GetTickDisplayMode()));
    settings.setValue(s_kDefaultTracksEntry, QByteArray(reinterpret_cast<const char*>(m_defaultTracksForEntityNode.data()),
        static_cast<int>(m_defaultTracksForEntityNode.size() * sizeof(AnimParamType))));
}

void CTrackViewDialog::ReadMiscSettings()
{
    QSettings settings;
    settings.beginGroup(s_kTrackViewSettingsSection);
    ESnappingMode snapMode = (ESnappingMode)(settings.value(s_kSnappingModeEntry, eSnappingMode_SnapNone).toInt());
    m_wndDopeSheet->SetSnappingMode(snapMode);
    if (snapMode == eSnappingMode_SnapNone)
    {
        m_actions[ID_TV_SNAP_NONE]->setChecked(true);
    }
    else if (snapMode == eSnappingMode_SnapMagnet)
    {
        m_actions[ID_TV_SNAP_MAGNET]->setChecked(true);
    }
    else if (snapMode == eSnappingMode_SnapTick)
    {
        m_actions[ID_TV_SNAP_TICK]->setChecked(true);
    }
    else if (snapMode == eSnappingMode_SnapFrame)
    {
        m_actions[ID_TV_SNAP_FRAME]->setChecked(true);
    }

    if (settings.contains(s_kFrameSnappingFPSEntry))
    {
        float fps = settings.value(s_kFrameSnappingFPSEntry).toFloat();
        if (fps >= s_kMinimumFrameSnappingFPS && fps <= s_kMaximumFrameSnappingFPS)
        {
            m_wndDopeSheet->SetSnapFPS(FloatToIntRet(fps));
            m_wndCurveEditor->SetFPS(fps);
        }
    }

    ETVTickMode tickMode = (ETVTickMode)(settings.value(s_kTickDisplayModeEntry, eTVTickMode_InSeconds).toInt());
    m_wndDopeSheet->SetTickDisplayMode(tickMode);
    m_wndCurveEditor->SetTickDisplayMode(tickMode);

    if (settings.contains(s_kDefaultTracksEntry))
    {
        const QByteArray ba = settings.value(s_kDefaultTracksEntry).toByteArray();
        m_defaultTracksForEntityNode.clear();
        for (int x = 0; x < ba.size() / sizeof(AnimParamType); x++)
        {
            AnimParamType track = ((AnimParamType*)(ba.data()))[x];
            m_defaultTracksForEntityNode.push_back(track);
        }
    }
}

void CTrackViewDialog::SaveLayouts()
{
    QSettings settings("O3DE", "O3DE");
    settings.beginGroup("TrackView");
    QByteArray stateData = this->saveState();
    settings.setValue("layout", stateData);
    settings.setValue("lastViewMode", (int)m_lastMode);
    QStringList sl;
    foreach(int i, m_wndSplitter->sizes())
    sl << QString::number(i);
    settings.setValue("splitter", sl.join(","));
    settings.endGroup();
    settings.sync();
}

void CTrackViewDialog::ReadLayouts()
{
    QSettings settings("O3DE", "O3DE");
    settings.beginGroup("TrackView");

    setViewMode(static_cast<ViewMode>(settings.value("lastViewMode").toInt()));

    if (settings.contains("layout"))
    {
        QByteArray layoutData = settings.value("layout").toByteArray();
        if (!layoutData.isEmpty())
        {
            restoreState(layoutData);
        }
    }
    if (settings.contains("splitter"))
    {
        const QStringList sl = settings.value("splitter").toString().split(",");
        QList<int> szl;
        szl.reserve(sl.size());
        for (const QString& s : sl)
        {
            szl << s.toInt();
        }
        if (!sl.isEmpty())
        {
            m_wndSplitter->setSizes(szl);
        }
    }
}

void CTrackViewDialog::setViewMode(ViewMode mode)
{
    switch (mode)
    {
    case ViewMode::TrackView:
        OnModeDopeSheet();
        break;
    case ViewMode::CurveEditor:
        OnModeCurveEditor();
        break;
    case ViewMode::Both:
        OnOpenCurveEditor();
        break;
    }
}

void CTrackViewDialog::SaveTrackColors() const
{
    CTVCustomizeTrackColorsDlg::SaveColors(s_kTrackViewSettingsSection);
}

void CTrackViewDialog::ReadTrackColors()
{
    CTVCustomizeTrackColorsDlg::LoadColors(s_kTrackViewSettingsSection);
}

void CTrackViewDialog::SetCursorPosText(float fTime)
{
    QString sText;
    int fps = FloatToIntRet(m_wndCurveEditor->GetFPS());
    int nMins = int ( fTime / 60.0f );
    int nSecs = int (fTime - float ( nMins ) * 60.0f);
    int nFrames = fps > 0 ? int(fTime * m_wndCurveEditor->GetFPS()) % fps : 0;

    sText = QString("%1:%2:%3 (%4fps)").arg(nMins).arg(nSecs, 2, 10, QLatin1Char('0')).arg(nFrames, 2, 10, QLatin1Char('0')).arg(fps);
    m_cursorPos->setText(sText);
}

void CTrackViewDialog::OnCustomizeTrackColors()
{
    CTVCustomizeTrackColorsDlg dlg(this);
    dlg.exec();
}

void CTrackViewDialog::OnBatchRender()
{
    CSequenceBatchRenderDialog dlg(m_wndCurveEditor->GetFPS(), this);
    dlg.exec();
}

void CTrackViewDialog::UpdateTracksToolBar()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        ClearTracksToolBar();

        CTrackViewAnimNodeBundle selectedNodes = sequence->GetSelectedAnimNodes();

        if (selectedNodes.GetCount() == 1)
        {
            CTrackViewAnimNode* pAnimNode = selectedNodes.GetNode(0);
            SetNodeForTracksToolBar(pAnimNode);

            const AnimNodeType nodeType = pAnimNode->GetType();
            int paramCount = 0;
            IAnimNode::AnimParamInfos animatableProperties;
            CTrackViewNode* parentNode = pAnimNode->GetParentNode();

            // all AZ::Entity entities are animated through components. Component nodes always have a parent - the containing AZ::Entity
            if (nodeType == AnimNodeType::Component && parentNode)
            {
                // component node - query all the animatable tracks via an EBus request

                // all AnimNodeType::Component are parented to AnimNodeType::AzEntityNodes - get the parent to get it's AZ::EntityId to use for the EBus request
                if (parentNode->GetNodeType() == eTVNT_AnimNode)
                {
                    // this cast is safe because we check that the type is eTVNT_AnimNode
                    const AZ::EntityId azEntityId = static_cast<CTrackViewAnimNode*>(parentNode)->GetAzEntityId();

                    // query the animatable component properties from the Sequence Component
                    Maestro::EditorSequenceComponentRequestBus::Event(const_cast<CTrackViewAnimNode*>(pAnimNode)->GetSequence()->GetSequenceComponentEntityId(),
                        &Maestro::EditorSequenceComponentRequestBus::Events::GetAllAnimatablePropertiesForComponent,
                        animatableProperties, azEntityId, pAnimNode->GetComponentId());

                    paramCount = static_cast<int>(animatableProperties.size());
                }
            }
            else
            {
                // legacy Entity
                paramCount = pAnimNode->GetParamCount();
            }

            for (int i = 0; i < paramCount; ++i)
            {
                CAnimParamType paramType;
                QString name;

                // get the animatable param name
                if (nodeType == AnimNodeType::Component)
                {
                    paramType = animatableProperties[i].paramType;

                    // Skip over any hidden params
                    if (animatableProperties[i].flags & IAnimNode::ESupportedParamFlags::eSupportedParamFlags_Hidden)
                    {
                        continue;
                    }
                }
                else
                {

                    paramType = pAnimNode->GetParamType(i);

                    if (paramType.GetType() == AnimParamType::Invalid)
                    {
                        continue;
                    }
                }

                CTrackViewTrack* pTrack = pAnimNode->GetTrackForParameter(paramType);
                if (pTrack && !(pAnimNode->GetParamFlags(paramType) & IAnimNode::eSupportedParamFlags_MultipleTracks))
                {
                    continue;
                }

                name = QString::fromUtf8(pAnimNode->GetParamName(paramType).c_str());

                QString sToolTipText("Add " + name + " Track");
                QIcon hIcon = m_wndNodesCtrl->GetIconForTrack(pTrack);
                AddButtonToTracksToolBar(paramType, hIcon, sToolTipText);
            }
        }
    }
}

void CTrackViewDialog::ClearTracksToolBar()
{
    m_tracksToolBar->clear();
    m_tracksToolBar->addWidget(new QLabel("Tracks:"));

    m_pNodeForTracksToolBar = nullptr;
    m_toolBarParamTypes.clear();
    m_currentToolBarParamTypeId = 0;
}

void CTrackViewDialog::AddButtonToTracksToolBar(const CAnimParamType& paramId, const QIcon& hIcon, const QString& title)
{
    const int paramTypeToolBarID = ID_TV_TRACKS_TOOLBAR_BASE + m_currentToolBarParamTypeId;
    if (paramTypeToolBarID <= ID_TV_TRACKS_TOOLBAR_LAST)
    {
        m_toolBarParamTypes.push_back(paramId);
        ++m_currentToolBarParamTypeId;

        QAction* a = m_tracksToolBar->addAction(hIcon, title);
        a->setData(paramTypeToolBarID);
        connect(a, &QAction::triggered, this, &CTrackViewDialog::OnTracksToolBar);
    }
}

void CTrackViewDialog::OnTracksToolBar()
{
    QAction* action = static_cast<QAction*>(sender());
    int nID = action->data().toInt();
    const unsigned int paramTypeToolBarID = nID - ID_TV_TRACKS_TOOLBAR_BASE;

    if (paramTypeToolBarID < m_toolBarParamTypes.size())
    {
        if (m_pNodeForTracksToolBar && m_toolBarParamTypes[paramTypeToolBarID].GetType() != AnimParamType::Invalid)
        {
            CTrackViewSequence* sequence = m_pNodeForTracksToolBar->GetSequence();
            AZ_Assert(sequence, "Expected valid sequence");

            if (sequence)
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Add Track via Toolbar");
                m_pNodeForTracksToolBar->CreateTrack(m_toolBarParamTypes[paramTypeToolBarID]);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

            UpdateTracksToolBar();
        }
    }
}

void CTrackViewDialog::OnToggleDisable()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        CTrackViewAnimNodeBundle selectedNodes = sequence->GetSelectedAnimNodes();
        const unsigned int numSelectedNodes = selectedNodes.GetCount();
        for (unsigned int i = 0; i < numSelectedNodes; ++i)
        {
            CTrackViewAnimNode* pNode = selectedNodes.GetNode(i);
            pNode->SetDisabled(!pNode->IsDisabled());
        }

        CTrackViewTrackBundle selectedTracks = sequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetDisabled(!pTrack->IsDisabled());
        }
        UpdateActions();
    }
}

void CTrackViewDialog::OnToggleMute()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        CTrackViewTrackBundle selectedTracks = sequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(!pTrack->IsMuted());
        }
        UpdateActions();
    }
}

void CTrackViewDialog::OnMuteAll()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        CTrackViewTrackBundle selectedTracks = sequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(true);
        }
        UpdateActions();
    }
}

void CTrackViewDialog::OnUnmuteAll()
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();
    if (sequence)
    {
        CTrackViewTrackBundle selectedTracks = sequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CTrackViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(false);
        }
        UpdateActions();
    }
}

void CTrackViewDialog::OnNodeSelectionChanged(CTrackViewSequence* sequence)
{
    CTrackViewSequence* pCurrentSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pCurrentSequence && pCurrentSequence == sequence)
    {
        UpdateTracksToolBar();
        UpdateActions();
    }
}

void CTrackViewDialog::OnNodeRenamed(CTrackViewNode* pNode, [[maybe_unused]] const char* pOldName)
{
    // React to sequence name changes
    if (pNode->GetNodeType() == eTVNT_Sequence)
    {
        ReloadSequencesComboBox();
    }
}

void CTrackViewDialog::UpdateDopeSheetTime(CTrackViewSequence* sequence)
{
    Range timeRange = sequence->GetTimeRange();
    m_wndDopeSheet->SetTimeRange(timeRange.start, timeRange.end);
    m_wndDopeSheet->SetStartMarker(timeRange.start);
    m_wndDopeSheet->SetEndMarker(timeRange.end);
    m_wndDopeSheet->SetTimeScale(m_wndDopeSheet->GetTimeScale(), 0);
}

void CTrackViewDialog::OnEntityDestruction(const AZ::EntityId& entityId)
{
    if (m_currentSequenceEntityId == entityId)
    {
        // The currently selected sequence is about to be deleted, make sure to clear the selection right now.
        GetIEditor()->GetAnimation()->SetSequence(nullptr, false, false);

        // Refresh the records in m_wndNodesCtrl, the sequence will not be selected in Track View
        // so the current sequence will be nullptr and the records will be cleared preventing
        // dangling pointers.
        m_wndNodesCtrl->OnSequenceChanged();
    }
}

void CTrackViewDialog::OnSequenceSettingsChanged(CTrackViewSequence* sequence)
{
    CTrackViewSequence* pCurrentSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pCurrentSequence && pCurrentSequence == sequence)
    {
        UpdateDopeSheetTime(sequence);
        m_wndNodesCtrl->update();
    }
}

void CTrackViewDialog::OnSequenceAdded([[maybe_unused]] CTrackViewSequence* sequence)
{
    ReloadSequencesComboBox();
    UpdateActions();
}

void CTrackViewDialog::OnSequenceRemoved([[maybe_unused]] CTrackViewSequence* sequence)
{
    ReloadSequencesComboBox();
    UpdateActions();
}

void CTrackViewDialog::AddSequenceListeners(CTrackViewSequence* sequence)
{
    if (sequence)
    {
        sequence->AddListener(this);
        sequence->AddListener(m_wndNodesCtrl);
        sequence->AddListener(m_wndKeyProperties);
        sequence->AddListener(m_wndCurveEditor);
        sequence->AddListener(m_wndDopeSheet);
    }
}

void CTrackViewDialog::RemoveSequenceListeners(CTrackViewSequence* sequence)
{
    if (sequence)
    {
        sequence->RemoveListener(m_wndDopeSheet);
        sequence->RemoveListener(m_wndCurveEditor);
        sequence->RemoveListener(m_wndKeyProperties);
        sequence->RemoveListener(m_wndNodesCtrl);
        sequence->RemoveListener(this);
    }
}

void CTrackViewDialog::AddDialogListeners()
{
    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetAnimation()->AddListener(this);
    GetIEditor()->GetSequenceManager()->AddListener(this);
    GetIEditor()->GetUndoManager()->AddListener(this);
}

void CTrackViewDialog::RemoveDialogListeners()
{
    GetIEditor()->GetUndoManager()->RemoveListener(this);
    GetIEditor()->GetSequenceManager()->RemoveListener(this);
    GetIEditor()->GetAnimation()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

void CTrackViewDialog::BeginUndoTransaction()
{
    m_bDoingUndoOperation = true;
}

void CTrackViewDialog::EndUndoTransaction()
{
    m_bDoingUndoOperation = false;
}

void CTrackViewDialog::AfterEntitySelectionChanged(
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlySelectedEntities,
    [[maybe_unused]] const AzToolsFramework::EntityIdList& newlyDeselectedEntities)
{
    UpdateActions();
}

#include <TrackView/moc_TrackViewDialog.cpp>
