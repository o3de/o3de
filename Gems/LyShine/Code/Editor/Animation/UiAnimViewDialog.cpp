/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


//----- UI_ANIMATION_REVISIT - this is required to compile since something we include still uses MFC

// prevent inclusion of conflicting definitions of INT8_MIN etc
#define _INTSAFE_H_INCLUDED_

// ----- End UI_ANIMATION_REVISIT

#include "EditorDefs.h"
#include "Editor/Resource.h"

#include "UiAnimViewDialog.h"

#include "ViewPane.h"
#include "UiAVSequenceProps.h"
#include "ViewManager.h"
#include "AnimationContext.h"
#include "UiAnimViewFindDlg.h"
#include "UiAnimViewUndo.h"
#include "UiAnimViewAnimNode.h"
#include "UiAnimViewTrack.h"
#include "UiAnimViewSequence.h"
#include "UiAnimViewSequenceManager.h"
#include "UiAVCustomizeTrackColorsDlg.h"
#include "QtUtilWin.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>

#include "PluginManager.h"
#include "Util/3DConnexionDriver.h"
#include "UiAnimViewNewSequenceDialog.h"
#include "UiAnimViewCurveEditor.h"

#include <LyShine/Animation/IUiAnimation.h>

#include "UiAnimViewKeyPropertiesDlg.h"
#include "UiEditorAnimationBus.h"

#include "EditorCommon.h"

#include <QAction>
#include <QComboBox>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTextDocumentFragment>

//////////////////////////////////////////////////////////////////////////
namespace
{
    const char* s_kUiAnimViewSettingsSection = "UiAnimView";
    const char* s_kSnappingModeEntry = "SnappingMode";
    const char* s_kFrameSnappingFPSEntry = "FrameSnappingFPS";
    const char* s_kTickDisplayModeEntry = "TickDisplayMode";

    const char* s_kNoSequenceComboBoxEntry = "--- No Sequence ---";
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewDialog* CUiAnimViewDialog::s_pUiAnimViewDialog = NULL;

//////////////////////////////////////////////////////////////////////////
class CUiAnimViewExpanderWatcher
    : public QObject
{
public:
    CUiAnimViewExpanderWatcher(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool eventFilter(QObject* obj, QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            {
                if (qobject_cast<QToolButton*>(obj))
                {
                    auto mouseEvent = static_cast<QMouseEvent*>(event);
                    auto expansion = qobject_cast<QToolButton*>(obj);

                    expansion->setPopupMode(QToolButton::InstantPopup);
                    auto menu = new QMenu(expansion);

                    auto toolbar = qobject_cast<QToolBar*>(expansion->parentWidget());

                    for (auto toolbarAction : toolbar->actions())
                    {
                        auto actionWidget = toolbar->widgetForAction(toolbarAction);
                        if (actionWidget && !actionWidget->isVisible() && !toolbarAction->text().isEmpty())
                        {
                            QString plainText = QTextDocumentFragment::fromHtml(actionWidget->toolTip()).toPlainText();
                            toolbarAction->setText(plainText);
                            menu->addAction(toolbarAction);
                        }
                    }

                    menu->exec(mouseEvent->globalPos());
                    return true;
                }

                break;
            }
        }

        return QObject::eventFilter(obj, event);
    }
};

//////////////////////////////////////////////////////////////////////////
CUiAnimViewDialog::CUiAnimViewDialog(QWidget* pParent /*=NULL*/)
    : QMainWindow(pParent)
    , m_animationSystem(nullptr)
{
    s_pUiAnimViewDialog = this;
    m_bRecord = false;
    m_bPause = false;
    m_bPlay = false;
    m_fLastTime = -1.0f;
    m_bNeedReloadSequence = false;
    m_bIgnoreUpdates = false;
    m_bDoingUndoOperation = false;

    m_findDlg = nullptr;

    m_lazyInitDone = false;
    m_bEditLock = false;

    m_currentToolBarParamTypeId = 0;
    m_expanderWatcher = new CUiAnimViewExpanderWatcher(this);

    UiEditorAnimationStateBus::Handler::BusConnect();
    UiEditorAnimListenerBus::Handler::BusConnect();

    GetIEditor()->RegisterNotifyListener(this);

    m_sequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
    if (m_sequenceManager)
    {
        m_animationContext = m_sequenceManager->GetAnimationContext();

        m_animationContext->AddListener(this);
        CUiAnimViewSequenceManager::GetSequenceManager()->AddListener(this);
    }
    else
    {
        m_animationContext = nullptr;
    }
    UiAnimUndoManager::Get()->AddListener(this);

    // There may already be a loaded canvas (since UI Editor is a separate window)
    m_animationSystem = CUiAnimViewSequenceManager::GetSequenceManager()->GetAnimationSystem();

    OnInitDialog();

    // update the status of the actions
    UpdateActions();

    if (!m_animationSystem)
    {
        setEnabled(false);
    }
}

CUiAnimViewDialog::~CUiAnimViewDialog()
{
    SaveMiscSettings();
    SaveTrackColors();

    if (m_findDlg)
    {
        m_findDlg->deleteLater();
        m_findDlg = nullptr;
    }
    s_pUiAnimViewDialog = 0;

    const CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
    CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(m_currentSequenceName);
    if (pSequence)
    {
        pSequence->RemoveListener(this);
        pSequence->RemoveListener(m_wndNodesCtrl);
        pSequence->RemoveListener(m_wndKeyProperties);
        pSequence->RemoveListener(m_wndCurveEditor);
        pSequence->RemoveListener(m_wndDopeSheet);
    }

    UiAnimUndoManager::Get()->RemoveListener(this);
    CUiAnimViewSequenceManager::GetSequenceManager()->RemoveListener(this);
    m_animationContext->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);

