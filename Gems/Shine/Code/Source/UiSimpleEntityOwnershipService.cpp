/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiSimpleEntityOwnershipService.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSimpleEntityOwnershipService::UiSimpleEntityOwnershipService(
    const AzFramework::EntityContextId& contextId, AZ::SerializeContext* serializeContext)
    : m_contextId(contextId)
    , m_serializeContext(serializeContext)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiSimpleEntityOwnershipService::~UiSimpleEntityOwnershipService()
{
    Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::Initialize()
{
    m_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSimpleEntityOwnershipService::IsInitialized()
{
    return m_initialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::Destroy()
{
    Reset();
    m_initialized = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::Reset()
{
    m_entities.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::AddEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Null entity passed to AddEntity");
    m_entities.insert(entity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::AddEntities(const AzFramework::EntityList& entities)
{
    for (AZ::Entity* entity : entities)
    {
        m_entities.insert(entity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSimpleEntityOwnershipService::DestroyEntity(AZ::Entity* entity)
{
    auto it = m_entities.find(entity);
    if (it != m_entities.end())
    {
        m_entities.erase(it);
        delete entity;
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSimpleEntityOwnershipService::DestroyEntityById(AZ::EntityId entityId)
{
    for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
    {
        if ((*it)->GetId() == entityId)
        {
            AZ::Entity* entity = *it;
            m_entities.erase(it);
            delete entity;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::GetNonPrefabEntities(AzFramework::EntityList& entityList)
{
    // All entities in the UI context are "non-prefab" entities
    GetAllEntities(entityList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSimpleEntityOwnershipService::GetAllEntities(AzFramework::EntityList& entityList)
{
    entityList.reserve(entityList.size() + m_entities.size());
    for (AZ::Entity* entity : m_entities)
    {
        entityList.push_back(entity);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::InstantiateAllPrefabs()
{
    // No prefabs to instantiate in the simple service
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::HandleEntitiesAdded(const AzFramework::EntityList& entities)
{
    for (AZ::Entity* entity : entities)
    {
        m_entities.insert(entity);
    }

    if (m_entitiesAddedCallback)
    {
        m_entitiesAddedCallback(entities);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiSimpleEntityOwnershipService::LoadFromStream(
    AZ::IO::GenericStream& stream, bool remapIds,
    EntityIdToEntityIdMap* idRemapTable,
    const AZ::ObjectStream::FilterDescriptor& filterDesc)
{
    AZ_UNUSED(filterDesc);
    AZ_UNUSED(remapIds);
    AZ_UNUSED(idRemapTable);

    // Deserialize entities from stream
    AZ::Entity* containerEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(stream, m_serializeContext);
    if (!containerEntity)
    {
        return false;
    }

    // The container entity is just a wrapper - extract its child entities if applicable
    // For UI contexts, we don't use this path - entities are loaded via LoadEntities()
    delete containerEntity;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::SetEntitiesAddedCallback(AzFramework::OnEntitiesAddedCallback onEntitiesAddedCallback)
{
    m_entitiesAddedCallback = AZStd::move(onEntitiesAddedCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::SetEntitiesRemovedCallback(AzFramework::OnEntitiesRemovedCallback onEntitiesRemovedCallback)
{
    m_entitiesRemovedCallback = AZStd::move(onEntitiesRemovedCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::SetValidateEntitiesCallback(AzFramework::ValidateEntitiesCallback validateEntitiesCallback)
{
    m_validateEntitiesCallback = AZStd::move(validateEntitiesCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::HandleEntityBeingDestroyed(const AZ::EntityId& entityId)
{
    for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
    {
        if ((*it)->GetId() == entityId)
        {
            m_entities.erase(it);
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiSimpleEntityOwnershipService::LoadEntities(
    const AZStd::vector<AZ::Entity*>& entities, bool remapIds,
    EntityIdToEntityIdMap* idRemapTable)
{
    if (remapIds)
    {
        // Generate new entity IDs and fix references
        for (AZ::Entity* entity : entities)
        {
            AZ::EntityId oldId = entity->GetId();
            AZ::EntityId newId = AZ::Entity::MakeId();
            entity->SetId(newId);
            if (idRemapTable)
            {
                (*idRemapTable)[oldId] = newId;
            }
        }

        // Fix up entity references within the loaded entities
        if (idRemapTable && !idRemapTable->empty())
        {
            AZ::IdUtils::Remapper<AZ::EntityId>::ReplaceIdsAndIdRefs(
                const_cast<AZStd::vector<AZ::Entity*>*>(&entities),
                [idRemapTable](const AZ::EntityId& originalId, bool /*isEntityId*/, const AZStd::function<AZ::EntityId()>& /*idGenerator*/) -> AZ::EntityId
                {
                    auto it = idRemapTable->find(originalId);
                    if (it != idRemapTable->end())
                    {
                        return it->second;
                    }
                    return originalId;
                },
                m_serializeContext);
        }
    }

    // Add entities to our tracking set
    for (AZ::Entity* entity : entities)
    {
        m_entities.insert(entity);
    }

    // Notify that entities have been added
    if (m_entitiesAddedCallback)
    {
        m_entitiesAddedCallback(entities);
    }
}
