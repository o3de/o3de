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

        static AZ::Outcome<Physics::ShapeConfigurationPair> CreateColliderByType(const AZ::TypeId& typeId);
        static AZ::Outcome<Physics::ShapeConfigurationPair> CreateColliderByType(const AZ::TypeId& typeId, AZStd::string& outResult);
        static void AutoSizeCollider(Physics::ShapeConfigurationPair& collider, const Actor* actor, const Node* node);

        static void Reflect(AZ::ReflectContext* context);

    private:
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        static const char* s_colliderConfigTypeVisualNames[5];

        Physics::AnimationConfiguration m_config;
    };
} // namespace EMotionFX
