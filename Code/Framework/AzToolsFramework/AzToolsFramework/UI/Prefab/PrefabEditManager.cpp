/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabEditManager.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerCacheBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabEditManager::PrefabEditManager()
        {
            m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
            if (m_prefabPublicInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabPublicInterface on PrefabEditManager construction.");
                return;
            }

            AZ::Interface<PrefabEditInterface>::Register(this);
            AZ::Interface<EditorInteractionInterface>::Register(this);
        }

        PrefabEditManager::~PrefabEditManager()
        {
            AZ::Interface<EditorInteractionInterface>::Unregister(this);
            AZ::Interface<PrefabEditInterface>::Unregister(this);
        }

        void PrefabEditManager::EditOwningPrefab(AZ::EntityId entityId)
        {
            if (!entityId.IsValid())
            {
                // Clear current stack, notify the Outliner of the changes
                while (!m_instanceEditStack.empty())
                {
                    EntityOutlinerCacheNotificationBus::Broadcast(
                        &EntityOutlinerCacheNotifications::EntityCacheChanged, m_instanceEditStack.top());
                    m_instanceEditStack.pop();
                }

                return;
            }

            AZ::EntityId containerEntityId = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            AZ::EntityId parentEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(entityId);
            AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

            AZStd::vector<AZ::EntityId> changedEntityIds;

            if (!m_instanceEditStack.empty())
            {
                if (parentEntityId == m_instanceEditStack.top())
                {
                    m_instanceEditStack.push(containerEntityId);
                    changedEntityIds.push_back(containerEntityId);
                }
                else
                {
                    if (m_instanceEditStack.contains(containerEntityId))
                    {
                        // Remove from the stack until you get to the correct one
                        while (m_instanceEditStack.top() != containerEntityId)
                        {
                            changedEntityIds.push_back(m_instanceEditStack.top());
                            m_instanceEditStack.pop();
                        }
                    }
                    else
                    {
                        // Clear current stack, then loop through all ancestors and add them to the stack
                        while (!m_instanceEditStack.empty())
                        {
                            changedEntityIds.push_back(m_instanceEditStack.top());
                            m_instanceEditStack.pop();
                        }

                        AZ::EntityId currentEntityId = containerEntityId;
                        while (currentEntityId != levelContainerEntityId)
                        {
                            m_instanceEditStack.push_bottom(currentEntityId);
                            changedEntityIds.push_back(currentEntityId);
                            currentEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(entityId);
                        }
                    }
                }
            }
            else
            {
                if (parentEntityId == levelContainerEntityId)
                {
                    m_instanceEditStack.push(containerEntityId);
                    changedEntityIds.push_back(containerEntityId);
                }
                else
                {
                    // Loop through all ancestors and add them to the stack
                    AZ::EntityId currentEntityId = containerEntityId;
                    while (currentEntityId != levelContainerEntityId)
                    {
                        m_instanceEditStack.push_bottom(currentEntityId);
                        changedEntityIds.push_back(currentEntityId);
                        currentEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(entityId);
                    }
                }
            }

            // Notify Outliner of changes to update cache
            for (AZ::EntityId changedEntityId : changedEntityIds)
            {
                EntityOutlinerCacheNotificationBus::Broadcast(&EntityOutlinerCacheNotifications::EntityCacheChanged, changedEntityId);
            }
        }

        bool PrefabEditManager::IsOwningPrefabBeingEdited(AZ::EntityId entityId)
        {
            if (m_instanceEditStack.empty())
            {
                return false;
            }

            AZ::EntityId containerEntity = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_instanceEditStack.top() == containerEntity;
        }

        bool PrefabEditManager::IsOwningPrefabInEditStack(AZ::EntityId entityId)
        {
            if (m_instanceEditStack.empty())
            {
                return false;
            }

            AZ::EntityId containerEntityId = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_instanceEditStack.contains(containerEntityId);
        }

        AZ::EntityId PrefabEditManager::RedirectEntitySelection(AZ::EntityId entityId)
        {
            // Retrieve the Level Container
            AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

            // Find container entity for owning prefab of passed entity
            AZ::EntityId containerEntityId =  m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);

            // If the entity belongs to the level instance or an instance that is currently being edited, it can be selected
            if (containerEntityId == levelContainerEntityId || m_instanceEditStack.contains(containerEntityId))
            {
                return entityId;
            }

            // Else keep looping until you can find an instance that is being edited, or the level instance
            AZ::EntityId parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);

            while (parentContainerEntityId.IsValid() && parentContainerEntityId != levelContainerEntityId &&
                   !m_instanceEditStack.contains(parentContainerEntityId))
            {
                // Else keep going up the hierarchy
                containerEntityId = parentContainerEntityId;
                parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);
            }

            return containerEntityId;
        }

        void PrefabEditManager::PrefabInstanceStack::push(AZ::EntityId entityId)
        {
            m_stack.push_back(entityId);
        }

        AZ::EntityId PrefabEditManager::PrefabInstanceStack::top()
        {
            return m_stack.back();
        }

        void PrefabEditManager::PrefabInstanceStack::pop()
        {
            m_stack.pop_back();
        }

        void PrefabEditManager::PrefabInstanceStack::push_bottom(AZ::EntityId entityId)
        {
            m_stack.push_front(entityId);
        }

        bool PrefabEditManager::PrefabInstanceStack::empty()
        {
            return m_stack.empty();
        }

        bool PrefabEditManager::PrefabInstanceStack::contains(AZ::EntityId entityId)
        {
            return AZStd::find(m_stack.begin(), m_stack.end(), entityId) != m_stack.end();
        }
    }
}
