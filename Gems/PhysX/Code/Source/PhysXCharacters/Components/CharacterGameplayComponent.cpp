/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXCharacters/Components/CharacterGameplayComponent.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    void CharacterGameplayConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterGameplayConfiguration>()
                ->Version(1)
                ->Field("GravityMultiplier", &CharacterGameplayConfiguration::m_gravityMultiplier)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CharacterGameplayConfiguration>(
                    "PhysX Character Gameplay Configuration", "PhysX Character Gameplay Configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterGameplayConfiguration::m_gravityMultiplier,
                        "Gravity Multiplier", "Multiplier for global gravity value that applies only to this character entity.")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ;
            }
        }
    }

    void CharacterGameplayComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXCharacterGameplayService", 0xfacd7876));
    }

    void CharacterGameplayComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXCharacterGameplayService", 0xfacd7876));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void CharacterGameplayComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PhysXCharacterControllerService", 0x428de4fa));
    }

    void CharacterGameplayComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    CharacterGameplayComponent::CharacterGameplayComponent(const CharacterGameplayConfiguration& config)
        : m_gravityMultiplier(config.m_gravityMultiplier)
    {
    }

    void CharacterGameplayComponent::Reflect(AZ::ReflectContext* context)
    {
        CharacterGameplayConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterGameplayComponent, AZ::Component>()
                ->Version(1)
                ->Field("GravityMultiplier", &CharacterGameplayComponent::m_gravityMultiplier)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<CharacterGameplayRequestBus>("CharacterGameplayRequestBus", "Character Gameplay")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                ->Event("IsOnGround", &CharacterGameplayRequests::IsOnGround, "Is On Ground")
                ->Event("GetGravityMultiplier", &CharacterGameplayRequests::GetGravityMultiplier, "Get Gravity Multiplier")
                ->Event("SetGravityMultiplier", &CharacterGameplayRequests::SetGravityMultiplier, "Set Gravity Multiplier")
                ->Event("GetFallingVelocity", &CharacterGameplayRequests::GetFallingVelocity, "Get Falling Velocity")
                ->Event("SetFallingVelocity", &CharacterGameplayRequests::SetFallingVelocity, "Set Falling Velocity")
                ;
        }
    }

    // CharacterGameplayRequestBus
    bool CharacterGameplayComponent::IsOnGround() const
    {
        Physics::Character* character = nullptr;
        Physics::CharacterRequestBus::EventResult(character, GetEntityId(), &Physics::CharacterRequests::GetCharacter);
        if (!character)
        {
            return true;
        }

        auto pxController = static_cast<physx::PxController*>(character->GetNativePointer());
        if (!pxController)
        {
            return true;
        }

        physx::PxControllerState state;
        pxController->getState(state);
        return
            state.touchedActor != nullptr ||
            (state.collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN) != 0;
    }

    float CharacterGameplayComponent::GetGravityMultiplier() const
    {
        return m_gravityMultiplier;
    }

    void CharacterGameplayComponent::SetGravityMultiplier(float gravityMultiplier)
    {
        m_gravityMultiplier = gravityMultiplier;
    }

    AZ::Vector3 CharacterGameplayComponent::GetFallingVelocity() const
    {
        return m_fallingVelocity;
    }

    void CharacterGameplayComponent::SetFallingVelocity(const AZ::Vector3& fallingVelocity)
    {
        m_fallingVelocity = fallingVelocity;
    }

    // AZ::Component
    void CharacterGameplayComponent::Init()
    {
        //setup AZ::events
        m_onGravityChangedHandler = AzPhysics::SceneEvents::OnSceneGravityChangedEvent::Handler(
            [this]([[maybe_unused]] AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& newGravity)
            {
                OnGravityChanged(newGravity);
            });

        m_preSimulateHandler = AzPhysics::SystemEvents::OnPresimulateEvent::Handler(
            [this](float deltaTime)
            {
                OnPreSimulate(deltaTime);
            }
        );
    }

    void CharacterGameplayComponent::Activate()
    {
        AzPhysics::SimulatedBody* worldBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(worldBody, GetEntityId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        if (worldBody)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                m_gravity = sceneInterface->GetGravity(worldBody->m_sceneOwner);
                sceneInterface->RegisterSceneGravityChangedEvent(worldBody->m_sceneOwner, m_onGravityChangedHandler);
            }
        }

        if (auto* physXSystem = GetPhysXSystem())
        {
            physXSystem->RegisterPreSimulateEvent(m_preSimulateHandler);
        }

        Physics::Character* character = nullptr;
        Physics::CharacterRequestBus::EventResult(character, GetEntityId(), &Physics::CharacterRequests::GetCharacter);
        if (character)
        {
            auto controller = static_cast<PhysX::CharacterController*>(character);
            controller->SetFilterFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER);
            if (auto callbackManager = controller->GetCallbackManager())
            {
                callbackManager->SetControllerFilter(CollisionLayerBasedControllerFilter);
                callbackManager->SetObjectPreFilter(CollisionLayerBasedObjectPreFilter);
            }
        }

        CharacterGameplayRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterGameplayComponent::Deactivate()
    {
        CharacterGameplayRequestBus::Handler::BusDisconnect();
        m_onGravityChangedHandler.Disconnect();
        m_preSimulateHandler.Disconnect();
    }

    // Physics::SystemEvent
    void CharacterGameplayComponent::OnPreSimulate(float deltaTime)
    {
        ApplyGravity(deltaTime);
    }

    void CharacterGameplayComponent::OnGravityChanged(const AZ::Vector3& gravity)
    {
        // project the falling velocity onto the new gravity direction
        const float gravityMagnitudeSquared = gravity.GetLengthSq();
        m_fallingVelocity = gravityMagnitudeSquared > AZ::Constants::FloatEpsilon
            ? m_fallingVelocity.Dot(gravity) * gravity / gravityMagnitudeSquared
            : AZ::Vector3::CreateZero();

        m_gravity = gravity;
    }

    // CharacterGameplayComponent
    void CharacterGameplayComponent::ApplyGravity(float deltaTime)
    {
        if (IsOnGround())
        {
            m_fallingVelocity = AZ::Vector3::CreateZero();
            return;
        }

        m_fallingVelocity += m_gravityMultiplier * m_gravity * deltaTime;
        Physics::CharacterRequestBus::Event(GetEntityId(), &Physics::CharacterRequests::AddVelocity, m_fallingVelocity);
    }

    bool CollisionLayerBasedControllerFilter(
        const physx::PxController& controllerA, const physx::PxController& controllerB)
    {
        PHYSX_SCENE_READ_LOCK(controllerA.getActor()->getScene());
        physx::PxRigidDynamic* actorA = controllerA.getActor();
        physx::PxRigidDynamic* actorB = controllerB.getActor();

        if (actorA && actorA->getNbShapes() > 0 && actorB && actorB->getNbShapes() > 0)
        {
            physx::PxShape* shapeA = nullptr;
            actorA->getShapes(&shapeA, 1, 0);
            physx::PxFilterData filterDataA = shapeA->getSimulationFilterData();
            physx::PxShape* shapeB = nullptr;
            actorB->getShapes(&shapeB, 1, 0);
            physx::PxFilterData filterDataB = shapeB->getSimulationFilterData();
            return PhysX::Utils::Collision::ShouldCollide(filterDataA, filterDataB);
        }

        return true;
    }

    physx::PxQueryHitType::Enum CollisionLayerBasedObjectPreFilter(
        const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor,
        [[maybe_unused]] physx::PxHitFlags& queryFlags)
    {
        // non-kinematic dynamic bodies should not impede the movement of the character
        if (actor->getConcreteType() == physx::PxConcreteType::eRIGID_DYNAMIC)
        {
            const physx::PxRigidDynamic* rigidDynamic = static_cast<const physx::PxRigidDynamic*>(actor);
            if (!(rigidDynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC))
            {
                return physx::PxQueryHitType::eNONE;
            }
        }

        // all other cases should be determined by collision filters
        if (PhysX::Utils::Collision::ShouldCollide(filterData, shape->getSimulationFilterData()))
        {
            return physx::PxQueryHitType::eBLOCK;
        }

        return physx::PxQueryHitType::eNONE;
    }
} // namespace PhysX
