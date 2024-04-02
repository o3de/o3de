/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)

#include <AzCore/base.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityRuntimeActivationBus.h>
#include <AzToolsFramework/FocusMode/FocusModeNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSearchWidget.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>

//#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>

#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

#include <QCheckBox>
#include <QMap>
#include <QRect>
#include <QStyledItemDelegate>
#include <QString>
#include <QVector>
#include <QWidget>
#endif

#pragma once

namespace AzToolsFramework
{
    class EditorEntityUiInterface;
    class FocusModeInterface;
    class PrefabEditorEntityOwnershipInterface;
    class ReadOnlyEntityPublicInterface;

    namespace EntityOutliner
    {
        enum class DisplaySortMode : unsigned char;
    }

    namespace Prefab
    {
        class PrefabSystemComponentInterface;
    }

    //! Model for items in the OutlinerTreeView.
    //! Each item represents an Entity.
    //! Items are parented in the tree according to their transform hierarchy.
    class EntityOutlinerListModelFromPrefab
        : public QAbstractItemModel
        , private EditorEntityContextNotificationBus::Handler
        , private FocusModeNotificationBus::Handler
        , private ToolsApplicationEvents::Bus::Handler
        , private EntityCompositionNotificationBus::Handler
        , private EditorEntityRuntimeActivationChangeNotificationBus::Handler
        , private AZ::EntitySystemBus::Handler
        , private ContainerEntityNotificationBus::Handler
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerListModelFromPrefab, AZ::SystemAllocator);

        //! Columns of data to display about each Entity.
        enum Column
        {
            ColumnName,                 //!< Entity name
            ColumnVisibilityToggle,     //!< Visibility Icons
            ColumnLockToggle,           //!< Lock Icons
            ColumnSpacing,              //!< Spacing to allow for drag select
            ColumnSortIndex,            //!< Index of sort order
            ColumnCount                 //!< Total number of columns
        };

        enum ReparentForInvalid 
        {
            None,                       //!< For an invalid location the entity does not change location
            AppendEnd,                  //!< Append Item to end of target parent list
            AppendBeginning,            //!< Append Item to the beginning of target parent list
        };

        // Note: the ColumnSortIndex column isn't shown, hence the -1 and the need for a separate counter.
        // A wrong column count number causes refresh issues and hover mismatch on model update.
        static const int VisibleColumnCount = ColumnCount - 1;

        enum Roles
        {
            VisibilityRole = Qt::UserRole + 1,
            EntityIdRole,
            SelectedRole,
            ChildSelectedRole,
            PartiallyVisibleRole,
            PartiallyLockedRole,
            InvisibleAncestorRole,
            LockedAncestorRole,
            ChildCountRole,
            ExpandedRole,
            RoleCount
        };

        enum class GlobalSearchCriteriaFlags : int
        {
            Unlocked = 1 << aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Unlocked),
            Locked = 1 << aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Locked),
            Visible = 1 << aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Visible),
            Hidden = 1 << aznumeric_cast<int>(EntityOutlinerSearchWidget::GlobalSearchCriteria::Hidden)
        };

        struct ComponentTypeValue
        {
            AZ::Uuid m_uuid;
            int m_globalVal;
        };

        // Spacing is appropriate and matches the outliner concept work from the UI team.
        static const int s_OutlinerSpacing = 7;
        static const int s_OutlinerSpacingForLevel = 10;

        static bool s_paintingName;

        EntityOutlinerListModelFromPrefab(QObject* parent = nullptr);
        ~EntityOutlinerListModelFromPrefab();

        void Initialize();

        // FocusModeNotificationBus overrides ...
        void OnEditorFocusChanged(AZ::EntityId previousFocusEntityId, AZ::EntityId newFocusEntityId) override;

        // Qt overrides.
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex&) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        Qt::DropActions supportedDropActions() const override;
        Qt::DropActions supportedDragActions() const override;
        bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
        bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        QStringList mimeTypes() const override;

        QModelIndex GetIndexFromEntity(AZ::EntityId entityId, int column = 0) const;
        QModelIndex GetIndexFromEntityAlias(const Prefab::EntityAlias& entityAlias, int column = 0) const;
        AZ::EntityId GetEntityFromIndex(const QModelIndex& index) const;
        AZStd::optional<Prefab::EntityAlias> GetEntityAliasFromIndex(const QModelIndex& index) const;

        bool FilterEntity(const AZ::EntityId& entityId);

        void EnableAutoExpand(bool enable);

        AZStd::string GetFilterString() const
        {
            return m_filterString;
        }

        void SetSortMode(EntityOutliner::DisplaySortMode sortMode) { m_sortMode = sortMode; }
        void SetDropOperationInProgress(bool inProgress);
        void ProcessEntityUpdates();

    Q_SIGNALS:
        void SelectEntity(const AZ::EntityId& entityId, bool select);
        void EnableSelectionUpdates(bool enable);
        void ResetFilter();
        void ReapplyFilter();

    public Q_SLOTS:
        void SearchStringChanged(const AZStd::string& filter);
        void SearchFilterChanged(const AZStd::vector<ComponentTypeValue>& componentFilters);
        void OnEntityExpanded(const AZ::EntityId& entityId);
        void OnEntityCollapsed(const AZ::EntityId& entityId);

        // Buffer Processing Slots - These are called using single-shot events when the buffers begin to fill.
        bool CanReparentEntities(const AZ::EntityId& newParentId, const EntityIdList& selectedEntityIds) const;
        bool ReparentEntities(const AZ::EntityId& newParentId, const EntityIdList& selectedEntityIds, const AZ::EntityId& beforeEntityId = AZ::EntityId(), ReparentForInvalid forInvalid = None);

        //! Use the current filter setting and re-evaluate the filter.
        void InvalidateFilter();

    protected:
        // EditorEntityContextNotificationBus overrides ...
        void OnEditorEntityDuplicated(const AZ::EntityId& oldEntity, const AZ::EntityId& newEntity) override;
        void OnPrepareForContextReset() override;
        void OnStartPlayInEditorBegin() override;
        void OnStartPlayInEditor() override;

        bool m_beginStartPlayInEditor = false;

        void QueueEntityUpdate(AZ::EntityId entityId);
        void QueueAncestorUpdate(AZ::EntityId entityId);
        void QueueEntityToExpand(AZ::EntityId entityId, bool expand);
        AZStd::unordered_set<AZ::EntityId> m_entitySelectQueue;
        AZStd::unordered_set<AZ::EntityId> m_entityChangeQueue;
        bool m_entityChangeQueued;
        bool m_entityLayoutQueued;
        bool m_dropOperationInProgress = false;

        bool m_autoExpandEnabled = true;
        bool m_layoutResetQueued = false;
        bool m_suppressNextSelectEntity = false;

        AZStd::string m_filterString;
        AZStd::vector<ComponentTypeValue> m_componentFilters;
        bool m_isFilterDirty = true;

        void OnEntityCompositionChanged(const EntityIdList& entityIds) override;

        void OnEntityInitialized(const AZ::EntityId& entityId) override;
        void AfterEntitySelectionChanged(const EntityIdList&, const EntityIdList&) override;

        // EditorEntityRuntimeActivationChangeNotificationBus::Handler
        void OnEntityRuntimeActivationChanged(AZ::EntityId entityId, bool activeOnStart) override;

        // ContainerEntityNotificationBus overrides ...
        void OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open) override;

        // Drag/Drop of components from Component Palette.
        bool dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

        // Drag/Drop of entities.  Internally handled.
        bool canDropMimeDataForEntityIds(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const;
        bool dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

        // Drag/Drop of assets that are not internally handled
        bool DropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
        bool CanDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const;

        QMap<int, QVariant> itemData(const QModelIndex& index) const override;
        QVariant dataForAll(const QModelIndex& index, int role) const;
        QVariant dataForName(const QModelIndex& index, int role) const;
        QVariant dataForVisibility(const QModelIndex& index, int role) const;
        QVariant dataForLock(const QModelIndex& index, int role) const;
        QVariant dataForSortIndex(const QModelIndex& index, int role) const;

        //! Request a hierarchy expansion
        void ExpandAncestors(const AZ::EntityId& entityId);
        bool IsExpanded(const AZ::EntityId& entityId) const;
        AZStd::unordered_map<AZ::EntityId, bool> m_entityExpansionState;

        void RestoreDescendantExpansion(const AZ::EntityId& entityId);
        void RestoreDescendantSelection(const AZ::EntityId& entityId);

        bool IsFiltered(const AZ::EntityId& entityId) const;
        AZStd::unordered_map<AZ::EntityId, bool> m_entityFilteredState;

        bool HasSelectedDescendant(const AZ::EntityId& entityId) const;

        bool AreAllDescendantsSameLockState(const AZ::EntityId& entityId) const;
        bool AreAllDescendantsSameVisibleState(const AZ::EntityId& entityId) const;

        // These are needed until we completely disassociated selection control from the outliner state to
        // keep track of selection state before/during/after filtering and searching
        EntityIdList m_unfilteredSelectionEntityIds;
        void CacheSelectionIfAppropriate();
        void RestoreSelectionIfAppropriate();
        bool ShouldOverrideUnfilteredSelection();

        EntityOutliner::DisplaySortMode m_sortMode;

    private:
        QVariant GetEntityIcon(const AZ::EntityId& id) const;
        QVariant GetEntityTooltip(const AZ::EntityId& id) const;
        
        EditorEntityUiInterface* m_editorEntityUiInterface = nullptr;
        FocusModeInterface* m_focusModeInterface = nullptr;
        PrefabEditorEntityOwnershipInterface* m_prefabEditorEntityOwnershipInterface = nullptr;
        Prefab::PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;

        //Prefab::TemplateId m_rootTemplateId = Prefab::InvalidTemplateId;
        Prefab::InstanceOptionalReference m_rootInstance;

        // TODO - rename this better
        void Generate();
        void GenerateCacheForEntity(Prefab::PrefabDomValueConstReference entityDomRef);
        void GenerateModelHierarchyRecursively(const QString& entityAlias, QModelIndex parentIndex, int row);

        struct ItemCache
        {
            ItemCache() = default;
            ItemCache(QString name, QVector<QString> children)
                : m_name(std::move(name))
                , m_children(std::move(children))
            {
            }

            QString m_name;
            QVector<QString> m_children;
        };

        QString m_rootItemAlias;
        QMap<QString, ItemCache> m_itemInfo;

        QMap<QModelIndex, QString> m_indexToEntityAliasMap;
        QMap<QString, QModelIndex> m_entityAliasToIndexMap;
        QMap<QModelIndex, QVector<QModelIndex>> m_indicesHierarchy;
    };
}

