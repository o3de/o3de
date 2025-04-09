/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"
#include "Editor/Resource.h"
#include "UiEditorAnimationBus.h"
#include "UiAnimViewNodes.h"
#include "UiAnimViewDopeSheetBase.h"
#include "UiAnimViewUndo.h"
#include "UiAnimViewDialog.h"
#include "UiAVEventsDialog.h"

#include "Clipboard.h"

#include <LyShine/Animation/IUiAnimation.h>

#include "ViewManager.h"
#include <Editor/Util/fastlib.h>

#include "QtUtilWin.h"
#include "QtUtil.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QDebug>
#include <QTreeWidget>
#include <QCompleter>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QScrollBar>

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

CUiAnimViewNodesCtrl::CRecord::CRecord(CUiAnimViewNode* pNode /*= nullptr*/)
    : m_pNode(pNode)
    , m_bVisible(false)
{
    if (pNode)
    {
        QVariant v;
        v.setValue<CUiAnimViewNodePtr>(pNode);
        setData(0, Qt::UserRole, v);
    }
}

class CUiAnimViewNodesCtrlDelegate
    : public QStyledItemDelegate
{
public:
    CUiAnimViewNodesCtrlDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        bool enabled = index.data(CUiAnimViewNodesCtrl::CRecord::EnableRole).toBool();
        QStyleOptionViewItem opt = option;
        if (!enabled)
        {
            opt.state &= ~QStyle::State_Enabled;
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};


class CUiAnimViewNodesTreeWidget
    : public QTreeWidget
{
public:
    CUiAnimViewNodesTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
        , m_controller(nullptr)
    {
        setItemDelegate(new CUiAnimViewNodesCtrlDelegate(this));
    }

    void setController(CUiAnimViewNodesCtrl* p)
    {
        m_controller = p;
    }

protected:
    void dragMoveEvent([[maybe_unused]] QDragMoveEvent* event)
    {
        // For now we do not support any drag and drop in the Nodes pane
        return;
    }

    void dropEvent([[maybe_unused]] QDropEvent* event)
    {
        // For now we do not support any drag and drop in the Nodes pane
        return;
    }

    void keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Tab:
            if (m_controller)
            {
                m_controller->ShowNextResult();
                event->accept();
            }
            return;
        default:
            break;
        }
        QTreeWidget::keyPressEvent(event);
    }


    bool focusNextPrevChild([[maybe_unused]] bool next)
    {
        return false;   // so we get the tab key
    }

private:
    QList<CUiAnimViewAnimNode*> draggedNodes(QDropEvent* event)
    {
        QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        QList<CUiAnimViewAnimNode*> nodes;
        while (!stream.atEnd())
        {
            int row, col;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;

            QVariant v = roleDataMap[Qt::UserRole];
            if (v.isValid())
            {
                CUiAnimViewNode* pNode = v.value<CUiAnimViewNodePtr>();
                if (pNode && pNode->GetNodeType() == eUiAVNT_AnimNode)
                {
                    nodes << (CUiAnimViewAnimNode*)pNode;
                }
            }
        }
        return nodes;
    }

    CUiAnimViewNodesCtrl*   m_controller;
};

QDataStream& operator<<(QDataStream& out, const CUiAnimViewNodePtr& obj)
{
    out.writeRawData((const char*) &obj, sizeof(obj));
    return out;
}

QDataStream& operator>>(QDataStream& in, CUiAnimViewNodePtr& obj)
{
    in.readRawData((char*) &obj, sizeof(obj));
    return in;
}



enum EMenuItem
{
    eMI_SelectInViewport = 603,
    eMI_RemoveSelected = 10,
    eMI_CopyKeys = 599,
    eMI_CopySelectedKeys = 600,
    eMI_PasteKeys = 601,
    eMI_AddTrackBase = 1000,
    eMI_RemoveTrack = 299,
    eMI_ExpandAll = 650,
    eMI_CollapseAll = 659,
    eMI_ExpandFolders = 660,
    eMI_CollapseFolders = 661,
    eMI_ExpandEntities = 651,
    eMI_CollapseEntities = 652,
    eMI_ExpandCameras = 653,
    eMI_CollapseCameras = 654,
    eMI_ExpandMaterials = 655,
    eMI_CollapseMaterials = 656,
    eMI_ExpandEvents = 657,
    eMI_CollapseEvents = 658,
    eMI_AddDirectorNode = 501,
    eMI_AddConsoleVariable = 502,
    eMI_AddScriptVariable = 503,
    eMI_AddMaterial = 504,
    eMI_AddEvent = 505,
    eMI_AddCommentNode = 507,
    eMI_AddRadialBlur = 508,
    eMI_AddColorCorrection = 509,
    eMI_AddDOF = 510,
    eMI_AddScreenfader = 511,
    eMI_AddHDRSetup = 512,
    eMI_AddShadowSetup = 513,
    eMI_AddEnvironment = 514,
    eMI_AddScreenDropsSetup = 515,
    eMI_AddSelectedUiElements = 516,
    eMI_EditEvents = 550,
    eMI_SetAsViewCamera = 13,
    eMI_SetAsActiveDirector = 15,
    eMI_Disable = 17,
    eMI_Mute = 18,
    eMI_CustomizeTrackColor = 19,
    eMI_ClearCustomTrackColor = 20,
    eMI_ShowHideBase = 100,
    eMI_SelectSubmaterialBase = 2000,
    eMI_SetAnimationLayerBase = 3000,
};

// The 'MI' represents a Menu Item.

#include <Editor/Animation/ui_UiAnimViewNodes.h>


