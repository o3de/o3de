/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <Misc/RecastHelpers.h>
#include <Misc/RecastNavigationPhysXProviderCommon.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

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
        : public AZ::Component
        , public RecastNavigationProviderRequestBus::Handler
        , public RecastNavigationPhysXProviderCommon
    {
    public:
        AZ_COMPONENT(RecastNavigationPhysXProviderComponent, "{4bc92ce5-e179-4985-b0b1-f22bff6006dd}");

        //! Can be invoked by the Editor version of this component to pass the configuration.
        //! @param debugDrawInputData if enabled, debug draw is enabled to show the triangles collected
        explicit RecastNavigationPhysXProviderComponent(bool debugDrawInputData = false);
        static void Reflect(AZ::ReflectContext* context);

        //! AZ::Component overrides ...
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! RecastNavigationProviderRequestBus overrides ...
        //! @{
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) override;
        void CollectGeometryAsync(float tileSize, float borderSize, AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback) override;
        AZ::Aabb GetWorldBounds() const override;
        int GetNumberOfTiles(float tileSize) const override;
        //! @}

    private:
        //! If enabled, debug draw is enabled to show the triangles collected in the Editor scene for the navigation mesh
        bool m_debugDrawInputData = false;
    };
} // namespace RecastNavigation
