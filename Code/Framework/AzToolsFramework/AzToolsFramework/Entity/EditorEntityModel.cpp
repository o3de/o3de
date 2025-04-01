/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorEntityModel.h"
#include "EditorEntitySortBus.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceMetadataInfoBus.h>

#include <AzCore/std/sort.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

DECLARE_EBUS_INSTANTIATION(AzToolsFramework::EditorEntityInfoRequests);

#include <QtWidgets/QMessageBox>

namespace
{
    template<typename T>
    bool HasDifferences(T* sourceElem, T* compareElem, bool isRoot,
        AZ::SerializeContext* serializeContext)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!sourceElem || !compareElem)
        {
            // if only one is valid then technically they are different
            return (sourceElem || compareElem);
        }

        using namespace AzToolsFramework;

        InstanceDataHierarchy source;
        source.AddRootInstance<T>(sourceElem);
        source.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        InstanceDataHierarchy target;
        target.AddRootInstance<T>(compareElem);
        target.Build(serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        bool hasDifferences = false;
        const auto nodeChanged =
            [isRoot, &hasDifferences](const InstanceDataNode* rootNode)
        {
            const InstanceDataNode* node = rootNode;

            // if the node has any unhidden parent, count it as a difference.
            while (node)
            {
                if (!SliceUtilities::IsNodePushable(*node, isRoot))
                {
                    break;
                }

                const AzToolsFramework::NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*node, true);
                if (visibility == AzToolsFramework::NodeDisplayVisibility::Visible)
                {
                    hasDifferences = true;
                    break;
                }

                node = node->GetParent();
            }
        };

        const auto newCallback = [&nodeChanged](
            InstanceDataNode* targetNode, AZStd::vector<AZ::u8>& /*data*/)
        {
            nodeChanged(targetNode);
        };

        const auto removedCallback = [&nodeChanged](
            const InstanceDataNode* sourceNode, InstanceDataNode* /*targetNodeParent*/)
        {
            nodeChanged(sourceNode);
        };

        const auto changedCallback = [&nodeChanged](
            const InstanceDataNode* sourceNode, InstanceDataNode* /*targetNode*/,
            AZStd::vector<AZ::u8>& /*sourceData*/, AZStd::vector<AZ::u8>& /*targetData*/)
        {
            nodeChanged(sourceNode);
        };

        InstanceDataHierarchy::CompareHierarchies(&source, &target,
            InstanceDataHierarchy::DefaultValueComparisonFunction,
            serializeContext, newCallback, removedCallback, changedCallback);

        return hasDifferences;
    }
}

namespace AzToolsFramework
{
    EditorEntityModel::EditorEntityModel()
    {
        EntityCompositionNotificationBus::Handler::BusConnect();
        EditorOnlyEntityComponentNotificationBus::Handler::BusConnect();
        EditorEntityRuntimeActivationChangeNotificationBus::Handler::BusConnect();
        EditorTransformChangeNotificationBus::Handler::BusConnect();
        ToolsApplicationEvents::Bus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EditorEntityModelRequestBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
        if (!editorEntityContextId.IsNull())
        {
            AzFramework::EntityContextEventBus::Handler::BusConnect(editorEntityContextId);
        }
        AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusConnect();

        Reset();
    }

