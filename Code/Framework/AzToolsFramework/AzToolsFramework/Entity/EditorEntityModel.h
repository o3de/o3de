/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorEntityInfoBus.h"
#include "EditorEntitySortBus.h"

#include <AzCore/std/functional.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityRuntimeActivationBus.h>
#include <AzToolsFramework/Entity/EditorEntitySortBus.h>
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>
#include <AzToolsFramework/Entity/EditorEntityModelBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

namespace AzToolsFramework
{
    class EditorEntityModel
        : public AzFramework::EntityContextEventBus::Handler
        , public EditorEntityContextNotificationBus::Handler
        , public EditorEntitySortNotificationBus::MultiHandler
        , public ToolsApplicationEvents::Bus::Handler
        , public EditorOnlyEntityComponentNotificationBus::Handler
        , public EditorEntityRuntimeActivationChangeNotificationBus::Handler
        , public EditorTransformChangeNotificationBus::Handler
        , public EntityCompositionNotificationBus::Handler
        , public EditorEntityModelRequestBus::Handler
        , public AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler
        , public AZ::EntityBus::MultiHandler
        , public AZ::TickBus::Handler
    {
    public:
        enum class ComponentCompositionAction
        {
            Add,
            Remove,
            Enable,
            Disable
        };

        EditorEntityModel();
        ~EditorEntityModel();
        EditorEntityModel(const EditorEntityModel&) = delete;

        ////////////////////////////////////////////////////////////////////////
        // ToolsApplicationEvents::Bus
        ////////////////////////////////////////////////////////////////////////
        void EntityRegistered(AZ::EntityId entityId) override;
        void EntityDeregistered(AZ::EntityId entityId) override;
        void EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId) override;
        void SetEntityInstantiationPosition(const AZ::EntityId& parent, const AZ::EntityId& beforeEntity) override;
        void ClearEntityInstantiationPosition() override;

        ///////////////////////////////////
        // EditorEntitySortNotificationBus
        ///////////////////////////////////
        void ChildEntityOrderArrayUpdated() override;

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus::Handler
        //////////////////////////////////////////////////////////////////////////
        void OnPrepareForContextReset() override;
        void OnEntityStreamLoadBegin() override;
        void OnEntityStreamLoadSuccess() override;
        void OnEntityStreamLoadFailed() override;
        void SetForceAddEntitiesToBackFlag(bool forceAddToBack) override;

