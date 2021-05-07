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
            AZ::EntityId containerEntityId = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);

            // Early out if no change - saves time on refreshing the views
            if (containerEntityId == entityId)
            {
                return;
            }

            AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

            // Notify Outliner of changes to what was previously in the cache
            for (AZ::EntityId changedEntityId : m_editedPrefabHierarchyCache)
            {
                EntityOutlinerCacheNotificationBus::Broadcast(&EntityOutlinerCacheNotifications::EntityCacheChanged, changedEntityId);
            }

            if (!entityId.IsValid() || containerEntityId == levelContainerEntityId)
            {
                // Clear variables, go back to editing the level
                m_editedPrefabContainerId = AZ::EntityId();
                m_editedPrefabHierarchyCache.clear();
            }
            else
            {
                // Set edited prefab to owning instance
                m_editedPrefabContainerId = containerEntityId;

                // Clear cache and populate with all ancestors of edited prefab
                m_editedPrefabHierarchyCache.clear();

                while (containerEntityId != levelContainerEntityId)
                {
                    m_editedPrefabHierarchyCache.insert(containerEntityId);
                    containerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);
                }
            }

            // Notify Outliner of changes to what was added to the cache
            for (AZ::EntityId changedEntityId : m_editedPrefabHierarchyCache)
            {
                EntityOutlinerCacheNotificationBus::Broadcast(&EntityOutlinerCacheNotifications::EntityCacheChanged, changedEntityId);
            }
        }

        bool PrefabEditManager::IsOwningPrefabBeingEdited(AZ::EntityId entityId)
        {
            if (!m_editedPrefabContainerId.IsValid())
            {
                return false;
            }

            AZ::EntityId containerEntity = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_editedPrefabContainerId == containerEntity;
        }

        bool PrefabEditManager::IsOwningPrefabInEditStack(AZ::EntityId entityId)
        {
            if (!m_editedPrefabContainerId.IsValid())
            {
                return false;
            }

            AZ::EntityId containerEntityId = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_editedPrefabHierarchyCache.contains(containerEntityId);
        }

        AZ::EntityId PrefabEditManager::RedirectEntitySelection(AZ::EntityId entityId)
        {
            // Retrieve the Level Container
            AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

            // Find container entity for owning prefab of passed entity
            AZ::EntityId containerEntityId =  m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);

            // If the entity belongs to the level instance or an instance that is currently being edited, it can be selected
            if (containerEntityId == levelContainerEntityId || m_editedPrefabHierarchyCache.contains(containerEntityId))
            {
                return entityId;
            }

            // Else keep looping until you can find an instance that is being edited, or the level instance
            AZ::EntityId parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);

            while (parentContainerEntityId.IsValid() && parentContainerEntityId != levelContainerEntityId &&
                   !m_editedPrefabHierarchyCache.contains(parentContainerEntityId))
            {
                // Else keep going up the hierarchy
                containerEntityId = parentContainerEntityId;
                parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);
            }

            return containerEntityId;
        }
    }
}
