/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ForceRegionForces.h>

#include <PhysX/ColliderComponentBus.h>
#include <PhysX/ForceRegionComponentBus.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace PhysX
{
    namespace Pipeline
    {
        class MeshAsset;
    }

    //! Force region internal representation. Computes net force exerted on bodies in a force region. 
    class ForceRegion : private AZ::TransformNotificationBus::MultiHandler
        , private LmbrCentral::SplineComponentNotificationBus::Handler
        , private PhysX::ForceRegionRequestBus::Handler
        , private Physics::ColliderComponentEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceRegion, AZ::SystemAllocator);
        AZ_TYPE_INFO(ForceRegion, "{A04BF43D-242B-4B67-AEE9-201FBD36DFEB}");
        static void Reflect(AZ::ReflectContext* context);

        ForceRegion() = default;
        ForceRegion(const ForceRegion& forceRegion);

        void Activate(AZ::EntityId entityId);
        void Deactivate();

        //! Add a force to this force region and activate it.
        void AddAndActivateForce(AZStd::unique_ptr<BaseForce> force);

        //! Computes net force exerted on an entity.
        AZ::Vector3 CalculateNetForce(const EntityParams& entity) const;

        //! Removes all forces in force region.
        void ClearForces();

        //! Get region parameters - entity ID, position, rotation, spline, AABB
        PhysX::RegionParams GetRegionParams() const;

    private:
        // ForceRegionRequestBus
        void AddForceWorldSpace(const AZ::Vector3& direction, float magnitude) override;
        void AddForceLocalSpace(const AZ::Vector3& direction, float magnitude) override;
        void AddForcePoint(float magnitude) override;
        void AddForceSplineFollow(float dampingRatio, float frequency, float targetSpeed, float lookAhead) override;
        void AddForceSimpleDrag(float dragCoefficient, float volumeDensity) override;
        void AddForceLinearDamping(float damping) override;

        // SplineComponentNotificationBus
        void OnSplineChanged() override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // Physics::ColliderComponentEventBus
        void OnColliderChanged() override;

        AZ::EntityId m_entityId; ///< Entity id of the volume.
        AZ::Transform m_worldTransform; ///< The world transform of the volume.
        AZStd::vector<AZStd::unique_ptr<BaseForce>> m_forces; ///< List of forces attached to the volume.
        PhysX::RegionParams m_regionParams;
    };

    namespace ForceRegionUtil
    {
        //! Creates a structure with params about the force region used to calculate a resulting force.
        RegionParams CreateRegionParams(AZ::EntityId entityId);

        //! Creates a structure with params about en entity used to calculate a resulting force.
        EntityParams CreateEntityParams(AZ::EntityId entityId);

        //! Gets aggregated Aabb of all colliders assigned to entity.
        AZ::Aabb GetForceRegionAabb(AZ::EntityId entityId);
    }
}
