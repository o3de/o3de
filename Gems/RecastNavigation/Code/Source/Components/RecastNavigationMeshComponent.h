/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <Misc/RecastNavigationMeshCommon.h>
#include <Misc/RecastNavigationMeshConfig.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    //! Calculates a navigation mesh with the triangle data provided by @RecastNavigationPhysXProviderComponent.
    //! Path calculation is done using @DetourNavigationComponent.
    class RecastNavigationMeshComponent final
        : public AZ::Component
        , public RecastNavigationMeshCommon
        , public RecastNavigationMeshRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationMeshComponent, "{a281f314-a525-4c05-876d-17eb632f14b4}");
        RecastNavigationMeshComponent() = default;

        //! Used by the Editor version of this component.
        //! @param config navigation mesh configuration.
        //! @param drawDebug if true, draws debug view of the navigation mesh in the game mode.
        RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config, bool drawDebug);

        static void Reflect(AZ::ReflectContext* context);

        //! RecastNavigationRequestBus overrides ...
        //! @{
        void UpdateNavigationMeshBlockUntilCompleted() override;
        void UpdateNavigationMeshAsync() override;
        AZStd::shared_ptr<NavMeshQuery> GetNavigationObject() override;
        //! @}

        //! AZ::Component overrides ...
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

    private:

        //! In-game navigation mesh configuration.
        RecastNavigationMeshConfig m_meshConfig;

        //! Debug draw option.
        bool m_showNavigationMesh = false;

        void OnSendNotificationTick();

        //! Tick event to notify on navigation mesh updates from the main thread.
        //! This is often needed for script environment, such as Script Canvas.
        AZ::ScheduledEvent m_sendNotificationEvent{ [this]() { OnSendNotificationTick(); }, AZ::Name("RecastNavigationMeshUpdated") };

        //! If debug draw was specified, then this call will be invoked every frame.
        void OnDebugDrawTick();

        //! Tick event for the optional debug draw.
        AZ::ScheduledEvent m_tickEvent{ [this]() { OnDebugDrawTick(); }, AZ::Name("RecastNavigationDebugViewTick") };

        void OnReceivedAllNewTiles();

        //! Tick event to notify on navigation mesh updates from the main thread.
        //! This is often needed for script environment, such as Script Canvas.
        AZ::ScheduledEvent m_receivedAllNewTilesEvent{ [this]() { OnReceivedAllNewTiles(); }, AZ::Name("RecastNavigationReceivedTiles") };

        void OnTileProcessedEvent(AZStd::shared_ptr<TileGeometry> tile);
    };

} // namespace RecastNavigation
