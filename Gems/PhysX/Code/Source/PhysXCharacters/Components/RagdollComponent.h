/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PhysXCharacters/API/Ragdoll.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/CharacterPhysicsDataBus.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    /// Component used to simulate a hierarchy of rigid bodies connected by joints, typically used for characters.
    class RagdollComponent
        : public AZ::Component
        , public AzFramework::RagdollPhysicsRequestBus::Handler
        , public AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public AzFramework::CharacterPhysicsDataNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(PhysX::RagdollComponent, "{B89498F8-4718-42FE-A457-A377DD0D61A0}");

        static void Reflect(AZ::ReflectContext* context);

        RagdollComponent() = default;
        ~RagdollComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
            provided.push_back(AZ_CRC_CE("PhysicsRagdollService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PhysicsRagdollService"));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
            // Incompatible with static/dynamic rigid bodies, but still compatible
            // with character controller (which also provides rigid body service).
            incompatible.push_back(AZ_CRC_CE("PhysicsStaticRigidBodyService"));
            incompatible.push_back(AZ_CRC_CE("PhysicsDynamicRigidBodyService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("CharacterPhysicsDataService"));
        }

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // RagdollPhysicsBus
        void EnableSimulation(const Physics::RagdollState& initialState) override;
        void EnableSimulationQueued(const Physics::RagdollState& initialState) override;
        void DisableSimulation() override;
        void DisableSimulationQueued() override;
        Physics::Ragdoll* GetRagdoll() override;
        void GetState(Physics::RagdollState& ragdollState) const override;
        void SetState(const Physics::RagdollState& ragdollState) override;
        void SetStateQueued(const Physics::RagdollState& ragdollState) override;
        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override;
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override;
        Physics::RagdollNode* GetNode(size_t nodeIndex) const override;

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // CharacterPhysicsDataNotificationBus
        void OnRagdollConfigurationReady(const Physics::RagdollConfiguration& ragdollConfiguration) override;

        // deprecated Cry functions
        void EnterRagdoll() override;
        void ExitRagdoll() override;

    private:
        void CreateRagdoll(const Physics::RagdollConfiguration& ragdollConfiguration);
        void DestroyRagdoll();
        Ragdoll* GetPhysXRagdoll();
        const Ragdoll* GetPhysXRagdollConst() const;

        bool IsJointProjectionVisible() const;
        bool IsMaxMassRatioVisible() const;

        AzPhysics::SimulatedBodyHandle m_ragdollHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
        /// Minimum number of position iterations to perform in the PhysX solver.
        /// Lower iteration counts are less expensive but may behave less realistically.
        AZ::u32 m_positionIterations = 16;
        /// Minimum number of velocity iterations to perform in the PhysX solver.
        AZ::u32 m_velocityIterations = 8;
        /// Whether to use joint projection to preserve joint constraints in demanding
        /// situations at the expense of potentially reducing physical correctness.
        bool m_enableJointProjection = true;
        /// Linear joint error above which projection will be applied.
        float m_jointProjectionLinearTolerance = 1e-3f;
        /// Angular joint error (in degrees) above which projection will be applied.
        float m_jointProjectionAngularToleranceDegrees = 1.0f;
        /// Allows ragdoll node mass values to be overridden to avoid unstable mass ratios.
        bool m_enableMassRatioClamping = false;
        /// If mass ratio clamping is enabled, masses will be clamped to within this ratio.
        float m_maxMassRatio = 2.0f;
    };
} // namespace PhysX