    UiEditorAnimationStateBus::Handler::BusDisconnect();
    UiEditorAnimListenerBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnAddEntityNodeMenu()
{
    // UI_ANIMATION_REVISIT - is there any need for this function?
}

//////////////////////////////////////////////////////////////////////////
BOOL CUiAnimViewDialog::OnInitDialog()
{
    InitToolbar();
    InitMenu();

    QWidget* w = new QWidget();
    QVBoxLayout* l = new QVBoxLayout;
    l->setMargin(0);

    m_wndSplitter = new QSplitter(w);
    m_wndSplitter->setOrientation(Qt::Horizontal);

    m_wndNodesCtrl = new CUiAnimViewNodesCtrl(this, this);
    m_wndNodesCtrl->SetUiAnimViewDialog(this);

    m_wndDopeSheet = new CUiAnimViewDopeSheetBase(this);
    m_wndDopeSheet->SetTimeRange(0, 20);
    m_wndDopeSheet->SetTimeScale(100, 0);

    m_wndDopeSheet->SetNodesCtrl(m_wndNodesCtrl);
    m_wndNodesCtrl->SetDopeSheet(m_wndDopeSheet);

    m_wndSplitter->addWidget(m_wndNodesCtrl);
    m_wndSplitter->addWidget(m_wndDopeSheet);
    m_wndSplitter->setStretchFactor(0, 1);
    m_wndSplitter->setStretchFactor(1, 10);
    m_wndSplitter->setChildrenCollapsible(false);
    l->addWidget(m_wndSplitter);
    w->setLayout(l);
    setCentralWidget(w);

    m_wndKeyProperties = new CUiAnimViewKeyPropertiesDlg(this);
    m_wndKeyPropertiesDock = new AzQtComponents::StyledDockWidget(this);
    m_wndKeyPropertiesDock->setObjectName("m_wndKeyProperties");
    m_wndKeyPropertiesDock->setWindowTitle("Key");
    m_wndKeyPropertiesDock->setWidget(m_wndKeyProperties);
    addDockWidget(Qt::RightDockWidgetArea, m_wndKeyPropertiesDock);
    m_wndKeyProperties->PopulateVariables();
    m_wndKeyProperties->SetKeysCtrl(m_wndDopeSheet);

    m_wndCurveEditorDock = new AzQtComponents::StyledDockWidget(this);
    m_wndCurveEditorDock->setObjectName("m_wndCurveEditorDock");
    m_wndCurveEditorDock->setWindowTitle("Curve Editor");
    m_wndCurveEditor = new UiAnimViewCurveEditorDialog(this);
    m_wndCurveEditorDock->setWidget(m_wndCurveEditor);
    addDockWidget(Qt::BottomDockWidgetArea, m_wndCurveEditorDock);
    m_wndCurveEditor->SetPlayCallback([this] { OnPlay();
        });

    // In order to prevent the track editor view from collapsing and becoming invisible, we use the
    // minimum size of the curve editor for the track editor as well. Since both editors use the same
    // view widget in the UI animation editor when not in 'Both' mode, the sizes can be identical.
    m_wndDopeSheet->setMinimumSize(m_wndCurveEditor->minimumSizeHint());

    InitSequences();

    m_lazyInitDone = false;

    SetViewMode(ViewMode::TrackView);
    QTimer::singleShot(0, this, SLOT(ReadLayouts()));
    //  ReadLayouts();
    ReadMiscSettings();
    ReadTrackColors();

    QString cursorPosText = QString("0.000(%1fps)").arg(FloatToIntRet(m_wndCurveEditor->GetFPS()));
    m_cursorPos->setText(cursorPosText);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CUiAnimViewDialog::InitToolbar()
{
    m_mainToolBar = addToolBar("Sequence/Node Toolbar");
    m_mainToolBar->setObjectName("m_mainToolBar");
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->addWidget(new QLabel("Sequence/Node:"));
    QAction* qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-00.png"), "Add Sequence");
    qaction->setData(ID_TV_ADD_SEQUENCE);
    m_actions[ID_TV_ADD_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnAddSequence);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-01.png"), "Delete Sequence");
    qaction->setData(ID_TV_DEL_SEQUENCE);
    m_actions[ID_TV_DEL_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnDelSequence);
    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-02.png"), "Edit Sequence");
    qaction->setData(ID_TV_EDIT_SEQUENCE);
    m_actions[ID_TV_EDIT_SEQUENCE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnEditSequence);
    m_sequencesComboBox = new QComboBox(this);
    connect(m_sequencesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSequenceComboBox()));
    m_mainToolBar->addWidget(m_sequencesComboBox);
    m_mainToolBar->addSeparator();

    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-03.png"), "Add Selected Node");
    qaction->setData(ID_ADDNODE);
    m_actions[ID_ADDNODE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnAddSelectedNode);

    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-05.png"), "Find");
    qaction->setData(ID_FIND);
    m_actions[ID_FIND] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnFindNode);

