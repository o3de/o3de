/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommon.h"

#include <AzQtComponents/Components/Widgets/TreeView.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>

#include <QTreeWidget>
#endif

class QMimeData;

class HierarchyWidget
    : public AzQtComponents::StyledTreeWidget
    , private AzToolsFramework::EditorPickModeNotificationBus::Handler
    , private AzToolsFramework::EntityHighlightMessages::Bus::Handler
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(HierarchyWidget, AZ::SystemAllocator);

    HierarchyWidget(EditorWindow* editorWindow);
    virtual ~HierarchyWidget();

    void SetIsDeleting(bool b);

    EntityHelpers::EntityToHierarchyItemMap& GetEntityItemMap();
    EditorWindow* GetEditorWindow();

    void ActiveCanvasChanged();
    void EntityContextChanged();

    void CreateItems(const LyShine::EntityArray& elements);
    void RecreateItems(const LyShine::EntityArray& elements);
    void ClearItems();

    AZ::Entity* CurrentSelectedElement() const;

    void SetUniqueSelectionHighlight(QTreeWidgetItem* item);
    void SetUniqueSelectionHighlight(const AZ::Entity* element);

    void AddElement(const QTreeWidgetItemRawPtrQList& selectedItems, const QPoint* optionalPos);

    void SignalUserSelectionHasChanged(const QTreeWidgetItemRawPtrQList& selectedItems);

    //! When we delete the Editor window we call this. It avoid the element Entities
    //! being deleted when the HierarchyItem is deleted
    void ClearAllHierarchyItemEntityIds();

    void ApplyElementIsExpanded();

    void ClearItemBeingHovered();

    //! Update the appearance of all hierarchy items to reflect their slice status
    void UpdateSliceInfo();

    //! Drop assets from asset browser
    void DropMimeDataAssets(const QMimeData* data,
        const AZ::EntityId& targetEntityId,
        bool onElement,
        int childIndex,
        const QPoint* newElementPosition = nullptr);

public slots:
    void DeleteSelectedItems();
    void Cut();
    void Copy();
    void PasteAsSibling();
    void PasteAsChild();
    void SetEditorOnlyForSelectedItems(bool editorOnly);

signals:

    void SetUserSelection(HierarchyItemRawPtrList* items);
    void editorOnlyStateChangedOnSelectedElements();

private slots:

    // This is used to detect the user changing the selection in the Hierarchy.
    void CurrentSelectionHasChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void DataHasChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

    void HandleItemAdd(HierarchyItem* item);
    void HandleItemRemove(HierarchyItem* item);

protected:

    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void leaveEvent(QEvent* ev) override;
    void contextMenuEvent(QContextMenuEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    void dropEvent(QDropEvent* ev) override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QTreeWidgetItem*> items) const override;

private:

    // EditorPickModeNotificationBus
    void OnEntityPickModeStarted() override;
    void OnEntityPickModeStopped() override;

    // EntityHighlightMessages
    void EntityHighlightRequested(AZ::EntityId entityId) override;
    void EntityStrongHighlightRequested(AZ::EntityId entityId) override;
    // ~EntityHighlightMessages

    void PickItem(HierarchyItem* item);

    bool IsEntityInEntityContext(AZ::EntityId entityId);

    void ReparentItems(const QTreeWidgetItemRawPtrList& baseParentItems,
        const HierarchyItemRawPtrList& childItems);

    void ToggleVisibility(HierarchyItem* hierarchyItem);
    void DeleteSelectedItems(const QTreeWidgetItemRawPtrQList& selectedItems);

    bool AcceptsMimeData(const QMimeData* mimeData);
    
    // Drag/drop assets from asset browser
    void DropMimeDataAssetsAtHierarchyPosition(const QMimeData* data, const QPoint& position);
    void DropMimeDataAssets(const QMimeData* data,
        QTreeWidgetItem* targetWidgetItem,
        bool onElement,
        int childIndex,
        const QPoint* newElementPosition);

    bool m_isDeleting;

    EditorWindow* m_editorWindow;

    EntityHelpers::EntityToHierarchyItemMap m_entityItemMap;

    HierarchyItem* m_itemBeingHovered;

    QTreeWidgetItemRawPtrQList m_beforeDragSelection;
    QTreeWidgetItemRawPtrQList m_dragSelection;
    bool m_inDragStartState;
    bool m_selectionChangedBeforeDrag;
    bool m_signalSelectionChange;

    bool m_inObjectPickMode;

    // Used to restore the normal hierarchy mode after pick mode is complete
    QAbstractItemView::SelectionMode m_selectionModeBeforePickMode;
    EditTriggers m_editTriggersBeforePickMode;
    QModelIndex m_currentItemBeforePickMode;

    bool m_isInited;
};
