/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;
    class Node;
    class Skeleton;

    class PhysicsSetup
    {
    public:
        AZ_RTTI(PhysicsSetup, "{4749DFCB-5CBE-434D-9551-34F4C0CCA428}")
        AZ_CLASS_ALLOCATOR_DECL

        PhysicsSetup() = default;
        virtual ~PhysicsSetup() = default;

        Physics::CharacterColliderConfiguration& GetHitDetectionConfig();
        Physics::RagdollConfiguration& GetRagdollConfig();
        const Physics::RagdollConfiguration& GetRagdollConfig() const;

        Physics::AnimationConfiguration& GetConfig();

        enum ColliderConfigType
        {
            HitDetection = 0,
            Ragdoll = 1,
            Cloth = 2,
            SimulatedObjectCollider = 3,
            Unknown = 4
        };
        static const char* GetStringForColliderConfigType(ColliderConfigType configType);
        static const char* GetVisualNameForColliderConfigType(ColliderConfigType configType);
        static ColliderConfigType GetColliderConfigTypeFromString(const AZStd::string& configTypeString);
        Physics::CharacterColliderConfiguration* GetColliderConfigByType(ColliderConfigType configType);

        const Node* FindRagdollParentNode(const Node* node) const;

        Physics::CharacterColliderConfiguration& GetClothConfig();
        const Physics::CharacterColliderConfiguration& GetClothConfig() const;
        Physics::CharacterColliderConfiguration& GetSimulatedObjectColliderConfig();
        const Physics::CharacterColliderConfiguration& GetSimulatedObjectColliderConfig() const;

        void LogRagdollConfig(Actor* actor, const char* title);

        void OptimizeForServer();

        static AZ::Outcome<AzPhysics::ShapeColliderPair> CreateColliderByType(const AZ::TypeId& typeId);
        static AZ::Outcome<AzPhysics::ShapeColliderPair> CreateColliderByType(const AZ::TypeId& typeId, AZStd::string& outResult);
        static void AutoSizeCollider(AzPhysics::ShapeColliderPair& collider, const Actor* actor, const Node* node);

        static void Reflect(AZ::ReflectContext* context);

    private:
        static const char* s_colliderConfigTypeVisualNames[5];

        Physics::AnimationConfiguration m_config;
    };

    //! Computes an estimate of the direction of the bone, based on a weighted average of the bone's children,
    //! or pointing away from the parent in the case where there are no children.
    AZ::Vector3 GetBoneDirection(const Skeleton* skeleton, const Node* node);
} // namespace EMotionFX