    qaction = m_mainToolBar->addAction(QIcon(":/Trackview/main/tvmain-06.png"), "Toggle Disable");
    qaction->setCheckable(true);
    qaction->setData(ID_TRACKVIEW_TOGGLE_DISABLE);
    m_actions[ID_TRACKVIEW_TOGGLE_DISABLE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnToggleDisable);

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_mainToolBar))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

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
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnModeDopeSheet);
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-01.png"), "Curve Editor");
    qaction->setData(ID_TV_MODE_CURVEEDITOR);
    qaction->setShortcut(QKeySequence("Ctrl+R"));
    qaction->setCheckable(true);
    m_actions[ID_TV_MODE_CURVEEDITOR] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnModeCurveEditor);
    qaction = m_viewToolBar->addAction(QIcon(":/Trackview/view/tvview-02.png"), "Both");
    qaction->setData(ID_TV_MODE_OPENCURVEEDITOR);
    qaction->setShortcut(QKeySequence("Ctrl+B"));
    m_actions[ID_TV_MODE_OPENCURVEEDITOR] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnOpenCurveEditor);

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_viewToolBar))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    m_playToolBar = addToolBar("Play Toolbar");
    m_playToolBar->setObjectName("m_playToolBar");
    m_playToolBar->setFloatable(false);
    m_playToolBar->addWidget(new QLabel("Play:"));
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-00.png"), "Go to start of sequence");
    qaction->setData(ID_TV_JUMPSTART);
    m_actions[ID_TV_JUMPSTART] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnGoToStart);

    QToolButton* toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/play/tvplay-01.png"), "Play Animation", this);
    qaction->setData(ID_TV_PLAY);
    qaction->setCheckable(true);
    m_actions[ID_TV_PLAY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnPlay);
    qaction->setShortcut(QKeySequence(Qt::Key_Space));
    qaction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
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
            connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnPlaySetScale);
            qaction->setCheckable(true);
            qaction->setChecked(i == 1.);
            ag->addAction(qaction);
        }
        buttonMenu->addSeparator();
    }
    m_playToolBar->addWidget(toolButton);

    toolButton = new QToolButton(m_playToolBar);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    qaction = new QAction(QIcon(":/Trackview/play/tvplay-02.png"), "Stop", this);
    qaction->setData(ID_TV_STOP);
    m_actions[ID_TV_STOP] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnStop);
    toolButton->setDefaultAction(qaction);
    {
        QMenu* buttonMenu = new QMenu(this);
        toolButton->setMenu(buttonMenu);
        qaction = buttonMenu->addAction("Stop");
        connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnStop);
        toolButton->addAction(qaction);
        qaction = buttonMenu->addAction("Stop with Hard Reset");
        qaction->setData(true);
        connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnStopHardReset);
    }
    m_playToolBar->addWidget(toolButton);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-03.png"), "Pause");
    qaction->setData(ID_TV_PAUSE);
    qaction->setCheckable(true);
    m_actions[ID_TV_PAUSE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnPause);
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-04.png"), "Go to end of sequence");
    qaction->setData(ID_TV_JUMPEND);
    m_actions[ID_TV_JUMPEND] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnGoToEnd);

    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-05.png"), "Start Animation Recording");
    qaction->setData(ID_TV_RECORD);
    qaction->setCheckable(true);
    m_actions[ID_TV_RECORD] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnRecord);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-07.png"), "Loop");
    qaction->setData(ID_PLAY_LOOP);
    qaction->setCheckable(true);
    m_actions[ID_PLAY_LOOP] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnLoop);

    m_playToolBar->addSeparator();
    m_cursorPos = new QLabel(this);
    m_playToolBar->addWidget(m_cursorPos);

    m_playToolBar->addSeparator();
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-09.png"), "Undo");
    qaction->setData(ID_UNDO);
    m_actions[ID_UNDO] = qaction;
    connect(qaction, &QAction::triggered, this, []()
        {
            UiAnimUndoManager::Get()->Undo();
        });
    qaction = m_playToolBar->addAction(QIcon(":/Trackview/play/tvplay-10.png"), "Redo");
    qaction->setData(ID_REDO);
    m_actions[ID_REDO] = qaction;
    connect(qaction, &QAction::triggered, this, []()
        {
            UiAnimUndoManager::Get()->Redo();
        });

    addToolBarBreak(Qt::TopToolBarArea);

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_playToolBar))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    m_keysToolBar = addToolBar("Keys Toolbar");
    m_keysToolBar->setObjectName("m_keysToolBar");
    m_keysToolBar->setFloatable(false);
    m_keysToolBar->addWidget(new QLabel("Keys:"));
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-00.png"), "Go to previous key");
    qaction->setData(ID_TV_PREVKEY);
    m_actions[ID_TV_PREVKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnGoToPrevKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-01.png"), "Go to next key");
    qaction->setData(ID_TV_NEXTKEY);
    m_actions[ID_TV_NEXTKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnGoToNextKey);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-02.png"), "Move Keys");
    qaction->setData(ID_TV_MOVEKEY);
    m_actions[ID_TV_MOVEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnMoveKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-03.png"), "Slide Keys");
    qaction->setData(ID_TV_SLIDEKEY);
    m_actions[ID_TV_SLIDEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnSlideKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-04.png"), "Scale Keys");
    qaction->setData(ID_TV_SCALEKEY);
    m_actions[ID_TV_SCALEKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnScaleKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-05.png"), "Add Keys");
    qaction->setData(ID_TV_ADDKEY);
    m_actions[ID_TV_ADDKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnAddKey);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-06.png"), "Delete Keys");
    qaction->setData(ID_TV_DELKEY);
    m_actions[ID_TV_DELKEY] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnDelKey);
    m_keysToolBar->addSeparator();
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-07.png"), "No Snapping");
    qaction->setData(ID_TV_SNAP_NONE);
    m_actions[ID_TV_SNAP_NONE] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnSnapNone);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-08.png"), "Magnet Snapping");
    qaction->setData(ID_TV_SNAP_MAGNET);
    m_actions[ID_TV_SNAP_MAGNET] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnSnapMagnet);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-09.png"), "Frame Snapping");
    qaction->setData(ID_TV_SNAP_FRAME);
    m_actions[ID_TV_SNAP_FRAME] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnSnapFrame);
    qaction = m_keysToolBar->addAction(QIcon(":/Trackview/keys/tvkeys-10.png"), "Tick Snapping");
    qaction->setData(ID_TV_SNAP_TICK);
    m_actions[ID_TV_SNAP_TICK] = qaction;
    connect(qaction, &QAction::triggered, this, &CUiAnimViewDialog::OnSnapTick);

    if (QToolButton* expansion = AzQtComponents::ToolBar::getToolBarExpansionButton(m_keysToolBar))
    {
        expansion->installEventFilter(m_expanderWatcher);
    }

    QActionGroup* ag = new QActionGroup(this);
    ag->addAction(m_actions[ID_TV_ADDKEY]);
    ag->addAction(m_actions[ID_TV_MOVEKEY]);
    ag->addAction(m_actions[ID_TV_SLIDEKEY]);
    ag->addAction(m_actions[ID_TV_SCALEKEY]);
    foreach(QAction* qaction2, ag->actions())
    {
        qaction2->setCheckable(true);
    }
    m_actions[ID_TV_MOVEKEY]->setChecked(true);
    ag = new QActionGroup(this);
    ag->addAction(m_actions[ID_TV_SNAP_NONE]);
    ag->addAction(m_actions[ID_TV_SNAP_MAGNET]);
    ag->addAction(m_actions[ID_TV_SNAP_FRAME]);
    ag->addAction(m_actions[ID_TV_SNAP_TICK]);
    foreach(QAction* qaction2, ag->actions())
    {
        qaction2->setCheckable(true);
    }
    m_actions[ID_TV_SNAP_NONE]->setChecked(true);

    m_bRecord = false;
    m_bPause = false;
    m_bPlay = false;
}

