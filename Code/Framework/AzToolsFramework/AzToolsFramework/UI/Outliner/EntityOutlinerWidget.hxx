/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>

#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerCacheBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSearchWidget.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>

#include <QIcon>
#include <QWidget>
#endif

#pragma once

class QAction;
class QItemSelection;

namespace Ui
{
    class EntityOutlinerWidgetUI;
}

namespace AzToolsFramework
{
    class EditorEntityUiInterface;
    class EntityOutlinerListModel;
    class EntityOutlinerContainerProxyModel;
    class EntityOutlinerSortFilterProxyModel;

    namespace EntityOutliner
    {
        class DisplayOptionsMenu;
        enum class DisplaySortMode : unsigned char;
        enum class DisplayOption : unsigned char;
    }

    class EntityOutlinerWidget
        : public QWidget
        , private EditorPickModeNotificationBus::Handler
        , private EntityHighlightMessages::Bus::Handler
        , private EntityOutlinerModelNotificationBus::Handler
        , private ToolsApplicationEvents::Bus::Handler
        , private EditorEntityContextNotificationBus::Handler
        , private EditorEntityInfoNotificationBus::Handler
        , private ViewportEditorModeNotificationsBus::Handler
        , private Prefab::PrefabPublicNotificationBus::Handler
        , private EditorWindowUIRequestBus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerWidget, AZ::SystemAllocator, 0)

        explicit EntityOutlinerWidget(QWidget* pParent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~EntityOutlinerWidget();

    private Q_SLOTS:
        void OnSelectionChanged(const QItemSelection&, const QItemSelection&);
        void OnOpenTreeContextMenu(const QPoint& pos);

        void OnSearchTextChanged(const QString& activeTextFilter);
        void OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

        void OnSortModeChanged(EntityOutliner::DisplaySortMode sortMode);
        void OnDisplayOptionChanged(EntityOutliner::DisplayOption displayOption, bool enable);

    private:
        AzFramework::EntityContextId GetPickModeEntityContextId();

        // EntityHighlightMessages
        virtual void EntityHighlightRequested(AZ::EntityId) override;
        virtual void EntityStrongHighlightRequested(AZ::EntityId) override;

        // EditorPickModeNotificationBus
        void OnEntityPickModeStarted() override;
        void OnEntityPickModeStopped() override;

        // EditorEntityContextNotificationBus
        void OnEditorEntityCreated(const AZ::EntityId& entityId) override;
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;
        void OnFocusInEntityOutliner(const EntityIdList& entityIdList) override;

        /// EditorEntityInfoNotificationBus implementation
        void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) override;
        void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& /*name*/) override;

        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

        // PrefabPublicNotificationBus
        void OnPrefabInstancePropagationBegin() override;
        void OnPrefabInstancePropagationEnd() override;

        // EditorWindowUIRequestBus overrides
        void SetEditorUiEnabled(bool enable) override;

        // Build a selection object from the given entities. Entities already in the Widget's selection buffers are ignored.
        template <class EntityIdCollection>
        QItemSelection BuildSelectionFromEntities(const EntityIdCollection& entityIds);

        Ui::EntityOutlinerWidgetUI* m_gui;
        EntityOutlinerListModel* m_listModel;
        EntityOutlinerContainerProxyModel* m_containerModel;
        EntityOutlinerSortFilterProxyModel* m_proxyModel;
        AZ::u64 m_selectionContextId;
        AZStd::vector<AZ::EntityId> m_selectedEntityIds;

        void PrepareSelection();
        void DoCreateEntity();
        void DoCreateEntityWithParent(const AZ::EntityId& parentId);
        void DoDuplicateSelection();
        void DoDeleteSelection();
        void DoDeleteSelectionAndDescendants();
        void DoRenameSelection();
        void DoMoveEntityUp();
        void DoMoveEntityDown();
        void GoToEntitiesInViewport();

        void SetIndexAsCurrentAndSelected(const QModelIndex& index);

        void SetupActions();

        QAction* m_actionToCreateEntity;
        QAction* m_actionToDeleteSelection;
        QAction* m_actionToDeleteSelectionAndDescendants;
        QAction* m_actionToRenameSelection;
        QAction* m_actionToReparentSelection;
        QAction* m_actionToMoveEntityUp;
        QAction* m_actionToMoveEntityDown;
        QAction* m_actionGoToEntitiesInViewport;

        void OnTreeItemClicked(const QModelIndex& index);
        void OnTreeItemDoubleClicked(const QModelIndex& index);
        void OnTreeItemExpanded(const QModelIndex& index);
        void OnTreeItemCollapsed(const QModelIndex& index);
        void OnExpandEntity(const AZ::EntityId& entityId, bool expand);
        void OnSelectEntity(const AZ::EntityId& entityId, bool selected);
        void OnEnableSelectionUpdates(bool enable);
        void OnDropEvent();
        bool m_inObjectPickMode;

        void InvalidateFilter();
        void ClearFilter();

        AZ::EntityId GetEntityIdFromIndex(const QModelIndex& index) const;
        QModelIndex GetIndexFromEntityId(const AZ::EntityId& entityId) const;
        void ExtractEntityIdsFromSelection(const QItemSelection& selection, EntityIdList& entityIdList) const;
        void EnableUi(bool enable);

        // OutlinerModelNotificationBus::Handler
        // Receive notification from the outliner model that we should scroll
        // to a given entity
        void QueueScrollToNewContent(const AZ::EntityId& entityId) override;

        void SetDefaultTreeViewEditTriggers();

        void ScrollToNewContent();
        bool m_scrollToNewContentQueued;
        bool m_scrollToSelectedEntity;
        bool m_dropOperationInProgress;
        bool m_expandSelectedEntity;
        bool m_focusInEntityOutliner;
        AZ::EntityId m_scrollToEntityId;

        void QueueUpdateSelection();
        void UpdateSelection();
        EntityIdSet m_entitiesToSelect;
        EntityIdSet m_entitiesToDeselect;
        EntityIdSet m_entitiesSelectedByOutliner;
        EntityIdSet m_entitiesDeselectedByOutliner;
        bool m_selectionChangeQueued;
        bool m_selectionChangeInProgress;
        bool m_enableSelectionUpdates;

        QIcon m_emptyIcon;
        QIcon m_clearIcon;

        void QueueContentUpdateSort(const AZ::EntityId& entityId);
        void SortContent();

        EntityOutliner::DisplayOptionsMenu* m_displayOptionsMenu;
        EntityIdSet m_entitiesToSort;
        EntityOutliner::DisplaySortMode m_sortMode;
        bool m_sortContentQueued;

        EditorEntityUiInterface* m_editorEntityUiInterface = nullptr;
    };

}
