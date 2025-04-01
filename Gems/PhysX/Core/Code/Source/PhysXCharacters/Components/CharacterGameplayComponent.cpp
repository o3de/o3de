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
                ->Field("GroundDetectionBoxHeight", &CharacterGameplayConfiguration::m_groundDetectionBoxHeight)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CharacterGameplayConfiguration>(
                    "PhysX Character Gameplay Configuration", "PhysX Character Gameplay Configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterGameplayConfiguration::m_gravityMultiplier,
                        "Gravity Multiplier", "Multiplier for global gravity value that applies only to this character entity.")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &CharacterGameplayConfiguration::m_groundDetectionBoxHeight,
                        "Ground Detection Box Height",
                        "Vertical size of box centered on the character's foot position used when testing for ground contact.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                    ;
            }
        }
    }

    void CharacterGameplayComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsCharacterGameplayService"));
    }

    void CharacterGameplayComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsCharacterGameplayService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void CharacterGameplayComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsCharacterControllerService"));
    }

    void CharacterGameplayComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    CharacterGameplayComponent::CharacterGameplayComponent(const CharacterGameplayConfiguration& config)
        : m_gravityMultiplier(config.m_gravityMultiplier)
        , m_groundDetectionBoxHeight(config.m_groundDetectionBoxHeight)
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
                ->Field("GroundDetectionBoxHeight", &CharacterGameplayComponent::m_groundDetectionBoxHeight)
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
                ->Event(
                    "GetGroundDetectionBoxHeight",
                    &CharacterGameplayRequests::GetGroundDetectionBoxHeight,
                    "Get Ground Detection Box Height")
                ->Event(
                    "SetGroundDetectionBoxHeight",
                    &CharacterGameplayRequests::SetGroundDetectionBoxHeight,
                    "Set Ground Detection Box Height")
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

        // use an overlap query to see if there's any geometry immediately below the character's foot position
        if (auto* scene = character->GetScene())
        {
            // use a box shape for the overlap, even if the character geometry is a capsule, to avoid difficulties with
            // the curved base of the capsule
            AZ::Vector3 footBoxDimensions(0.0f, 0.0f, m_groundDetectionBoxHeight);
            if (pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
            {
                const float radius = static_cast<physx::PxCapsuleController*>(pxController)->getRadius();
                footBoxDimensions = AZ::Vector3(2.0f * radius, 2.0f * radius, m_groundDetectionBoxHeight);
            }
            else if (pxController->getType() == physx::PxControllerShapeType::eBOX)
            {
                const auto* boxController = static_cast<physx::PxBoxController*>(pxController);
                footBoxDimensions = AZ::Vector3(
                    2.0f * boxController->getHalfSideExtent(), 2.0f * boxController->getHalfForwardExtent(), m_groundDetectionBoxHeight);
            }

            AZ::Transform footBoxTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), character->GetUpDirection()), character->GetBasePosition());
            AzPhysics::OverlapRequest overlapRequest =
                AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(footBoxDimensions, footBoxTransform);
            overlapRequest.m_collisionGroup = character->GetCollisionGroup();
            overlapRequest.m_maxResults = 2;
            AZ::EntityId entityId = GetEntityId();
            AzPhysics::SceneQueryHits sceneQueryHits = scene->QueryScene(&overlapRequest);
            return AZStd::any_of(
                sceneQueryHits.m_hits.begin(),
                sceneQueryHits.m_hits.end(),
                [&entityId](const AzPhysics::SceneQueryHit& hit)
                {
                    return hit.m_entityId != entityId;
                });
        }

        return true;
    }

    float CharacterGameplayComponent::GetGravityMultiplier() const
    {
        return m_gravityMultiplier;
    }

    void CharacterGameplayComponent::SetGravityMultiplier(float gravityMultiplier)
    {
        m_gravityMultiplier = gravityMultiplier;
    }

    float CharacterGameplayComponent::GetGroundDetectionBoxHeight() const
    {
        return m_groundDetectionBoxHeight;
    }

    void CharacterGameplayComponent::SetGroundDetectionBoxHeight(float groundDetectionBoxHeight)
    {
        m_groundDetectionBoxHeight = AZ::GetMax(0.0f, groundDetectionBoxHeight);
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

        m_sceneSimulationStartHandler = AzPhysics::SceneEvents::OnSceneSimulationStartHandler(
            [this](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                float fixedDeltaTime)
            {
                OnSceneSimulationStart(fixedDeltaTime);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation));
    }

    void CharacterGameplayComponent::Activate()
    {
        AzPhysics::SceneHandle attachedSceneHandle = AzPhysics::InvalidSceneHandle;
        Physics::DefaultWorldBus::BroadcastResult(attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        if (attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to retrieve default scene.");
            return;
        }

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_gravity = sceneInterface->GetGravity(attachedSceneHandle);
            sceneInterface->RegisterSceneGravityChangedEvent(attachedSceneHandle, m_onGravityChangedHandler);
            sceneInterface->RegisterSceneSimulationStartHandler(attachedSceneHandle, m_sceneSimulationStartHandler);
        }

        CharacterGameplayRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterGameplayComponent::Deactivate()
    {
        CharacterGameplayRequestBus::Handler::BusDisconnect();
        m_onGravityChangedHandler.Disconnect();
        m_sceneSimulationStartHandler.Disconnect();
    }

    // Physics::SystemEvent
    void CharacterGameplayComponent::OnSceneSimulationStart(float physicsTimestep)
    {
        ApplyGravity(physicsTimestep);
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
        Physics::CharacterRequestBus::Event(GetEntityId(), &Physics::CharacterRequests::AddVelocityForPhysicsTimestep, m_fallingVelocity);
    }
} // namespace PhysX