void CUiAnimViewDialog::InitMenu()
{
    QMenuBar* mb = this->menuBar();

    QMenu* m = mb->addMenu("&Sequence");
    QAction* a = m->addAction("New Sequence...");
    a->setData(ID_TV_SEQUENCE_NEW);
    m_actions[ID_TV_SEQUENCE_NEW] = a;
    connect(a, &QAction::triggered, this, &CUiAnimViewDialog::OnAddSequence);

    m = mb->addMenu("&View");
    m->addAction(m_actions[ID_TV_MODE_DOPESHEET]);
    m->addAction(m_actions[ID_TV_MODE_CURVEEDITOR]);
    m->addAction(m_actions[ID_TV_MODE_OPENCURVEEDITOR]);
    m->addSeparator();
    a = m->addAction("Tick in Seconds");
    a->setData(ID_VIEW_TICKINSECONDS);
    a->setCheckable(true);
    m_actions[ID_VIEW_TICKINSECONDS] = a;
    connect(a, &QAction::triggered, this, &CUiAnimViewDialog::OnViewTickInSeconds);
    a = m->addAction("Tick in Frames");
    a->setData(ID_VIEW_TICKINFRAMES);
    a->setCheckable(true);
    m_actions[ID_VIEW_TICKINFRAMES] = a;
    connect(a, &QAction::triggered, this, &CUiAnimViewDialog::OnViewTickInFrames);

#if UI_ANIMATION_REMOVED
    // This dialog makes no sense while we only support component property tracks
    // if we add support for event tracks it might make sense
    // Currently we do not save the customized track colors
    m = mb->addMenu("T&ools");
    a = m->addAction("Customize &Track Colors...");
    a->setData(ID_TV_TOOLS_CUSTOMIZETRACKCOLORS);
    m_actions[ID_TV_TOOLS_CUSTOMIZETRACKCOLORS] = a;
    connect(a, &QAction::triggered, this, &CUiAnimViewDialog::OnCustomizeTrackColors);
#endif
}

