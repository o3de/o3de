/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/UiEntityContext.h>
#include <LyShine/Bus/UiGameEntityContextBus.h>
#include <AzFramework/Entity/SliceEntityOwnershipService.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UiGameEntityContext is used for a canvas that is loaded in game as oppoed to being
//! open for editing
class UiGameEntityContext
    : public UiEntityContext
    , public UiGameEntityContextBus::Handler
    , private AzFramework::SliceInstantiationResultBus::MultiHandler
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
    AzFramework::SliceInstantiationTicket InstantiateDynamicSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Vector2& position, bool isViewportPostion,
        AZ::Entity* parent, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper) override;
    // ~UiGameEntityContextBus

    // SliceInstantiationResultBus
    void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override;
    void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override;
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
    // ~SliceInstantiationResultBus

    void SetCanvasEntity(AZ::EntityId canvasEntityId) { m_canvasEntityId = canvasEntityId; }

protected: // member functions

    void OnContextEntitiesAdded(const AzFramework::EntityList& entities) override;
    void InitializeEntities(const AzFramework::EntityList& entities);

    // Used to validate that the entities in an instantiated slice are valid entities for this context
    bool ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities) override;

protected: // data

    struct InstantiatingDynamicSlice
    {
        InstantiatingDynamicSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            const AZ::Vector2& position, bool isViewportPosition, AZ::Entity* parent)
            : m_asset(asset)
            , m_position(position)
            , m_isViewportPosition(isViewportPosition)
            , m_parent(parent) {}

        AZ::Data::Asset<AZ::Data::AssetData>    m_asset;
        AZ::Vector2                             m_position;
        bool                                    m_isViewportPosition;
        AZ::Entity*                             m_parent;
    };

    AZStd::unordered_map<AzFramework::SliceInstantiationTicket, InstantiatingDynamicSlice> m_instantiatingDynamicSlices;

    AZ::EntityId m_canvasEntityId;
};
