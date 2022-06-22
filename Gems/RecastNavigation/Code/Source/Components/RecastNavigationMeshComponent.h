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
#include <AzFramework/Components/ComponentAdapter.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <Misc/RecastNavigationConstants.h>
#include <Misc/RecastNavigationMeshComponentController.h>
#include <Misc/RecastNavigationMeshConfig.h>

namespace RecastNavigation
{
    //! Calculates a navigation mesh with the triangle data provided by @RecastNavigationPhysXProviderComponent.
    //! Path calculation is done using @DetourNavigationComponent.
    class RecastNavigationMeshComponent final
        : public AzFramework::Components::ComponentAdapter<RecastNavigationMeshComponentController, RecastNavigationMeshConfig>
        , public AzFramework::GameEntityContextEventBus::Handler
    {
        using BaseClass = AzFramework::Components::ComponentAdapter<RecastNavigationMeshComponentController, RecastNavigationMeshConfig>;
    public:
        AZ_COMPONENT(RecastNavigationMeshComponent, RecastNavigationMeshComponentTypeId, BaseClass);
        RecastNavigationMeshComponent() = default;
        explicit RecastNavigationMeshComponent(const RecastNavigationMeshConfig& config);

        static void Reflect(AZ::ReflectContext* context);

        //! ComponentAdapter overrides ...
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! GameEntityContextEventBus overrides ...
        //! @{
        void OnGameEntitiesStarted() override;
        //! @}
    };
} // namespace RecastNavigation
