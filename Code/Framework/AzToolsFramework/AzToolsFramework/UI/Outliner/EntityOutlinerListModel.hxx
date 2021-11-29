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
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityRuntimeActivationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerSearchWidget.h>
#include <AzToolsFramework/UI/SearchWidget/SearchCriteriaWidget.hxx>

#include <QCheckBox>
#include <QRect>
#include <QStyledItemDelegate>
#include <QWidget>
#endif

#pragma once

namespace AzToolsFramework
{
    class EditorEntityUiInterface;
    class FocusModeInterface;

    namespace EntityOutliner
    {
        enum class DisplaySortMode : unsigned char;
    }

    //! Model for items in the OutlinerTreeView.
    //! Each item represents an Entity.
    //! Items are parented in the tree according to their transform hierarchy.
    class EntityOutlinerListModel
        : public QAbstractItemModel
        , private EditorEntityContextNotificationBus::Handler
        , private EditorEntityInfoNotificationBus::Handler
        , private ToolsApplicationEvents::Bus::Handler
        , private EntityCompositionNotificationBus::Handler
        , private EditorEntityRuntimeActivationChangeNotificationBus::Handler
        , private AZ::EntitySystemBus::Handler
        , private ContainerEntityNotificationBus::Handler
    {
        Q_OBJECT;

    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerListModel, AZ::SystemAllocator, 0);

        //! Columns of data to display about each Entity.
        enum Column
        {
            ColumnName,                 //!< Entity name
            ColumnVisibilityToggle,     //!< Visibility Icons
            ColumnLockToggle,           //!< Lock Icons
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

        static bool s_paintingName;

        EntityOutlinerListModel(QObject* parent = nullptr);
        ~EntityOutlinerListModel();

        void Initialize();

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

        QModelIndex GetIndexFromEntity(const AZ::EntityId& entityId, int column = 0) const;
        AZ::EntityId GetEntityFromIndex(const QModelIndex& index) const;

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
        void ExpandEntity(const AZ::EntityId& entityId, bool expand);
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

        //! Editor entity context notification bus
        void OnEditorEntityDuplicated(const AZ::EntityId& oldEntity, const AZ::EntityId& newEntity) override;
        void OnContextReset() override;
        void OnStartPlayInEditorBegin() override;
        void OnStartPlayInEditor() override;

        bool m_beginStartPlayInEditor = false;

        void QueueEntityUpdate(AZ::EntityId entityId);
        void QueueAncestorUpdate(AZ::EntityId entityId);
        void QueueEntityToExpand(AZ::EntityId entityId, bool expand);
        void ProcessEntityInfoResetEnd();
        AZStd::unordered_set<AZ::EntityId> m_entitySelectQueue;
        AZStd::unordered_set<AZ::EntityId> m_entityExpandQueue;
        AZStd::unordered_set<AZ::EntityId> m_entityChangeQueue;
        bool m_entityChangeQueued;
        bool m_entityLayoutQueued;
        bool m_dropOperationInProgress = false;

        bool m_autoExpandEnabled = true;
        bool m_layoutResetQueued = false;

        AZStd::string m_filterString;
        AZStd::vector<ComponentTypeValue> m_componentFilters;
        bool m_isFilterDirty = true;

        void OnEntityCompositionChanged(const EntityIdList& entityIds) override;

        void OnEntityInitialized(const AZ::EntityId& entityId) override;
        void AfterEntitySelectionChanged(const EntityIdList&, const EntityIdList&) override;

