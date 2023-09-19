/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/UserDataTypes.h>
#include <Source/RigidBody.h>
#include <Source/Articulation/ArticulationLinkConfiguration.h>

namespace physx
{
    class PxArticulationLink;
    class PxArticulationReducedCoordinate;
}

namespace PhysX
{
    //! Maximum number of articulation links in a single articulation.
    constexpr size_t MaxArticulationLinks = 64;

    //! Configuration data for an articulation link. Contains references to child links.
    struct ArticulationLinkData 
    {
        AZ_CLASS_ALLOCATOR(ArticulationLinkData, AZ::SystemAllocator);
        AZ_TYPE_INFO(PhysX::ArticulationLinkData, "{0FA03CD7-0FD2-4A80-8DB7-45DB944C8B24}");
        static void Reflect(AZ::ReflectContext* context);

        //! Articulation link specific properties for constructing PxArticulationLink.
        //! This data comes from Articulation Link Component in the Editor.
        ArticulationLinkConfiguration m_articulationLinkConfiguration;

        //! Data related to the collision shapes for this link.
        AzPhysics::ShapeColliderPairList m_shapeColliderConfigurationList;

        //! Cached local transform of this link relative to its parent.
        //! This is needed because at the time of constructing the articulation
        //! child entities corresponding to the links won't be active yet,
        //! so there's no way to query their local transform.
        AZ::Transform m_localTransform = AZ::Transform::CreateIdentity();

        //! Extra data for the articulation joint that is not in the link configuration.
        AZ::Transform m_jointLeadLocalFrame = AZ::Transform::CreateIdentity();
        AZ::Transform m_jointFollowerLocalFrame = AZ::Transform::CreateIdentity();

        //! List of child links. Together this forms a tree-like data structure representing the entire articulation.
        AZStd::vector<AZStd::shared_ptr<ArticulationLinkData>> m_childLinks;
    };

    //! Represents a single articulation link.
    class ArticulationLink
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(ArticulationLink, AZ::SystemAllocator);
        AZ_RTTI(PhysX::ArticulationLink, "{48A87D2B-3F12-4411-BE24-6F7534C77287}", AzPhysics::SimulatedBody);

        ~ArticulationLink() override = default;

        void SetupFromLinkData(const ArticulationLinkData& thisLinkData);
        void SetPxArticulationLink(physx::PxArticulationLink* pxLink);

        // AzPhysics::SimulatedBody overrides...
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;
        AZ::EntityId GetEntityId() const override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;

    private:
        void AddCollisionShape(const ArticulationLinkData& thisLinkData);

        // PxArticulationLinks are managed by the articulation,
        // so we don't need to worry about calling release() here.
        physx::PxArticulationLink* m_pxLink = nullptr;

        ActorData m_actorData;
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> m_physicsShapes;
    };

    ArticulationLink* CreateArticulationLink(const ArticulationLinkConfiguration* articulationConfig);

} // namespace PhysX
