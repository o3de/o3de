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
#include <AzFramework/Spawnable/Spawnable.h>

#include <Shine/Bus/UiSpawnerBus.h>
#include <Shine/Bus/UiGameEntityContextBus.h>

/**
* UiSpawnerComponent
*
* UiSpawnerComponent facilitates spawning of a design-time selected or run-time provided spawnable at an entity's location with an optional offset.
*/
class UiSpawnerComponent
    : public AZ::Component
    , private UiSpawnerBus::Handler
    , private UiGameEntityContextSpawnResultsBus::MultiHandler
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
    AzFramework::EntitySpawnTicket::Id Spawn() override;
    AzFramework::EntitySpawnTicket::Id SpawnRelative(const AZ::Vector2& relative) override;
    AzFramework::EntitySpawnTicket::Id SpawnViewport(const AZ::Vector2& pos) override;
    AzFramework::EntitySpawnTicket::Id SpawnSpawnable(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable) override;
    AzFramework::EntitySpawnTicket::Id SpawnSpawnableRelative(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& relative) override;
    AzFramework::EntitySpawnTicket::Id SpawnSpawnableViewport(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& pos) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // UiGameEntityContextSpawnResultsBus::MultiHandler
    void OnEntityContextSpawnCompleted(const AZStd::vector<AZ::EntityId>& spawnedEntities) override;
    void OnEntityContextSpawnFailed() override;
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
    AzFramework::EntitySpawnTicket::Id SpawnSpawnableInternal(const AZ::Data::Asset<AzFramework::Spawnable>& spawnable, const AZ::Vector2& position, bool isViewportPosition);
    AZStd::vector<AZ::EntityId> GetTopLevelEntities(const AZStd::vector<AZ::EntityId>& entityIds);
    //////////////////////////////////////////////////////////////////////////

    // Serialized members
    AZ::Data::Asset<AzFramework::Spawnable> m_spawnableAsset{ AZ::Data::AssetLoadBehavior::PreLoad };
    bool m_spawnOnActivate = false;

    // Map UiSpawnId to a pseudo EntitySpawnTicket::Id for bus compatibility
    AZStd::unordered_map<UiSpawnId, AzFramework::EntitySpawnTicket::Id> m_activeSpawns;
    AzFramework::EntitySpawnTicket::Id m_nextTicketId = 1;
};