    EditorEntityModel::~EditorEntityModel()
    {
        AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityContextEventBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        EditorTransformChangeNotificationBus::Handler::BusDisconnect();
        EditorEntityRuntimeActivationChangeNotificationBus::Handler::BusDisconnect();
        EditorOnlyEntityComponentNotificationBus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::Reset()
    {
        m_preparingForContextReset = false;
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //disconnect all entity ids
        EditorEntitySortNotificationBus::MultiHandler::BusDisconnect();

        //resetting model content and state, notifying any observers
        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetBegin);

        m_enableChildReorderHandler = false;

        m_entityInfoTable.clear();
        m_entityOrphanTable.clear();

        ClearQueuedEntityAdds();

        //AZ::EntityId::InvalidEntityId is the value of the parent entity id for any non-parented entity
        //registering AZ::EntityId::InvalidEntityId as a ghost root entity so requests can be made without special cases
        AddEntity(AZ::EntityId());

        m_enableChildReorderHandler = true;

        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetEnd);
    }

    namespace Internal
    {
        // BasicEntityData is used by ProcessQueuedEntityAdds() to sort entities
        // before their EditorEntityModelEntry is created.
        struct BasicEntityData
        {
            AZ::EntityId m_entityId;
            AZ::EntityId m_parentId;
            AZ::u64      m_indexForSorting = std::numeric_limits<AZ::u64>::max();

            bool operator< (const BasicEntityData& rhs) const
            {
                if (m_indexForSorting < rhs.m_indexForSorting)
                {
                    return true;
                }
                if (m_indexForSorting > rhs.m_indexForSorting)
                {
                    return false;
                }
                return m_entityId < rhs.m_entityId;
            }
        };
    } // namespace Internal

    void EditorEntityModel::ProcessQueuedEntityAdds()
    {
        using Internal::BasicEntityData;

        // Clear the queued entity data
        AZStd::unordered_set<AZ::EntityId> unsortedEntitiesToAdd = AZStd::move(m_queuedEntityAdds);
        ClearQueuedEntityAdds();

        // Add pending entities in breadth-first order.
        // This prevents temporary orphans and the need to re-sort children,
        // which can have a serious performance impact in large levels.
        AZStd::vector<AZ::EntityId> sortedEntitiesToAdd;
        sortedEntitiesToAdd.reserve(unsortedEntitiesToAdd.size());

        { // Sort pending entities
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityModel::AddEntityBatch:Sort");

            // Gather basic sorting data for each pending entity and
            // create map from parent ID to child entries.
            AZStd::unordered_map<AZ::EntityId, AZStd::vector<BasicEntityData>> parentIdToChildData;
            for (AZ::EntityId entityId : unsortedEntitiesToAdd)
            {
                BasicEntityData entityData;
                entityData.m_entityId = entityId;
                AZ::TransformBus::EventResult(entityData.m_parentId, entityId, &AZ::TransformBus::Events::GetParentId);

                // Can the parent be found?
                if (GetInfo(entityData.m_parentId).IsConnected() ||
                    unsortedEntitiesToAdd.find(entityData.m_parentId) != unsortedEntitiesToAdd.end())
                {
                    EditorEntitySortRequestBus::EventResult(entityData.m_indexForSorting, GetEntityIdForSortInfo(entityData.m_parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, entityId);
                }
                else
                {
                    // For the sake of sorting, treat orphaned entities as children of the root node.
                    // Note that the sort-index remains very high, so they'll be added after valid children.
                    entityData.m_parentId.SetInvalid();
                }

                parentIdToChildData[entityData.m_parentId].emplace_back(entityData);
            }

            // Sort siblings
            for (auto parentChildrenPair : parentIdToChildData)
            {
                AZStd::vector<BasicEntityData>& children = parentChildrenPair.second;

                AZStd::sort(children.begin(), children.end());
            }

            // General algorithm for breadth-first search is:
            // - create a list, populated with any root entities
            // - in a loop:
            // -- grab next entity in list
            // -- pushing its children to the back of the list,

            // Find root entities. Note that the pending entities might not all share a common root.
            AZStd::vector<AZ::EntityId> parentsToInspect;
            parentsToInspect.reserve(unsortedEntitiesToAdd.size() + 1); // Reserve space for pending + roots. Guess that there's one root.

            for (auto& parentChildrenPair : parentIdToChildData)
            {
                AZ::EntityId parentId = parentChildrenPair.first;

                if (unsortedEntitiesToAdd.find(parentId) == unsortedEntitiesToAdd.end())
                {
                    parentsToInspect.emplace_back(parentId);
                }
            }

            // Sort roots. This isn't necessary for correctness, but ensures a stable sort.
            AZStd::sort(parentsToInspect.begin(), parentsToInspect.end());

            parentsToInspect.reserve(parentsToInspect.size() + unsortedEntitiesToAdd.size());

            // Inspect entities in a loop...
            for (size_t i = 0; i < parentsToInspect.size(); ++i)
            {
                AZ::EntityId parentId = parentsToInspect[i];

                auto foundChildren = parentIdToChildData.find(parentId);
                if (foundChildren != parentIdToChildData.end())
                {
                    AZStd::vector<BasicEntityData>& children = foundChildren->second;

                    for (const BasicEntityData& childData : children)
                    {
                        parentsToInspect.emplace_back(childData.m_entityId);
                        sortedEntitiesToAdd.emplace_back(childData.m_entityId);
                    }

                    parentIdToChildData.erase(foundChildren);
                }
            }

            // Grab any pending entities that failed to sort.
            // This should only happen in edge-cases (ex: circular parent linkage)
            if (!parentIdToChildData.empty())
            {
                AZ_Assert(0, "Couldn't pre-sort all new entities.");
                for (auto& parentChildrenPair : parentIdToChildData)
                {
                    AZStd::vector<BasicEntityData>& children = parentChildrenPair.second;

                    for (BasicEntityData& child : children)
                    {
                        sortedEntitiesToAdd.emplace_back(child.m_entityId);
                    }
                }
            }
        }

        { // Add sorted entities
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityModel::AddEntityBatch:Add");
            for (AZ::EntityId entityId : sortedEntitiesToAdd)
            {
                AddEntity(entityId);
            }
        }
    }

    void EditorEntityModel::ClearQueuedEntityAdds()
    {
        m_queuedEntityAdds.clear();
        m_queuedEntityAddsNotYetActivated.clear();
        AZ::EntityBus::MultiHandler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::AddEntity(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        auto& entityInfo = GetInfo(entityId);

        //initialize and connect this entry to the entity id
        entityInfo.Connect();

        if (entityId.IsValid())
        {
            if (m_gotInstantiateSliceDetails)
            {
                AddChildToParent(m_postInstantiateSliceParent, entityId);
            }
            else
            {
                AddChildToParent(entityInfo.GetParent(), entityId);
            }
        }

        //re-parent any orphaned entities that may have either
        //1) been orphaned by a parent that was destroyed without destroying children
        //2) been temporarily attached to the "root" because parent didn't yet exist (during load/undo/redo)
        AZStd::unordered_set<AZ::EntityId> children;
        AZStd::swap(children, m_entityOrphanTable[entityId]);
        for (auto childId : children)
        {
            // When an orphan child entity is added before its transform parent, the orphan child entity
            // will be added to the root entity (root entity has an invalid EntityId) first and then
            // reparented to its real transform parent later after the parent is added. Therefore if the
            // root entity doesn't contain the orphan child entity, it means the orphan child entity has
            // not been added, so we skip reparenting.
            AZ::EntityId rootEntityId = AZ::EntityId();
            auto& parentInfo = GetInfo(rootEntityId);
            if (parentInfo.HasChild(childId))
            {
                ReparentChild(childId, entityId, AZ::EntityId());
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::RemoveDirtyEntity, childId);
            }
        }

        EditorEntitySortNotificationBus::MultiHandler::BusConnect(entityId);
    }

    void EditorEntityModel::RemoveEntity(AZ::EntityId entityId)
    {
        if (m_preparingForContextReset)
        {
            // A context reset is going to clear all entity information at once.
            // Skip doing slow, unecessary work for this bulk operations.
            return;
        }
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        auto& entityInfo = GetInfo(entityId);
        if (!entityInfo.IsConnected())
        {
            return;
        }

        // Even though these child entities will immediately be destroyed, their entity info may be recycled
        // Ensure they don't have any lingering inaccurate parent data
        auto children = entityInfo.GetChildren();
        for (auto childId : children)
        {
            ReparentChild(childId, AZ::EntityId(), entityId);
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::RemoveDirtyEntity, childId);
            m_entityOrphanTable[entityId].insert(childId);
        }

        m_savedOrderInfo[entityId] = AZStd::make_pair(entityInfo.GetParent(), entityInfo.GetIndexForSorting());

        if (entityId.IsValid())
        {
            RemoveChildFromParent(entityInfo.GetParent(), entityId);
        }

        //disconnect entry from any buses
        entityInfo.Disconnect();

        EditorEntitySortNotificationBus::MultiHandler::BusDisconnect(entityId);
    }

    void EditorEntityModel::AddChildToParent(AZ::EntityId parentId, AZ::EntityId childId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ_Assert(childId != parentId, "AddChildToParent called with same child and parent");
        if (childId == parentId || !childId.IsValid())
        {
            return;
        }

        auto& childInfo = GetInfo(childId);
        auto& parentInfo = GetInfo(parentId);
        if (!parentInfo.IsConnected())
        {
            AddChildToParent(AZ::EntityId(), childId);
            m_entityOrphanTable[parentId].insert(childId);
            return;
        }

        childInfo.SetParentId(parentId);

        AZ::TransformBus::Event(childId, &AZ::TransformBus::Events::SetParentRelative, parentId);

        if (!parentInfo.HasChild(childId))
        {
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedAddChildBegin, parentId, childId);

            parentInfo.AddChild(childId);

            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedAddChildEnd, parentId, childId);
        }

        AZStd::unordered_map<AZ::EntityId, AZStd::pair<AZ::EntityId, AZ::u64>>::const_iterator orderItr = m_savedOrderInfo.find(childId);
        if (orderItr == m_savedOrderInfo.end() || orderItr->second.first != parentId)
        {
            if (m_gotInstantiateSliceDetails)
            {
                AzToolsFramework::AddEntityIdToSortInfo(parentId, childId, m_postInstantiateBeforeEntity);
                m_gotInstantiateSliceDetails = false;
            }
            else
            {
                AzToolsFramework::AddEntityIdToSortInfo(parentId, childId, m_forceAddToBack);
            }
        }

        UpdateSliceInfoHierarchy(childInfo.GetId());
        childInfo.UpdateOrderInfo(true);
    }

    void EditorEntityModel::RemoveChildFromParent(AZ::EntityId parentId, AZ::EntityId childId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ_Assert(childId != parentId, "RemoveChildFromparent called with same child and parent");
        AZ_Assert(childId.IsValid(), "RemoveChildFromparent called with an invalid child entity id");
        if (childId == parentId || !childId.IsValid())
        {
            return;
        }

        auto& childInfo = GetInfo(childId);

        //always override the parent to match internal state
        m_entityOrphanTable[parentId].erase(childId);
        parentId = childInfo.GetParent();
        m_entityOrphanTable[parentId].erase(childId);

        auto& parentInfo = GetInfo(parentId);
        if (parentInfo.IsConnected())
        {
            //creating/pushing slices doesn't always destroy/de-register the original entity before adding the replacement
            if (parentInfo.HasChild(childId))
            {
                EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedRemoveChildBegin, parentId, childId);

                parentInfo.RemoveChild(childId);

                EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedRemoveChildEnd, parentId, childId);
            }
        }

        // During prefab propagation, we don't need to update the sort order. It'll be taken care by prefab instance deserialization.
        if (!m_isPrefabPropagationInProgress)
        {
            AzToolsFramework::RemoveEntityIdFromSortInfo(parentId, childId);
        }

        childInfo.SetParentId(AZ::EntityId());
        UpdateSliceInfoHierarchy(childInfo.GetId());

        RemoveFromAncestorCyclicDependencyList(parentId, childId);
    }

    void EditorEntityModel::RemoveFromAncestorCyclicDependencyList(const AZ::EntityId& parentId, const AZ::EntityId& entityId)
    {
        EditorEntityModelEntry& parentInfo = GetInfo(parentId);
        if (parentInfo.GetCyclicDependencyList().size() == 0)
        {
            return;
        }

        // Remove the entity and its decendants from the cyclic dependency list of its ancestors
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        AzToolsFramework::EntityIdList entitiesToRemove = entityInfo.GetCyclicDependencyList();
        entitiesToRemove.push_back(entityId);

        AZ::EntityId ancestorId = parentId;
        while (ancestorId.IsValid())
        {
            for (const AZ::EntityId& entityToRemove : entitiesToRemove)
            {
                EditorEntityInfoRequestBus::Event(ancestorId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::RemoveFromCyclicDependencyList, entityToRemove);
            }

            AZ::EntityId currentParentId = ancestorId;
            ancestorId.SetInvalid();
            EditorEntityInfoRequestBus::EventResult(ancestorId, currentParentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        }
    }

    void EditorEntityModel::ReparentChild(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ_Assert(oldParentId != entityId, "ReparentChild gave us an oldParentId that is the same as the entityId. An entity cannot be a parent of itself, ignoring old parent");
        AZ_Assert(newParentId != entityId, "ReparentChild gave us an newParentId that is the same as the entityId. An entity cannot be a parent of itself, ignoring old parent");
        if (oldParentId != entityId && newParentId != entityId)
        {
            RemoveChildFromParent(oldParentId, entityId);
            AddChildToParent(newParentId, entityId);
        }
    }

    EditorEntityModel::EditorEntityModelEntry& EditorEntityModel::GetInfo(const AZ::EntityId& entityId)
    {
        //retrieve or add an entity entry to the table
        //the entry must exist, even if not connected, so children and other data can be assigned
        [[maybe_unused]] auto [it, inserted] = m_entityInfoTable.try_emplace(entityId);
        auto& entityInfo = it->second;

        //the entity id defaults to invalid and must be set to match the requested id
        //disconnect and reassign if there's a mismatch
        if (entityInfo.GetId() != entityId)
        {
            entityInfo.Disconnect();
            entityInfo.SetId(entityId);
        }
        return entityInfo;
    }

    void EditorEntityModel::EntityRegistered(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //when an editor entity is created and registered, add it to a pending list.
        //once all entities in the pending list are activated, add them to model.
        bool isEditorEntity = false;
        EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &EditorEntityContextRequestBus::Events::IsEditorEntity, entityId);
        if (isEditorEntity)
        {
            // As an optimization, new entities are queued.
            // They're added to the model once they've all activated, or the next tick happens.
            m_queuedEntityAdds.insert(entityId);
            m_queuedEntityAddsNotYetActivated.insert(entityId);
            AZ::EntityBus::MultiHandler::BusConnect(entityId);
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void EditorEntityModel::EntityDeregistered(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //when an editor entity is de-registered, stop tracking it
        if (m_entityInfoTable.find(entityId) != m_entityInfoTable.end())
        {
            RemoveEntity(entityId);
        }

        if (m_queuedEntityAdds.find(entityId) != m_queuedEntityAdds.end())
        {
            m_queuedEntityAdds.erase(entityId);
            m_queuedEntityAddsNotYetActivated.erase(entityId);
            AZ::EntityBus::MultiHandler::BusDisconnect(entityId);

            // If deregistered entity was the last one being waited on, the queued entities can now be processed.
            if (m_queuedEntityAddsNotYetActivated.empty() && !m_queuedEntityAdds.empty())
            {
                ProcessQueuedEntityAdds();
            }
        }
    }

    void EditorEntityModel::SetEntityInstantiationPosition(const AZ::EntityId& parent, const AZ::EntityId& beforeEntity)
    {
        m_postInstantiateBeforeEntity = beforeEntity;
        m_postInstantiateSliceParent = parent;
        m_gotInstantiateSliceDetails = true;
    }

    void EditorEntityModel::ClearEntityInstantiationPosition()
    {
        m_postInstantiateBeforeEntity.SetInvalid();
        m_postInstantiateSliceParent.SetInvalid();
        m_gotInstantiateSliceDetails = false;
    }

    void EditorEntityModel::EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (GetInfo(entityId).IsConnected())
        {
            ReparentChild(entityId, newParentId, oldParentId);
        }
    }

    void EditorEntityModel::SetForceAddEntitiesToBackFlag(bool forceAddToBack)
    {
        m_forceAddToBack = forceAddToBack;
    }

    void EditorEntityModel::ChildEntityOrderArrayUpdated()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //when notified that a parent has reordered its children, they must be updated
        if (m_enableChildReorderHandler)
        {
            auto orderEntityId = *EditorEntitySortNotificationBus::GetCurrentBusId();
            auto parentEntityId = orderEntityId;
            // If we are being called for the invalid entity sort info id, then use it as the parent for updating purposes
            if (orderEntityId == GetEntityIdForSortInfo(AZ::EntityId()))
            {
                parentEntityId = AZ::EntityId();
            }
            auto& entityInfo = GetInfo(parentEntityId);
            for (auto childId : entityInfo.GetChildren())
            {
                auto& childInfo = GetInfo(childId);
                childInfo.UpdateOrderInfo(true);
            }

            entityInfo.OnChildSortOrderChanged();
        }
    }

    void EditorEntityModel::OnPrepareForContextReset()
    {
        m_preparingForContextReset = true;
        Reset();
    }

    void EditorEntityModel::OnEntityStreamLoadBegin()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityStreamLoadSuccess()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        //block internal reorder event handling to avoid recursion since we're manually updating everything
        m_enableChildReorderHandler = false;

        //prepare to completely reset any observers to the current model state
        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetBegin);

        //refresh all order info while blocking related events (keeps UI observers from updating until refresh is complete)
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityModel::OnEntityStreamLoadSuccess:UpdateChildOrderInfo");
            for (auto& entityInfoPair : m_entityInfoTable)
            {
                if (entityInfoPair.second.IsConnected())
                {
                    //add order info if missing
                    entityInfoPair.second.UpdateChildOrderInfo(m_forceAddToBack);
                }
            }
        }
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "EditorEntityModel::OnEntityStreamLoadSuccess:UpdateOrderInfo");
            for (auto& entityInfoPair : m_entityInfoTable)
            {
                if (entityInfoPair.second.IsConnected())
                {
                    entityInfoPair.second.UpdateOrderInfo(false);
                }
            }
        }

        EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoResetEnd);

        m_enableChildReorderHandler = true;
    }

    void EditorEntityModel::OnEntityStreamLoadFailed()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityContextReset()
    {
        Reset();
    }

    void EditorEntityModel::OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList&)
    {
        // Register as handler for sort updates to the root slice metadata entity as it handles all unparented entity sorting
        EditorEntitySortNotificationBus::MultiHandler::BusConnect(GetEntityIdForSortInfo(AZ::EntityId()));
    }

    void EditorEntityModel::OnEditorOnlyChanged(AZ::EntityId entityId, bool isEditorOnly)
    {
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.OnEditorOnlyChanged(isEditorOnly);
    }

    void EditorEntityModel::OnEntityRuntimeActivationChanged(AZ::EntityId entityId, bool activeOnStart)
    {
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.OnEntityRuntimeActivationChanged(activeOnStart);
    }

    void EditorEntityModel::OnEntityTransformChanged(const AzToolsFramework::EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        for (const AZ::EntityId& entityId : entityIds)
        {
            EditorEntityModelEntry& entityInfo = GetInfo(entityId);
            entityInfo.OnTransformChanged();
        }
    }

    void EditorEntityModel::OnEntityComponentAdded(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.OnComponentCompositionChanged(componentId, ComponentCompositionAction::Add);
    }

    void EditorEntityModel::OnEntityComponentRemoved(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.OnComponentCompositionChanged(componentId, ComponentCompositionAction::Remove);
    }

    void EditorEntityModel::OnEntityComponentEnabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.OnComponentCompositionChanged(componentId, ComponentCompositionAction::Enable);
    }

    void EditorEntityModel::OnEntityComponentDisabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.OnComponentCompositionChanged(componentId, ComponentCompositionAction::Disable);
    }

    void EditorEntityModel::OnEntityExists(const AZ::EntityId& entityId)
    {
        // as soon as the Entity is initialized, ensure we connect to the
        // EditorEntityInfoRequestBus and update the entity lock and visibility
        // state (other components may care about this in their Activate call)
        EditorEntityModelEntry& entityInfo = GetInfo(entityId);
        entityInfo.EntityInfoRequestConnect();
    }

    void EditorEntityModel::OnEntityActivated(const AZ::EntityId& activatedEntityId)
    {
        // Stop listening for this entity's activation.
        AZ::EntityBus::MultiHandler::BusDisconnect(activatedEntityId);
        m_queuedEntityAddsNotYetActivated.erase(activatedEntityId);

        // Once all queued entities have activated, add them all to the model.
        if (m_queuedEntityAddsNotYetActivated.empty())
        {
            ProcessQueuedEntityAdds();
        }
    }

    void EditorEntityModel::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        // If a tick has elapsed, and entities are still queued, add them all to the model.
        if (!m_queuedEntityAdds.empty())
        {
            ProcessQueuedEntityAdds();
        }

        AZ::TickBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::OnPrefabInstancePropagationBegin()
    {
        m_isPrefabPropagationInProgress = true;
    }

    void EditorEntityModel::OnPrefabInstancePropagationEnd()
    {
        m_isPrefabPropagationInProgress = false;
    }

    void EditorEntityModel::UpdateSliceInfoHierarchy(AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        auto& entityInfo = GetInfo(entityId);
        entityInfo.UpdateOrderInfo(false);
        entityInfo.UpdateSliceInfo();

        for (auto childId : entityInfo.GetChildren())
        {
            UpdateSliceInfoHierarchy(childId);
        }
    }

    void EditorEntityModel::AddToChildrenWithOverrides(const EntityIdList& parentEntityIds, const AZ::EntityId& entityId)
    {
        for (auto& parentEntityId : parentEntityIds)
        {
            auto& entityInfo = GetInfo(parentEntityId);
            entityInfo.m_overriddenChildren.insert(entityId);
        }
    }

    void EditorEntityModel::RemoveFromChildrenWithOverrides(const EntityIdList& parentEntityIds, const AZ::EntityId& entityId)
    {
        for (auto& parentEntityId : parentEntityIds)
        {
            auto& entityInfo = GetInfo(parentEntityId);
            entityInfo.m_overriddenChildren.erase(entityId);
        }
    }

    EditorEntityModel::EditorEntityModelEntry::~EditorEntityModelEntry()
    {
        Disconnect();
    }

    void EditorEntityModel::EditorEntityModelEntry::EntityInfoRequestConnect()
    {
        // Ensure parent is invalid in case GetParentId doesn't have a parent to return a result
        m_parentId.SetInvalid();
        AZ::TransformBus::EventResult(m_parentId, m_entityId, &AZ::TransformBus::Events::GetParentId);

        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            m_visible, m_entityId, &AzToolsFramework::EditorVisibilityRequests::GetVisibilityFlag);
        AzToolsFramework::EditorLockComponentRequestBus::EventResult(
            m_locked, m_entityId, &AzToolsFramework::EditorLockComponentRequests::GetLocked);

        EditorEntityInfoRequestBus::Handler::BusConnect(m_entityId);
    }

    void EditorEntityModel::EditorEntityModelEntry::Connect()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        Disconnect();

        EntityInfoRequestConnect();

        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_selected, &AzToolsFramework::ToolsApplicationRequests::IsSelected, m_entityId);

        AZ::ComponentApplicationBus::BroadcastResult(
            m_entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
        m_name = m_entity ? m_entity->GetName() : "";

        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EditorEntityModelEntry", m_serializeContext, "Could not retrieve application serialize context.");

        AZ::EntityBus::Handler::BusConnect(m_entityId);
        EditorLockComponentNotificationBus::Handler::BusConnect(m_entityId);
        EditorVisibilityNotificationBus::Handler::BusConnect(m_entityId);
        EntitySelectionEvents::Bus::Handler::BusConnect(m_entityId);
        EditorEntityAPIBus::Handler::BusConnect(m_entityId);
        EditorInspectorComponentNotificationBus::Handler::BusConnect(m_entityId);
        PropertyEditorEntityChangeNotificationBus::Handler::BusConnect(m_entityId);

        // Slice status is required before we attempt to determine sibling sort index or slice root status
        UpdateSliceInfo();
        UpdateOrderInfo(false);

        m_connected = true;

        // Visibility and lock state needs to be refreshed after connecting to the relevant buses.
        ComponentEntityEditorRequestBus::Event(m_entityId, &ComponentEntityEditorRequestBus::Events::RefreshVisibilityAndLock);
    }

    void EditorEntityModel::EditorEntityModelEntry::Disconnect()
    {
        m_connected = false;
        m_baseAncestorId = AZ::EntityId();

        PropertyEditorEntityChangeNotificationBus::Handler::BusDisconnect();
        EditorInspectorComponentNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoRequestBus::Handler::BusDisconnect();
        EditorEntityAPIBus::Handler::BusDisconnect();
        EntitySelectionEvents::Bus::Handler::BusDisconnect();
        EditorVisibilityNotificationBus::Handler::BusDisconnect();
        EditorLockComponentNotificationBus::Handler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateSliceInfo()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        //reset slice info
        m_sliceFlags = (m_sliceFlags & SliceFlag_OverridesMask); // only hold on to the override flags
        m_sliceAssetName.clear();

        //determine if entity belongs to a slice
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, m_entityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        auto sliceReference = sliceAddress.GetReference();
        auto sliceInstance = sliceAddress.GetInstance();
        if (sliceReference && sliceInstance)
        {
            m_sliceFlags |= SliceFlag_Entity;

            //determine slice asset name
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(m_sliceAssetName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceReference->GetSliceAsset().GetId());

            // Asset is not known by the AssetCatalog yet.
            // This is likely a newly created slice
            // Update the name using the SliceAsset's hint
            if (m_sliceAssetName.empty())
            {
                m_sliceAssetName = sliceReference->GetSliceAsset().GetHint();
            }

            //determine if entity parent belongs to a slice
            AZ::SliceComponent::SliceInstanceAddress parentSliceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(parentSliceAddress, m_parentId,
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

            //we're a slice root if we don't have a slice reference or instance or our parent slice reference or instances don't match
            auto parentSliceReference = parentSliceAddress.GetReference();
            auto parentSliceInstance = parentSliceAddress.GetInstance();
            if (!parentSliceReference || !parentSliceInstance || (sliceReference != parentSliceReference) || (sliceInstance->GetId() != parentSliceInstance->GetId()))
            {
                m_sliceFlags |= SliceFlag_Root;
            }

            AZ::SliceComponent::EntityAncestorList tempAncestors;
            sliceAddress.GetReference()->GetInstanceEntityAncestry(m_entityId, tempAncestors);

            // If we still aren't a slice root, check to see if maybe we are a subslice root
            if (!IsSliceRoot())
            {
                for (const AZ::SliceComponent::Ancestor& ancestor : tempAncestors)
                {
                    if (ancestor.m_sliceAddress.IsValid() && (ancestor.m_sliceAddress.GetReference() != sliceReference))
                    {
                        m_sliceFlags |= SliceFlag_Subslice;
                    }

                    if (ancestor.m_entity && SliceUtilities::IsRootEntity(*ancestor.m_entity))
                    {
                        m_sliceFlags |= (SliceFlag_Subslice | SliceFlag_Root);
                        break;
                    }
                }
            }

            if (!tempAncestors.empty())
            {
                // only care about the immediate ancestor
                const AZ::SliceComponent::Ancestor& entityAncestor = tempAncestors.front();

                if (m_baseAncestorId != entityAncestor.m_entity->GetId())
                {
                    AZStd::unique_ptr<AZ::Entity> compareClone = SliceUtilities::CloneSliceEntityForComparison(*entityAncestor.m_entity, *sliceAddress.GetInstance(), *m_serializeContext);
                    if (compareClone)
                    {
                        m_baseAncestorId = entityAncestor.m_entity->GetId();
                        m_sourceClone.reset(m_serializeContext->CloneObject<AZ::Entity>(compareClone.get()));

                        DetectInitialOverrides();
                    }
                    else
                    {
                        m_baseAncestorId = AZ::EntityId();
                        AZ_Error("EditorEntityModelEntry", false, "Failed to build internal override cache for slice entity %s", m_name.c_str());
                    }
                }
            }
        }

        UpdateCyclicDependencyInfo();
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateOrderInfo(bool notify)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ::u64 oldIndex = m_indexForSorting;
        AZ::u64 newIndex = 0;

        //because sort info is stored on ether a parent or a meta data entity, need to determine which before querying
        EditorEntitySortRequestBus::EventResult(newIndex, GetEntityIdForSortInfo(m_parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, m_entityId);

        if (notify && oldIndex != newIndex)
        {
            //notifications still use actual parent
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedOrderBegin, m_parentId, m_entityId, m_indexForSorting);
        }

        m_indexForSorting = newIndex;

        if (notify && oldIndex != newIndex)
        {
            //notifications still use actual parent
            EditorEntityInfoNotificationBus::Broadcast(&EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedOrderEnd, m_parentId, m_entityId, m_indexForSorting);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateChildOrderInfo(bool forceAddToBack)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //add order info if missing
        for (auto childId : m_children)
        {
            AzToolsFramework::AddEntityIdToSortInfo(m_entityId, childId, forceAddToBack);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::SetId(const AZ::EntityId& entityId)
    {
        m_entityId = entityId;
    }

    void EditorEntityModel::EditorEntityModelEntry::SetParentId(AZ::EntityId parentId)
    {
        m_parentId = parentId;
    }

    void EditorEntityModel::EditorEntityModelEntry::SetName(AZStd::string name)
    {
        AzToolsFramework::ScopedUndoBatch undo("Rename Entity via API");

        m_entity->SetName(AZStd::move(name));

        undo.MarkEntityDirty(m_entity->GetId());

        OnEntityNameChanged(m_entity->GetName());
    }

    void EditorEntityModel::EditorEntityModelEntry::SetParent(AZ::EntityId parentId)
    {
        AZ::EntityId oldParentId = GetParent();

        if (oldParentId == parentId || parentId == m_parentId)
        {
            return;
        }

        AzToolsFramework::ScopedUndoBatch undo("Reparent Entities via API");

        // The new parent is dirty due to sort change(s)
        undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(parentId));

        // The old parent is dirty due to sort change
        undo.MarkEntityDirty(AzToolsFramework::GetEntityIdForSortInfo(oldParentId));

        // The re-parented entity is dirty due to parent change
        undo.MarkEntityDirty(m_entityId);

        // Guarding this to prevent the entity from being marked dirty when the parent doesn't change.
        AZ::TransformBus::Event(m_entityId, &AZ::TransformBus::Events::SetParent, parentId);

        bool isParentVisible = AzToolsFramework::IsEntitySetToBeVisible(parentId);
        AzToolsFramework::SetEntityVisibility(m_entityId, isParentVisible);
        AzToolsFramework::ComponentEntityEditorRequestBus::Event(m_entityId, &AzToolsFramework::ComponentEntityEditorRequestBus::Events::RefreshVisibilityAndLock);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }

    void EditorEntityModel::EditorEntityModelEntry::SetLockState(bool state)
    {
        AzToolsFramework::ScopedUndoBatch undo("Set Entity Lock State via API");
        SetEntityLockState(m_entityId, state);
    }

    void EditorEntityModel::EditorEntityModelEntry::SetVisibilityState(bool state)
    {
        AzToolsFramework::ScopedUndoBatch undo("Set Entity Visibility via API");
        SetEntityVisibility(m_entityId, state);
    }

    void EditorEntityModel::EditorEntityModelEntry::SetStartStatus(EditorEntityStartStatus status)
    {
        switch (status)
        {
        case EditorEntityStartStatus::StartActive:
            // [[fallthrough]]
        case EditorEntityStartStatus::StartInactive:
            SetStartActiveStatus(status == EditorEntityStartStatus::StartActive);
            break;
        case EditorEntityStartStatus::EditorOnly:      
            {
                ScopedUndoBatch undo("Set EditorOnly via API");
                EditorOnlyEntityComponentRequestBus::Event(m_entityId, &EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity, true);
            }

            EditorEntityRuntimeActivationChangeNotificationBus::Broadcast(
                &EditorEntityRuntimeActivationChangeNotificationBus::Events::OnEntityRuntimeActivationChanged,
                m_entityId, m_entity->IsRuntimeActiveByDefault());
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
            break;
        default:
            AZ_Error("EditorEntityModel", false, "Invalid Editor Entity Start Status.");
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::SetStartActiveStatus(bool isActive)
    {
        {
            ScopedUndoBatch undo("Set Start Status via API");
            EditorOnlyEntityComponentRequestBus::Event(m_entityId, &EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity, false);

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_entityId);

            if (entity)
            {
                EditorEntityRuntimeActivationChangeNotificationBus::Broadcast(
                    &EditorEntityRuntimeActivationChangeNotificationBus::Events::OnEntityRuntimeActivationChanged,
                    m_entityId, isActive);

                entity->SetRuntimeActiveByDefault(isActive);
            }
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorEntityModel::EditorEntityModelEntry::AddChild(AZ::EntityId childId)
    {
        auto childItr = m_childIndexCache.find(childId);
        if (childItr == m_childIndexCache.end())
        {
            // m_children is guaranteed to be ordered by EntityId, do a sorted insertion
            auto insertedChildIndex = AZStd::upper_bound(m_children.begin(), m_children.end(), childId);
            insertedChildIndex = m_children.insert(insertedChildIndex, childId);

            // Cache all affected child indices for fast lookup
            for (auto it = insertedChildIndex; it != m_children.end(); ++it)
            {
                const AZ::u64 newChildIndex = static_cast<AZ::u64>(it - m_children.begin());
                m_childIndexCache[*it] = newChildIndex;
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::RemoveChild(AZ::EntityId childId)
    {
        // Retrieve our child index from the cache
        auto cachedIndexItr = m_childIndexCache.find(childId);
        if (cachedIndexItr == m_childIndexCache.end())
        {
            AZ_Assert(false, "Attempted to remove an unknown child");
            return;
        }

        // Build an iterator for m_children based on our cached index
        auto childItr = m_children.begin() + cachedIndexItr->second;

        // Remove our child from the cache
        m_childIndexCache.erase(cachedIndexItr);

        // Remove our child, fix up the cache entries for any subsequent children
        auto elementsToFixItr = m_children.erase(childItr);
        for (auto it = elementsToFixItr; it != m_children.end(); ++it)
        {
            const AZ::u64 newChildIndex = static_cast<AZ::u64>(it - m_children.begin());
            m_childIndexCache[*it] = newChildIndex;
        }
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasChild(AZ::EntityId childId) const
    {
        auto childItr = m_childIndexCache.find(childId);
        return childItr != m_childIndexCache.end();
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetId() const
    {
        return m_entityId;
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetParent() const
    {
        return m_parentId;
    }

    EntityIdList EditorEntityModel::EditorEntityModelEntry::GetChildren() const
    {
        return m_children;
    }

    AZ::EntityId EditorEntityModel::EditorEntityModelEntry::GetChild(AZStd::size_t index) const
    {
        return index < m_children.size() ? m_children[index] : AZ::EntityId();
    }

    AZStd::size_t EditorEntityModel::EditorEntityModelEntry::GetChildCount() const
    {
        return m_children.size();
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::GetChildIndex(AZ::EntityId childId) const
    {
        // Return the cached index, if available.
        auto childItr = m_childIndexCache.find(childId);
        if (childItr != m_childIndexCache.end())
        {
            return childItr->second;
        }

        // On initialization, GetChildIndex may be queried for a childId that is not yet in the child list.
        // Return the position it would be inserted at in EditorEntityModelEntry::AddChild
        auto targetChildPositionItr = AZStd::upper_bound(m_children.begin(), m_children.end(), childId);
        return static_cast<AZ::u64>(targetChildPositionItr - m_children.begin());
    }

    AZStd::string EditorEntityModel::EditorEntityModelEntry::GetName() const
    {
        return m_name;
    }

    AZStd::string EditorEntityModel::EditorEntityModelEntry::GetSliceAssetName() const
    {
        return m_sliceAssetName;
    }

    AzToolsFramework::EntityIdList EditorEntityModel::EditorEntityModelEntry::GetCyclicDependencyList() const
    {
        return m_cyclicDependencyList;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSliceEntity() const
    {
        return (m_sliceFlags & SliceFlag_Entity) == SliceFlag_Entity;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSubsliceEntity() const
    {
        return (m_sliceFlags & SliceFlag_Subslice) == SliceFlag_Subslice;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSliceRoot() const
    {
        AZ::u8 flags = (m_sliceFlags & ~SliceFlag_OverridesMask);
        return (flags == SliceFlag_SliceRoot);
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSubsliceRoot() const
    {
        AZ::u8 flags = (m_sliceFlags & ~SliceFlag_OverridesMask);
        return (flags == SliceFlag_SubsliceRoot);
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasSliceEntityAnyChildrenAddedOrDeleted() const
    {
        return (m_sliceFlags & SliceFlag_EntityHasAdditionsDeletions) != 0;
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasSliceEntityPropertyOverridesInTopLevel() const
    {
        return (m_sliceFlags & SliceFlag_EntityHasNonChildOverrides) != 0;
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasSliceEntityOverrides() const
    {
        return (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0;
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasSliceChildrenOverrides() const
    {
        return (!m_overriddenChildren.empty()) || HasSliceEntityAnyChildrenAddedOrDeleted();
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasSliceAnyOverrides() const
    {
        return HasSliceEntityOverrides() || HasSliceChildrenOverrides();
    }

    bool EditorEntityModel::EditorEntityModelEntry::HasCyclicDependency() const
    {
        if (!IsSliceRoot() || m_cyclicDependencyList.size() == 0)
        {
            return false;
        }

        // Entities in m_cyclicDependencyList could have cyclic dependency on the ancestors of the current entity rather than the current entity itself
        // Need to check the situation where none of the entities in the list has cyclic depdency on the current entity
        for (const AZ::EntityId& entityId : m_cyclicDependencyList)
        {
            AZStd::string sliceAssetName = "";
            EditorEntityInfoRequestBus::EventResult(sliceAssetName, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetSliceAssetName);

            if (sliceAssetName == m_sliceAssetName)
            {
                return true;
            }
        }

        return false;
    }

    AZ::u64 EditorEntityModel::EditorEntityModelEntry::GetIndexForSorting() const
    {
        AZ::u64 sortIndex = 0;
        EditorEntitySortRequestBus::EventResult(sortIndex, GetEntityIdForSortInfo(m_parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, m_entityId);
        return sortIndex;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsSelected() const
    {
        return m_selected;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsVisible() const
    {
        return m_visible;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsHidden() const
    {
        return !IsVisible();
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsLocked() const
    {
        return m_locked;
    }

    EditorEntityStartStatus EditorEntityModel::EditorEntityModelEntry::GetStartStatus() const
    {
        bool isEditorOnly = false;
        EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, m_entityId, &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

        if (isEditorOnly)
        {
            return EditorEntityStartStatus::EditorOnly;
        }
        else
        {
            if (m_entity->IsRuntimeActiveByDefault())
            {
                return EditorEntityStartStatus::StartActive;
            }
            else
            {
                return EditorEntityStartStatus::StartInactive;
            }
        }
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsJustThisEntityLocked() const
    {
        return m_locked;
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsConnected() const
    {
        return m_connected;
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityLockFlagChanged(bool locked)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_locked != locked)
        {
            m_locked = locked;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedLocked, m_entityId, m_locked);

            EditorEntityLockComponentNotificationBus::Event(
                m_entityId, &EditorEntityLockComponentNotificationBus::Events::OnEntityLockChanged, locked);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityVisibilityFlagChanged(bool visibility)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_visible != visibility)
        {
            m_visible = visibility;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedVisibility, m_entityId, m_visible);

            EditorEntityVisibilityNotificationBus::Event(
                m_entityId, &EditorEntityVisibilityNotifications::OnEntityVisibilityChanged, visibility);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnSelected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (!m_selected)
        {
            m_selected = true;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedSelection, m_entityId, m_selected);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnDeselected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (m_selected)
        {
            m_selected = false;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedSelection, m_entityId, m_selected);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityNameChanged(const AZStd::string& name)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (m_name != name)
        {
            m_name = name;
            EditorEntityInfoNotificationBus::Broadcast(
                &EditorEntityInfoNotificationBus::Events::OnEntityInfoUpdatedName, m_entityId, m_name);

            if (CanProcessOverrides())
            {
                AZ::u8 lastFlags = m_sliceFlags;
                m_sliceFlags = (m_name != m_sourceClone->GetName() ? m_sliceFlags | SliceFlag_EntityNameOverridden
                                                                   : m_sliceFlags & ~SliceFlag_EntityNameOverridden);
                ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnTransformChanged()
    {
        if (CanProcessOverrides())
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            using TransformComponent = AzToolsFramework::Components::TransformComponent;

            if (AZ::Component* transformComponent = m_entity->FindComponent<TransformComponent>())
            {
                OnEntityComponentPropertyChanged(transformComponent->GetId());
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnComponentOrderChanged()
    {
        if (CanProcessOverrides())
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            using EditorInspectorComponent = AzToolsFramework::Components::EditorInspectorComponent;

            if (AZ::Component* componentOrderComponent = m_entity->FindComponent<EditorInspectorComponent>())
            {
                OnEntityComponentPropertyChanged(componentOrderComponent->GetId());
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        if (CanProcessOverrides())
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ::Component* liveComponent = m_entity->FindComponent(componentId);
            AZ::Component* sourceComponent = m_sourceClone->FindComponent(componentId);

            if (HasDifferences<AZ::Component>(liveComponent, sourceComponent, IsSliceRoot(), m_serializeContext))
            {
                AddOverriddenComponent(componentId);
            }
            else
            {
                RemoveOverriddenComponent(componentId);
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEditorOnlyChanged(bool /*isEditorOnly*/)
    {
        if (CanProcessOverrides())
        {
            using EditorOnlyEntityComponent = AzToolsFramework::Components::EditorOnlyEntityComponent;

            if (AZ::Component* editorOnlyComponent = m_entity->FindComponent<EditorOnlyEntityComponent>())
            {
                OnEntityComponentPropertyChanged(editorOnlyComponent->GetId());
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnEntityRuntimeActivationChanged(bool activeOnStart)
    {
        if (CanProcessOverrides())
        {
            AZ::u8 lastFlags = m_sliceFlags;
            m_sliceFlags = (activeOnStart != m_sourceClone->IsRuntimeActiveByDefault()
                                ? m_sliceFlags | SliceFlag_EntityActivationOverridden
                                : m_sliceFlags & ~SliceFlag_EntityActivationOverridden);
            ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnComponentCompositionChanged(const AZ::ComponentId& componentId,
                                                                                  ComponentCompositionAction action)
    {
        if (CanProcessOverrides())
        {
            switch (action)
            {
                case ComponentCompositionAction::Add:
                    AddOverriddenComponent(componentId);
                    break;

                case ComponentCompositionAction::Remove:
                    // if the component was a pending add, we can remove it from the overrides list
                    if (m_overriddenComponents.find(componentId) != m_overriddenComponents.end())
                    {
                        RemoveOverriddenComponent(componentId);
                    }
                    else
                    {
                        AddOverriddenComponent(componentId);
                    }
                    break;

                case ComponentCompositionAction::Enable:
                case ComponentCompositionAction::Disable:
                    OnEntityComponentPropertyChanged(componentId);
                    break;

                default:
                    AZ_Warning("EditorEntityModelEntry", false, "Unknown entity component composition action received");
                    break;
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::OnChildSortOrderChanged()
    {
        if (CanProcessOverrides())
        {
            using EditorEntitySortComponent = AzToolsFramework::Components::EditorEntitySortComponent;

            if (EditorEntitySortComponent* liveSortOrderComponent = m_entity->FindComponent<EditorEntitySortComponent>())
            {
                const AZ::ComponentId& componentId = liveSortOrderComponent->GetId();

                if (EditorEntitySortComponent* sourceSortOrderComponent = m_sourceClone->FindComponent<EditorEntitySortComponent>(componentId))
                {
                    // for reasons unknown, the instance data hierarchy will produce different compare results for the EditorEntitySortComponent
                    // when passed directly in as a component vs. indirectly from the owning entity.  until this can be diagnosed, we have to
                    // rely on manually comparing the child entity sort order.  furthermore, certain operations (such as redo) will "correct"
                    // the entity IDs with the remapped ones so the redirect needs to be included in the manual comparison.
                    EntityOrderArray liveChildSortOrder = liveSortOrderComponent->GetChildEntityOrderArray();
                    EntityOrderArray sourceChildSortOrder = sourceSortOrderComponent->GetChildEntityOrderArray();

                    if (liveChildSortOrder.size() == sourceChildSortOrder.size())
                    {
                        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                        AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, m_entityId,
                            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                        bool isDifferent = false;
                        if (!sliceAddress.GetInstance())
                        {
                            isDifferent = true;
                        }
                        else
                        {
                            const AZ::SliceComponent::EntityIdToEntityIdMap& liveToSourceIds = sliceAddress.GetInstance()->GetEntityIdToBaseMap();

                            for (int sortOrderIndex = 0; sortOrderIndex < liveChildSortOrder.size(); ++sortOrderIndex)
                            {
                                const AZ::EntityId& liveId = liveChildSortOrder[sortOrderIndex];
                                const AZ::EntityId& sourceId = sourceChildSortOrder[sortOrderIndex];

                                if (liveId != sourceId)
                                {
                                    const auto& sourceMapping = liveToSourceIds.find(liveId);
                                    if (sourceMapping == liveToSourceIds.end()
                                        || sourceMapping->second != sourceId)
                                    {
                                        isDifferent = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (isDifferent)
                        {
                            AddChildAddedDeleted(componentId);
                        }
                        else
                        {
                            RemoveChildAddedDeleted(componentId);

                        }
                    }
                    else
                    {
                        AddChildAddedDeleted(componentId);
                    }
                }
            }
        }
    }

    bool EditorEntityModel::EditorEntityModelEntry::CanProcessOverrides() const
    {
        return (IsSliceEntity() && m_serializeContext && m_entity && m_sourceClone);
    }

    void EditorEntityModel::EditorEntityModelEntry::AddChildAddedDeleted(AZ::ComponentId componentId)
    {
        m_addedRemovedComponents.insert(componentId);
        AZ::u8 lastFlags = m_sliceFlags;
        m_sliceFlags |= SliceFlag_EntityHasAdditionsDeletions;

        ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_Values);
    }

    void EditorEntityModel::EditorEntityModelEntry::RemoveChildAddedDeleted(AZ::ComponentId componentId)
    {
        m_addedRemovedComponents.erase(componentId);
        AZ::u8 lastFlags = m_sliceFlags;
        if (m_addedRemovedComponents.empty())
        {
            m_sliceFlags &= ~SliceFlag_EntityHasAdditionsDeletions;
        }

        ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_Values);
    }

    void EditorEntityModel::EditorEntityModelEntry::AddOverriddenComponent(AZ::ComponentId componentId)
    {
        m_overriddenComponents.insert(componentId);
        AZ::u8 lastFlags = m_sliceFlags;
        m_sliceFlags |= SliceFlag_EntityComponentsOverridden;

        ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_Values);
    }

    void EditorEntityModel::EditorEntityModelEntry::RemoveOverriddenComponent(AZ::ComponentId componentId)
    {
        m_overriddenComponents.erase(componentId);
        AZ::u8 lastFlags = m_sliceFlags;
        if (m_overriddenComponents.empty())
        {
            m_sliceFlags &= ~SliceFlag_EntityComponentsOverridden;
        }

        ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_Values);
    }

    void EditorEntityModel::EditorEntityModelEntry::DetectInitialOverrides()
    {
        if (!IsSliceEntity())
        {
            return;
        }

        if (!CanProcessOverrides())
        {
            AZ_Error("EditorEntityModelEntry", false, "Failed to build internal override cache for slice entity %s",
                     m_name.c_str());
            return;
        }

        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ::u8 lastFlags = m_sliceFlags;

        m_sliceFlags &= ~SliceFlag_EntityHasOverrides;
        m_overriddenComponents.clear();
        m_addedRemovedComponents.clear();

        // reset the cached entity name so we can leverage OnEntityNameChanged to detect if the name is overridden and avoid duplicate code
        m_name = "";
        OnEntityNameChanged(m_entity->GetName());
        OnEntityRuntimeActivationChanged(m_entity->IsRuntimeActiveByDefault());

        // build a map of the source entity components that will be pruned down as the components from the
        // live entity are processed, this way we can determine which components have been deleted
        AZStd::unordered_map<AZ::ComponentId, AZ::Component*> sourceComponentMap;
        for (AZ::Component* sourceComponent : m_sourceClone->GetComponents())
        {
            sourceComponentMap[sourceComponent->GetId()] = sourceComponent;
        }

        // process the components on the live entity to find which ones have overrides
        for (AZ::Component* liveComponent : m_entity->GetComponents())
        {
            const AZ::ComponentId componentId = liveComponent->GetId();

            auto sourceComponentIter = sourceComponentMap.find(componentId);
            if (sourceComponentIter != sourceComponentMap.end())
            {
                if (HasDifferences<AZ::Component>(liveComponent, sourceComponentIter->second, IsSliceRoot(), m_serializeContext))
                {
                    m_overriddenComponents.insert(componentId);
                }

                // mark the component as processed by removing it from the source map
                sourceComponentMap.erase(componentId);
            }
            // the component must be new if it wasn't found in the source map
            else
            {
                m_overriddenComponents.insert(componentId);
            }
        }

        // the remaining components in the source map must have been deleted, so add them to the overridden list
        for (auto& sourceComponentEntry : sourceComponentMap)
        {
            m_overriddenComponents.insert(sourceComponentEntry.first);
        }

        if (!m_overriddenComponents.empty())
        {
            m_sliceFlags |= SliceFlag_EntityComponentsOverridden;
        }

        ModifyParentsOverriddenChildren(m_entityId, lastFlags, (m_sliceFlags & SliceFlag_EntityHasOverrides) != 0);
    }

    bool EditorEntityModel::EditorEntityModelEntry::IsComponentExpanded(AZ::ComponentId id) const
    {
        auto expandedItr = m_componentExpansionStateMap.find(id);
        return (expandedItr == m_componentExpansionStateMap.end() || expandedItr->second);
    }

    void EditorEntityModel::EditorEntityModelEntry::SetComponentExpanded(AZ::ComponentId id, bool expanded)
    {
        if (expanded)
        {
            // We can simply erase the element if it is expanded since not present equates to expanded.
            // This should keep our data stored to a minimum
            m_componentExpansionStateMap.erase(id);
        }
        else
        {
            m_componentExpansionStateMap[id] = expanded;
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::ModifyParentsOverriddenChildren(AZ::EntityId childEntityId, AZ::u8 lastFlags, bool childHasOverrides)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (((lastFlags & SliceFlag_EntityHasOverrides) == 0) != ((m_sliceFlags & SliceFlag_EntityHasOverrides) == 0))
        {
            AZStd::vector<AZ::EntityId> parentsIds;
            AZ::EntityId currentId = childEntityId;
            do
            {
                AZ::EntityId parentId;
                AZ::TransformBus::EventResult(parentId, currentId, &AZ::TransformBus::Events::GetParentId);
                if (parentId.IsValid())
                {
                    parentsIds.push_back(parentId);
                }
                currentId = parentId;
            } while (currentId.IsValid());

            if (childHasOverrides)
            {
                EditorEntityModelRequestBus::Broadcast(&EditorEntityModelRequestBus::Events::AddToChildrenWithOverrides,
                    parentsIds, childEntityId);
            }
            else
            {
                EditorEntityModelRequestBus::Broadcast(&EditorEntityModelRequestBus::Events::RemoveFromChildrenWithOverrides,
                    parentsIds, childEntityId);
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::UpdateCyclicDependencyInfo()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // Only check cyclic dependency if the current entity is a slice root
        if (!IsSliceRoot())
        {
            return;
        }

        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, m_entityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        AzToolsFramework::EntityIdList cyclicDependencyChain;

        AZ::EntityId parentId = GetParent();

        while (parentId.IsValid())
        {
            cyclicDependencyChain.push_back(parentId);

            bool parentIsSliceRoot = false;
            EditorEntityInfoRequestBus::EventResult(parentIsSliceRoot, parentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);

            // Only check cyclic dependency on slice root entities
            if (parentIsSliceRoot)
            {
                AZ::SliceComponent::SliceInstanceAddress parentSliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(parentSliceAddress, parentId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                if (!AzToolsFramework::SliceUtilities::CheckSliceAdditionCyclicDependencySafe(sliceAddress, parentSliceAddress))
                {
                    // Cyclic dependency is detected
                    break;
                }
            }

            AZ::EntityId currentParentId = parentId;
            parentId.SetInvalid();
            EditorEntityInfoRequestBus::EventResult(parentId, currentParentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);
        }

        // Update the cyclic denpendency list of each ancestor entity
        if (parentId.IsValid())
        {
            for (const AZ::EntityId& entityId : cyclicDependencyChain)
            {
                EditorEntityInfoRequestBus::Event(entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::AddToCyclicDependencyList, m_entityId);
            }
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::AddToCyclicDependencyList(const AZ::EntityId& entityId)
    {
        // Each entity in this list has circular dependency on the current entity or its ancenstor
        if(AZStd::find(m_cyclicDependencyList.begin(), m_cyclicDependencyList.end(), entityId) == m_cyclicDependencyList.end())
        {
            m_cyclicDependencyList.push_back(entityId);
        }
    }

    void EditorEntityModel::EditorEntityModelEntry::RemoveFromCyclicDependencyList(const AZ::EntityId& entityId)
    {
        AzToolsFramework::EntityIdList::iterator it = AZStd::find(m_cyclicDependencyList.begin(), m_cyclicDependencyList.end(), entityId);
        if (it != m_cyclicDependencyList.end())
        {
            m_cyclicDependencyList.erase(it);
        }
    }
}   // namespace AzToolsFramework
