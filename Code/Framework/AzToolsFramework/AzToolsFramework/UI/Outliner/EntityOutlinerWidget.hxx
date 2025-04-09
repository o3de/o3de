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
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerCacheBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerRequestBus.h>
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
    class FocusModeInterface;
    class ReadOnlyEntityPublicInterface;

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
        , private Prefab::PrefabFocusNotificationBus::Handler
        , private Prefab::PrefabPublicNotificationBus::Handler
        , private EditorWindowUIRequestBus::Handler
        , private EntityOutlinerRequestBus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerWidget, AZ::SystemAllocator)

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

        // PrefabFocusNotificationBus overrides ...
        void OnPrefabEditScopeChanged() override;

        // PrefabPublicNotificationBus  overrides ...
        void OnPrefabInstancePropagationBegin() override;
        void OnPrefabInstancePropagationEnd() override;
        void OnPrefabTemplateDirtyFlagUpdated(Prefab::TemplateId templateId, bool status) override;

        // EditorWindowUIRequestBus overrides ...
        void SetEditorUiEnabled(bool enable) override;

        // EntityOutlinerRequestBus overrides ...
        void TriggerRenameEntityUi(const AZ::EntityId& entityId) override;

        // Build a selection object from the given entities. Entities already in the Widget's selection buffers are ignored.
        template <class EntityIdCollection>
        QItemSelection BuildSelectionFromEntities(const EntityIdCollection& entityIds);

        Ui::EntityOutlinerWidgetUI* m_gui;
        EntityOutlinerListModel* m_listModel;
        EntityOutlinerSortFilterProxyModel* m_proxyModel;
        AZStd::vector<AZ::EntityId> m_selectedEntityIds;

        // ToolsApplicationEventBus handler
        void BeforeUndoRedo() override;
        void AfterUndoRedo() override;

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

        void OnTreeItemClicked(const QModelIndex& index);
        void OnTreeItemDoubleClicked(const QModelIndex& index);
        void OnTreeItemExpanded(const QModelIndex& index);
        void OnTreeItemCollapsed(const QModelIndex& index);
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

        bool m_isDuringUndoRedo = false;

        void QueueContentUpdateSort(const AZ::EntityId& entityId);
        void SortContent();

        EntityOutliner::DisplayOptionsMenu* m_displayOptionsMenu;
        EntityIdSet m_entitiesToSort;
        EntityOutliner::DisplaySortMode m_sortMode;
        bool m_sortContentQueued;

        AzFramework::EntityContextId m_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        EditorEntityUiInterface* m_editorEntityUiInterface = nullptr;
        FocusModeInterface* m_focusModeInterface = nullptr;
        ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;
    };

}
