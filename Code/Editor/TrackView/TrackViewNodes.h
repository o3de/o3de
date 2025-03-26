/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : TrackView's tree control.


#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include "AnimationContext.h"
#include "TrackViewNode.h"
#include "TrackViewSequence.h"
#include "Undo/Undo.h"

#include <IMovieSystem.h>
#include <QMap>
#include <QTreeWidgetItem>

#include <QWidget>
#endif

// forward declarations.
class CTrackViewAnimNode;
class CTrackViewDialog;
class CTrackViewDopeSheetBase;
class CTrackViewNode;
class CTrackViewSequence;
class CTrackViewTrack;

class QLineEdit;


namespace Ui {
    class CTrackViewNodesCtrl;
}

//////////////////////////////////////////////////////////////////////////
class CTrackViewNodesCtrl
    : public QWidget
    , public ITrackViewSequenceListener
    , public IUndoManagerListener
    , public IAnimationContextListener
    , public ITrackViewSequenceManagerListener
{
    Q_OBJECT
public:
    class CRecord
        : public QTreeWidgetItem
    {
        friend class CTrackViewNodesCtrl;

    public:
        enum Roles
        {
            EnableRole = Qt::UserRole + 1
        };

        CRecord(CTrackViewNode* pNode = nullptr);
        CTrackViewNode* GetNode() const { return m_pNode; }
        bool IsGroup() const { return m_pNode->GetChildCount() != 0; }
        const QString GetName() const { return QString::fromUtf8(m_pNode->GetName().c_str()); }

        // Workaround: CXTPReportRecord::IsVisible is
        // unreliable after the last visible element
        bool IsVisible() const { return m_bVisible; }

        QRect GetRect() const { return treeWidget()->visualItemRect(this);  }

    private:
        bool m_bVisible;
        CTrackViewNode* m_pNode;
    };

    CTrackViewNodesCtrl(QWidget* hParentWnd, CTrackViewDialog* parent = 0);
    ~CTrackViewNodesCtrl();

    void SetTrackViewDialog(CTrackViewDialog* dlg) { m_pTrackViewDialog = dlg; }
    void OnSequenceChanged();

    void SetDopeSheet(CTrackViewDopeSheetBase* keysCtrl);

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    float SaveVerticalScrollPos() const;
    void RestoreVerticalScrollPos(float fScrollPos);

    CRecord* GetNodeRecord(const CTrackViewNode* pNode) const;

    virtual void Reload();
    virtual void OnFillItems();

    // ITrackViewSequenceListener
    void OnNodeChanged(CTrackViewNode* pNode, ITrackViewSequenceListener::ENodeChangeType type) override;
    void OnNodeRenamed(CTrackViewNode* pNode, const char* pOldName) override;
    void OnNodeSelectionChanged(CTrackViewSequence* pSequence) override;
    void OnKeysChanged(CTrackViewSequence* pSequence) override;
    void OnKeySelectionChanged(CTrackViewSequence* pSequence) override;

    // IUndoManagerListener
    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;

    // IAnimationContextListener
    void OnSequenceChanged(CTrackViewSequence* pNewSequence) override;

    // ITrackViewSequenceManagerListener
    void OnSequenceRemoved(CTrackViewSequence* pSequence) override;

    // Helper for dialog
    QIcon GetIconForTrack(const CTrackViewTrack* pTrack);
    void ShowNextResult();

    void Update();

    static QIcon TrackViewIcon(const CTrackViewTrack* track);
    static QIcon TrackViewNodeIcon(AnimNodeType type);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* o, QEvent* e) override;

private slots:
    void OnNMRclick(QPoint pos);
    void OnItemExpanded(QTreeWidgetItem*);
    void OnSelectionChanged();
    void OnItemDblClick(QTreeWidgetItem* item, int);
    void OnFilterChange(const QString& text);

