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
    class RecastNavigationSurveyorComponent final
        : public AZ::Component
        , protected RecastNavigationSurveyorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationSurveyorComponent, "{202de120-29f3-4b64-b95f-268323d86349}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // RecastNavigationSurveyorRequestBus interface implementation
        void CollectGeometry(Geometry& geometryData) override;
        AZ::Aabb GetWorldBounds() override;

    private:
        // Append the geometry within the world volume
        void AppendColliderGeometry(Geometry& geometry, const AZ::Aabb& aabb, const AzPhysics::SceneQueryHits& overlapHits);
    };
} // namespace RecastNavigation
