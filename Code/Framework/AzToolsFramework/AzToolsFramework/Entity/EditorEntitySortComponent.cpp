/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorEntitySortComponent.h"
#include "EditorEntityInfoBus.h"
#include "EditorEntityHelpers.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponentSerializer.h>

static_assert(sizeof(AZ::u64) == sizeof(AZ::EntityId), "We use AZ::EntityId for Persistent ID, which is a u64 under the hood. These must be the same size otherwise the persistent id will have to be rewritten");

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorEntitySortComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntityOrderEntry>()
                    // Persistent IDs for this are simply the entity id
                    ->PersistentId([](const void* instance) -> AZ::u64
                {
                    return static_cast<AZ::u64>(reinterpret_cast<const EntityOrderEntry*>(instance)->m_entityId);
                })
                    ->Version(1)
                    ->Field("EntityId", &EntityOrderEntry::m_entityId)
                    ->Field("SortIndex", &EntityOrderEntry::m_sortIndex)
                    ;
                serializeContext->Class<EditorEntitySortComponent, EditorComponentBase>()
                    ->Version(2, &SerializationConverter)
                    ->EventHandler<EntitySortSerializationEvents>()
                    ->Field("ChildEntityOrderEntryArray", &EditorEntitySortComponent::m_childEntityOrderEntryArray)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorEntitySortComponent>("Child Entity Sort Order", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden | AZ::Edit::SliceFlags::DontGatherReference)
                        ;
                }
            }

            AZ::JsonRegistrationContext* jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context);
            if (jsonRegistration)
            {
                jsonRegistration->Serializer<JsonEditorEntitySortComponentSerializer>()->HandlesType<EditorEntitySortComponent>();
            }
        }

        void EditorEntitySortComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorChildEntitySortService"));
        }

        void EditorEntitySortComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorChildEntitySortService"));
        }

        bool EditorEntitySortComponent::SerializationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Prior to version 2, we left the version unspecified so it was technically 0
            if (classElement.GetVersion() <= 1)
            {
                // Convert the vector of entity ids to an array of order entries
                auto entityOrderArrayElement = classElement.FindSubElement(AZ_CRC_CE("ChildEntityOrderArray"));
                if (!entityOrderArrayElement)
                {
                    return false;
                }

                EntityOrderArray entityOrderArray;
                entityOrderArray.reserve(entityOrderArrayElement->GetNumSubElements());
                for (int arrayElementIndex = 0; arrayElementIndex < entityOrderArrayElement->GetNumSubElements(); ++arrayElementIndex)
                {
                    AZ::SerializeContext::DataElementNode& elementNode = entityOrderArrayElement->GetSubElement(arrayElementIndex);
                    AZ::SerializeContext::DataElementNode& idNode = elementNode.GetSubElement(0);
                    AZ::u64 id;
                    if (idNode.GetData(id))
                    {
                        entityOrderArray.push_back(AZ::EntityId(id));
                    }
                }

                AZ_Error("EditorEntitySortComponent", entityOrderArray.size() == entityOrderArrayElement->GetNumSubElements(), "Unable to get all the expected elements for the old entity order array");

                // Get rid of the old array
                classElement.RemoveElementByName(AZ_CRC_CE("ChildEntityOrderArray"));

                // Add a new empty array (unable to use AddElementWithData, fails stating that AZStd::vector is not registered)
                int newArrayElementIndex = classElement.AddElement<EntityOrderEntryArray>(context, "ChildEntityOrderEntryArray");
                if (newArrayElementIndex == -1)
                {
                    return false;
                }

                auto& newArrayElement = classElement.GetSubElement(newArrayElementIndex);

                bool elementAddSucceeded = true;
                for (size_t entityIndex = 0; entityIndex < entityOrderArray.size(); ++entityIndex)
                {
                    int orderEntryElement = newArrayElement.AddElementWithData<EntityOrderEntry>(context, "element", EntityOrderEntry{ entityOrderArray[entityIndex], entityIndex });
                    if (orderEntryElement == -1)
                    {
                        elementAddSucceeded = false;
                    }
                }

                return elementAddSucceeded;
            }
            return true;
        }

        EditorEntitySortComponent::~EditorEntitySortComponent()
        {
            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorEntityContextNotificationBus::Handler::BusDisconnect();
            EditorEntitySortRequestBus::Handler::BusDisconnect();
        }

        EntityOrderArray EditorEntitySortComponent::GetChildEntityOrderArray()
        {
            return m_childEntityOrderArray;
        }

        bool EditorEntitySortComponent::SetChildEntityOrderArray(const EntityOrderArray& entityOrderArray)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            if (m_childEntityOrderArray != entityOrderArray)
            {
                m_childEntityOrderArray = entityOrderArray;
                RebuildEntityOrderCache();
                MarkDirtyAndSendChangedEvent();
                return true;
            }
            return false;
        }

        bool EditorEntitySortComponent::AddChildEntityInternal(const AZ::EntityId& entityId, bool addToBack, EntityOrderArray::iterator insertPosition)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (m_ignoreIncomingOrderChanges)
            {
                return true;
            }

            auto entityItr = m_childEntityOrderCache.find(entityId);
            if (entityItr == m_childEntityOrderCache.end())
            {
                if (addToBack)
                {
                    m_childEntityOrderCache[entityId] = m_childEntityOrderArray.size();
                    m_childEntityOrderArray.push_back(entityId);
                }
                else
                {
                    m_childEntityOrderArray.insert(insertPosition, entityId);
                    RebuildEntityOrderCache();
                }

                MarkDirtyAndSendChangedEvent();
                
                return true;
            }
            return false;
        }

        bool EditorEntitySortComponent::AddChildEntity(const AZ::EntityId& entityId, bool addToBack)
        {
            bool retval = false;

            if (addToBack)
            {
                retval = AddChildEntityInternal(entityId, true, nullptr);
            }
            else
            {
                EntityOrderArray::iterator insertPosition = GetFirstSelectedEntityPosition();
                if (insertPosition != m_childEntityOrderArray.end())
                {
                    ++insertPosition;
                }
                retval = AddChildEntityInternal(entityId, false, insertPosition);
            }

            return retval;
        }

        bool EditorEntitySortComponent::AddChildEntityAtPosition(const AZ::EntityId& entityId, const AZ::EntityId& beforeEntity)
        {
            EntityOrderArray::iterator selectedEntityPos = AZStd::find(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), beforeEntity);
            EntityOrderArray::iterator insertPosition = selectedEntityPos < m_childEntityOrderArray.end() ? selectedEntityPos : m_childEntityOrderArray.begin();

            bool retval = AddChildEntityInternal(entityId, false, insertPosition);

            return retval;
        }

        bool EditorEntitySortComponent::RemoveChildEntity(const AZ::EntityId& entityId)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            if (m_ignoreIncomingOrderChanges)
            {
                return true;
            }

            auto entityItr = m_childEntityOrderCache.find(entityId);
            if (entityItr != m_childEntityOrderCache.end())
            {
                m_childEntityOrderArray.erase(m_childEntityOrderArray.begin() + entityItr->second);
                RebuildEntityOrderCache();

                MarkDirtyAndSendChangedEvent();

                return true;
            }
            return false;
        }

        AZ::u64 EditorEntitySortComponent::GetChildEntityIndex(const AZ::EntityId& entityId)
        {
            auto entityItr = m_childEntityOrderCache.find(entityId);
            return entityItr != m_childEntityOrderCache.end() ? entityItr->second : std::numeric_limits<AZ::u64>::max();
        }

        bool EditorEntitySortComponent::CanMoveChildEntityUp(const AZ::EntityId& entityId)
        {
            // Ensure the entityId is valid.
            if (!entityId.IsValid())
            {
                return false;
            }

            // Only return true if the entityId is in the sort order array and isn't first.
            auto entityIt = AZStd::find(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), entityId);
            if (m_childEntityOrderArray.empty() || m_childEntityOrderArray.front() == entityId || entityIt == m_childEntityOrderArray.end())
            {
                return false;
            }

            return true;
        }

        void EditorEntitySortComponent::MoveChildEntityUp(const AZ::EntityId& entityId)
        {
            // Ensure the entityId is valid.
            if (!entityId.IsValid())
            {
                return;
            }

            // Only return true if the entityId is in the sort order array and isn't last.
            auto entityItr = AZStd::find(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), entityId);
            if (m_childEntityOrderArray.empty() || m_childEntityOrderArray.front() == entityId || entityItr == m_childEntityOrderArray.end())
            {
                return;
            }

            AZStd::swap(*entityItr, *(entityItr - 1));
            RebuildEntityOrderCache();
            MarkDirtyAndSendChangedEvent();
        }

        bool EditorEntitySortComponent::CanMoveChildEntityDown(const AZ::EntityId& entityId)
        {
            // Ensure the entityId is valid.
            if (!entityId.IsValid())
            {
                return false;
            }

            // Only return true if the entityId is in the sort order array and isn't last.
            auto entityItr = AZStd::find(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), entityId);
            if (m_childEntityOrderArray.empty() || m_childEntityOrderArray.back() == entityId || entityItr == m_childEntityOrderArray.end())
            {
                return false;
            }

            return true;
        }

        void EditorEntitySortComponent::MoveChildEntityDown(const AZ::EntityId& entityId)
        {
            // Ensure the entityId is valid.
            if (!entityId.IsValid())
            {
                return;
            }

            // Only return true if the entityId is in the sort order array and isn't last.
            auto entityItr = AZStd::find(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), entityId);
            if (m_childEntityOrderArray.empty() || m_childEntityOrderArray.back() == entityId || entityItr == m_childEntityOrderArray.end())
            {
                return;
            }

            AZStd::swap(*entityItr, *(entityItr + 1));
            RebuildEntityOrderCache();
            MarkDirtyAndSendChangedEvent();
        }

        void EditorEntitySortComponent::OnEntityStreamLoadSuccess()
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            m_childEntityOrderCache.clear();
            if (!m_childEntityOrderArray.empty())
            {
                // remove invalid entities
                m_childEntityOrderArray.erase(
                    AZStd::remove_if(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), [](const AZ::EntityId& id) {return !GetEntityById(id);}),
                    m_childEntityOrderArray.end());

                // If no data, sort children by name
                if (m_childEntityOrderEntryArray.empty())
                {
                    AZStd::sort(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(),
                        [](const EntityOrderArray::value_type& leftEntityId, const EntityOrderArray::value_type& rightEntityId)
                    {
                        return GetEntityById(leftEntityId)->GetName() < GetEntityById(rightEntityId)->GetName();
                    }
                    );
                }

                RebuildEntityOrderCache();

                m_entityOrderIsDirty = (m_childEntityOrderArray.size() != m_childEntityOrderEntryArray.size());
                EditorEntitySortNotificationBus::Event(GetEntityId(), &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
            }
        }

        void EditorEntitySortComponent::OnPrefabInstancePropagationBegin()
        {
            m_ignoreIncomingOrderChanges = true;
        }

        void EditorEntitySortComponent::OnPrefabInstancePropagationEnd()
        {
            m_ignoreIncomingOrderChanges = false;

            if (m_shouldSanityCheckStateAfterPropagation)
            {
                SanitizeOrderEntryArray();
                m_shouldSanityCheckStateAfterPropagation = false;
            }
        }

        void EditorEntitySortComponent::MarkDirtyAndSendChangedEvent()
        {
            // mark the order as dirty before sending the ChildEntityOrderArrayUpdated event in order for PrepareSave to be properly handled in the case 
            // one of the event listeners needs to build the InstanceDataHierarchy
            m_entityOrderIsDirty = true;

            // Use the ToolsApplication to mark the entity dirty, this will only do something if we already have an undo batch
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::AddDirtyEntity, GetEntityId());
            EditorEntitySortNotificationBus::Event(GetEntityId(), &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }

        void EditorEntitySortComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorEntitySortRequestBus::Handler::BusConnect(GetEntityId());
            EditorEntityContextNotificationBus::Handler::BusConnect();
            AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler::BusConnect();
        }

        void EditorEntitySortComponent::Activate()
        {
            // Run the post-serialize handler because PostLoad won't be called automatically
            m_shouldSanityCheckStateAfterPropagation = true;

            // Send out that the order for our entity is now updated
            EditorEntitySortNotificationBus::Event(GetEntityId(), &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }

        void EditorEntitySortComponent::PrepareSave()
        {
            // If we didn't dirty the entity order, we do not need to regenerate the serialized entry array
            if (!m_entityOrderIsDirty)
            {
                return;
            }

            // Clear the actual persistent id storage to get rebuilt from the order entry array
            m_childEntityOrderEntryArray.clear();
            m_childEntityOrderEntryArray.reserve(m_childEntityOrderArray.size());

            // Write our vector data back to the order element array
            for (size_t entityIndex = 0; entityIndex < m_childEntityOrderArray.size(); ++entityIndex)
            {
                m_childEntityOrderEntryArray.push_back({ m_childEntityOrderArray[entityIndex], entityIndex });
            }

            m_entityOrderIsDirty = false;
        }

        void EditorEntitySortComponent::SanitizeOrderEntryArray()
        {
            bool shouldEmitDirtyState = false;

            // Remove invalid and duplicate entries that point at non-existent entities
            AZStd::unordered_set<AZ::EntityId> duplicateIds;
            for (auto it = m_childEntityOrderArray.begin(); it != m_childEntityOrderArray.end();)
            {
                if (!it->IsValid() || GetEntityById(*it) == nullptr || duplicateIds.contains(*it))
                {
                    it = m_childEntityOrderArray.erase(it);
                    shouldEmitDirtyState = true;
                }
                else
                {
                    duplicateIds.insert(*it);
                    ++it;
                }
            }

            // Append any missing children
            EntityIdList children;
            AZ::TransformBus::EventResult(children, GetEntityId(), &AZ::TransformBus::Events::GetChildren);
            for (auto it = m_childEntityOrderArray.begin(); it != m_childEntityOrderArray.end(); ++it)
            {
                if (auto removedChildrenIt = AZStd::remove(children.begin(), children.end(), *it); removedChildrenIt != children.end())
                {
                    children.erase(removedChildrenIt);
                }
            }

            AZStd::sort(children.begin(), children.end(), [](AZ::EntityId lhs, AZ::EntityId rhs)
            {
                return GetEntityById(lhs)->GetName() < GetEntityById(rhs)->GetName();
            });

            if (!children.empty())
            {
                shouldEmitDirtyState = true;
                EntityOrderArray::iterator insertPosition = GetFirstSelectedEntityPosition();
                if (insertPosition != m_childEntityOrderArray.end())
                {
                    ++insertPosition;
                }
                m_childEntityOrderArray.insert(insertPosition, children.begin(), children.end());
            }

            // Clear out the vector to be rebuilt from persistent id
            m_childEntityOrderEntryArray.resize(m_childEntityOrderArray.size());
            for (size_t i = 0; i < m_childEntityOrderArray.size(); ++i)
            {
                m_childEntityOrderEntryArray[i] = {
                    m_childEntityOrderArray[i],
                    static_cast<AZ::u64>(i)
                };
            }

            RebuildEntityOrderCache();

            if (shouldEmitDirtyState)
            {
                ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::Bus::Events::AddDirtyEntity, GetEntityId());
            }

            m_entityOrderIsDirty = false;
        }

        void EditorEntitySortComponent::PostLoad()
        {
            // Clear out the vector to be rebuilt from persistent id
            m_childEntityOrderArray.clear();
            m_childEntityOrderArray.reserve(m_childEntityOrderEntryArray.size());

            // This will sort all the entity order entries by sort index (primary) and entity id (secondary) which should never result in any collisions
            // This is used since slice data patching may create duplicate entries for the same sort index, missing indices and the like.
            // It should never result in multiple entity id entries since the serialization of this data uses a persistent id which is the entity id
            AZStd::sort(m_childEntityOrderEntryArray.begin(), m_childEntityOrderEntryArray.end(),
                [](const EntityOrderEntry& lhs, const EntityOrderEntry& rhs) -> bool
            {
                return lhs.m_sortIndex < rhs.m_sortIndex || (lhs.m_sortIndex == rhs.m_sortIndex && lhs.m_entityId < rhs.m_entityId);
            }
            );

            for (auto& entityOrderEntry : m_childEntityOrderEntryArray)
            {
                m_childEntityOrderArray.push_back(entityOrderEntry.m_entityId);
            }

            RebuildEntityOrderCache();
            m_entityOrderIsDirty = false;
        }

        void EditorEntitySortComponent::RebuildEntityOrderCache()
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            m_childEntityOrderCache.clear();
            for (auto entityId : m_childEntityOrderArray)
            {
                m_childEntityOrderCache[entityId] = m_childEntityOrderCache.size();
            }
        }

        EntityOrderArray::iterator EditorEntitySortComponent::GetFirstSelectedEntityPosition()
        {
            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            EntityOrderArray::iterator firstSelectedEntityPos = m_childEntityOrderArray.end();
            for (const AZ::EntityId& selectedEntityId : selectedEntities)
            {
                EntityOrderArray::iterator selectedEntityPos = AZStd::find(m_childEntityOrderArray.begin(), m_childEntityOrderArray.end(), selectedEntityId);
                firstSelectedEntityPos = selectedEntityPos < firstSelectedEntityPos ? selectedEntityPos : firstSelectedEntityPos;
            }

            return firstSelectedEntityPos;
        }
    }
} // namespace AzToolsFramework
