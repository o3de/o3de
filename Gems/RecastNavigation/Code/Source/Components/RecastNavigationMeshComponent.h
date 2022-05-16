/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DetourNavMesh.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <Components/RecastNavigationMeshCommon.h>
#include <Components/RecastNavigationMeshConfig.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

namespace RecastNavigation
{
    //! Calculates a navigation mesh with the triangle data provided by @RecastNavigationSurveyorComponent.
    //! Path calculation is done using @DetourNavigationComponent.
    class RecastNavigationMeshComponent final
        : public AZ::Component
        , public RecastNavigationMeshCommon
        , public RecastNavigationMeshRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationMeshComponent, "{a281f314-a525-4c05-876d-17eb632f14b4}");
        RecastNavigationMeshComponent() = default;
        RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config, bool drawDebug);

        static void Reflect(AZ::ReflectContext* context);

        //! RecastNavigationRequestBus interface implementation
        //! @{
        void UpdateNavigationMeshBlockUntilCompleted() override;
        dtNavMesh* GetNativeNavigationMap() const override { return m_navMesh.get(); }
        dtNavMeshQuery* GetNativeNavigationQuery() const override { return m_navQuery.get(); }
        //! @}

        //! AZ::Component interface implementation
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

    private:
        //! Tick event for the optional debug draw.
        AZ::ScheduledEvent m_tickEvent{ [this]() { OnDebugDrawTick(); }, AZ::Name("RecastNavigationDebugViewTick")};
        void OnDebugDrawTick();

        //! In-game navigation mesh configuration.
        RecastNavigationMeshConfig m_meshConfig;

        //! Debug draw option.
        bool m_showNavigationMesh = true;
    };

} // namespace RecastNavigation
