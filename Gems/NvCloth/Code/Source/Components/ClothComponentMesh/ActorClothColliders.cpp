/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Integration/ActorComponentBus.h>

#include <Components/ClothComponentMesh/ActorClothColliders.h>

namespace NvCloth
{
    namespace Internal
    {
        extern const size_t NvClothMaxNumSphereColliders = 32;
        extern const size_t NvClothMaxNumCapsuleColliders = 32;

        SphereCollider CreateSphereCollider(
            const Physics::ColliderConfiguration* colliderConfig,
            const Physics::SphereShapeConfiguration* sphereShapeConfig,
            int jointIndex, int sphereIndex)
        {
            SphereCollider sphereCollider;

            sphereCollider.m_jointIndex = jointIndex;
            sphereCollider.m_offsetTransform =
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    colliderConfig->m_rotation,
                    colliderConfig->m_position);
            sphereCollider.m_radius = sphereShapeConfig->m_radius;
            sphereCollider.m_nvSphereIndex = sphereIndex;

            return sphereCollider;
        }

        CapsuleCollider CreateCapsuleCollider(
            const Physics::ColliderConfiguration* colliderConfig,
            const Physics::CapsuleShapeConfiguration* capsuleShapeConfig,
            int jointIndex, int capsuleIndex, int sphereAIndex, int sphereBIndex)
        {
            CapsuleCollider capsuleCollider;

            capsuleCollider.m_jointIndex = jointIndex;
            capsuleCollider.m_offsetTransform =
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    colliderConfig->m_rotation,
                    colliderConfig->m_position);
            capsuleCollider.m_radius = capsuleShapeConfig->m_radius;
            capsuleCollider.m_height = capsuleShapeConfig->m_height;
            capsuleCollider.m_capsuleIndex = capsuleIndex;
            capsuleCollider.m_sphereAIndex = sphereAIndex;
            capsuleCollider.m_sphereBIndex = sphereBIndex;