private:
    void CreateFolder(CTrackViewAnimNode* pGroupNode);
    void EditEvents();

    void AddMenuSeperatorConditional(QMenu& menu, bool& bAppended);
    void AddGroupNodeAddItems(struct SContextMenu& contextMenu, CTrackViewAnimNode* pAnimNode);
    int ShowPopupMenuSingleSelection(struct SContextMenu& contextMenu, CTrackViewSequence* pSequence, CTrackViewNode* pNode);
    int ShowPopupMenuMultiSelection(struct SContextMenu& contextMenu);
    int ShowPopupMenu(QPoint point, const CRecord* pItemInfo);

    bool FillAddTrackMenu(struct STrackMenuTreeNode& menuAddTrack, const CTrackViewAnimNode* pAnimNode);

    void CreateAddTrackMenuRec(QMenu& parent, const QString& name, CTrackViewAnimNode* animNode, struct STrackMenuTreeNode& node, unsigned int& currentId);

    void SetPopupMenuLock(QMenu* menu);
    void CreateSetAnimationLayerPopupMenu(QMenu& menuSetLayer, CTrackViewTrack* pTrack) const;

    void AddNodeRecord(CRecord* pParentRecord, CTrackViewNode* pNode);
    CRecord* AddTrackRecord(CRecord* pParentRecord, CTrackViewTrack* pTrack);
    CRecord* AddAnimNodeRecord(CRecord* pParentRecord, CTrackViewAnimNode* pNode);

    void FillNodesRec(CRecord* pRecord, CTrackViewNode* pCurrentNode);

    void EraseNodeRecordRec(CTrackViewNode* pNode);

    void UpdateNodeRecord(CRecord* pRecord);
    void UpdateTrackRecord(CRecord* pRecord, CTrackViewTrack* pTrack);
    void UpdateAnimNodeRecord(CRecord* pRecord, CTrackViewAnimNode* pAnimNode);

    void FillAutoCompletionListForFilter();

    // Utility function for handling material nodes
    // It'll return -1 if the found material isn't a multi-material.
    static int GetMatNameAndSubMtlIndexFromName(QString& matName, const char* nodeName);

    void CustomizeTrackColor(CTrackViewTrack* pTrack);
    void ClearCustomTrackColor(CTrackViewTrack* pTrack);

    // For drawing dope sheet
    void UpdateRecordVisibility();

    void UpdateDopeSheet();

    int GetInsertPosition(CRecord* pParentRecord, CTrackViewNode* pNode);

    void SelectRow(CTrackViewNode* pNode, const bool bEnsureVisible, const bool bDeselectOtherRows);
    void DeselectRow(CTrackViewNode* pNode);

    CTrackViewDopeSheetBase* m_pDopeSheet;
    CTrackViewDialog* m_pTrackViewDialog;

    typedef AZStd::vector<CRecord*> ItemInfos;
    ItemInfos m_itemInfos;

    bool m_bSelectionChanging;
    bool m_bEditLock;

    QCursor m_arrowCursor;
    QCursor m_noIcon;

    UINT m_currentMatchIndex;
    UINT m_matchCount;

    bool m_bIgnoreNotifications;
    bool m_bNeedReload;
    float m_storedScrollPosition;

    // Drag and drop
    CTrackViewAnimNodeBundle m_draggedNodes;

    AZStd::unordered_map<unsigned int, CAnimParamType> m_menuParamTypeMap;
    AZStd::unordered_map<const CTrackViewNode*, CRecord*> m_nodeToRecordMap;

    QScopedPointer<Ui::CTrackViewNodesCtrl> ui;

    //! Cached map of component icons.
    //! Key: Component's RTTI type
    //! Value: Icon for this component
    AZStd::unordered_map<AZ::Uuid, QIcon> m_componentTypeToIconMap;
};


typedef CTrackViewNode* CTrackViewNodePtr;
Q_DECLARE_METATYPE(CTrackViewNodePtr);
QDataStream& operator<<(QDataStream& out, const CTrackViewNodePtr& obj);
QDataStream& operator>>(QDataStream& in, CTrackViewNodePtr& obj);

