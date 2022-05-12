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
    //! This component requires a box shape component that defines a world space to collect geometry from
    //! static physical colliders present within the bounds of a shape component on the same entity.
    //!
    //! @note You can provide your implementation of collecting geometry instead of this component.
    //!       If you do, in @GetProvidedServices specify AZ_CRC_CE("RecastNavigationSurveyorService"),
    //!       which is needed by @RecastNavigationMeshComponent.
    class RecastNavigationSurveyorComponent final
        : public AZ::Component
        , public RecastNavigationSurveyorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationSurveyorComponent, "{202de120-29f3-4b64-b95f-268323d86349}");

        RecastNavigationSurveyorComponent() = default;
        explicit RecastNavigationSurveyorComponent(const AZStd::vector<AZ::u32>& tags);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // RecastNavigationSurveyorRequestBus interface implementation
        AZStd::vector<AZStd::shared_ptr<TileGeometry>> CollectGeometry(float tileSize, float borderSize) override;
        AZ::Aabb GetWorldBounds() const override;
        bool IsTiled() const override { return false; }

    private:
        // Append the geometry within a volume
        void AppendColliderGeometry(TileGeometry& geometry, const AzPhysics::SceneQueryHits& overlapHits);
        
        AZStd::vector<AZ::u32> m_tags;
    };
} // namespace RecastNavigation
