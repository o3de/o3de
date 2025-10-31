/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <AzCore/Component/TransformBus.h>
#include <PhysX/CharacterControllerBus.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    class CharacterControllerConfiguration;

    /// Component used to physically represent characters for basic interactions with the physical world, for example to
    /// prevent walking through walls or falling through terrain.
    class CharacterControllerComponent
        : public AZ::Component
        , public AZ::EntityBus::Handler
        , public Physics::CharacterRequestBus::Handler
        , public AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public CharacterControllerRequestBus::Handler
        , public Physics::CollisionFilteringRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CharacterControllerComponent, "{BCBD8448-2FFC-450D-B82F-7C297D2F0C8C}");

        static void Reflect(AZ::ReflectContext* context);

        CharacterControllerComponent();
        CharacterControllerComponent(AZStd::unique_ptr<Physics::CharacterConfiguration> characterConfig,
            AZStd::shared_ptr<Physics::ShapeConfiguration> shapeConfig);
        ~CharacterControllerComponent();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
            // Character controller acts as dynamic kinematic rigid body,
            // so it also serves the rigid body service.
            provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
            provided.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
            incompatible.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
        }

        Physics::CharacterConfiguration& GetCharacterConfiguration()
        {
            return *m_characterConfig;
        }

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::EntityBus overrides ...
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        // Physics::CharacterRequestBus
        AZ::Vector3 GetBasePosition() const override;
        void SetBasePosition(const AZ::Vector3& position) override;
        AZ::Vector3 GetCenterPosition() const override;
        float GetStepHeight() const override;
        void SetStepHeight(float stepHeight) override;
        AZ::Vector3 GetUpDirection() const override;
        void SetUpDirection(const AZ::Vector3& upDirection) override;
        float GetSlopeLimitDegrees() const override;
        void SetSlopeLimitDegrees(float slopeLimitDegrees) override;
        float GetMaximumSpeed() const override;
        void SetMaximumSpeed(float maximumSpeed) override;
        AZ::Vector3 GetVelocity() const override;
        void AddVelocityForTick(const AZ::Vector3& velocity) override;
        void AddVelocityForPhysicsTimestep(const AZ::Vector3& velocity) override;
        bool IsPresent() const override { return IsPhysicsEnabled(); }
        Physics::Character* GetCharacter() override;

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // CharacterControllerRequestBus
        void Resize(float height) override;
        float GetHeight() override;
        void SetHeight(float height) override;
        float GetRadius() override;
        void SetRadius(float radius) override;
        float GetHalfSideExtent() override;
        void SetHalfSideExtent(float halfSideExtent) override;
        float GetHalfForwardExtent() override;
        void SetHalfForwardExtent(float halfForwardExtent) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // CollisionFilteringRequestBus
        void SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag) override;
        AZStd::string GetCollisionLayerName() override;
        void SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag) override;
        AZStd::string GetCollisionGroupName() override;
        void ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled) override;

    private:
        const PhysX::CharacterController* GetControllerConst() const;
        PhysX::CharacterController* GetController();
        // Creates the physics character controller in the current default physics scene.
        // This will do nothing if the controller is already created.
        void CreateController();
        // Removes the physics character controller from the scene and cleans up all
        // references and events used with the physics character controller.
        void DestroyController();

        void OnPostSimulate(float deltaTime);
        void OnSceneSimulationStart(float physicsTimestep);

        AZStd::unique_ptr<Physics::CharacterConfiguration> m_characterConfig;
        AZStd::shared_ptr<Physics::ShapeConfiguration> m_shapeConfig;
        AzPhysics::SimulatedBodyHandle m_controllerBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SystemEvents::OnPostsimulateEvent::Handler m_postSimulateHandler;
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler m_sceneSimulationStartHandler;
        AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler m_onSimulatedBodyRemovedHandler;
    };
} // namespace PhysX
