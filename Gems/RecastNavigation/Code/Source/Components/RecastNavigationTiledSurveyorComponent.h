/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RecastNavigationDebugDraw.h"
#include "RecastNavigationMeshConfig.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <Components/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

namespace RecastNavigation
{
    //! This component requires an axis aligned box shape component that defines a world space to collect geometry from
    //! static physical colliders present within the bounds of a shape component on the same entity.
    //! The geometry is collected in portions of vertical tiles and is fed into @RecastNavigationMeshComponent.
    //!
    //! @note You can provide your implementation of collecting geometry instead of this component.
    //!       If you do, in your component's @GetProvidedServices specify AZ_CRC_CE("RecastNavigationSurveyorService"),
    //!       which is needed by @RecastNavigationMeshComponent.
    class RecastNavigationTiledSurveyorComponent final
        : public AZ::Component
        , public RecastNavigationSurveyorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationTiledSurveyorComponent, "{4bc92ce5-e179-4985-b0b1-f22bff6006dd}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! AZ::Component interface implementation
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! RecastNavigationSurveyorRequestBus interface implementation
        //! @{
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) override;
        AZ::Aabb GetWorldBounds() const override;
        int GetNumberOfTiles(float tileSize) const override;
        //! @}

    private:

        //! A container of shapes and their respective Entity Ids
        using ShapeHits = AZStd::vector<AZStd::pair<AZ::EntityId, Physics::Shape*>>;

        //! Collect all the shapes within a given volume.
        void CollectGeometryWithinVolume(const AZ::Aabb& volume, ShapeHits& overlapHits);

        //! Append the triangle geometry within a volume to a tile structure.
        void AppendColliderGeometry(TileGeometry& geometry, const ShapeHits& overlapHits);
    };
} // namespace RecastNavigation
