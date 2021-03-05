/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "EditorEntitySortComponent.h"
#include "EditorEntityInfoBus.h"
#include "EditorEntityHelpers.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>

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
        }

        void EditorEntitySortComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorChildEntitySortService", 0x916caa82));
        }

        void EditorEntitySortComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorChildEntitySortService", 0x916caa82));
        }

        bool EditorEntitySortComponent::SerializationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Prior to version 2, we left the version unspecified so it was technically 0
            if (classElement.GetVersion() <= 1)
            {
                // Convert the vector of entity ids to an array of order entries
                auto entityOrderArrayElement = classElement.FindSubElement(AZ_CRC("ChildEntityOrderArray", 0xc37d344b));
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
                classElement.RemoveElementByName(AZ_CRC("ChildEntityOrderArray", 0xc37d344b));

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
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
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
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
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

                // Use the ToolsApplication to mark the entity dirty, this will only do something if we already have an undo batch
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::AddDirtyEntity, GetEntityId());
                
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
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            auto entityItr = m_childEntityOrderCache.find(entityId);
            if (entityItr != m_childEntityOrderCache.end())
            {
                m_childEntityOrderArray.erase(m_childEntityOrderArray.begin() + entityItr->second);
                RebuildEntityOrderCache();

                MarkDirtyAndSendChangedEvent();

                // Use the ToolsApplication to mark the entity dirty, this will only do something if we already have an undo batch
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::AddDirtyEntity, GetEntityId());

                return true;
            }
            return false;
        }

        AZ::u64 EditorEntitySortComponent::GetChildEntityIndex(const AZ::EntityId& entityId)
        {
            auto entityItr = m_childEntityOrderCache.find(entityId);
            return entityItr != m_childEntityOrderCache.end() ? entityItr->second : std::numeric_limits<AZ::u64>::max();
        }

        void EditorEntitySortComponent::OnEntityStreamLoadSuccess()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

        void EditorEntitySortComponent::MarkDirtyAndSendChangedEvent()
        {
            // mark the order as dirty before sending the ChildEntityOrderArrayUpdated event in order for PrepareSave to be properly handled in the case 
            // one of the event listeners needs to build the InstanceDataHierarchy
            m_entityOrderIsDirty = true;
            EditorEntitySortNotificationBus::Event(GetEntityId(), &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }

        void EditorEntitySortComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorEntitySortRequestBus::Handler::BusConnect(GetEntityId());
            EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        void EditorEntitySortComponent::Activate()
        {
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
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
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

            return firstSelectedEntityPos == m_childEntityOrderArray.end() ? m_childEntityOrderArray.begin() : firstSelectedEntityPos;
        }
    }
} // namespace AzToolsFramework
