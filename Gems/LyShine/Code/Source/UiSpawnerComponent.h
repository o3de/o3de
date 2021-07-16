/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzFramework/Entity/EntityContextBus.h>

#include <LyShine/Bus/UiSpawnerBus.h>
#include <LyShine/Bus/UiGameEntityContextBus.h>

/**
* SpawnerComponent
*
* SpawnerComponent facilitates spawning of a design-time selected or run-time provided "*.dynamicslice" at an entity's location with an optional offset.
*/
class UiSpawnerComponent
    : public AZ::Component
    , private UiSpawnerBus::Handler
    , private UiGameEntityContextSliceInstantiationResultsBus::MultiHandler
{
public:
    AZ_COMPONENT(UiSpawnerComponent, "{5AF19874-04A4-4540-82FC-5F29EC854E31}");

    UiSpawnerComponent();
    ~UiSpawnerComponent() override = default;

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // UiSpawnerBus::Handler
    AzFramework::SliceInstantiationTicket Spawn() override;
    AzFramework::SliceInstantiationTicket SpawnRelative(const AZ::Vector2& relative) override;
    AzFramework::SliceInstantiationTicket SpawnViewport(const AZ::Vector2& pos) override;
    AzFramework::SliceInstantiationTicket SpawnSlice(const AZ::Data::Asset<AZ::Data::AssetData>& slice) override;
    AzFramework::SliceInstantiationTicket SpawnSliceRelative(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& relative) override;
    AzFramework::SliceInstantiationTicket SpawnSliceViewport(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& pos) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // UiGameEntityContextSliceInstantiationResultsBus::MultiHandler
    void OnEntityContextSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
    void OnEntityContextSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
    void OnEntityContextSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
    //////////////////////////////////////////////////////////////////////////

private:

    //////////////////////////////////////////////////////////////////////////
    // Component descriptor
    static void Reflect(AZ::ReflectContext* context);
    static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Private helpers
    AzFramework::SliceInstantiationTicket SpawnSliceInternal(const AZ::Data::Asset<AZ::Data::AssetData>& slice, const AZ::Vector2& position, bool isViewportPosition);
    AZStd::vector<AZ::EntityId> GetTopLevelEntities(const AZ::SliceComponent::EntityList& entities);
    //////////////////////////////////////////////////////////////////////////

    // Serialized members
    AZ::Data::Asset<AZ::DynamicSliceAsset> m_sliceAsset{ AZ::Data::AssetLoadBehavior::PreLoad };
    bool m_spawnOnActivate = false;
};