void CUiAnimViewDialog::UpdateActions()
{
    if (m_actions.empty())
    {
        return;
    }

    if (m_animationContext->IsRecordMode())
    {
        m_actions[ID_TV_RECORD]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_RECORD]->setChecked(false);
    }
    if (m_animationContext->IsPlayMode())
    {
        m_actions[ID_TV_PLAY]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_PLAY]->setChecked(false);
    }
    if (m_animationContext->IsPaused())
    {
        m_actions[ID_TV_PAUSE]->setChecked(true);
    }
    else
    {
        m_actions[ID_TV_PAUSE]->setChecked(false);
    }
    if (m_animationContext->IsLoopMode())
    {
        m_actions[ID_PLAY_LOOP]->setChecked(true);
    }
    else
    {
        m_actions[ID_PLAY_LOOP]->setChecked(false);
    }
    if (m_wndDopeSheet->GetTickDisplayMode() == eUiAVTickMode_InSeconds)
    {
        m_actions[ID_VIEW_TICKINSECONDS]->setChecked(true);
    }
    else
    {
        m_actions[ID_VIEW_TICKINSECONDS]->setChecked(false);
    }
    if (m_wndDopeSheet->GetTickDisplayMode() == eUiAVTickMode_InFrames)
    {
        m_actions[ID_VIEW_TICKINFRAMES]->setChecked(true);
    }
    else
    {
        m_actions[ID_VIEW_TICKINFRAMES]->setChecked(false);
    }

    m_actions[ID_TV_DEL_SEQUENCE]->setEnabled(m_bEditLock ? false : true);

    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();
    if (pSequence)
    {
        bool bLightAnimationSetActive = (m_currentSequenceName == LIGHT_ANIMATION_SET_NAME)
            && (pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_LightAnimationSet);

        if (m_bEditLock || bLightAnimationSetActive)
        {
            m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(false);
        }
        else
        {
            m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(true);
        }

        CUiAnimViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();
        CUiAnimViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();

        const unsigned int selectedNodeCount = selectedNodes.GetCount();
        const unsigned int selectedTrackCount = selectedTracks.GetCount();

        bool updated_ID_TRACKVIEW_TOGGLE_DISABLE = false;
        if (selectedNodeCount + selectedTrackCount == 1)
        {
            if (selectedNodeCount == 1)
            {
                CUiAnimViewAnimNode* pAnimNode = selectedNodes.GetNode(0);

                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setChecked(pAnimNode->IsDisabled() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_DISABLE = true;
            }

            if (selectedTrackCount == 1)
            {
                CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(0);

                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(true);
                m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setChecked(pTrack->IsDisabled() ? true : false);
                updated_ID_TRACKVIEW_TOGGLE_DISABLE = true;
            }
        }

        if (!updated_ID_TRACKVIEW_TOGGLE_DISABLE)
        {
            m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(false);
        }

        m_actions[ID_ADDNODE]->setEnabled(true);

        m_actions[ID_TV_PLAY]->setShortcut(QKeySequence(Qt::Key_Space)); // re-enable the shortcut
    }
    else
    {
        m_actions[ID_TV_DEL_SEQUENCE]->setEnabled(false);
        m_actions[ID_TV_EDIT_SEQUENCE]->setEnabled(false);
        m_actions[ID_TRACKVIEW_TOGGLE_DISABLE]->setEnabled(false);
        m_actions[ID_ADDNODE]->setEnabled(false);

        m_actions[ID_TV_PLAY]->setShortcut(QKeySequence()); // clear the shortcut to give parent widgets a chance to handle the same shortcut
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::InitSequences()
{
    ReloadSequences();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::InvalidateSequence()
{
    m_bNeedReloadSequence = true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::InvalidateDopeSheet()
{
    if (m_wndDopeSheet)
    {
        m_wndDopeSheet->update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::Update()
{
    if (m_bNeedReloadSequence)
    {
        m_bNeedReloadSequence = false;
        const CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
        CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(m_currentSequenceName);

        CUiAnimationContext* pAnimationContext = m_animationContext;
        pAnimationContext->SetSequence(pSequence, true, false);
    }

    CUiAnimationContext* pAnimationContext = m_animationContext;
    float fTime = pAnimationContext->GetTime();

    if (fTime != m_fLastTime)
    {
        m_fLastTime = fTime;
        SetCursorPosText(fTime);
    }

    // UI_ANIMATION_REVISIT, render here rather than using pViewport->AddPostRenderer in the animation context
    // there may be a better way to do this
    m_animationContext->OnPostRender();
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnGoToPrevKey()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    CUiAnimViewSequence* pSequence = pAnimationContext->GetSequence();

    if (pSequence)
    {
        float time = pAnimationContext->GetTime();

        CUiAnimViewNode* pNode = pSequence->GetFirstSelectedNode();
        pNode = pNode ? pNode : pSequence;

        if (pNode->SnapTimeToPrevKey(time))
        {
            pAnimationContext->SetTime(time);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnGoToNextKey()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    CUiAnimViewSequence* pSequence = pAnimationContext->GetSequence();

    if (pSequence)
    {
        float time = pAnimationContext->GetTime();

        CUiAnimViewNode* pNode = pSequence->GetFirstSelectedNode();
        pNode = pNode ? pNode : pSequence;

        if (pNode->SnapTimeToNextKey(time))
        {
            pAnimationContext->SetTime(time);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnAddKey()
{
    m_wndDopeSheet->SetMouseActionMode(eUiAVActionMode_AddKeys);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnDelKey()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    CUiAnimViewSequence* pSequence = pAnimationContext->GetSequence();

    if (pSequence)
    {
        UiAnimUndo undo("Delete Keys");
        pSequence->DeleteSelectedKeys();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnMoveKey()
{
    m_wndDopeSheet->SetMouseActionMode(eUiAVActionMode_MoveKey);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSlideKey()
{
    m_wndDopeSheet->SetMouseActionMode(eUiAVActionMode_SlideKey);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnScaleKey()
{
    m_wndDopeSheet->SetMouseActionMode(eUiAVActionMode_ScaleKey);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnAddSequence()
{
    if (!m_animationSystem)
    {
        // No UI canvas is loaded/active so can't do anything
        return;
    }

    CUiAVNewSequenceDialog dlg(this);

    if (dlg.exec() == QDialog::Accepted)
    {
        QString sequenceName = dlg.GetSequenceName();

        if (sequenceName != s_kNoSequenceComboBoxEntry)
        {
            UiAnimUndo undoAddSequence("Add Sequence");
            CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
            {
                CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(sequenceName);
                if (pSequence)
                {
                    AZ_Error("UiAnimViewDialog", false, "A sequence with this name already exists");
                    return;
                }

                UiAnimUndo undo("Create UiAnimView sequence");
                pSequenceManager->CreateSequence(sequenceName);
            }

            CUiAnimViewSequence* pNewSequence = pSequenceManager->GetSequenceByName(sequenceName);

            CUiAnimationContext* pAnimationContext = m_animationContext;
            pAnimationContext->SetSequence(pNewSequence, true, false, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::ReloadSequences()
{
    if (!m_animationSystem || m_bIgnoreUpdates || m_bDoingUndoOperation)
    {
        return;
    }

    CUiAnimationContext* pAnimationContext = m_animationContext;
    CUiAnimViewSequence* pSequence = pAnimationContext->GetSequence();
    CUiAnimViewSequenceNoNotificationContext context(pSequence);

    if (pSequence)
    {
        pSequence->UnBindFromEditorObjects();
    }

    if (pAnimationContext->IsPlaying())
    {
        pAnimationContext->SetPlaying(false);
    }

    ReloadSequencesComboBox();

    SaveZoomScrollSettings();

    if (!m_currentSequenceName.isEmpty())
    {
        CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
        pSequence = pSequenceManager->GetSequenceByName(m_currentSequenceName);

        const float prevTime = pAnimationContext->GetTime();
        pAnimationContext->SetSequence(pSequence, true, true);
        pAnimationContext->SetTime(prevTime);
    }
    else
    {
        pAnimationContext->SetSequence(nullptr, true, false);
        m_sequencesComboBox->setCurrentIndex(0);
    }

    if (pSequence)
    {
        pSequence->BindToEditorObjects();
    }

    pAnimationContext->ForceAnimation();

    UpdateSequenceLockStatus();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::ReloadSequencesComboBox()
{
    m_sequencesComboBox->blockSignals(true);
    m_sequencesComboBox->clear();
    m_sequencesComboBox->addItem(QString(s_kNoSequenceComboBoxEntry));

    {
        CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
        const unsigned int numSequences = pSequenceManager->GetCount();

        for (unsigned int k = 0; k < numSequences; ++k)
        {
            CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByIndex(k);
            QString fullname = QString::fromUtf8(pSequence->GetName().c_str());
            m_sequencesComboBox->addItem(fullname);
        }
    }

    if (m_currentSequenceName.isEmpty())
    {
        m_sequencesComboBox->setCurrentIndex(0);
    }
    else
    {
        m_sequencesComboBox->setCurrentIndex(m_sequencesComboBox->findText(m_currentSequenceName));
    }
    m_sequencesComboBox->blockSignals(false);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::UpdateSequenceLockStatus()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();

    if (!pSequence)
    {
        SetEditLock(true);
    }
    else
    {
        SetEditLock(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SetEditLock(bool bLock)
{
    m_bEditLock = bLock;

    m_wndDopeSheet->SetEditLock(bLock);
    m_wndNodesCtrl->SetEditLock(bLock);
    m_wndNodesCtrl->update();

    m_wndCurveEditor->SetEditLock(bLock);
    m_wndCurveEditor->update();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnDelSequence()
{
    if (m_sequencesComboBox->currentIndex() <= 0)
    {
        return;
    }

    if (QMessageBox::question(this, "UI Animation", "Delete current sequence?") == QMessageBox::Yes)
    {
        int sel = m_sequencesComboBox->currentIndex();
        QString seq = m_sequencesComboBox->currentText();
        m_sequencesComboBox->removeItem(sel);
        m_sequencesComboBox->setCurrentIndex(0);

        OnSequenceComboBox();
        {
            CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
            CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(seq);
            if (pSequence)
            {
                pSequenceManager->DeleteSequence(pSequence);
                return;
            }

            AZ_Error("UiAnimViewDialog", false, "Could not find sequence");
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnEditSequence()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();

    if (pSequence)
    {
        float fps = m_wndCurveEditor->GetFPS();
        CUiAVSequenceProps dlg(pSequence, fps, this);
        if (dlg.exec() == QDialog::Accepted)
        {
            // Sequence updated.
            ReloadSequences();
        }
        m_wndDopeSheet->update();
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSequenceComboBox()
{
    int sel = m_sequencesComboBox->currentIndex();
    if (sel == -1)
    {
        m_animationContext->SetSequence(nullptr, false, false, true);
        return;
    }
    QString name = m_sequencesComboBox->currentText();

    // Display current sequence.
    CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
    CUiAnimViewSequence* pSequence = pSequenceManager->GetSequenceByName(name);

    CUiAnimationContext* pAnimationContext = m_animationContext;
    pAnimationContext->SetSequence(pSequence, false, false, true);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSequenceChanged(CUiAnimViewSequence* pSequence)
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    // Remove listeners from previous sequence
    CUiAnimViewSequenceManager* pSequenceManager = CUiAnimViewSequenceManager::GetSequenceManager();
    CUiAnimViewSequence* pPrevSequence = pSequenceManager->GetSequenceByName(m_currentSequenceName);
    if (pPrevSequence)
    {
        pPrevSequence->RemoveListener(this);
        pPrevSequence->RemoveListener(m_wndNodesCtrl);
        pPrevSequence->RemoveListener(m_wndKeyProperties);
        pPrevSequence->RemoveListener(m_wndCurveEditor);
        pPrevSequence->RemoveListener(m_wndDopeSheet);
    }

    if (pSequence)
    {
        m_currentSequenceName = QString::fromUtf8(pSequence->GetName().c_str());

        pSequence->Reset(true);
        SaveZoomScrollSettings();

        UpdateDopeSheetTime(pSequence);

        m_sequencesComboBox->blockSignals(true);
        m_sequencesComboBox->setCurrentText(m_currentSequenceName);
        m_sequencesComboBox->blockSignals(false);

        pSequence->ClearSelection();

        pSequence->AddListener(this);
        pSequence->AddListener(m_wndNodesCtrl);
        pSequence->AddListener(m_wndKeyProperties);
        pSequence->AddListener(m_wndCurveEditor);
        pSequence->AddListener(m_wndDopeSheet);
    }
    else
    {
        m_currentSequenceName = "";
        m_sequencesComboBox->setCurrentIndex(0);
        m_wndCurveEditor->GetSplineCtrl().SetEditLock(true);
    }

    m_wndNodesCtrl->OnSequenceChanged();
    m_wndKeyProperties->OnSequenceChanged(pSequence);

    m_animationContext->ForceAnimation();

    m_wndNodesCtrl->update();
    m_wndDopeSheet->update();

    UpdateSequenceLockStatus();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnRecord()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    pAnimationContext->SetRecording(!pAnimationContext->IsRecording());
    m_wndDopeSheet->update();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnGoToStart()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().start);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);

    CUiAnimViewSequence* pSequence = pAnimationContext->GetSequence();
    if (pSequence)
    {
        // Reset sequence to the beginning.
        pSequence->Reset(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnGoToEnd()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().end);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnPlay()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    if (!pAnimationContext->IsPlaying())
    {
        CUiAnimViewSequence* pSequence = pAnimationContext->GetSequence();
        if (pSequence)
        {
            {
                CUiAnimationContext* pAnimationContext2 = nullptr;
                UiEditorAnimationBus::BroadcastResult(pAnimationContext2, &UiEditorAnimationBus::Events::GetAnimationContext);
                if (pAnimationContext2->IsPlaying())
                {
                    AZ_Error("UiAnimViewDialog", false, "A sequence is already playing");
                    return;
                }

                pAnimationContext2->SetPlaying(true);
            }
        }
    }
    else
    {
        {
            CUiAnimationContext* pAnimationContext2 = nullptr;
            UiEditorAnimationBus::BroadcastResult(pAnimationContext2, &UiEditorAnimationBus::Events::GetAnimationContext);
            if (!pAnimationContext2->IsPlaying())
            {
                AZ_Error("UiAnimViewDialog", false, "A sequence is playing");
                return;
            }

            pAnimationContext2->SetPlaying(false);
        }
    }
}

void CUiAnimViewDialog::OnPlaySetScale()
{
    QAction* action = static_cast<QAction*>(sender());
    float v = action->data().toFloat();
    if (v > 0.f)
    {
        m_animationContext->SetTimeScale(1.f / v);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnStop()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;

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

void CUiAnimViewDialog::OnStopHardReset()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    pAnimationContext->SetTime(pAnimationContext->GetMarkers().start);
    pAnimationContext->SetPlaying(false);
    pAnimationContext->SetRecording(false);

    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();
    if (pSequence)
    {
        pSequence->ResetHard();
    }
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnPause()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
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

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnLoop()
{
    CUiAnimationContext* pAnimationContext = m_animationContext;
    pAnimationContext->SetLoopMode(!pAnimationContext->IsLoopMode());
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginLoad:
    case eNotify_OnBeginSceneSave:
    case eNotify_OnBeginGameMode:
        m_bIgnoreUpdates = true;
        break;
    case eNotify_OnEndNewScene:
    case eNotify_OnEndLoad:
        m_bIgnoreUpdates = false;
        ReloadSequences();
        break;
    case eNotify_OnEndSceneSave:
    case eNotify_OnEndGameMode:
        m_bIgnoreUpdates = false;
        break;
    case eNotify_OnIdleUpdate:
        if (!m_bIgnoreUpdates)
        {
            Update();
        }
        break;
    case eNotify_OnQuit:
        SaveLayouts();
        SaveMiscSettings();
        SaveTrackColors();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnAddSelectedNode()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();

    if (pSequence)
    {
        UiAnimUndo undo("Add Elements to Animation");
        pSequence->AddSelectedUiElements();
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnAddDirectorNode()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();

    if (pSequence)
    {
        UiAnimUndo undo("Create Animation Director Node");
        QString name = pSequence->GetAvailableNodeNameStartingWith("Director");
        pSequence->CreateSubNode(name, eUiAnimNodeType_Director);
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnFindNode()
{
    if (!m_findDlg)
    {
        m_findDlg = new CUiAnimViewFindDlg("Find Node in UI Canvas Sequences", this);
        m_findDlg->Init(this);
        connect(m_findDlg, SIGNAL(finished(int)), m_wndNodesCtrl->findChild<QTreeView*>(), SLOT(setFocus()), Qt::QueuedConnection);
    }
    m_findDlg->FillData();
    m_findDlg->show();
    m_findDlg->raise();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::NoModifier)
    {
        event->accept();
        m_animationContext->TogglePlay();
    }
    return QMainWindow::keyPressEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::closeEvent([[maybe_unused]] QCloseEvent* event)
{
    m_wndKeyPropertiesDock->hide();
    m_wndCurveEditorDock->hide();
}

void CUiAnimViewDialog::showEvent([[maybe_unused]] QShowEvent* event)
{
    m_wndKeyPropertiesDock->show();
    if (m_lastMode == ViewMode::Both)
    {
        m_wndCurveEditorDock->show();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnModeDopeSheet()
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
    m_wndCurveEditor->OnSequenceChanged(m_animationContext->GetSequence());
    m_lastMode = ViewMode::TrackView;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnModeCurveEditor()
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
    m_wndCurveEditor->OnSequenceChanged(m_animationContext->GetSequence());
    m_lastMode = ViewMode::CurveEditor;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnOpenCurveEditor()
{
    OnModeDopeSheet();
    m_wndCurveEditorDock->show();
    m_wndCurveEditorDock->toggleViewAction()->setEnabled(true);
    m_actions[ID_TV_MODE_DOPESHEET]->setChecked(true);
    m_actions[ID_TV_MODE_CURVEEDITOR]->setChecked(true);
    m_wndCurveEditor->OnSequenceChanged(m_animationContext->GetSequence());
    m_lastMode = ViewMode::Both;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSnapNone()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapNone);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSnapMagnet()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapMagnet);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSnapFrame()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapFrame);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSnapTick()
{
    m_wndDopeSheet->SetSnappingMode(eSnappingMode_SnapTick);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSnapFPS()
{
    int fps = FloatToIntRet(m_wndCurveEditor->GetFPS());
    bool ok = false;
    fps = QInputDialog::getInt(this, tr("Frame rate for frame snapping"), QStringLiteral(""), fps, 1, 120, 1, &ok);
    if (ok)
    {
        m_wndDopeSheet->SetSnapFPS(fps);
        m_wndCurveEditor->SetFPS(static_cast<float>(fps));

        SetCursorPosText(m_animationContext->GetTime());
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnViewTickInSeconds()
{
    m_wndDopeSheet->SetTickDisplayMode(eUiAVTickMode_InSeconds);
    m_wndCurveEditor->SetTickDisplayMode(eUiAVTickMode_InSeconds);
    SetCursorPosText(m_animationContext->GetTime());
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnViewTickInFrames()
{
    m_wndDopeSheet->SetTickDisplayMode(eUiAVTickMode_InFrames);
    m_wndCurveEditor->SetTickDisplayMode(eUiAVTickMode_InFrames);
    SetCursorPosText(m_animationContext->GetTime());
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SaveMiscSettings() const
{
#if UI_ANIMATION_REMOVED // we want to save settings using same system as UI editor
    QSettings settings;
    for (auto g : QString(s_kUiAnimViewSettingsSection).split('\\'))
    {
        settings.beginGroup(g);
    }

    settings.setValue(s_kSnappingModeEntry, (int)m_wndDopeSheet->GetSnappingMode());
    settings.setValue(s_kFrameSnappingFPSEntry, m_wndCurveEditor->GetFPS());
    settings.setValue(s_kTickDisplayModeEntry, (int)m_wndDopeSheet->GetTickDisplayMode());
#endif
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::ReadMiscSettings()
{
    QSettings settings;
    for (auto g : QString(s_kUiAnimViewSettingsSection).split('\\'))
    {
        settings.beginGroup(g);
    }

    ESnappingMode snapMode = static_cast<ESnappingMode>(settings.value(s_kSnappingModeEntry, eSnappingMode_SnapNone).toInt());
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
        m_wndDopeSheet->SetSnapFPS(FloatToIntRet(fps));
        m_wndCurveEditor->SetFPS(fps);
    }

    EUiAVTickMode tickMode = static_cast<EUiAVTickMode>(settings.value(s_kTickDisplayModeEntry, eUiAVTickMode_InSeconds).toInt());
    m_wndDopeSheet->SetTickDisplayMode(tickMode);
    m_wndCurveEditor->SetTickDisplayMode(tickMode);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SaveLayouts()
{
    QSettings settings("O3DE", "O3DE");
    settings.beginGroup("UiAnimView");
    QByteArray stateData = this->saveState();
    settings.setValue("layout", stateData);
    settings.setValue("lastViewMode", static_cast<int>(m_lastMode));
    QStringList sl;
    foreach(int i, m_wndSplitter->sizes())
    sl << QString::number(i);
    settings.setValue("splitter", sl.join(","));
    settings.endGroup();
    settings.sync();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::ReadLayouts()
{
    QSettings settings("O3DE", "O3DE");
    settings.beginGroup("UiAnimView");
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
        QStringList sl = settings.value("splitter").toString().split(",");
        QList<int> szl;
        foreach (QString s, sl)
        {
            szl << s.toInt();
        }
        if (!sl.isEmpty())
        {
            m_wndSplitter->setSizes(szl);
        }
    }

    QVariant defaultMode(static_cast<int>(m_lastMode));
    SetViewMode(static_cast<ViewMode>(settings.value("lastViewMode", defaultMode).toInt()));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SetViewMode(ViewMode mode)
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

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SaveTrackColors() const
{
    CUiAVCustomizeTrackColorsDlg::SaveColors(s_kUiAnimViewSettingsSection);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::ReadTrackColors()
{
    CUiAVCustomizeTrackColorsDlg::LoadColors(s_kUiAnimViewSettingsSection);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SetCursorPosText(float fTime)
{
    QString sText;
    int fps = FloatToIntRet(m_wndCurveEditor->GetFPS());
    int nMins = int ( fTime / 60.0f );
    int nSecs = int (fTime - float ( nMins ) * 60.0f);
    int nFrames = int(fTime * m_wndCurveEditor->GetFPS()) % fps;

    sText = QString("%1:%2:%3 (%4fps)").arg(nMins).arg(nSecs, 2, 10, QLatin1Char('0')).arg(nFrames, 2, 10, QLatin1Char('0')).arg(fps);
    m_cursorPos->setText(sText);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnCustomizeTrackColors()
{
    CUiAVCustomizeTrackColorsDlg dlg(this);
    dlg.exec();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnBatchRender()
{
#if UI_ANIMATION_REMOVED    // not supporting batch render
    CSequenceBatchRenderDialog dlg(m_wndCurveEditor->GetFPS(), this);
    dlg.exec();
#endif
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnToggleDisable()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();
    if (pSequence)
    {
        CUiAnimViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();
        const unsigned int numSelectedNodes = selectedNodes.GetCount();
        for (unsigned int i = 0; i < numSelectedNodes; ++i)
        {
            CUiAnimViewAnimNode* pNode = selectedNodes.GetNode(i);
            pNode->SetDisabled(!pNode->IsDisabled());
        }

        CUiAnimViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetDisabled(!pTrack->IsDisabled());
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnToggleMute()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();
    if (pSequence)
    {
        CUiAnimViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(!pTrack->IsMuted());
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnMuteAll()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();
    if (pSequence)
    {
        CUiAnimViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(true);
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnUnmuteAll()
{
    CUiAnimViewSequence* pSequence = m_animationContext->GetSequence();
    if (pSequence)
    {
        CUiAnimViewTrackBundle selectedTracks = pSequence->GetSelectedTracks();
        const unsigned int numSelectedTracks = selectedTracks.GetCount();
        for (unsigned int i = 0; i < numSelectedTracks; ++i)
        {
            CUiAnimViewTrack* pTrack = selectedTracks.GetTrack(i);
            pTrack->SetMuted(false);
        }
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::SaveZoomScrollSettings()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnNodeSelectionChanged(CUiAnimViewSequence* pSequence)
{
    CUiAnimViewSequence* pCurrentSequence = m_animationContext->GetSequence();

    if (pCurrentSequence && pCurrentSequence == pSequence)
    {
        UpdateActions();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnNodeRenamed(CUiAnimViewNode* pNode, const char* pOldName)
{
    // React to sequence name changes
    if (pNode->GetNodeType() == eUiAVNT_Sequence)
    {
        if (m_currentSequenceName == QString(pOldName))
        {
            m_currentSequenceName = QString::fromUtf8(pNode->GetName().c_str());
        }

        ReloadSequencesComboBox();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::UpdateDopeSheetTime(CUiAnimViewSequence* pSequence)
{
    Range timeRange = pSequence->GetTimeRange();
    m_wndDopeSheet->SetTimeRange(timeRange.start, timeRange.end);
    m_wndDopeSheet->SetStartMarker(timeRange.start);
    m_wndDopeSheet->SetEndMarker(timeRange.end);
    m_wndDopeSheet->SetTimeScale(m_wndDopeSheet->GetTimeScale(), 0);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::EditorAboutToClose()
{
    m_wndCurveEditorDock->setFloating(false);
    m_wndKeyPropertiesDock->setFloating(false);
    SaveLayouts();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSequenceSettingsChanged(CUiAnimViewSequence* pSequence)
{
    CUiAnimViewSequence* pCurrentSequence = m_animationContext->GetSequence();

    if (pCurrentSequence && pCurrentSequence == pSequence)
    {
        UpdateDopeSheetTime(pSequence);
        m_wndNodesCtrl->update();
    }
}

//////////////////////////////////////////////////////////////////////////
UiEditorAnimationStateInterface::UiEditorAnimationEditState CUiAnimViewDialog::GetCurrentEditState()
{
    UiEditorAnimationStateInterface::UiEditorAnimationEditState animEditState;

    animEditState.m_sequenceName = m_animationContext->GetSequence() ? m_animationContext->GetSequence()->GetName() : "";
    animEditState.m_time = m_animationContext->GetTime();
    animEditState.m_timelineScale = m_wndDopeSheet->GetTimeScale();
    animEditState.m_timelineScrollOffset = m_wndDopeSheet->GetScrollOffset();

    return animEditState;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::RestoreCurrentEditState(const UiEditorAnimationStateInterface::UiEditorAnimationEditState& animEditState)
{
    CUiAnimViewSequence* pSequence = animEditState.m_sequenceName.empty() ? nullptr : m_sequenceManager->GetSequenceByName(animEditState.m_sequenceName.c_str());
    m_animationContext->SetSequence(pSequence, true, false);

    m_animationContext->SetTime(animEditState.m_time);

    m_wndDopeSheet->SetTimeScale(animEditState.m_timelineScale, 0.0f);
    m_wndDopeSheet->SetScrollOffset(animEditState.m_timelineScrollOffset);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnActiveCanvasChanged()
{
    m_animationSystem = CUiAnimViewSequenceManager::GetSequenceManager()->GetAnimationSystem();

    setEnabled(m_animationSystem ? true : false);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnUiElementsDeletedOrReAdded()
{
    m_wndNodesCtrl->UpdateAllNodesForElementChanges();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSequenceAdded([[maybe_unused]] CUiAnimViewSequence* pSequence)
{
    ReloadSequencesComboBox();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::OnSequenceRemoved([[maybe_unused]] CUiAnimViewSequence* pSequence)
{
    ReloadSequencesComboBox();
    UpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::BeginUndoTransaction()
{
    m_bDoingUndoOperation = true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewDialog::EndUndoTransaction()
{
    m_bDoingUndoOperation = false;
}

#include <Animation/moc_UiAnimViewDialog.cpp>
