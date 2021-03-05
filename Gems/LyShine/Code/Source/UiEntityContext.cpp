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

#include "LyShine_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetDatabase.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <LyShine/UiEntityContext.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
UiEntityContext::UiEntityContext()
    : AzFramework::EntityContext(AzFramework::EntityContextId::CreateRandom())
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiEntityContext::~UiEntityContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::Activate()
{
    InitContext();

    GetRootSlice()->Instantiate();

    UiEntityContextRequestBus::Handler::BusConnect(GetContextId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::Deactivate()
{
    UiEntityContextRequestBus::Handler::BusDisconnect();

    DestroyContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiEntityContext::GetRootAssetEntity()
{
    return m_rootAsset.Get()->GetEntity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiEntityContext::CloneRootAssetEntity()
{
    AZ::SerializeContext* context = nullptr;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);

    AZ::Entity* rootAssetEntity = GetRootAssetEntity();

    AZ::Entity* clonedRootAssetEntity = context->CloneObject(rootAssetEntity);
    return clonedRootAssetEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiEntityContext::CreateUiEntity(const char* name)
{
    AZ::Entity* entity = CreateEntity(name);

    if (entity)
    {
        // we don't currently do anything extra here, UI entities are not automatically
        // Init'ed and Activate'd when they are created. We wait until the required components
        // are added before Init and Activate
    }

    return entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::AddUiEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Supplied entity is invalid.");

    AddEntity(entity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::AddUiEntities(const AzFramework::EntityContext::EntityList& entities)
{
    AZ::PrefabAsset* rootSlice = m_rootAsset.Get();

    for (AZ::Entity* entity : entities)
    {
        AZ_Assert(!AzFramework::EntityIdContextQueryBus::MultiHandler::BusIsConnectedId(entity->GetId()), "Entity already in context.");
        rootSlice->GetComponent()->AddEntity(entity);
    }

    HandleEntitiesAdded(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEntityContext::CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityContext::EntityList& resultEntities)
{
    resultEntities.clear();

    AZ::PrefabComponent::InstantiatedContainer sourceObjects;
    for (const AZ::EntityId& id : sourceEntities)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);
        if (entity)
        {
            sourceObjects.m_entities.push_back(entity);
        }
    }

    AZ::PrefabComponent::EntityIdToEntityIdMap idMap;
    AZ::PrefabComponent::InstantiatedContainer* clonedObjects =
        AZ::Utils::CloneObjectAndFixEntities(&sourceObjects, idMap);
    if (!clonedObjects)
    {
        AZ_Error("UiEntityContext", false, "Failed to clone source entities.");
        return false;
    }

    resultEntities = clonedObjects->m_entities;

    AddUiEntities(resultEntities);

    sourceObjects.m_entities.clear();
    clonedObjects->m_entities.clear();
    delete clonedObjects;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEntityContext::DestroyUiEntity(AZ::EntityId entityId)
{
    if (DestroyEntity(entityId))
    {
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEntityContext::HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds, AZ::PrefabComponent::EntityIdToEntityIdMap* idRemapTable)
{
    AZ_Assert(m_rootAsset, "The context has not been initialized.");

    if (!AzFramework::EntityContext::HandleLoadedRootSliceEntity(rootEntity, remapIds, idRemapTable))
    {
        return false;
    }

    AZ::PrefabComponent::EntityList entities;
    GetRootSlice()->GetEntities(entities);

    GetRootSlice()->SetIsDynamic(true);

    InitializeEntities(entities);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::OnContextEntitiesAdded(const AzFramework::EntityContext::EntityList& entities)
{
    EntityContext::OnContextEntitiesAdded(entities);

    InitializeEntities(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::OnContextEntityRemoved(const AZ::EntityId& entityId)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::SetupUiEntity(AZ::Entity* entity)
{
    InitializeEntities({ entity });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEntityContext::InitializeEntities(const AzFramework::EntityContext::EntityList& entities)
{
    // UI entities are now automatically activated on creation

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
        {
            entity->Init();
        }
    }

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::ES_INIT)
        {
            entity->Activate();
        }
    }
}
