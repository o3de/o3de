/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef OUTLINER_VIEW_H
#define OUTLINER_VIEW_H

#if !defined(Q_MOC_RUN)
#include "OutlinerCacheBus.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>
#include "OutlinerSearchWidget.h"

#include <QWidget>
#include <QtGui/QIcon>
#endif

#pragma once

class QAction;

namespace Ui
{
    class OutlinerWidgetUI;
}

class QItemSelection;

class OutlinerListModel;
class OutlinerSortFilterProxyModel;

namespace EntityOutliner
{
    class DisplayOptionsMenu;
    enum class DisplaySortMode : unsigned char;
    enum class DisplayOption : unsigned char;
}

class OutlinerWidget
    : public QWidget
    , private AzToolsFramework::EditorPickModeNotificationBus::Handler
    , private AzToolsFramework::EntityHighlightMessages::Bus::Handler
    , private OutlinerModelNotificationBus::Handler
    , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
    , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
    , private AzToolsFramework::EditorWindowUIRequestBus::Handler
{
    Q_OBJECT;
public:
    AZ_CLASS_ALLOCATOR(OutlinerWidget, AZ::SystemAllocator, 0)

    OutlinerWidget(QWidget* pParent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
    virtual ~OutlinerWidget();

private Q_SLOTS:
    void OnSelectionChanged(const QItemSelection&, const QItemSelection&);

    void OnSearchTextChanged(const QString& activeTextFilter);
    void OnFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

    void OnSortModeChanged(EntityOutliner::DisplaySortMode sortMode);
    void OnDisplayOptionChanged(EntityOutliner::DisplayOption displayOption, bool enable);

private:
    void contextMenuEvent(QContextMenuEvent* event) override;

    QString FindCommonSliceAssetName(const AZStd::vector<AZ::EntityId>& entityList) const;

    AzFramework::EntityContextId GetPickModeEntityContextId();

    // EntityHighlightMessages
    virtual void EntityHighlightRequested(AZ::EntityId) override;
    virtual void EntityStrongHighlightRequested(AZ::EntityId) override;

    // EditorPickModeNotificationBus
    void OnEntityPickModeStarted() override;
    void OnEntityPickModeStopped() override;

    // SliceEditorEntityOwnershipServiceNotificationBus
    void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/, const AzFramework::SliceInstantiationTicket& /*ticket*/) override;

    // EditorEntityContextNotificationBus
    void OnEditorEntityCreated(const AZ::EntityId& entityId) override;
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    void OnFocusInEntityOutliner(const AzToolsFramework::EntityIdList& entityIdList) override;

    /// AzToolsFramework::EditorEntityInfoNotificationBus implementation
    void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId /*parentId*/, AZ::EntityId /*childId*/) override;
    void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& /*name*/) override;

    // ViewportEditorModeNotificationsBus overrides ...
    void OnEditorModeActivated(
        const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
    void OnEditorModeDeactivated(
        const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

    // EditorWindowUIRequestBus overrides
    void SetEditorUiEnabled(bool enable) override;

    // Build a selection object from the given entities. Entities already in the Widget's selection buffers are ignored.
    template <class EntityIdCollection>
    QItemSelection BuildSelectionFromEntities(const EntityIdCollection& entityIds);

    Ui::OutlinerWidgetUI* m_gui;
    OutlinerListModel* m_listModel;
    OutlinerSortFilterProxyModel* m_proxyModel;
    AZ::u64 m_selectionContextId;
    AZStd::vector<AZ::EntityId> m_selectedEntityIds;

    void PrepareSelection();
    void DoCreateEntity();
    void DoCreateEntityWithParent(const AZ::EntityId& parentId);
    void DoShowSlice();
    void DoDuplicateSelection();
    void DoDeleteSelection();
    void DoDeleteSelectionAndDescendants();
    void DoRenameSelection();
    void DoMoveEntityUp();
    void DoMoveEntityDown();
    void GoToEntitiesInViewport();

    void DoSelectSliceRootAboveSelection();
    void DoSelectSliceRootBelowSelection();
    void DoSelectTopSliceRoot();
    void DoSelectBottomSliceRoot();

    void DoSelectSliceRootNextToSelection(bool above);
    void DoSelectEdgeSliceRoot(bool top);

    void SetIndexAsCurrentAndSelected(const QModelIndex& index);

    void SetupActions();
    void SelectSliceRoot();

    QAction* m_actionToShowSlice;
    QAction* m_actionToCreateEntity;
    QAction* m_actionToDeleteSelection;
    QAction* m_actionToDeleteSelectionAndDescendants;
    QAction* m_actionToRenameSelection;
    QAction* m_actionToReparentSelection;
    QAction* m_actionToMoveEntityUp;
    QAction* m_actionToMoveEntityDown;
    QAction* m_actionGoToEntitiesInViewport;

    QAction* m_actionToSelectSliceRootAboveSelection;
    QAction* m_actionToSelectSliceRootBelowSelection;
    QAction* m_actionToSelectTopSliceRoot;
    QAction* m_actionToSelectBottomSliceRoot;

    void OnTreeItemClicked(const QModelIndex &index);
    void OnTreeItemExpanded(const QModelIndex &index);
    void OnTreeItemCollapsed(const QModelIndex &index);
    void OnExpandEntity(const AZ::EntityId& entityId, bool expand);
    void OnSelectEntity(const AZ::EntityId& entityId, bool selected);
    void OnEnableSelectionUpdates(bool enable);
    void OnDropEvent();
    bool m_inObjectPickMode;

    void InvalidateFilter();
    void ClearFilter();

    AZ::EntityId GetEntityIdFromIndex(const QModelIndex& index) const;
    QModelIndex GetIndexFromEntityId(const AZ::EntityId& entityId) const;
    void ExtractEntityIdsFromSelection(const QItemSelection& selection, AzToolsFramework::EntityIdList& entityIdList) const;
    void EnableUi(bool enable);

    // AzToolsFramework::OutlinerModelNotificationBus::Handler
    // Receive notification from the outliner model that we should scroll
    // to a given entity
    void QueueScrollToNewContent(const AZ::EntityId& entityId) override;

    void ScrollToNewContent();
    bool m_scrollToNewContentQueued;
    bool m_scrollToSelectedEntity;
    bool m_dropOperationInProgress;
    bool m_expandSelectedEntity;
    bool m_focusInEntityOutliner;
    AZ::EntityId m_scrollToEntityId;

    void QueueUpdateSelection();
    void UpdateSelection();
    AzToolsFramework::EntityIdSet m_entitiesToSelect;
    AzToolsFramework::EntityIdSet m_entitiesToDeselect;
    AzToolsFramework::EntityIdSet m_entitiesSelectedByOutliner;
    AzToolsFramework::EntityIdSet m_entitiesDeselectedByOutliner;
    bool m_selectionChangeQueued;
    bool m_selectionChangeInProgress;
    bool m_enableSelectionUpdates;

    QIcon m_emptyIcon;
    QIcon m_clearIcon;

    void QueueContentUpdateSort(const AZ::EntityId& entityId);
    void SortContent();

    EntityOutliner::DisplayOptionsMenu* m_displayOptionsMenu;
    AzToolsFramework::EntityIdSet m_entitiesToSort;
    EntityOutliner::DisplaySortMode m_sortMode;
    bool m_sortContentQueued;
};

#endif