        //! EditorEntityInfoNotificationBus::Handler
        //! Get notifications when the EditorEntityInfo changes so we can update our model
        void OnEntityInfoResetBegin() override;
        void OnEntityInfoResetEnd() override;
        void OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index) override;
        void OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index) override;
        void OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected) override;
        void OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool locked) override;
        void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;
        void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name) override;
        void OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId) override;

        // EditorEntityRuntimeActivationChangeNotificationBus::Handler
        void OnEntityRuntimeActivationChanged(AZ::EntityId entityId, bool activeOnStart) override;

        // ContainerEntityNotificationBus overrides ...
        void OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open) override;

        // Drag/Drop of components from Component Palette.
        bool dropMimeDataComponentPalette(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

        // Drag/Drop of entities.
        bool canDropMimeDataForEntityIds(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const;
        bool dropMimeDataEntities(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

        // Drag/Drop of assets from asset browser.
        using ComponentAssetPair = AZStd::pair<AZ::TypeId, AZ::Data::AssetId>;
        using ComponentAssetPairs = AZStd::vector<ComponentAssetPair>;
        bool DecodeAssetMimeData(const QMimeData* data, AZStd::optional<ComponentAssetPairs*> componentAssetPairs = AZStd::nullopt,
            AZStd::optional<AZStd::vector<AZStd::string>*> sourceFiles = AZStd::nullopt) const;
        bool DropMimeDataAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
        bool CanDropMimeDataAssets(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const;

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

        enum LayerProperty
        {
            Locked,
            Invisible
        };
        bool IsInLayerWithProperty(AZ::EntityId entityId, const LayerProperty& layerProperty) const;

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
    };

    class EntityOutlinerCheckBox
        : public QCheckBox
    {
        Q_OBJECT

    public:
        explicit EntityOutlinerCheckBox(QWidget* parent = nullptr);

        void draw(QPainter* painter);

    private:
        const int m_toggleColumnWidth = 16;
    };

    /*!
    * OutlinerItemDelegate exists to render custom item-types.
    * Other item-types render in the default fashion.
    */
    class EntityOutlinerItemDelegate
        : public QStyledItemDelegate
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerItemDelegate, AZ::SystemAllocator, 0);

        EntityOutlinerItemDelegate(QWidget* parent = nullptr);

        // Qt overrides
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    protected:
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    private:
        // Paint the background for every ancestor that overrides it
        void PaintAncestorBackgrounds(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

        // Paint the selection or hover rect for the item
        void PaintSelectionHoverRect(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index,
            bool isSelected, bool isHovered) const;

        // Paint the background for every ancestor that overrides it
        void PaintAncestorForegrounds(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

        // Paint the visibility and lock custom checkboxes
        void PaintCheckboxes(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, bool isHovered) const;

        // Paint the dashed rect signaling a descendant of a collapsed entity is selected
        void PaintDescendantSelectedRect(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

        // Paint the entity name using rich text
        void PaintEntityNameAsRichText(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

        struct CheckboxGroup
        {
            EntityOutlinerCheckBox m_default;
            EntityOutlinerCheckBox m_mixed;
            EntityOutlinerCheckBox m_overridden;

            EntityOutlinerCheckBox m_defaultHover;
            EntityOutlinerCheckBox m_mixedHover;
            EntityOutlinerCheckBox m_overriddenHover;

            CheckboxGroup(QWidget* parent, AZStd::string prefix, EntityOutlinerListModel::Roles mixedRole, EntityOutlinerListModel::Roles overriddenRole);
            EntityOutlinerCheckBox* SelectCheckboxToRender(const QModelIndex& index, bool isHovered);

        private:
            EntityOutlinerListModel::Roles m_mixedRole;
            EntityOutlinerListModel::Roles m_overriddenRole;
        };

        // Mutability added because these are being used ONLY as renderers
        // for custom check boxes. The decision of whether or not to draw
        // them checked is tracked by the individual entities and items in
        // the hierarchy cache.
        mutable CheckboxGroup m_visibilityCheckBoxes;
        mutable CheckboxGroup m_lockCheckBoxes;

        // this is a cache, and is hence mutable
        mutable QRect m_cachedBoundingRectOfTallCharacter;

        const QColor m_selectedColor = QColor(255, 255, 255, 45);
        const QColor m_hoverColor = QColor(255, 255, 255, 30);

        EditorEntityUiInterface* m_editorEntityFrameworkInterface = nullptr;
    };

}

Q_DECLARE_METATYPE(AZ::ComponentTypeList); // allows type to be stored by QVariable
