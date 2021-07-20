/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>

namespace NvCloth
{
    extern const int InvalidIndex;

    //! Base collider class with transform and joint information.
    struct Collider
    {
        //! Offset transform relative to the joint attached.
        AZ::Transform m_offsetTransform = AZ::Transform::CreateIdentity();

        //! Current transform in model space after animation applied.
        AZ::Transform m_currentModelSpaceTransform = AZ::Transform::CreateIdentity();

        //! Joint this collider is attached to.
        int m_jointIndex = InvalidIndex;
    };

    //! Describes the shape on an sphere collider.
    struct SphereCollider
        : public Collider
    {
        //! Radius of the sphere.
        float m_radius = 0.0f;

        int m_nvSphereIndex = InvalidIndex; //!< Identifies the sphere within m_spheres in ActorClothColliders.
    };

    //! Describes the shape on an sphere collider.
    struct CapsuleCollider
        : public Collider
    {
        //! Height of the capsule.
        float m_height = 0.0f;

        //! Radius of the capsule.
        float m_radius = 0.0f;

        int m_capsuleIndex = InvalidIndex; //!< Identifies first index of the capsule within m_capsuleIndices in ActorClothColliders.
        int m_sphereAIndex = InvalidIndex; //!< Identifies the first sphere within m_spheres in ActorClothColliders.
        int m_sphereBIndex = InvalidIndex; //!< Identifies the second sphere within m_spheres in ActorClothColliders.
    };

    //! Class to retrieve cloth colliders information from an actor on the same entity
    //! and updates their transform from skinning animation.
    //!
    //! @note There is a limit of 32 sphere colliders and 32 capsule colliders.
    //!       In the case that all capsules use unique spheres then the maximum
    //!       number of capsule would go down to 16, limited by the maximum number of spheres (32).
    class ActorClothColliders
    {
    public:
        AZ_TYPE_INFO(ActorClothColliders, "{EA2D9B6A-2493-4B6A-972E-BB639E16798E}");

        static AZStd::unique_ptr<ActorClothColliders> Create(AZ::EntityId entityId);

        explicit ActorClothColliders(AZ::EntityId entityId);

        //! Updates the colliders' transforms with the current pose of the actor.
        void Update();

        const AZStd::vector<SphereCollider>& GetSphereColliders() const;
        const AZStd::vector<CapsuleCollider>& GetCapsuleColliders() const;

        const AZStd::vector<AZ::Vector4>& GetSpheres() const;
        const AZStd::vector<uint32_t>& GetCapsuleIndices() const;

    private:
        void UpdateSphere(const SphereCollider& sphere);
        void UpdateCapsule(const CapsuleCollider& capsule);

        AZ::EntityId m_entityId;

        // Configuration data of spheres and capsules, describing their shape and transforms relative to joints.
        AZStd::vector<SphereCollider> m_sphereColliders;
        AZStd::vector<CapsuleCollider> m_capsuleColliders;

        // The current positions and radius of sphere colliders.
        // Every update, these positions are computed with the current pose of the actor.
        // Note: The spheres used to formed capsules are also part of this list.
        AZStd::vector<AZ::Vector4> m_spheres;

        // The sphere collider indices associated with capsules.
        // Each capsule is 2 indices within the list.
        AZStd::vector<uint32_t> m_capsuleIndices;
    };
} // namespace NvCloth