//////////////////////////////////////////////////////////////////////////
CUiAnimViewNodesCtrl::CUiAnimViewNodesCtrl(QWidget* hParentWnd, CUiAnimViewDialog* parent /* = 0 */)
    : QWidget(hParentWnd)
    , m_bIgnoreNotifications(false)
    , m_bNeedReload(false)
    , m_bSelectionChanging(false)
    , ui(new Ui::CUiAnimViewNodesCtrl)
    , m_pUiAnimViewDialog(parent)
{
    ui->setupUi(this);
    m_pDopeSheet = 0;
    m_currentMatchIndex = 0;
    m_matchCount = 0;

    qRegisterMetaType<CUiAnimViewNodePtr>("CUiAnimViewNodePtr");
    qRegisterMetaTypeStreamOperators<CUiAnimViewNodePtr>("CUiAnimViewNodePtr");

    ui->treeWidget->hide();
    ui->searchField->hide();
    ui->searchCount->hide();
    ui->searchField->installEventFilter(this);

    ui->treeWidget->setController(this);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &CUiAnimViewNodesCtrl::OnNMRclick);
    connect(ui->treeWidget, &QTreeWidget::itemExpanded, this, &CUiAnimViewNodesCtrl::OnItemExpanded);
    connect(ui->treeWidget, &QTreeWidget::itemCollapsed, this, &CUiAnimViewNodesCtrl::OnItemExpanded);
    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CUiAnimViewNodesCtrl::OnSelectionChanged);

    connect(ui->searchField, &QLineEdit::textChanged, this, &CUiAnimViewNodesCtrl::OnFilterChange);

    m_bEditLock = false;

    m_arrowCursor = Qt::ArrowCursor;
    m_noIcon = Qt::ForbiddenCursor;

    UiAnimUndoManager::Get()->AddListener(this);

    // Create an action and shortcut to handle the delete key for the nodes ctrl.
    // We can't do this using keyPressEvent because the UI editor window has a shortcut
    // for delete and that will override any keyPressEvent in child widgets.
    // So we need our own shortcut to override the Editor window one when the focus
    // is in this widget.
    setFocusPolicy(Qt::StrongFocus);
    QAction* action = new QAction(QString("Delete"), this);
    action->setShortcut(QKeySequence::Delete);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(action,
        &QAction::triggered, this,
        [this]([[maybe_unused]] bool checked)
        {
            CUiAnimViewSequence* pSequence = nullptr;
            UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
            if (pSequence)
            {
                UiAnimUndo undo("Delete selected UiAnimView Nodes/Tracks");
                BeginUndoTransaction();
                pSequence->DeleteSelectedNodes();
                EndUndoTransaction();
            }
        });
    addAction(action);
};

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNodesCtrl::~CUiAnimViewNodesCtrl()
{
    UiAnimUndoManager::Get()->RemoveListener(this);
}

