/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Recast.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Task/TaskDescriptor.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <RecastNavigation/RecastHelpers.h>
#include <Misc/RecastNavigationDebugDraw.h>
#include <Misc/RecastNavigationMeshConfig.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    //! Common navigation mesh logic for Recast navigation components. Recommended use is as a base class.
    //! The method provided are not thread-safe. Use the mutex from @m_navObject to synchronize as necessary at the higher level.
    class RecastNavigationMeshComponentController
        : public RecastNavigationMeshRequestBus::Handler
    {
        friend class EditorRecastNavigationMeshComponent;
    public:
        AZ_CLASS_ALLOCATOR(RecastNavigationMeshComponentController, AZ::SystemAllocator);
        AZ_RTTI(RecastNavigationMeshComponentController, "{D34CD5E0-8C29-4545-8734-9C7A92F03740}");

        RecastNavigationMeshComponentController();
        explicit RecastNavigationMeshComponentController(const RecastNavigationMeshConfig& config);
        ~RecastNavigationMeshComponentController() override = default;

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();
        void SetConfiguration(const RecastNavigationMeshConfig& config);
        const RecastNavigationMeshConfig& GetConfiguration() const;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //! Allocates and initializes Recast navigation mesh into @m_navMesh.
        //! @param meshEntityId the entity's positions will be used as the center of the navigation mesh.
        //! @return true if the navigation mesh object was successfully created.
        bool CreateNavigationMesh(AZ::EntityId meshEntityId);

        //! Given a Recast data add a tile to the navigation mesh @m_navMesh.
        //! @param navigationTileData the raw data of a Recast tile
        //! @return true if successful.
        bool AttachNavigationTileToMesh(NavigationTileData& navigationTileData);

        //! Given a set of geometry and configuration create a Recast tile that can be attached using @AttachNavigationTileToMesh.
        //! @param geom A set of geometry, triangle data.
        //! @param meshConfig Recast navigation mesh configuration.
        //! @param context Recast context object, @rcContext.
        //! @returns the tile data that can be attached to the navigation mesh using @AttachNavigationTileToMesh
        NavigationTileData CreateNavigationTile(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig, rcContext* context);

        //! Creates a task graph with tasks to process received tile data.
        //! @param config navigation mesh configuration to apply to the tile data
        //! @param sendNotificationEvent once all the tiles are processed and added to the navigation update notify on the main thread
        void ReceivedAllNewTilesImpl(const RecastNavigationMeshConfig& config, AZ::ScheduledEvent& sendNotificationEvent);

        //! RecastNavigationRequestBus overrides ...
        //! @{
        bool UpdateNavigationMeshBlockUntilCompleted() override;
        bool UpdateNavigationMeshAsync() override;
        AZStd::shared_ptr<NavMeshQuery> GetNavigationObject() override;
        //! @}

    protected:
        AZ::EntityComponentIdPair m_entityComponentIdPair;

        //! In-game navigation mesh configuration.
        RecastNavigationMeshConfig m_configuration;

        void OnSendNotificationTick();

        //! Tick event to notify on navigation mesh updates from the main thread.
        //! This is often needed for script environment, such as Script Canvas.
        AZ::ScheduledEvent m_sendNotificationEvent{ [this]() { OnSendNotificationTick(); }, AZ::Name("RecastNavigationMeshUpdated") };

        bool IsDebugDrawEnabled() const;

        //! If debug draw was specified, then this call will be invoked every frame.
        void OnDebugDrawTick();

        //! Tick event for the optional debug draw.
        AZ::ScheduledEvent m_tickEvent{ [this]() { OnDebugDrawTick(); }, AZ::Name("RecastNavigationDebugViewTick") };

        void OnReceivedAllNewTiles();

        //! Tick event to notify on navigation mesh updates from the main thread.
        //! This is often needed for script environment, such as Script Canvas.
        AZ::ScheduledEvent m_receivedAllNewTilesEvent{ [this]() { OnReceivedAllNewTiles(); }, AZ::Name("RecastNavigationReceivedTiles") };

        void OnTileProcessedEvent(AZStd::shared_ptr<TileGeometry> tile);
    
        //! Debug draw object for Recast navigation mesh.
        RecastNavigationDebugDraw m_customDebugDraw;

        //! Recast logging functionality and other optional tools.
        AZStd::unique_ptr<rcContext> m_context;

        //! Recast navigation objects.
        AZStd::shared_ptr<NavMeshQuery> m_navObject;

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> m_tilesToBeProcessed;
        AZStd::recursive_mutex m_tileProcessingMutex;

        //! A way to check if we should stop tile processing (because we might be deactivating, for example).
        AZStd::atomic<bool> m_shouldProcessTiles{ true };

        //! Task graph objects to process tile geometry into Recast tiles.
        AZ::TaskGraph m_taskGraph{ "RecastNavigation Tile Processing" };
        AZ::TaskExecutor m_taskExecutor;
        AZStd::unique_ptr<AZ::TaskGraphEvent> m_taskGraphEvent;
        AZ::TaskDescriptor m_taskDescriptor{ "Processing Tiles", "Recast Navigation" };

        //! If true, an update operation is in progress.
        AZStd::atomic<bool> m_updateInProgress{ false };
    };
} // namespace RecastNavigation
