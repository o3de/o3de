/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetworkCharacterComponent.h>
#include <Multiplayer/Components/NetworkRigidBodyComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/Character.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Utils.h>

namespace Multiplayer
{

    bool CollisionLayerBasedControllerFilter(const physx::PxController& controllerA, const physx::PxController& controllerB)
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
        const physx::PxFilterData& filterData,
        const physx::PxShape* shape,
        const physx::PxRigidActor* actor,
        [[maybe_unused]] physx::PxHitFlags& queryFlags)
    {
        // non-kinematic dynamic bodies should not impede the movement of the character
        if (actor->getConcreteType() == physx::PxConcreteType::eRIGID_DYNAMIC)
        {
            const physx::PxRigidDynamic* rigidDynamic = static_cast<const physx::PxRigidDynamic*>(actor);

            bool isKinematic = (rigidDynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC);
            if (isKinematic)
            {
                const PhysX::ActorData* actorData = PhysX::Utils::GetUserData(rigidDynamic);
                if (actorData)
                {
                    const AZ::EntityId entityId = actorData->GetEntityId();

                    if (Multiplayer::NetworkRigidBodyRequestBus::FindFirstHandler(entityId) != nullptr)
                    {
                        // Network rigid bodies are kinematic on the client but dynamic on the server,
                        // hence filtering treats these actors as dynamic to support client prediction and avoid desyncs
                        isKinematic = false;
                    }
                }
            }

            if (!isKinematic)
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

    void NetworkCharacterComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkCharacterComponent, NetworkCharacterComponentBase>()
                ->Version(1);
        }
        NetworkCharacterComponentBase::Reflect(context);
        NetworkCharacterComponentController::Reflect(context);
    }

    void NetworkCharacterComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        NetworkCharacterComponentBase::GetRequiredServices(required);
        required.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
    }

    NetworkCharacterComponent::NetworkCharacterComponent()
        : m_translationEventHandler([this](const AZ::Vector3& translation) { OnTranslationChangedEvent(translation); })
    {
    }


    void NetworkCharacterComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        // During activation the character controller is not created yet.
        // Connect to CharacterNotificationBus to listen when it's activated after creation.
        Physics::CharacterNotificationBus::Handler::BusConnect(GetEntityId());

        // Set up the network event handlers. These can be bound before the character controller is created
        // because they check for the character controller to exist inside of the handlers.

        GetNetBindComponent()->AddEntitySyncRewindEventHandler(m_syncRewindHandler);

        if (!HasController())
        {
            GetNetworkTransformComponent()->TranslationAddEvent(m_translationEventHandler);
        }
    }

    void NetworkCharacterComponent::OnCharacterActivated(const AZ::EntityId& entityId)
    {
        Physics::CharacterRequests* characterRequests = Physics::CharacterRequestBus::FindFirstHandler(entityId);
        AZ_Assert(characterRequests, "Character Controller component is required on entity %s", GetEntity()->GetName().c_str());

        m_physicsCharacter = characterRequests->GetCharacter();

        if (m_physicsCharacter)
        {
            auto controller = static_cast<PhysX::CharacterController*>(m_physicsCharacter);
            controller->SetFilterFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER);
            if (auto callbackManager = controller->GetCallbackManager())
            {
                callbackManager->SetControllerFilter(CollisionLayerBasedControllerFilter);
                callbackManager->SetObjectPreFilter(CollisionLayerBasedObjectPreFilter);
            }
        }
    }

    void NetworkCharacterComponent::OnCharacterDeactivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        m_physicsCharacter = nullptr;
    }

    void NetworkCharacterComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        Physics::CharacterNotificationBus::Handler::BusDisconnect();

        m_syncRewindHandler.Disconnect();
        m_translationEventHandler.Disconnect();

        m_physicsCharacter = nullptr;
    }

    void NetworkCharacterComponent::OnTranslationChangedEvent([[maybe_unused]] const AZ::Vector3& translation)
    {
        OnSyncRewind();
    }

    void NetworkCharacterComponent::OnSyncRewind()
    {
        if (m_physicsCharacter == nullptr)
        {
            return;
        }

        const AZ::Vector3 currPosition = m_physicsCharacter->GetBasePosition();
        if (!currPosition.IsClose(GetNetworkTransformComponent()->GetTranslation()))
        {
            uint32_t frameId = static_cast<uint32_t>(Multiplayer::GetNetworkTime()->GetHostFrameId());
            m_physicsCharacter->SetFrameId(frameId);
            //m_physicsCharacter->SetBasePosition(GetNetworkTransformComponent()->GetTranslation());
        }
    }

    void NetworkCharacterComponentController::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<NetworkCharacterRequestBus>("NetworkCharacterRequestBus")
                ->Event("TryMoveWithVelocity", &NetworkCharacterRequestBus::Events::TryMoveWithVelocity, {{ { "Velocity" }, { "DeltaTime" } }});

            behaviorContext->Class<NetworkCharacterComponentController>("NetworkCharacterComponentController")
                ->RequestBus("NetworkCharacterRequestBus");
        }
    }

    NetworkCharacterComponentController::NetworkCharacterComponentController(NetworkCharacterComponent& parent)
        : NetworkCharacterComponentControllerBase(parent)
    {
        ;
    }

    void NetworkCharacterComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        NetworkCharacterRequestBus::Handler::BusConnect(GetEntity()->GetId());
    }

    void NetworkCharacterComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        NetworkCharacterRequestBus::Handler::BusDisconnect(GetEntity()->GetId());
    }

    AZ::Vector3 NetworkCharacterComponentController::TryMoveWithVelocity(const AZ::Vector3& velocity, [[maybe_unused]] float deltaTime)
    {
        // Ensure any entities that we might interact with are properly synchronized to their rewind state
        if (IsNetEntityRoleAuthority())
        {
            const AZ::Aabb entityStartBounds = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->GetEntityWorldBoundsUnion(GetEntity()->GetId());
            const AZ::Aabb entityFinalBounds = entityStartBounds.GetTranslated(velocity);
            AZ::Aabb entitySweptBounds = entityStartBounds;
            entitySweptBounds.AddAabb(entityFinalBounds);
            Multiplayer::GetNetworkTime()->SyncEntitiesToRewindState(entitySweptBounds);
        }

        if (GetParent().m_physicsCharacter != nullptr)
        {
            GetParent().m_physicsCharacter->SetRotation(GetEntity()->GetTransform()->GetWorldRotationQuaternion());

            if (velocity.GetLengthSq() > 0.0f)
            {
                GetParent().m_physicsCharacter->Move(velocity * deltaTime, deltaTime);
                GetEntity()->GetTransform()->SetWorldTM(GetParent().m_physicsCharacter->GetTransform());
                AZLOG(
                    NET_Movement,
                    "Moved to position %f x %f x %f",
                    GetParent().m_physicsCharacter->GetBasePosition().GetX(),
                    GetParent().m_physicsCharacter->GetBasePosition().GetY(),
                    GetParent().m_physicsCharacter->GetBasePosition().GetZ());
            }
        }

        return GetEntity()->GetTransform()->GetWorldTranslation();
    }
}
