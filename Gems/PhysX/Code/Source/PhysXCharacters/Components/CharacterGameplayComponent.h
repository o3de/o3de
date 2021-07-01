/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>
#include <PhysX/CharacterGameplayBus.h>

namespace PhysX
{
    //! Configuration for storing character gameplay settings.
    class CharacterGameplayConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterGameplayConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CharacterGameplayConfiguration, "{A9E559C7-9436-462A-8A5D-304ACFFC7F90}");
        static void Reflect(AZ::ReflectContext* context);

        float m_gravityMultiplier = 1.0f; //!< Multiplier to be combined with world gravity setting when applying gravity to character.
    };

    //! Character Gameplay Component.
    //! Gameplay behavior is likely to be highly game dependent. This component is provided as an example to work
    //! alongside the PhysX Character Controller Component to give more intuitive behavior out of the box, but keep
    //! things separate to make it easier for users to modify or replace the game specific logic.
    //! For example, the Character Gameplay Component demonstrates one approach to allow the character to be affected
    //! by gravity, which is not intrinsic behavior since the PhysX character controller is kinematic rather than
    //! dynamic.
    class CharacterGameplayComponent
        : public AZ::Component
        , public CharacterGameplayRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CharacterGameplayComponent, "{7F17120F-B9E7-4E50-AC6B-C84491DC7508}");

        static void Reflect(AZ::ReflectContext* context);

        CharacterGameplayComponent() = default;
        CharacterGameplayComponent(const CharacterGameplayConfiguration& config);
        ~CharacterGameplayComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // CharacterGameplayRequestBus
        bool IsOnGround() const override;
        float GetGravityMultiplier() const override;
        void SetGravityMultiplier(float gravityMultiplier) override;
        AZ::Vector3 GetFallingVelocity() const override;
        void SetFallingVelocity(const AZ::Vector3& fallingVelocity) override;

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        void OnPreSimulate(float deltaTime);
        void OnGravityChanged(const AZ::Vector3& gravity);
        void ApplyGravity(float deltaTime);

        float m_gravityMultiplier = 1.0f;
        AZ::Vector3 m_gravity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_fallingVelocity = AZ::Vector3::CreateZero();

        AzPhysics::SystemEvents::OnPresimulateEvent::Handler m_preSimulateHandler;
        AzPhysics::SceneEvents::OnSceneGravityChangedEvent::Handler m_onGravityChangedHandler;
    };

    //! Example implementation of controller-controller filtering callback.
    //! This example causes controllers to impede each other's movement based on their collision filters.
    bool CollisionLayerBasedControllerFilter(const physx::PxController& controllerA, const physx::PxController& controllerB);

    //! Example implementation of controller-object filtering callback.
    //! This example causes static and kinematic bodies to impede the character based on collision layers.
    physx::PxQueryHitType::Enum CollisionLayerBasedObjectPreFilter(const physx::PxFilterData& filterData,
        const physx::PxShape* shape, const physx::PxRigidActor* actor, [[maybe_unused]] physx::PxHitFlags& queryFlags);
} // namespace PhysX