            return capsuleCollider;
        }
    }

    AZStd::unique_ptr<ActorClothColliders> ActorClothColliders::Create(AZ::EntityId entityId)
    {
        Physics::AnimationConfiguration* actorPhysicsConfig = nullptr;
        EMotionFX::Integration::ActorComponentRequestBus::EventResult(
            actorPhysicsConfig, entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetPhysicsConfig);
        if (!actorPhysicsConfig)
        {
            return nullptr;
        }

        const Physics::CharacterColliderConfiguration& clothConfig = actorPhysicsConfig->m_clothConfig;

        // Maximum number of spheres and capsules is imposed by NvCloth library
        size_t sphereCount = 0;
        size_t capsuleCount = 0;
        bool maxSphereCountReachedWarned = false;
        bool maxCapsuleCountReachedWarned = false;

        AZStd::vector<SphereCollider> sphereColliders;
        AZStd::vector<CapsuleCollider> capsuleColliders;

        for (const Physics::CharacterColliderNodeConfiguration& clothNodeConfig : clothConfig.m_nodes)
        {
            size_t jointIndex = EMotionFX::Integration::ActorComponentRequests::s_invalidJointIndex;
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                jointIndex, entityId, 
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetJointIndexByName, 
                clothNodeConfig.m_name.c_str());
            if (jointIndex == EMotionFX::Integration::ActorComponentRequests::s_invalidJointIndex)
            {
                AZ_Warning("ActorAssetHelper", false, "Joint '%s' not found", clothNodeConfig.m_name.c_str());
                continue;
            }

            for (const AzPhysics::ShapeColliderPair& shapeConfigPair : clothNodeConfig.m_shapes)
            {
                const auto& colliderConfig = shapeConfigPair.first;

                switch (shapeConfigPair.second->GetShapeType())
                {
                case Physics::ShapeType::Sphere:
                {
                    if (sphereCount >= Internal::NvClothMaxNumSphereColliders)
                    {
                        AZ_Warning("ActorAssetHelper", maxSphereCountReachedWarned,
                            "Maximum number of cloth sphere colliders (%zu) reached",
                            Internal::NvClothMaxNumSphereColliders);
                        maxSphereCountReachedWarned = true;
                        continue;
                    }

                    SphereCollider sphereCollider = Internal::CreateSphereCollider(
                        colliderConfig.get(),
                        static_cast<const Physics::SphereShapeConfiguration*>(shapeConfigPair.second.get()),
                        static_cast<int>(jointIndex),
                        static_cast<int>(sphereCount));

                    sphereColliders.push_back(sphereCollider);
                    ++sphereCount;
                }
                break;

                case Physics::ShapeType::Capsule:
                {
                    if (capsuleCount >= Internal::NvClothMaxNumCapsuleColliders)
                    {
                        AZ_Warning("ActorAssetHelper", maxCapsuleCountReachedWarned,
                            "Maximum number of cloth capsule colliders (%zu) reached",
                            Internal::NvClothMaxNumCapsuleColliders);
                        maxCapsuleCountReachedWarned = true;
                        continue;
                    }

                    // If there is only 1 sphere left to reach the maximum number
                    // of spheres the capsule won't fit as each capsule is formed of 2 spheres.
                    if (sphereCount >= Internal::NvClothMaxNumSphereColliders - 1)
                    {
                        AZ_Warning("ActorAssetHelper", maxCapsuleCountReachedWarned,
                            "Maximum number of cloth capsule colliders reached");
                        maxCapsuleCountReachedWarned = true;
                        continue;
                    }

                    CapsuleCollider capsuleCollider = Internal::CreateCapsuleCollider(
                        colliderConfig.get(),
                        static_cast<const Physics::CapsuleShapeConfiguration*>(shapeConfigPair.second.get()),
                        static_cast<int>(jointIndex),
                        static_cast<int>(capsuleCount * 2), // Each capsule holds 2 sphere indices
                        static_cast<int>(sphereCount + 0),  // First sphere index
                        static_cast<int>(sphereCount + 1)); // Second sphere index

                    capsuleColliders.push_back(capsuleCollider);
                    ++capsuleCount;
                    sphereCount += 2; // Adds 2 spheres per capsule
                }
                break;

                default:
                    AZ_Warning("ActorAssetHelper", false, "Joint '%s' has an unexpected shape type (%u) for cloth collider.",
                        clothNodeConfig.m_name.c_str(), static_cast<AZ::u8>(shapeConfigPair.second->GetShapeType()));
                    break;
                }
            }
        }

        if (sphereCount == 0 && capsuleCount == 0)
        {
            return nullptr;
        }

        AZStd::unique_ptr<ActorClothColliders> actorClothColliders = AZStd::make_unique<ActorClothColliders>(entityId);

        actorClothColliders->m_sphereColliders = AZStd::move(sphereColliders);
        actorClothColliders->m_capsuleColliders = AZStd::move(capsuleColliders);
        actorClothColliders->m_spheres.resize(sphereCount);
        actorClothColliders->m_capsuleIndices.resize(capsuleCount * 2); // 2 sphere indices per capsule

        for (const auto& capsuleCollider : actorClothColliders->m_capsuleColliders)
        {
            actorClothColliders->m_capsuleIndices[capsuleCollider.m_capsuleIndex + 0] = capsuleCollider.m_sphereAIndex;
            actorClothColliders->m_capsuleIndices[capsuleCollider.m_capsuleIndex + 1] = capsuleCollider.m_sphereBIndex;
        }

        // Calculates the current transforms for the colliders
        // and fills the data as nvcloth needs them, ready to be
        // queried by the cloth component.
        actorClothColliders->Update();

        return actorClothColliders;
    }

    ActorClothColliders::ActorClothColliders(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
    }

    void ActorClothColliders::Update()
    {
        for (auto& sphereCollider : m_sphereColliders)
        {
            AZ::Transform jointModelSpaceTransform = AZ::Transform::Identity();
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                jointModelSpaceTransform, m_entityId,
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetJointTransform,
                static_cast<size_t>(sphereCollider.m_jointIndex), EMotionFX::Integration::Space::ModelSpace);

            sphereCollider.m_currentModelSpaceTransform = jointModelSpaceTransform * sphereCollider.m_offsetTransform;
            UpdateSphere(sphereCollider);
        }

        for (auto& capsuleCollider : m_capsuleColliders)
        {
            AZ::Transform jointModelSpaceTransform = AZ::Transform::Identity();
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(
                jointModelSpaceTransform, m_entityId,
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetJointTransform,
                static_cast<size_t>(capsuleCollider.m_jointIndex), EMotionFX::Integration::Space::ModelSpace);

            capsuleCollider.m_currentModelSpaceTransform = jointModelSpaceTransform * capsuleCollider.m_offsetTransform;
            UpdateCapsule(capsuleCollider);
        }
    }

    void ActorClothColliders::UpdateSphere(const SphereCollider& sphere)
    {
        const AZ::Vector3 spherePosition = sphere.m_currentModelSpaceTransform.GetTranslation();
        AZ_Assert(sphere.m_nvSphereIndex != InvalidIndex, "Sphere collider has invalid index");
        m_spheres[sphere.m_nvSphereIndex].Set(spherePosition, sphere.m_radius);
    }

    void ActorClothColliders::UpdateCapsule(const CapsuleCollider& capsule)
    {
        const float halfHeightExclusive = 0.5f * capsule.m_height - capsule.m_radius;
        const AZ::Vector3 basisZ = capsule.m_currentModelSpaceTransform.GetBasisZ() * halfHeightExclusive;
        const AZ::Vector3 capsulePosition = capsule.m_currentModelSpaceTransform.GetTranslation();

        const AZ::Vector3 sphereAPosition = capsulePosition + basisZ;
        const AZ::Vector3 sphereBPosition = capsulePosition - basisZ;
        AZ_Assert(capsule.m_sphereAIndex != InvalidIndex, "Capsule collider has an invalid index for its first sphere");
        AZ_Assert(capsule.m_sphereBIndex != InvalidIndex, "Capsule collider has an invalid index for its second sphere");
        m_spheres[capsule.m_sphereAIndex].Set(sphereAPosition, capsule.m_radius);
        m_spheres[capsule.m_sphereBIndex].Set(sphereBPosition, capsule.m_radius);
    }

    const AZStd::vector<SphereCollider>& ActorClothColliders::GetSphereColliders() const
    {
        return m_sphereColliders;
    }

    const AZStd::vector<CapsuleCollider>& ActorClothColliders::GetCapsuleColliders() const
    {
        return m_capsuleColliders;
    }

    const AZStd::vector<AZ::Vector4>& ActorClothColliders::GetSpheres() const
    {
        return m_spheres;
    }

    const AZStd::vector<uint32_t>& ActorClothColliders::GetCapsuleIndices() const
    {
        return m_capsuleIndices;
    }
} // namespace NvCloth
