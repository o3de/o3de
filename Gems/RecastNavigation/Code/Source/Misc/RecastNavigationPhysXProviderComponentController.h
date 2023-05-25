/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <RecastNavigation/RecastHelpers.h>
#include <Misc/RecastNavigationPhysXProviderConfig.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

namespace RecastNavigation
{
    //! Common logic for Recast navigation tiled collector components. Recommended use is as a base class.
    //! The method provided are not thread-safe. Synchronize as necessary at the higher level.
    class RecastNavigationPhysXProviderComponentController
        : public RecastNavigationProviderRequestBus::Handler
    {
        friend class EditorRecastNavigationPhysXProviderComponent;
    public:
        AZ_CLASS_ALLOCATOR(RecastNavigationPhysXProviderComponentController, AZ::SystemAllocator);
        AZ_RTTI(RecastNavigationPhysXProviderComponentController, "{182D93F8-9E76-409B-9939-6816509A6F52}");

        RecastNavigationPhysXProviderComponentController();
        explicit RecastNavigationPhysXProviderComponentController(const RecastNavigationPhysXProviderConfig& config);
        ~RecastNavigationPhysXProviderComponentController() override = default;

        static void Reflect(AZ::ReflectContext* context);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();
        void SetConfiguration(const RecastNavigationPhysXProviderConfig& config);
        const RecastNavigationPhysXProviderConfig& GetConfiguration() const;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! RecastNavigationProviderRequestBus overrides ...
        //! @{
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) override;
        bool CollectGeometryAsync(float tileSize, float borderSize, AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback) override;
        AZ::Aabb GetWorldBounds() const override;
        int GetNumberOfTiles(float tileSize) const override;
        //! @}

        //! A container of PhysX overlap scene hits (has PhysX colliders and their position/orientation).
        using QueryHits = AZStd::vector<AzPhysics::SceneQueryHit>;

        //! Blocking call. Collects all the relevant PhysX geometry within a provided volume.
        //! @param tileSize the result is packaged in tiles, which are squares covering the provided volume of @worldVolume
        //! @param borderSize an additional extend in all direction around the tile volume, this additional geometry will allow Recast to connect tiles together.
        //! @param worldVolume the overall volume to collect static PhysX geometry
        //! @returns an array of tiles, each containing indexed geometry
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometryImpl(
            float tileSize,
            float borderSize,
            const AZ::Aabb& worldVolume);

        //! Async variant of @CollectGeometryImpl. Tiles are returned via a callback @tileCallback.
        //!   Calls on @tileCallback will come from a task graph (not a main thread).
        //!   Is expected that the context for callback function will persist until all the tiles are processed.
        //!
        //! Note: the callback will be called at least once and one time for each tile geometry.
        //! For example if two tiles are found the following callbacks will be made:
        //!      tileCallback(tile1);
        //!      tileCallback(tile2);
        //!      tileCallback({}); //-- this indicates the end of the operation
        //!
        //! @param tileSize the result is packaged in tiles, which are squares covering the provided volume of @worldVolume
        //! @param borderSize an additional extend in all direction around the tile volume, this additional geometry will allow Recast to connect tiles together
        //! @param worldVolume worldVolume the overall volume to collect static PhysX geometry
        //! @param tileCallback an empty tile indicates the end of the operation, otherwise a valid shared_ptr is returned with tile geometry
        //! @returns true if an async operation was scheduled, false otherwise
        bool CollectGeometryAsyncImpl(
            float tileSize,
            float borderSize,
            const AZ::Aabb& worldVolume,
            AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback);

        //! Finds all the static PhysX colliders within a given volume.
        //! @param volume the world to look for static colliders
        //! @param overlapHits (out) found colliders will be attached to this container
        void CollectCollidersWithinVolume(const AZ::Aabb& volume, QueryHits& overlapHits);

        //! Given a container of static colliders return indexed triangle data.
        //! @param geometry (out) triangle data will be added
        //! @param overlapHits (in) an array of static PhysX colliders
        void AppendColliderGeometry(TileGeometry& geometry, const QueryHits& overlapHits);

        //! Returns the built-in names for the PhysX scene, either Editor or game scene.
        const char* GetSceneName() const;

    protected:
        void OnConfigurationChanged();

        AZ::EntityComponentIdPair m_entityComponentIdPair;
        RecastNavigationPhysXProviderConfig m_config;

        AzPhysics::CollisionGroup m_collisionGroup;

        //! A way to check if we should stop tile processing (because we might be deactivating, for example).
        AZStd::atomic<bool> m_shouldProcessTiles{ true };

        //! If true, an update operation is in progress.
        AZStd::atomic<bool> m_updateInProgress{ false };

        //! Task graph objects to collect geometry data in tiles over a grid.
        AZ::TaskGraph m_taskGraph{ "RecastNavigation PhysX" };
        AZ::TaskExecutor m_taskExecutor;
        AZStd::unique_ptr<AZ::TaskGraphEvent> m_taskGraphEvent;
        AZ::TaskDescriptor m_taskDescriptor{ "Collect Geometry", "Recast Navigation" };
    };
} // namespace RecastNavigation