        ////////////////////////////////////////////////
        // AzFramework::EntityContextEventBus::Handler
        ////////////////////////////////////////////////
        void OnEntityContextReset() override;
        void OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList&) override;

        ////////////////////////////////////////////////////////////////////////
        // EditorOnlyEntityComponentNotificationBus::Handler
        ////////////////////////////////////////////////////////////////////////
        void OnEditorOnlyChanged(AZ::EntityId entityId, bool isEditorOnly) override;

        ////////////////////////////////////////////////////////////////////////
        // EditorEntityRuntimeActivationChangeNotificationBus
        ////////////////////////////////////////////////////////////////////////
        void OnEntityRuntimeActivationChanged(AZ::EntityId entityId, bool activeOnStart) override;

        ////////////////////////////////////////////////////////////////////////
        // EditorTransformChangeNotificationBus::Handler
        ////////////////////////////////////////////////////////////////////////
        void OnEntityTransformChanged(const AzToolsFramework::EntityIdList& entityIds) override;

        ////////////////////////////////////////////////////////////////////////
        // EntityCompositionNotificationBusHandler
        ////////////////////////////////////////////////////////////////////////
        void OnEntityComponentAdded(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentRemoved(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentEnabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentDisabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::EntityBus
        ////////////////////////////////////////////////////////////////////////
        void OnEntityExists(const AZ::EntityId& entityId) override;
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        ////////////////////////////////////////////////////////////////////////
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        ////////////////////////////////////////////////////////////////////////
        // PrefabPublicNotificationBus
        ////////////////////////////////////////////////////////////////////////
        void OnPrefabInstancePropagationBegin() override;
        void OnPrefabInstancePropagationEnd() override;

        void AddToChildrenWithOverrides(const EntityIdList& parentEntityIds, const AZ::EntityId& entityId) override;
        void RemoveFromChildrenWithOverrides(const EntityIdList& parentEntityIds, const AZ::EntityId& entityId) override;

    private:
        void Reset();
        void ProcessQueuedEntityAdds();
        void ClearQueuedEntityAdds();
        void AddEntity(AZ::EntityId entityId);
        void RemoveEntity(AZ::EntityId entityId);

        void AddChildToParent(AZ::EntityId parentId, AZ::EntityId childId);
        void RemoveChildFromParent(AZ::EntityId parentId, AZ::EntityId childId);
        void RemoveFromAncestorCyclicDependencyList(const AZ::EntityId& parentId, const AZ::EntityId& entityId);
        void ReparentChild(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId);

        void UpdateSliceInfoHierarchy(AZ::EntityId entityId);

        class EditorEntityModelEntry
            : private AZ::EntityBus::Handler
            , private EditorLockComponentNotificationBus::Handler
            , private EditorVisibilityNotificationBus::Handler
            , private EntitySelectionEvents::Bus::Handler
            , public EditorEntityAPIBus::Handler
            , public EditorEntityInfoRequestBus::Handler
            , public EditorInspectorComponentNotificationBus::Handler
            , public PropertyEditorEntityChangeNotificationBus::Handler
        {
        public:
            ~EditorEntityModelEntry();

            // Separately connect to EditorEntityInfoRequestBus and refresh Entity
            // visibility and lock state - happens as part of Connect() but is also
            // refreshed earlier after Entity initialization.
            void EntityInfoRequestConnect();

            void Connect();
            void Disconnect();

            // Slices
            void UpdateSliceInfo();

            // Sibling sort index
            void UpdateOrderInfo(bool notify);

            void UpdateChildOrderInfo(bool forceAddToBack);

            void SetId(const AZ::EntityId& entityId);
            void SetParentId(AZ::EntityId parentId);
            void AddChild(AZ::EntityId childId);
            void RemoveChild(AZ::EntityId childId);
            bool HasChild(AZ::EntityId childId) const;

            /////////////////////////////
            // EditorEntityAPIRequests
            /////////////////////////////
            void SetName(AZStd::string name) override;
            void SetParent(AZ::EntityId parentId) override;
            void SetLockState(bool isLocked) override;
            void SetVisibilityState(bool isVisible) override;
            void SetStartStatus(EditorEntityStartStatus status) override;

            /////////////////////////////
            // EditorEntityInfoRequests
            /////////////////////////////
            AZ::EntityId GetId() const;
            AZ::EntityId GetParent() const override;
            EntityIdList GetChildren() const override;
            AZ::EntityId GetChild(AZStd::size_t index) const override;
            AZStd::size_t GetChildCount() const override;
            AZ::u64 GetChildIndex(AZ::EntityId childId) const override;
            AZStd::string GetName() const override;
            AZStd::string GetSliceAssetName() const override;
            bool IsSliceEntity() const override;
            bool IsSubsliceEntity() const override;
            bool IsSliceRoot() const override;
            bool IsSubsliceRoot() const override;
            //Does this entity have any children added or deleted. Ignore any other kind of change.
            bool HasSliceEntityAnyChildrenAddedOrDeleted() const override;
            // Does the entity have any property changes in its top level. Ignore any changes in children.
            bool HasSliceEntityPropertyOverridesInTopLevel() const override;
            bool HasSliceEntityOverrides() const override;
            bool HasSliceChildrenOverrides() const override;
            bool HasSliceAnyOverrides() const override;
            bool HasCyclicDependency() const override;
            AZ::u64 GetIndexForSorting() const override;
            bool IsSelected() const override;
            bool IsVisible() const override;
            bool IsHidden() const override;
            bool IsLocked() const override;
            // Lock status can be overwritten if an entity is in a locked layer.
            // However, in some cases (like the outliner), this entity's specific state needs to be known.
            EditorEntityStartStatus GetStartStatus() const override;
            bool IsJustThisEntityLocked() const override;
            bool IsConnected() const;
            void AddToCyclicDependencyList(const AZ::EntityId& entityId) override;
            void RemoveFromCyclicDependencyList(const AZ::EntityId& entityId) override;
            AzToolsFramework::EntityIdList GetCyclicDependencyList() const override;

            bool IsComponentExpanded(AZ::ComponentId id) const override;
            void SetComponentExpanded(AZ::ComponentId id, bool expanded) override;

            ////////////////////////////////////////////////////////////////////////
            // EditorLockComponentNotificationBus::Handler
            ////////////////////////////////////////////////////////////////////////
            void OnEntityLockFlagChanged(bool locked) override;

            ////////////////////////////////////////////////////////////////////////
            // EditorVisibilityNotificationBus::Handler
            ////////////////////////////////////////////////////////////////////////
            void OnEntityVisibilityFlagChanged(bool visibility) override;

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::EntitySelectionEvents::Handler
            // Used to handle selection changes on a per-entity basis.
            //////////////////////////////////////////////////////////////////////////
            void OnSelected() override;
            void OnDeselected() override;

            ////////////////////////////////////////////////////////////////////////
            // AZ::EntityBus
            ////////////////////////////////////////////////////////////////////////
            void OnEntityNameChanged(const AZStd::string& name) override;

            ////////////////////////////////////////////////////////////////////////
            // EditorInspectorComponentNotificationBus
            ////////////////////////////////////////////////////////////////////////
            void OnComponentOrderChanged() override;

            ////////////////////////////////////////////////////////////////////////
            // PropertyEditorEntityChangeNotificationBus
            ////////////////////////////////////////////////////////////////////////
            void OnEntityComponentPropertyChanged(AZ::ComponentId componentId) override;

        private:
            friend class EditorEntityModel;

            enum SliceFlags
            {
                SliceFlag_None = 0,
                SliceFlag_Entity = 1 << 0,
                SliceFlag_Root = 1 << 1,
                SliceFlag_Subslice = 1 << 2,

                SliceFlag_EntityNameOverridden = 1 << 3, // special case since it's not saved in a component
                SliceFlag_EntityActivationOverridden = 1 << 4, // special case since it's not saved in a component
                SliceFlag_EntityComponentsOverridden = 1 << 5,
                SliceFlag_EntityHasAdditionsDeletions = 1 << 6,

                SliceFlag_EntityHasNonChildOverrides = (SliceFlag_EntityNameOverridden | SliceFlag_EntityActivationOverridden | SliceFlag_EntityComponentsOverridden),
                SliceFlag_EntityHasOverrides    = (SliceFlag_EntityNameOverridden | SliceFlag_EntityActivationOverridden | SliceFlag_EntityComponentsOverridden | SliceFlag_EntityHasAdditionsDeletions),

                SliceFlag_SliceRoot       = (SliceFlag_Entity | SliceFlag_Root),
                SliceFlag_SubsliceRoot    = (SliceFlag_Entity | SliceFlag_Root | SliceFlag_Subslice),
                SliceFlag_SubsliceEntity  = (SliceFlag_Entity | SliceFlag_Subslice),

                SliceFlag_OverridesMask = (SliceFlag_EntityHasOverrides),
            };

            void OnEditorOnlyChanged(bool isEditorOnly);
            void OnEntityRuntimeActivationChanged(bool activeOnStart);
            void OnTransformChanged();
            void OnComponentCompositionChanged(const AZ::ComponentId& componentId, ComponentCompositionAction action);
            void OnChildSortOrderChanged();

            bool CanProcessOverrides() const;

            void AddChildAddedDeleted(AZ::ComponentId componentId);
            void RemoveChildAddedDeleted(AZ::ComponentId componentId);
            void AddOverriddenComponent(AZ::ComponentId componentId);
            void RemoveOverriddenComponent(AZ::ComponentId componentId);

            void DetectInitialOverrides();
            void ModifyParentsOverriddenChildren(AZ::EntityId entityId, AZ::u8 lastFlags, bool hasOverrides);
            void UpdateCyclicDependencyInfo();

            void SetStartActiveStatus(bool isActive);

            AZ::EntityId m_entityId;
            AZ::EntityId m_parentId;
            EntityIdList m_children;
            AZStd::unordered_set<AZ::EntityId> m_overriddenChildren;
            AZ::u64 m_indexForSorting = 0;
            AZ::u8 m_sliceFlags = SliceFlag_None;
            bool m_selected = false;
            bool m_visible = true;
            bool m_locked = false;
            bool m_connected = false;
            AZStd::string m_name;
            AZStd::string m_sliceAssetName;
            AZStd::unordered_map<AZ::EntityId, AZ::u64> m_childIndexCache;
            AZStd::vector<AZ::EntityId> m_cyclicDependencyList;

            // slice entity override state cache
            AZ::Entity* m_entity = nullptr;
            AZ::EntityId m_baseAncestorId;
            AZStd::shared_ptr<AZ::Entity> m_sourceClone;
            AZ::SerializeContext* m_serializeContext = nullptr;

            AZStd::set<AZ::ComponentId> m_overriddenComponents;

            AZStd::unordered_map<AZ::ComponentId, bool> m_componentExpansionStateMap;

            AZStd::set<AZ::ComponentId> m_addedRemovedComponents;
        };

        EditorEntityModelEntry& GetInfo(const AZ::EntityId& entityId);
        AZStd::unordered_map<AZ::EntityId, EditorEntityModelEntry> m_entityInfoTable;
        AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<AZ::EntityId>> m_entityOrphanTable;
        // Sort order of recently deleted entities. Used to restore order in situations like entity is later replaced by sliced entity and undo/redo.
        AZStd::unordered_map<AZ::EntityId, AZStd::pair<AZ::EntityId,AZ::u64>> m_savedOrderInfo;
        bool m_enableChildReorderHandler = true;
        bool m_forceAddToBack = false;

        // Track if the context is being reset. If so, steps during entity removal can be skipped
        // because we know that all entities are going to be removed as part of the context reset.
        bool m_preparingForContextReset = false;

        // Don't add new entities to the model until they're all ready to go.
        // This lets us add the entities in a more efficient manner.
        AZStd::unordered_set<AZ::EntityId> m_queuedEntityAdds;
        AZStd::unordered_set<AZ::EntityId> m_queuedEntityAddsNotYetActivated;

        AZ::EntityId m_postInstantiateBeforeEntity;
        AZ::EntityId m_postInstantiateSliceParent;
        bool m_gotInstantiateSliceDetails = false;
        bool m_isPrefabPropagationInProgress = false;
    };
}
