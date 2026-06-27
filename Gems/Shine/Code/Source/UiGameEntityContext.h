/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Shine/UiEntityContext.h>
#include <Shine/Bus/UiGameEntityContextBus.h>
#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UiGameEntityContext is used for a canvas that is loaded in game as opposed to being
//! open for editing
class UiGameEntityContext
    : public UiEntityContext
    , public UiGameEntityContextBus::Handler
    , public AZ::Data::AssetBus::MultiHandler
{
public: // member functions

    UiGameEntityContext(AZ::EntityId canvasEntityId = AZ::EntityId());
    ~UiGameEntityContext() override;

    bool HandleLoadedRootSliceEntity(AZ::Entity* rootEntity, bool remapIds,
        AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr);

    // EntityContext
    bool DestroyEntity(AZ::Entity* entity) override;
    // ~EntityContext

    // UiEntityContext
    void InitUiContext() override;
    void DestroyUiContext() override;
    bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
    bool SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType) override;
    // ~UiEntityContext

    // UiEntityContextRequestBus
    AZ::Entity* CreateUiEntity(const char* name) override;
    void AddUiEntity(AZ::Entity* entity) override;
    void AddUiEntities(const AzFramework::EntityList& entities) override;
    bool CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities) override;
    bool DestroyUiEntity(AZ::EntityId entityId) override;
    // ~UiEntityContextRequestBus

    // UiGameEntityContextBus
    UiSpawnId SpawnSpawnable(
        const AZ::Data::Asset<AzFramework::Spawnable>& spawnableAsset, const AZ::Vector2& position, bool isViewportPosition,
        AZ::Entity* parent) override;
    // ~UiGameEntityContextBus

    // AZ::Data::AssetBus::MultiHandler
    void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
    void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
    // ~AZ::Data::AssetBus

    void SetCanvasEntity(AZ::EntityId canvasEntityId) { m_canvasEntityId = canvasEntityId; }

protected: // member functions

    void OnContextEntitiesAdded(const AzFramework::EntityList& entities) override;
    void InitializeEntities(const AzFramework::EntityList& entities);

    // Used to validate that the entities in an instantiated slice are valid entities for this context
    bool ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities) override;

    // Clone entities from a spawnable and integrate them into the UI context
    void ProcessSpawnableEntities(UiSpawnId spawnId, const AzFramework::Spawnable& spawnable);

protected: // data

    struct PendingSpawn
    {
        PendingSpawn(const AZ::Data::Asset<AzFramework::Spawnable>& asset,
            const AZ::Vector2& position, bool isViewportPosition, AZ::Entity* parent)
            : m_asset(asset)
            , m_position(position)
            , m_isViewportPosition(isViewportPosition)
            , m_parent(parent) {}

        AZ::Data::Asset<AzFramework::Spawnable> m_asset;
        AZ::Vector2                             m_position;
        bool                                    m_isViewportPosition;
        AZ::Entity*                             m_parent;
    };

    AZStd::unordered_map<UiSpawnId, PendingSpawn> m_pendingSpawns;
    UiSpawnId m_nextSpawnId = 1;

    AZ::EntityId m_canvasEntityId;
};
