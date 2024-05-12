/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include "UiAnimViewNode.h"
#include "UiAnimViewSequence.h"
#include "Undo/IUndoManagerListener.h"

#include <LyShine/Animation/IUiAnimation.h>
#include <QMap>
#include <QTreeWidgetItem>
#endif

// forward declarations.
class CUiAnimViewNode;
class CUiAnimViewAnimNode;
class CUiAnimViewTrack;
class CUiAnimViewSequence;
class CUiAnimViewDopeSheetBase;
class CUiAnimViewDialog;

class QLineEdit;


namespace Ui {
    class CUiAnimViewNodesCtrl;
}

//////////////////////////////////////////////////////////////////////////
class CUiAnimViewNodesCtrl
    : public QWidget
    , public IUiAnimViewSequenceListener
    , public IUndoManagerListener
{
    Q_OBJECT
public:
    class CRecord
        : public QTreeWidgetItem
    {
        friend class CUiAnimViewNodesCtrl;

    public:
        enum Roles
        {
            EnableRole = Qt::UserRole + 1
        };

        CRecord(CUiAnimViewNode* pNode = nullptr);
        CUiAnimViewNode* GetNode() const { return m_pNode; }
        bool IsGroup() const { return m_pNode->GetChildCount() != 0; }
        const QString GetName() const { return QString::fromUtf8(m_pNode->GetName().c_str()); }

        // Workaround: CXTPReportRecord::IsVisible is
        // unreliable after the last visible element
        bool IsVisible() const { return m_bVisible; }

        QRect GetRect() const { return treeWidget()->visualItemRect(this);  }

    private:
        bool m_bVisible;
        CUiAnimViewNode* m_pNode;
    };

    CUiAnimViewNodesCtrl(QWidget* hParentWnd, CUiAnimViewDialog* parent = 0);
    ~CUiAnimViewNodesCtrl();

    void SetUiAnimViewDialog(CUiAnimViewDialog* dlg) { m_pUiAnimViewDialog = dlg; }
    void OnSequenceChanged();

    void SetDopeSheet(CUiAnimViewDopeSheetBase* keysCtrl);

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    float SaveVerticalScrollPos() const;
    void RestoreVerticalScrollPos(float fScrollPos);

    CRecord* GetNodeRecord(const CUiAnimViewNode* pNode) const;

    virtual void Reload();
    virtual void OnFillItems();

    void UpdateAllNodesForElementChanges();

    // IUiAnimViewSequenceListener
    virtual void OnNodeChanged(CUiAnimViewNode* pNode, IUiAnimViewSequenceListener::ENodeChangeType type) override;
    virtual void OnNodeRenamed(CUiAnimViewNode* pNode, const char* pOldName) override;
    virtual void OnNodeSelectionChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnKeysChanged(CUiAnimViewSequence* pSequence) override;
    virtual void OnKeySelectionChanged(CUiAnimViewSequence* pSequence) override;

    // IUndoManagerListener
    virtual void BeginUndoTransaction() override;
    virtual void EndUndoTransaction() override;

    // Helper for dialog
    void ShowNextResult();

    QIcon GetIconForTrack(const CUiAnimViewTrack* pTrack);
    static QIcon NodeTypeToTrackViewIcon(EUiAnimNodeType type);
protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* o, QEvent* e) override;

private slots:
    void OnNMRclick(QPoint pos);
    void OnItemExpanded(QTreeWidgetItem*);
    void OnSelectionChanged();
    void OnFilterChange(const QString& text);

private:
    void AddMenuSeperatorConditional(QMenu& menu, bool& bAppended);
    void AddGroupNodeAddItems(struct UiAnimContextMenu& contextMenu, CUiAnimViewAnimNode* pAnimNode);
    int ShowPopupMenuSingleSelection(struct UiAnimContextMenu& contextMenu, CUiAnimViewSequence* pSequence, CUiAnimViewNode* pNode);
    int ShowPopupMenuMultiSelection(struct UiAnimContextMenu& contextMenu);
    int ShowPopupMenu(QPoint point, const CRecord* pItemInfo);
    void EditEvents();

    void SetPopupMenuLock(QMenu* menu);
    void CreateSetAnimationLayerPopupMenu(QMenu& menuSetLayer, CUiAnimViewTrack* pTrack) const;

    void AddNodeRecord(CRecord* pParentRecord, CUiAnimViewNode* pNode);
    CRecord* AddTrackRecord(CRecord* pParentRecord, CUiAnimViewTrack* pTrack);
    CRecord* AddAnimNodeRecord(CRecord* pParentRecord, CUiAnimViewAnimNode* pNode);

    void FillNodesRec(CRecord* pRecord, CUiAnimViewNode* pCurrentNode);

    void EraseNodeRecordRec(CUiAnimViewNode* pNode);

    void UpdateNodeRecord(CRecord* pRecord);
    void UpdateTrackRecord(CRecord* pRecord, CUiAnimViewTrack* pTrack);
    void UpdateUiAnimNodeRecord(CRecord* pRecord, CUiAnimViewAnimNode* pAnimNode);

    void FillAutoCompletionListForFilter();

    // Utility function for handling material nodes
    // It'll return -1 if the found material isn't a multi-material.
    static int GetMatNameAndSubMtlIndexFromName(QString& matName, const char* nodeName);

    void CustomizeTrackColor(CUiAnimViewTrack* pTrack);
    void ClearCustomTrackColor(CUiAnimViewTrack* pTrack);

    // For drawing dope sheet
    void UpdateRecordVisibility();

    void UpdateDopeSheet();

    int GetInsertPosition(CRecord* pParentRecord, CUiAnimViewNode* pNode);

    void SelectRow(CUiAnimViewNode* pNode, const bool bEnsureVisible, const bool bDeselectOtherRows);
    void DeselectRow(CUiAnimViewNode* pNode);

    CUiAnimViewDopeSheetBase* m_pDopeSheet;
    CUiAnimViewDialog* m_pUiAnimViewDialog;

    typedef std::vector<CRecord*> ItemInfos;
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
    CUiAnimViewAnimNodeBundle m_draggedNodes;

    std::unordered_map<const CUiAnimViewNode*, CRecord*> m_nodeToRecordMap;

    QScopedPointer<Ui::CUiAnimViewNodesCtrl> ui;
};


typedef CUiAnimViewNode* CUiAnimViewNodePtr;
Q_DECLARE_METATYPE(CUiAnimViewNodePtr);
QDataStream& operator<<(QDataStream& out, const CUiAnimViewNodePtr& obj);
QDataStream& operator>>(QDataStream& in, CUiAnimViewNodePtr& obj);
