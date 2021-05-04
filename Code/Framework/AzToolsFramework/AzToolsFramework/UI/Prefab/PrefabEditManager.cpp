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
        }

        PrefabEditManager::~PrefabEditManager()
        {
            AZ::Interface<PrefabEditInterface>::Unregister(this);
        }

        void PrefabEditManager::EditOwningPrefab(AZ::EntityId entityId)
        {
            // TODO - make sure the new instance is s child of the one on top of the stack before pushing...
            // Will likely need to change this API
            m_instanceEditStack.push_back(m_prefabPublicInterface->GetInstanceContainerEntityId(entityId));

            EntityOutlinerCacheNotificationBus::Broadcast(&EntityOutlinerCacheNotifications::EntityCacheChanged, entityId);
        }

        bool PrefabEditManager::IsOwningPrefabBeingEdited(AZ::EntityId entityId)
        {
            if (m_instanceEditStack.size() == 0)
            {
                return false;
            }

            AZ::EntityId containerEntity = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_instanceEditStack.back() == containerEntity;
        }

        bool PrefabEditManager::IsOwningPrefabInEditStack(AZ::EntityId entityId)
        {
            if (m_instanceEditStack.size() == 0)
            {
                return false;
            }

            AZ::EntityId containerEntity = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return AZStd::find(m_instanceEditStack.begin(), m_instanceEditStack.end(), containerEntity) != m_instanceEditStack.end();
        }

        AZ::EntityId PrefabEditManager::OverrideEntitySelectionInViewport(AZ::EntityId entityId)
        {
            // Retrieve the Level Container
            AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

            // Find container entity for owning prefab of passed entity
            AZ::EntityId containerEntityId =  m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);

            // If the entity belongs to the level instance or an instance that is currently being edited, it can be selected
            if (containerEntityId == levelContainerEntityId ||
                AZStd::find(m_instanceEditStack.begin(), m_instanceEditStack.end(), containerEntityId) != m_instanceEditStack.end())
            {
                return entityId;
            }

            // Else keep looping until you can find an instance that is being edited, or the level instance
            AZ::EntityId parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);

            while (parentContainerEntityId.IsValid() &&  parentContainerEntityId != levelContainerEntityId &&
                   AZStd::find(m_instanceEditStack.begin(), m_instanceEditStack.end(), parentContainerEntityId) == m_instanceEditStack.end())
            {
                // Else keep going up the hierarchy
                containerEntityId = parentContainerEntityId;
                parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);
            }

            return containerEntityId;
        }
    }
}
