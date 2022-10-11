/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <Misc/RecastNavigationConstants.h>
#include <Misc/RecastNavigationPhysXProviderComponentController.h>
#include <Misc/RecastNavigationPhysXProviderConfig.h>

namespace RecastNavigation
{
    //! This component requires an axis aligned box shape component that defines a world space to collect geometry from
    //! static PhysX colliders present within the bounds of a shape component on the same entity.
    //! The geometry is collected in portions of vertical tiles and is fed into @RecastNavigationMeshComponent.
    //!
    //! @note You can provide your implementation of collecting geometry instead of this component.
    //!       If you do, in your component's @GetProvidedServices specify AZ_CRC_CE("RecastNavigationProviderService"),
    //!       which is needed by @RecastNavigationMeshComponent.
    class RecastNavigationPhysXProviderComponent final
        : public AzFramework::Components::ComponentAdapter<RecastNavigationPhysXProviderComponentController, RecastNavigationPhysXProviderConfig>
    {
        using BaseClass = AzFramework::Components::ComponentAdapter<RecastNavigationPhysXProviderComponentController, RecastNavigationPhysXProviderConfig>;
    public:
        AZ_COMPONENT(RecastNavigationPhysXProviderComponent, RecastNavigationPhysXProviderComponentTypeId, BaseClass);
        RecastNavigationPhysXProviderComponent() = default;
        explicit RecastNavigationPhysXProviderComponent(const RecastNavigationPhysXProviderConfig& config);
        
        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace RecastNavigation
