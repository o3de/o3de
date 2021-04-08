/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>

namespace WhiteBox
{
    //! Runtime White Box Collider.
    class WhiteBoxColliderComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(WhiteBoxColliderComponent, "{B60C4D82-3299-414A-B91B-0299AA51BEF6}");
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxColliderComponent() = default;
        WhiteBoxColliderComponent(
            const Physics::CookedMeshShapeConfiguration& meshShape,
            const Physics::ColliderConfiguration& physicsColliderConfiguration,
            const WhiteBoxColliderConfiguration& whiteBoxColliderConfiguration);
        WhiteBoxColliderComponent(const WhiteBoxColliderComponent&) = delete;
        WhiteBoxColliderComponent& operator=(const WhiteBoxColliderComponent&) = delete;

    private:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        // AZ::TransformNotificationBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        Physics::CookedMeshShapeConfiguration m_shapeConfiguration; //!< The physics representation of the mesh.
        Physics::ColliderConfiguration
            m_physicsColliderConfiguration; //!< General physics collider configuration information.
        AzPhysics::SimulatedBodyHandle m_simulatedBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Simulated body to represent the White Box Mesh at runtime.
        WhiteBoxColliderConfiguration
            m_whiteBoxColliderConfiguration; //!< White Box specific collider configuration information.
    };
} // namespace WhiteBox
