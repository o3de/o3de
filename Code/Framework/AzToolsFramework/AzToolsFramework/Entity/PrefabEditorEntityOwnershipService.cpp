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

#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzCore/Component/Entity.h>

namespace AzToolsFramework
{
    void PrefabEditorEntityOwnershipService::Initialize()
    {
        m_prefabSystemComponent = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponent != nullptr, "Couldn't get prefab system component, it's a requirement for PrefabEntityOwnership system to work");

        // This path has to be updated with a proper path based where the level lives
        // Potentially, a level should be a prefab itself
        m_rootInstance = AZStd::unique_ptr<Prefab::Instance>(m_prefabSystemComponent->CreatePrefab({}, {}, "/"));
    }

    bool PrefabEditorEntityOwnershipService::IsInitialized()
    {
        return m_rootInstance != nullptr;
    }

    void PrefabEditorEntityOwnershipService::Destroy()
    {
        m_rootInstance.reset();
    }

    void PrefabEditorEntityOwnershipService::Reset()
    {
        // TBD
    }

    void PrefabEditorEntityOwnershipService::AddEntity(AZ::Entity* entity)
    {
        AZ_Assert(IsInitialized(), "Tried to add an entity without initializing the Entity Ownership Service");
        m_rootInstance->AddEntity(*entity);
        m_entitiesAddedCallback({ entity });
    }

    void PrefabEditorEntityOwnershipService::AddEntities(const EntityList& entities)
    {
        AZ_Assert(IsInitialized(), "Tried to add entities without initializing the Entity Ownership Service");
        for (AZ::Entity* entity : entities)
        {
            m_rootInstance->AddEntity(*entity);
        }
        m_entitiesAddedCallback(entities);
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntity(AZ::Entity* entity)
    {
        return DestroyEntityById(entity->GetId());
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ_Assert(IsInitialized(), "Tried to destroy an entity without initializing the Entity Ownership Service");
        AZStd::unique_ptr<AZ::Entity> detachedEntity = m_rootInstance->DetachEntity(entityId);
        if (detachedEntity)
        {
            m_entitiesRemovedCallback({ entityId });
            return true;
        }
        return false;
        // The detached entity now gets deleted because the unique_ptr gets out of scope
    }

    void PrefabEditorEntityOwnershipService::GetNonPrefabEntities(EntityList& entities)
    {
        m_rootInstance->GetEntities(entities, false);
    }

    bool PrefabEditorEntityOwnershipService::GetAllEntities(EntityList& entities)
    {
        m_rootInstance->GetEntities(entities, true);
        return true;
    }

    void PrefabEditorEntityOwnershipService::InstantiateAllPrefabs()
    {
    }

    void PrefabEditorEntityOwnershipService::HandleEntitiesAdded(const EntityList& /*entities*/)
    {
    }

    bool PrefabEditorEntityOwnershipService::LoadFromStream(AZ::IO::GenericStream& /*stream*/, bool /*remapIds*/,
        EntityIdToEntityIdMap* /*idRemapTable*/, const AZ::ObjectStream::FilterDescriptor& /*filterDesc*/)
    {
        return true;
    }

    void PrefabEditorEntityOwnershipService::SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback)
    {
        m_entitiesAddedCallback = AZStd::move(onEntitiesAddedCallback);
    }

    void PrefabEditorEntityOwnershipService::SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback)
    {
        m_entitiesRemovedCallback = AZStd::move(onEntitiesRemovedCallback);
    }

    void PrefabEditorEntityOwnershipService::SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback)
    {
        m_validateEntitiesCallback = AZStd::move(validateEntitiesCallback);
    }
}