bool CUiAnimViewNodesCtrl::eventFilter(QObject* o, QEvent* e)
{
    if (o == ui->searchField && e->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Tab && keyEvent->modifiers() == Qt::NoModifier)
        {
            ShowNextResult();
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}


//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnSequenceChanged()
{
    assert(m_pUiAnimViewDialog);

    m_nodeToRecordMap.clear();
    ui->treeWidget->clear();

    FillAutoCompletionListForFilter();

    Reload();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::SetDopeSheet(CUiAnimViewDopeSheetBase* pDopeSheet)
{
    m_pDopeSheet = pDopeSheet;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNodesCtrl::CRecord* CUiAnimViewNodesCtrl::AddAnimNodeRecord(CRecord* pParentRecord, CUiAnimViewAnimNode* pAnimNode)
{
    CRecord* pNewRecord = new CRecord(pAnimNode);

    pNewRecord->setText(0, QString::fromUtf8(pAnimNode->GetName().c_str()));
    UpdateUiAnimNodeRecord(pNewRecord, pAnimNode);
    pParentRecord->insertChild(GetInsertPosition(pParentRecord, pAnimNode), pNewRecord);
    FillNodesRec(pNewRecord, pAnimNode);

    return pNewRecord;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNodesCtrl::CRecord* CUiAnimViewNodesCtrl::AddTrackRecord(CRecord* pParentRecord, CUiAnimViewTrack* pTrack)
{
    CRecord* pNewTrackRecord = new CRecord(pTrack);
    pNewTrackRecord->setSizeHint(0, QSize(30, 18));
    pNewTrackRecord->setText(0, QString::fromUtf8(pTrack->GetName().c_str()));
    UpdateTrackRecord(pNewTrackRecord, pTrack);
    pParentRecord->insertChild(GetInsertPosition(pParentRecord, pTrack), pNewTrackRecord);
    FillNodesRec(pNewTrackRecord, pTrack);

    return pNewTrackRecord;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewNodesCtrl::GetInsertPosition(CRecord* pParentRecord, CUiAnimViewNode* pNode)
{
    // Search for insert position
    const int siblingCount = pParentRecord->childCount();
    for (int i = 0; i < siblingCount; ++i)
    {
        CRecord* pRecord = static_cast<CRecord*>(pParentRecord->child(i));
        CUiAnimViewNode* pSiblingNode = pRecord->GetNode();

        if (*pNode < *pSiblingNode)
        {
            return i;
        }
    }

    return siblingCount;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::AddNodeRecord(CRecord* pRecord, CUiAnimViewNode* pNode)
{
    assert(m_nodeToRecordMap.find(pNode) == m_nodeToRecordMap.end());
    if (m_nodeToRecordMap.find(pNode) != m_nodeToRecordMap.end())
    {
        // For safety. Shouldn't happen
        return;
    }

    CRecord* pNewRecord = nullptr;

    if (pNode->IsHidden())
    {
        return;
    }

    switch (pNode->GetNodeType())
    {
    case eUiAVNT_AnimNode:
        pNewRecord = AddAnimNodeRecord(pRecord, static_cast<CUiAnimViewAnimNode*>(pNode));
        break;
    case eUiAVNT_Track:
        pNewRecord = AddTrackRecord(pRecord, static_cast<CUiAnimViewTrack*>(pNode));
        break;
    }

    if (pNewRecord)
    {
        if (!pNode->IsGroupNode() && pNode->GetChildCount() == 0)       // groups and compound tracks are draggable
        {
            pNewRecord->setFlags(pNewRecord->flags() & ~Qt::ItemIsDragEnabled);
        }
        if (!pNode->IsGroupNode())                                      // only groups can be dropped into
        {
            pNewRecord->setFlags(pNewRecord->flags() & ~Qt::ItemIsDropEnabled);
        }
        if (pNode->IsExpanded())
        {
            pNewRecord->setExpanded(true);
        }

        if (pNode->IsSelected())
        {
            m_bIgnoreNotifications = true;
            SelectRow(pNode, false, false);
            m_bIgnoreNotifications = false;
        }

        m_nodeToRecordMap[pNode] = pNewRecord;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::FillNodesRec(CRecord* pRecord, CUiAnimViewNode* pCurrentNode)
{
    const unsigned int childCount = pCurrentNode->GetChildCount();

    for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        CUiAnimViewNode* pNode = pCurrentNode->GetChild(childIndex);

        if (!pNode->IsHidden())
        {
            AddNodeRecord(pRecord, pNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::UpdateNodeRecord(CRecord* pRecord)
{
    CUiAnimViewNode* pNode = pRecord->GetNode();

    if (pNode)
    {
        switch (pNode->GetNodeType())
        {
        case eUiAVNT_AnimNode:
            UpdateUiAnimNodeRecord(pRecord, static_cast<CUiAnimViewAnimNode*>(pNode));
            break;
        case eUiAVNT_Track:
            UpdateTrackRecord(pRecord, static_cast<CUiAnimViewTrack*>(pNode));
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::UpdateTrackRecord(CRecord* pRecord, CUiAnimViewTrack* pTrack)
{
    pRecord->setIcon(0, QIcon(QStringLiteral(":/nodes/tvnodes-13.png")));

    // Check if parameter is valid for non sub tracks
    CUiAnimViewAnimNode* pAnimNode = pTrack->GetAnimNode();
    const bool isParamValid = pTrack->IsSubTrack() || pAnimNode->IsParamValid(pTrack->GetParameterType());

    // Check if disabled or muted
    const bool bDisabledOrMuted = pTrack->IsDisabled() || pTrack->IsMuted();

    // If track is not valid and disabled/muted color node in grey
    pRecord->setData(0, CRecord::EnableRole, !bDisabledOrMuted && isParamValid);
}

QIcon CUiAnimViewNodesCtrl::NodeTypeToTrackViewIcon(EUiAnimNodeType nodeType)
{
    switch (nodeType)
    {
    case eUiAnimNodeType_AzEntity:
        return QIcon(QStringLiteral(":/nodes/tvnodes-21.png"));
    case eUiAnimNodeType_Director:
        return QIcon(QStringLiteral(":/nodes/tvnodes-27.png"));
    case eUiAnimNodeType_Camera:
        return QIcon(QStringLiteral(":/nodes/tvnodes-08.png"));
    case eUiAnimNodeType_CVar:
        return QIcon(QStringLiteral(":/nodes/tvnodes-15.png"));
    case eUiAnimNodeType_ScriptVar:
        return QIcon(QStringLiteral(":/nodes/tvnodes-14.png"));
    case eUiAnimNodeType_Material:
        return QIcon(QStringLiteral(":/nodes/tvnodes-16.png"));
    case eUiAnimNodeType_Event:
        return QIcon(QStringLiteral(":/nodes/tvnodes-06.png"));
    case eUiAnimNodeType_Group:
        return QIcon(QStringLiteral(":/nodes/tvnodes-01.png"));
    case eUiAnimNodeType_Layer:
        return QIcon(QStringLiteral(":/nodes/tvnodes-20.png"));
    case eUiAnimNodeType_Comment:
        return QIcon(QStringLiteral(":/nodes/tvnodes-23.png"));
    case eUiAnimNodeType_Light:
        return QIcon( QStringLiteral(":/nodes/tvnodes-18.png"));
    case eUiAnimNodeType_HDRSetup:
        return QIcon(QStringLiteral(":/nodes/tvnodes-26.png"));
    case eUiAnimNodeType_ShadowSetup:
        return QIcon(QStringLiteral(":/nodes/tvnodes-24.png"));
    }
    return QIcon(QStringLiteral(":/nodes/tvnodes-21.png"));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::UpdateUiAnimNodeRecord(CRecord* pRecord, CUiAnimViewAnimNode* pAnimNode)
{
    const QColor TEXT_COLOR_FOR_NORMAL_ENTITY_NODE(220, 220, 220);
    const QColor TEXT_COLOR_FOR_MISSING_ENTITY(255, 0, 0);
    const QColor TEXT_COLOR_FOR_INVALID_MATERIAL(255, 0, 0);
    const QColor BACK_COLOR_FOR_ACTIVE_DIRECTOR(192, 192, 255);
    const QColor BACK_COLOR_FOR_INACTIVE_DIRECTOR(224, 224, 224);

    QFont f = font();
    f.setBold(true);
    pRecord->setFont(0, f);

    EUiAnimNodeType nodeType = pAnimNode->GetType();
    QString nodeName = QString::fromUtf8(pAnimNode->GetName().c_str());

    pRecord->setIcon(0, NodeTypeToTrackViewIcon(nodeType));

    const bool bDisabled = pAnimNode->IsDisabled();
    pRecord->setData(0, CRecord::EnableRole, !bDisabled);

    if (nodeType == eUiAnimNodeType_AzEntity)
    {
        AZ::Entity* azEntity = pAnimNode->GetNodeEntityAz();
        if (azEntity)
        {
            pRecord->setForeground(0, TEXT_COLOR_FOR_NORMAL_ENTITY_NODE);
        }
        else
        {
            pRecord->setForeground(0, TEXT_COLOR_FOR_MISSING_ENTITY);
        }
    }
    else if (nodeType == eUiAnimNodeType_Group)
    {
        pRecord->setBackground(0, QColor(220, 255, 220));
        pRecord->setSizeHint(0, QSize(30, 20));
    }
    else if (nodeType == eUiAnimNodeType_Material)
    {
        pRecord->setForeground(0, TEXT_COLOR_FOR_INVALID_MATERIAL);
    }

    // Mark the active director and other directors properly.
    if (pAnimNode->IsActiveDirector())
    {
        pRecord->setBackground(0, BACK_COLOR_FOR_ACTIVE_DIRECTOR);
    }
    else if (nodeType == eUiAnimNodeType_Director)
    {
        pRecord->setBackground(0, BACK_COLOR_FOR_INACTIVE_DIRECTOR);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::Reload()
{
    ui->treeWidget->clear();
    OnFillItems();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnFillItems()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        CUiAnimViewSequenceNotificationContext context(pSequence);

        m_nodeToRecordMap.clear();

        CRecord* pRootGroupRec = new CRecord(pSequence);
        pRootGroupRec->setText(0, QString::fromUtf8(pSequence->GetName().c_str()));
        QFont f = font();
        f.setBold(true);
        pRootGroupRec->setData(0, Qt::FontRole, f);
        pRootGroupRec->setSizeHint(0, QSize(width(), 24));

        m_nodeToRecordMap[pSequence] = pRootGroupRec;
        ui->treeWidget->addTopLevelItem(pRootGroupRec);

        FillNodesRec(pRootGroupRec, pSequence);
        pRootGroupRec->setExpanded(pSequence->IsExpanded());

        // Additional empty record like space for scrollbar in key control
        CRecord* pGroupRec = new CRecord();
        pGroupRec->setSizeHint(0, QSize(width(), 18));
        ui->treeWidget->addTopLevelItem(pGroupRec);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnItemExpanded(QTreeWidgetItem* item)
{
    CRecord* pRecord = (CRecord*) item;

    if (pRecord && pRecord->GetNode())
    {
        pRecord->GetNode()->SetExpanded(item->isExpanded());
    }
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnSelectionChanged()
{
    // Need to avoid the second call to this, because GetSelectedRows is broken
    // with multi selection
    if (m_bSelectionChanging)
    {
        return;
    }
    m_bSelectionChanging = true;

    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        CUiAnimViewSequenceNotificationContext context(pSequence);
        pSequence->ClearSelection();

        QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
        int nCount = items.count();
        for (int i = 0; i < nCount; i++)
        {
            CRecord* pRecord = (CRecord*)items.at(i);

            if (pRecord && pRecord->GetNode())
            {
                if (!pRecord->GetNode()->IsSelected())
                {
                    pRecord->GetNode()->SetSelected(true);
                    ui->treeWidget->setCurrentItem(pRecord);
                }
            }
        }
    }

    m_bSelectionChanging = false;
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnNMRclick(QPoint point)
{
    CRecord* pRecord = 0;

    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    CUiAnimViewSequenceNotificationContext context(pSequence);

    // Find node under mouse.
    // Select the item that is at the point myPoint.
    pRecord = static_cast<CRecord*>(ui->treeWidget->itemAt(point));

    CUiAnimViewAnimNode* pGroupNode = nullptr;
    CUiAnimViewNode* pNode = nullptr;
    CUiAnimViewAnimNode* pAnimNode = nullptr;
    CUiAnimViewTrack* pTrack = nullptr;

    if (pRecord && pRecord->GetNode())
    {
        pNode = pRecord->GetNode();

        if (pNode)
        {
            EUiAnimViewNodeType nodeType = pNode->GetNodeType();

            if (nodeType == eUiAVNT_AnimNode)
            {
                pAnimNode = static_cast<CUiAnimViewAnimNode*>(pNode);

                if (pAnimNode->GetType() == eUiAnimNodeType_Director || pAnimNode->GetType() == eUiAnimNodeType_Group)
                {
                    pGroupNode = pAnimNode;
                }
            }
            else if (nodeType == eUiAVNT_Sequence)
            {
                pGroupNode = pSequence;
            }
            else if (nodeType == eUiAVNT_Track)
            {
                pTrack = static_cast<CUiAnimViewTrack*>(pNode);
                pAnimNode = pTrack->GetAnimNode();
            }
        }
    }
    else
    {
        pNode = pSequence;
        pGroupNode = pSequence;
        pRecord = m_nodeToRecordMap[pSequence];
    }

    int cmd = ShowPopupMenu(point, pRecord);

    float scrollPos = SaveVerticalScrollPos();

    if (cmd == eMI_RemoveSelected)
    {
        UiAnimUndo undo("Delete selected UiAnimView Nodes/Tracks");
        BeginUndoTransaction();
        pSequence->DeleteSelectedNodes();
        EndUndoTransaction();
    }

    if (pGroupNode)
    {
        if (cmd == eMI_AddSelectedUiElements)
        {
            UiAnimUndo undo("Add UI Elements to Animation");
            pGroupNode->AddSelectedUiElements();
            //pGroupNode->BindToEditorObjects(); // this causes problems (since it causes multiple registers with components?)
        }
        else if (cmd == eMI_AddScreenfader)
        {
            UiAnimUndo undo("Add UiAnimView Screen Fader Node");
            pGroupNode->CreateSubNode("ScreenFader", eUiAnimNodeType_ScreenFader);
        }
        else if (cmd == eMI_AddCommentNode)
        {
            UiAnimUndo undo("Add UiAnimView Comment Node");
            pGroupNode->CreateSubNode("Comment", eUiAnimNodeType_Comment);
        }
        else if (cmd == eMI_AddRadialBlur)
        {
            UiAnimUndo undo("Add UiAnimView Radial Blur Node");
            pGroupNode->CreateSubNode("RadialBlur", eUiAnimNodeType_RadialBlur);
        }
        else if (cmd == eMI_AddColorCorrection)
        {
            UiAnimUndo undo("Add UiAnimView Color Correction Node");
            pGroupNode->CreateSubNode("ColorCorrection", eUiAnimNodeType_ColorCorrection);
        }
        else if (cmd == eMI_AddDOF)
        {
            UiAnimUndo undo("Add UiAnimView Depth of Field Node");
            pGroupNode->CreateSubNode("DepthOfField", eUiAnimNodeType_DepthOfField);
        }
        else if (cmd == eMI_AddHDRSetup)
        {
            UiAnimUndo undo("Add UiAnimView HDR Setup Node");
            pGroupNode->CreateSubNode("HdrSetup", eUiAnimNodeType_HDRSetup);
        }
        else if (cmd == eMI_AddShadowSetup)
        {
            UiAnimUndo undo("Add UiAnimView Shadow Setup Node");
            pGroupNode->CreateSubNode("ShadowsSetup", eUiAnimNodeType_ShadowSetup);
        }
        else if (cmd == eMI_AddScreenDropsSetup)
        {
            UiAnimUndo undo("Add UiAnimView Screen Drops Setup Node");
            pGroupNode->CreateSubNode("ScreenDropsSetup", eUiAnimNodeType_ScreenDropsSetup);
        }
        else if (cmd == eMI_AddEnvironment)
        {
            UiAnimUndo undo("Add UiAnimView Environment Node");
            pGroupNode->CreateSubNode("Environment", eUiAnimNodeType_Environment);
        }
        else if (cmd == eMI_AddDirectorNode)
        {
            UiAnimUndo undo("Add UiAnimView Director Node");
            QString name = pGroupNode->GetAvailableNodeNameStartingWith("Director");
            pGroupNode->CreateSubNode(name, eUiAnimNodeType_Director);
        }
        else if (cmd == eMI_AddEvent)
        {
            UiAnimUndo undo("Add UiAnimView Event Node");
            pGroupNode->CreateSubNode("Events", eUiAnimNodeType_Event);
        }
    }

    if (cmd == eMI_EditEvents)
    {
        EditEvents();
    }
    else if (cmd == eMI_SetAsActiveDirector)
    {
        if (pNode && pNode->GetNodeType() == eUiAVNT_AnimNode)
        {
            CUiAnimViewAnimNode* pAnimNode2 = static_cast<CUiAnimViewAnimNode*>(pNode);
            pAnimNode2->SetAsActiveDirector();
        }
    }
    else if (cmd == eMI_RemoveTrack)
    {
        if (pTrack)
        {
            UiAnimUndo undo("Remove Animation Track");
            pTrack->GetAnimNode()->RemoveTrack(pTrack);
        }
    }
    else if (cmd >= eMI_ShowHideBase && cmd < eMI_ShowHideBase + 100)
    {
        if (pAnimNode)
        {
            unsigned int childIndex = cmd - eMI_ShowHideBase;

            if (childIndex < pAnimNode->GetChildCount())
            {
                CUiAnimViewNode* pChild = pAnimNode->GetChild(childIndex);
                pChild->SetHidden(!pChild->IsHidden());
            }
        }
    }
    else if (cmd == eMI_CopyKeys)
    {
        pSequence->CopyKeysToClipboard(false, true);
    }
    else if (cmd == eMI_CopySelectedKeys)
    {
        pSequence->CopyKeysToClipboard(true, true);
    }
    else if (cmd == eMI_PasteKeys)
    {
        UiAnimUndo undo("Paste Animation Keys");
        pSequence->PasteKeysFromClipboard(pAnimNode, pTrack);
    }
    else if (cmd == eMI_ExpandAll)
    {
        if (pGroupNode)
        {
            BeginUndoTransaction();
            pGroupNode->GetAllAnimNodes().ExpandAll();
            EndUndoTransaction();
        }
    }
    else if (cmd == eMI_CollapseAll)
    {
        if (pGroupNode)
        {
            BeginUndoTransaction();
            pGroupNode->GetAllAnimNodes().CollapseAll();
            EndUndoTransaction();
        }
    }
    else if (cmd == eMI_ExpandFolders)
    {
        if (pGroupNode)
        {
            BeginUndoTransaction();
            pGroupNode->GetAnimNodesByType(eUiAnimNodeType_Group).ExpandAll();
            pGroupNode->GetAnimNodesByType(eUiAnimNodeType_Director).ExpandAll();
            EndUndoTransaction();
        }
    }
    else if (cmd == eMI_CollapseFolders)
    {
        if (pGroupNode)
        {
            BeginUndoTransaction();
            pGroupNode->GetAnimNodesByType(eUiAnimNodeType_Group).CollapseAll();
            pGroupNode->GetAnimNodesByType(eUiAnimNodeType_Director).CollapseAll();
            EndUndoTransaction();
        }
    }
    else if (cmd == eMI_ExpandEntities)
    {
        if (pGroupNode)
        {
            BeginUndoTransaction();
            pGroupNode->GetAnimNodesByType(eUiAnimNodeType_Entity).ExpandAll();
            EndUndoTransaction();
        }
    }
    else if (cmd == eMI_CollapseEntities)
    {
        if (pGroupNode)
        {
            BeginUndoTransaction();
            pGroupNode->GetAnimNodesByType(eUiAnimNodeType_Entity).CollapseAll();
            EndUndoTransaction();
        }
    }
    else if (cmd >= eMI_SelectSubmaterialBase && cmd < eMI_SelectSubmaterialBase + 100)
    {
        if (pAnimNode)
        {
            QString matName;
            GetMatNameAndSubMtlIndexFromName(matName, pAnimNode->GetName().c_str());
            QString newMatName;
            newMatName = QStringLiteral("%1.[%2]").arg(matName).arg(cmd - eMI_SelectSubmaterialBase + 1);
            UiAnimUndo undo("Rename Animation node");
            pAnimNode->SetName(newMatName.toUtf8().data());
            pAnimNode->SetSelected(true);
            UpdateNodeRecord(pRecord);
        }
    }
    else if (cmd >= eMI_SetAnimationLayerBase && cmd < eMI_SetAnimationLayerBase + 100)
    {
        if (pNode && pNode->GetNodeType() == eUiAVNT_Track)
        {
            CUiAnimViewTrack* pTrack2 = static_cast<CUiAnimViewTrack*>(pNode);
            pTrack2->SetAnimationLayerIndex(cmd - eMI_SetAnimationLayerBase);
        }
    }
    else if (cmd == eMI_Disable)
    {
        if (pNode)
        {
            pNode->SetDisabled(!pNode->IsDisabled());
        }
    }
    else if (cmd == eMI_Mute)
    {
        if (pTrack)
        {
            pTrack->SetMuted(!pTrack->IsMuted());
        }
    }
    else if (cmd == eMI_CustomizeTrackColor)
    {
        CustomizeTrackColor(pTrack);
    }
    else if (cmd == eMI_ClearCustomTrackColor)
    {
        if (pTrack)
        {
            pTrack->ClearCustomColor();
        }
    }

    if (cmd)
    {
        RestoreVerticalScrollPos(scrollPos);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::EditEvents()
{
    CUiAVEventsDialog dlg;
    dlg.exec();
}

//////////////////////////////////////////////////////////////////////////
struct UiAnimTrackMenuTreeNode
{
    QMenu menu;
    CUiAnimParamType paramType;
    std::map<QString, std::unique_ptr<UiAnimTrackMenuTreeNode> > children;
};

//////////////////////////////////////////////////////////////////////////
struct UiAnimContextMenu
{
    QMenu main;
    QMenu expandSub;
    QMenu collapseSub;
    QMenu setLayerSub;
    UiAnimTrackMenuTreeNode addTrackSub;
};

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::AddGroupNodeAddItems(UiAnimContextMenu& contextMenu, CUiAnimViewAnimNode* pAnimNode)
{
    // only want this item on sequence node
    if (pAnimNode->GetNodeType() == eUiAVNT_Sequence)
    {
        contextMenu.main.addAction("Add Selected UI Element(s)")->setData(eMI_AddSelectedUiElements);
        contextMenu.main.addAction("Add Event Node")->setData(eMI_AddEvent);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::AddMenuSeperatorConditional(QMenu& menu, bool& bAppended)
{
    if (bAppended)
    {
        menu.addSeparator();
    }

    bAppended = false;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewNodesCtrl::ShowPopupMenuSingleSelection(UiAnimContextMenu& contextMenu, CUiAnimViewSequence* pSequence, CUiAnimViewNode* pNode)
{
    bool bAppended = false;

    const bool bOnSequence = pNode->GetNodeType() == eUiAVNT_Sequence;
    const bool bOnNode = pNode->GetNodeType() == eUiAVNT_AnimNode;
    const bool bOnTrack = pNode->GetNodeType() == eUiAVNT_Track;
    const bool bIsLightAnimationSet = pSequence->GetFlags() & IUiAnimSequence::eSeqFlags_LightAnimationSet;

    // Get track & anim node pointers
    CUiAnimViewTrack* pTrack = bOnTrack ? static_cast<CUiAnimViewTrack*>(pNode) : nullptr;
    const bool bOnTrackNotSub = bOnTrack && !pTrack->IsSubTrack();

    CUiAnimViewAnimNode* pAnimNode = nullptr;
    if (bOnSequence || bOnNode)
    {
        pAnimNode = static_cast<CUiAnimViewAnimNode*>(pNode);
    }
    else if (bOnTrack)
    {
        pAnimNode = pTrack->GetAnimNode();
    }

    if (bOnNode || bOnSequence || bOnTrackNotSub)
    {
        contextMenu.main.addAction("Delete")->setData(bOnTrackNotSub ? eMI_RemoveTrack : eMI_RemoveSelected);
        bAppended = true;
    }

    if (bOnTrack)
    {
        // Copy & paste keys
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        contextMenu.main.addAction("Copy Keys")->setData(eMI_CopyKeys);
        contextMenu.main.addAction("Copy Selected Keys")->setData(eMI_CopySelectedKeys);
        contextMenu.main.addAction("Paste Keys")->setData(eMI_PasteKeys);
        bAppended = true;
    }

    // Flags
    {
        bool bFlagAppended = false;

        if (!bOnSequence)
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            QAction* a = contextMenu.main.addAction("Disabled");
            a->setData(eMI_Disable);
            a->setCheckable(true);
            a->setChecked(pNode->IsDisabled());
            bFlagAppended = true;
        }

        bAppended = bAppended || bFlagAppended;
    }

    // Add/Remove
    {
        if (bOnSequence || pNode->IsGroupNode())
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            AddGroupNodeAddItems(contextMenu, pAnimNode);
            bAppended = true;
        }
    }

    // Events
    if (bOnSequence || pNode->IsGroupNode() && !bIsLightAnimationSet)
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        contextMenu.main.addAction("Edit Events...")->setData(eMI_EditEvents);
        bAppended = true;
    }

   // TODO: look into support track colors
   // We have removed support for saving out the custom colors per track
   // it may be added back at some point
   // Track color
   // if (bOnTrack)
   // {
   //     AddMenuSeperatorConditional(contextMenu.main, bAppended);
   //     contextMenu.main.addAction("Customize Track Color...")->setData(eMI_CustomizeTrackColor);
   //     if (pTrack->HasCustomColor())
   //     {
   //         contextMenu.main.addAction("Clear Custom Track Color")->setData(eMI_ClearCustomTrackColor);
   //     }
   //     bAppended = true;
   // }

    // Track hide/unhide flags
    if (bOnNode && !pNode->IsGroupNode())
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        QString string = QString("%1 Tracks").arg(QString::fromUtf8(pAnimNode->GetName().c_str()));
        contextMenu.main.addAction(string)->setEnabled(false);

        bool bAppendedTrackFlag = false;

        const unsigned int numChildren = pAnimNode->GetChildCount();
        for (unsigned int childIndex = 0; childIndex < numChildren; ++childIndex)
        {
            CUiAnimViewNode* pChild = pAnimNode->GetChild(childIndex);
            if (pChild->GetNodeType() == eUiAVNT_Track)
            {
                CUiAnimViewTrack* pTrack2 = static_cast<CUiAnimViewTrack*>(pChild);

                if (pTrack2->IsSubTrack())
                {
                    continue;
                }

                QAction* a = contextMenu.main.addAction(QString("  %1").arg(QString::fromUtf8(pTrack2->GetName().c_str())));
                a->setData(eMI_ShowHideBase + childIndex);
                a->setCheckable(true);
                a->setChecked(!pTrack2->IsHidden());
                bAppendedTrackFlag = true;
            }
        }

        bAppended = bAppendedTrackFlag || bAppended;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewNodesCtrl::ShowPopupMenuMultiSelection(UiAnimContextMenu& contextMenu)
{
    QList<QTreeWidgetItem*> records = ui->treeWidget->selectedItems();

    bool bNodeSelected = false;
    for (int currentNode = 0; currentNode < records.size(); ++currentNode)
    {
        CRecord* pItemInfo = (CRecord*)records[currentNode];

        if (pItemInfo->GetNode()->GetNodeType() == eUiAVNT_AnimNode)
        {
            bNodeSelected = true;
        }
    }

    contextMenu.main.addAction("Remove Selected Nodes/Tracks")->setData(eMI_RemoveSelected);

    if (bNodeSelected)
    {
        contextMenu.main.addSeparator();
        contextMenu.main.addAction("Select In Viewport")->setData(eMI_SelectInViewport);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewNodesCtrl::ShowPopupMenu([[maybe_unused]] QPoint point, const CRecord* pRecord)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return 0;
    }

    UiAnimContextMenu contextMenu;

    CUiAnimViewNode* pNode = pRecord ? pRecord->GetNode() : nullptr;
    if (!pNode)
    {
        return 0;
    }

    if (ui->treeWidget->selectedItems().size() > 1)
    {
        ShowPopupMenuMultiSelection(contextMenu);
    }
    else if (pNode)
    {
        ShowPopupMenuSingleSelection(contextMenu, pSequence, pNode);
    }

    if (m_bEditLock)
    {
        SetPopupMenuLock(&contextMenu.main);
    }

    QAction* action = contextMenu.main.exec(QCursor::pos());
    int ret = action ? action->data().toInt() : 0;

    return ret;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::SetPopupMenuLock(QMenu* menu)
{
    if (!m_bEditLock || !menu)
    {
        return;
    }

    UINT count = menu->actions().size();
    for (UINT i = 0; i < count; ++i)
    {
        QAction* a = menu->actions().at(i);
        QString menuString = a->text();

        if (menuString != "Expand" && menuString != "Collapse")
        {
            a->setEnabled(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
float CUiAnimViewNodesCtrl::SaveVerticalScrollPos() const
{
    const QScrollBar* scrollBar = ui->treeWidget->verticalScrollBar();
    const int sbMin = scrollBar->minimum();
    const int sbMax = scrollBar->maximum();
    return float(scrollBar->value()) / std::max(float(sbMax - sbMin), 1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::RestoreVerticalScrollPos(float fScrollPos)
{
    QScrollBar* scrollBar = ui->treeWidget->verticalScrollBar();
    const int sbMin = scrollBar->minimum();
    const int sbMax = scrollBar->maximum();
    const int newScrollPos = FloatToIntRet(fScrollPos * (sbMax - sbMin) + sbMin);
    scrollBar->setValue(newScrollPos);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::FillAutoCompletionListForFilter()
{
    QStringList strings;
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        ui->noitems->hide();
        ui->treeWidget->show();
        ui->searchField->show();
        ui->searchCount->show();
        CUiAnimViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();
        const unsigned int animNodeCount = animNodes.GetCount();

        for (unsigned int i = 0; i < animNodeCount; ++i)
        {
            strings << QString::fromUtf8(animNodes.GetNode(i)->GetName().c_str());
        }
    }
    else
    {
        ui->noitems->show();
        ui->treeWidget->hide();
        ui->searchField->hide();
        ui->searchCount->hide();
    }

    QCompleter* c = new QCompleter(strings, this);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    c->setCompletionMode(QCompleter::InlineCompletion);
    ui->searchField->setCompleter(c);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnFilterChange(const QString& text)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

    if (pSequence)
    {
        m_currentMatchIndex = 0;    // Reset the match index
        m_matchCount = 0;           // and the count.
        if (!text.isEmpty())
        {
            QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(text, Qt::MatchContains | Qt::MatchRecursive);

            CUiAnimViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();

            m_matchCount = items.size();                    // and the count.

            if (!items.empty())
            {
                ui->treeWidget->selectionModel()->clear();
                items.front()->setSelected(true);
            }
        }

        QString matchCountText = QString("%1/%2").arg(m_matchCount == 0 ? 0 : 1).arg(m_matchCount); // One-based indexing
        ui->searchCount->setText(matchCountText);
    }
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimViewNodesCtrl::GetMatNameAndSubMtlIndexFromName(QString& matName, const char* nodeName)
{
    if (const char* pCh = strstr(nodeName, ".["))
    {
        char matPath[MAX_PATH];
        azstrncpy(matPath, AZ_ARRAY_SIZE(matPath), nodeName, (size_t)(pCh - nodeName));
        matName = matPath;
        pCh += 2;
        if ((*pCh) != 0)
        {
            const int index = atoi(pCh) - 1;
            return index;
        }
    }
    else
    {
        matName = nodeName;
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////

void CUiAnimViewNodesCtrl::ShowNextResult()
{
    if (m_matchCount > 1)
    {
        CUiAnimViewSequence* pSequence = nullptr;
        UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);

        if (pSequence && !ui->searchField->text().isEmpty())
        {
            QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(ui->searchField->text(), Qt::MatchContains | Qt::MatchRecursive);

            CUiAnimViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();

            m_matchCount = items.size();                    // and the count.

            if (!items.empty())
            {
                ++m_currentMatchIndex;
                m_currentMatchIndex = m_currentMatchIndex % m_matchCount;
                ui->treeWidget->selectionModel()->clear();
                items[m_currentMatchIndex]->setSelected(true);
            }

            QString matchCountText = QString("%1/%2").arg(m_currentMatchIndex + 1).arg(m_matchCount); // One-based indexing
            ui->searchCount->setText(matchCountText);
        }
    }
}

void CUiAnimViewNodesCtrl::keyPressEvent([[maybe_unused]] QKeyEvent* event)
{
}

void CUiAnimViewNodesCtrl::CreateSetAnimationLayerPopupMenu([[maybe_unused]] QMenu& menuSetLayer, [[maybe_unused]] CUiAnimViewTrack* pTrack) const
{
    // UI_ANIMATION_REVISIT : not used
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::CustomizeTrackColor(CUiAnimViewTrack* pTrack)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    AZ::Color defaultColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (pTrack->HasCustomColor())
    {
        ColorB customColor = pTrack->GetCustomColor();
        defaultColor = AZ::Color(customColor.r, customColor.g, customColor.b, 255);
    }
    const AZ::Color color = AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGB, defaultColor, QObject::tr("Select Color"));

    if (color != defaultColor)
    {
        UiAnimUndo undo("Customize Track Color");
        UiAnimUndo::Record(new CUndoTrackObject(pTrack, pSequence));

        pTrack->SetCustomColor(ColorB(color.GetR8(), color.GetG8(), color.GetB8()));

        UpdateDopeSheet();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::ClearCustomTrackColor(CUiAnimViewTrack* pTrack)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    UiAnimUndo undo("Clear Custom Track Color");
    UiAnimUndo::Record(new CUndoTrackObject(pTrack, pSequence));

    pTrack->ClearCustomColor();
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewNodesCtrl::CRecord* CUiAnimViewNodesCtrl::GetNodeRecord(const CUiAnimViewNode* pNode) const
{
    auto findIter = m_nodeToRecordMap.find(pNode);
    if (findIter == m_nodeToRecordMap.end())
    {
        return nullptr;
    }

    assert (findIter->second->GetNode() == pNode);
    return findIter->second;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::UpdateDopeSheet()
{
    UpdateRecordVisibility();

    if (m_pDopeSheet)
    {
        m_pDopeSheet->update();
    }
}

//////////////////////////////////////////////////////////////////////////
// Workaround: CXTPReportRecord::IsVisible is
// unreliable after the last visible element
//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::UpdateRecordVisibility()
{
    // Mark all records invisible
    for (auto iter = m_nodeToRecordMap.begin(); iter != m_nodeToRecordMap.end(); ++iter)
    {
        iter->second->m_bVisible = ui->treeWidget->visualItemRect(iter->second).isValid();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::UpdateAllNodesForElementChanges()
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (pSequence)
    {
        CUiAnimViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();
        const unsigned int animNodeCount = animNodes.GetCount();
        for (unsigned int i = 0; i < animNodeCount; ++i)
        {
            CRecord* pNodeRecord = GetNodeRecord(animNodes.GetNode(i));
            UpdateNodeRecord(pNodeRecord);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnNodeChanged(CUiAnimViewNode* pNode, IUiAnimViewSequenceListener::ENodeChangeType type)
{
    CUiAnimViewSequence* pSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!pSequence)
    {
        return;
    }

    if (!m_bIgnoreNotifications)
    {
        CUiAnimViewNode* pParentNode = pNode->GetParentNode();

        CRecord* pNodeRecord = GetNodeRecord(pNode);
        CRecord* pParentNodeRecord = pParentNode ? GetNodeRecord(pParentNode) : nullptr;

        float storedScrollPosition = SaveVerticalScrollPos();

        switch (type)
        {
        case IUiAnimViewSequenceListener::eNodeChangeType_Added:
        case IUiAnimViewSequenceListener::eNodeChangeType_Unhidden:
            if (pParentNodeRecord)
            {
                AddNodeRecord(pParentNodeRecord, pNode);
            }
            break;
        case IUiAnimViewSequenceListener::eNodeChangeType_Removed:
        case IUiAnimViewSequenceListener::eNodeChangeType_Hidden:
            if (pNodeRecord)
            {
                EraseNodeRecordRec(pNode);
                delete pNodeRecord;
            }
            break;
        case IUiAnimViewSequenceListener::eNodeChangeType_Expanded:
            if (pNodeRecord)
            {
                pNodeRecord->setExpanded(true);
            }
            break;
        case IUiAnimViewSequenceListener::eNodeChangeType_Collapsed:
            if (pNodeRecord)
            {
                pNodeRecord->setExpanded(false);
            }
            break;
        case IUiAnimViewSequenceListener::eNodeChangeType_Disabled:
        case IUiAnimViewSequenceListener::eNodeChangeType_Enabled:
        case IUiAnimViewSequenceListener::eNodeChangeType_Muted:
        case IUiAnimViewSequenceListener::eNodeChangeType_Unmuted:
        case IUiAnimViewSequenceListener::eNodeChangeType_NodeOwnerChanged:
            if (pNodeRecord)
            {
                UpdateNodeRecord(pNodeRecord);
            }
        }

        switch (type)
        {
        case IUiAnimViewSequenceListener::eNodeChangeType_Added:
        case IUiAnimViewSequenceListener::eNodeChangeType_Unhidden:
        case IUiAnimViewSequenceListener::eNodeChangeType_Removed:
        case IUiAnimViewSequenceListener::eNodeChangeType_Hidden:
        case IUiAnimViewSequenceListener::eNodeChangeType_Expanded:
        case IUiAnimViewSequenceListener::eNodeChangeType_Collapsed:
            update();
            RestoreVerticalScrollPos(storedScrollPosition);
            break;
        case eNodeChangeType_SetAsActiveDirector:
            update();
            break;
        }
    }
    else
    {
        m_bNeedReload = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnNodeRenamed(CUiAnimViewNode* pNode, [[maybe_unused]] const char* pOldName)
{
    if (!m_bIgnoreNotifications)
    {
        CRecord* pNodeRecord = GetNodeRecord(pNode);
        pNodeRecord->setText(0, QString::fromUtf8(pNode->GetName().c_str()));

        update();
    }
    else
    {
        m_bNeedReload = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::BeginUndoTransaction()
{
    m_bNeedReload = false;
    m_bIgnoreNotifications = true;
    m_storedScrollPosition = SaveVerticalScrollPos();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::EndUndoTransaction()
{
    m_bIgnoreNotifications = false;

    if (m_bNeedReload)
    {
        Reload();
        RestoreVerticalScrollPos(m_storedScrollPosition);
        m_bNeedReload = false;
    }

    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
QIcon CUiAnimViewNodesCtrl::GetIconForTrack([[maybe_unused]] const CUiAnimViewTrack* pTrack)
{
    return QIcon(QStringLiteral(":/nodes/tvnodes-13.png"));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnKeysChanged(CUiAnimViewSequence* pSequence)
{
    CUiAnimViewSequence* pCurrentSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pCurrentSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!m_bIgnoreNotifications && pSequence && pSequence == pCurrentSequence)
    {
        UpdateDopeSheet();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnKeySelectionChanged(CUiAnimViewSequence* pSequence)
{
    OnKeysChanged(pSequence);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::OnNodeSelectionChanged(CUiAnimViewSequence* pSequence)
{
    if (m_bSelectionChanging)
    {
        return;
    }

    CUiAnimViewSequence* pCurrentSequence = nullptr;
    UiEditorAnimationBus::BroadcastResult(pCurrentSequence, &UiEditorAnimationBus::Events::GetCurrentSequence);
    if (!m_bIgnoreNotifications && pSequence && pSequence == pCurrentSequence)
    {
        UpdateDopeSheet();

        CUiAnimViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();
        const uint numNodes = animNodes.GetCount();
        for (uint i = 0; i < numNodes; ++i)
        {
            CUiAnimViewAnimNode* pNode = animNodes.GetNode(i);
            if (pNode->IsSelected())
            {
                SelectRow(pNode, false, false);
            }
            else
            {
                DeselectRow(pNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::SelectRow(CUiAnimViewNode* pNode, const bool bEnsureVisible, const bool bDeselectOtherRows)
{
    std::unordered_map<const CUiAnimViewNode*, CRecord*>::const_iterator it = m_nodeToRecordMap.find(pNode);
    if (it != m_nodeToRecordMap.end())
    {
        if (bDeselectOtherRows)
        {
            ui->treeWidget->selectionModel()->clear();
        }
        (*it).second->setSelected(true);
        if (bEnsureVisible)
        {
            ui->treeWidget->scrollToItem((*it).second);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::DeselectRow(CUiAnimViewNode* pNode)
{
    std::unordered_map<const CUiAnimViewNode*, CRecord*>::const_iterator it = m_nodeToRecordMap.find(pNode);
    if (it != m_nodeToRecordMap.end())
    {
        (*it).second->setSelected(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewNodesCtrl::EraseNodeRecordRec(CUiAnimViewNode* pNode)
{
    m_nodeToRecordMap.erase(pNode);

    const unsigned int numChildren = pNode->GetChildCount();
    for (unsigned int i = 0; i < numChildren; ++i)
    {
        EraseNodeRecordRec(pNode->GetChild(i));
    }
}

#include <Animation/moc_UiAnimViewNodes.cpp>
