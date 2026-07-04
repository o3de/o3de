/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Entity/EntityOwnershipService.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_set.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! A minimal entity ownership service for UI contexts.
//! Manages a flat list of entities directly.
class UiSimpleEntityOwnershipService
    : public AzFramework::EntityOwnershipService
{
public:
    UiSimpleEntityOwnershipService(const AzFramework::EntityContextId& contextId, AZ::SerializeContext* serializeContext);
    ~UiSimpleEntityOwnershipService() override;

    // EntityOwnershipService overrides
    void Initialize() override;
    bool IsInitialized() override;
    void Destroy() override;
    void Reset() override;
    void AddEntity(AZ::Entity* entity) override;
    void AddEntities(const AzFramework::EntityList& entities) override;
    bool DestroyEntity(AZ::Entity* entity) override;
    bool DestroyEntityById(AZ::EntityId entityId) override;
    void GetNonPrefabEntities(AzFramework::EntityList& entityList) override;
    bool GetAllEntities(AzFramework::EntityList& entityList) override;
    void InstantiateAllPrefabs() override;
    void HandleEntitiesAdded(const AzFramework::EntityList& entities) override;
    bool LoadFromStream(
        AZ::IO::GenericStream& stream, bool remapIds,
        EntityIdToEntityIdMap* idRemapTable = nullptr,
        const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor()) override;
    void SetEntitiesAddedCallback(AzFramework::OnEntitiesAddedCallback onEntitiesAddedCallback) override;
    void SetEntitiesRemovedCallback(AzFramework::OnEntitiesRemovedCallback onEntitiesRemovedCallback) override;
    void SetValidateEntitiesCallback(AzFramework::ValidateEntitiesCallback validateEntitiesCallback) override;
    void HandleEntityBeingDestroyed(const AZ::EntityId& entityId) override;

    //! Load pre-deserialized entities into the service. Used when loading canvas files.
    void LoadEntities(const AZStd::vector<AZ::Entity*>& entities, bool remapIds,
                      EntityIdToEntityIdMap* idRemapTable = nullptr);

    //! Get the context ID this service belongs to
    const AzFramework::EntityContextId& GetContextId() const { return m_contextId; }

private:
    AzFramework::EntityContextId m_contextId;
    AZ::SerializeContext* m_serializeContext;
    AZStd::unordered_set<AZ::Entity*> m_entities;
    bool m_initialized = false;
};
