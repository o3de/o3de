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

#include <PhysX_precompiled.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>
#include <PhysX/ColliderComponentBus.h>
#include <System/PhysXSystem.h>
#include <AzFramework/Physics/Utils.h>
#include <PhysXCharacters/API/CharacterUtils.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

namespace PhysX
{
    void CharacterControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        CharacterControllerConfiguration::Reflect(context);
        CharacterController::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterControllerComponent, AZ::Component>()
                ->Version(1)
                ->Field("CharacterConfig", &CharacterControllerComponent::m_characterConfig)
                ->Field("ShapeConfig", &CharacterControllerComponent::m_shapeConfig)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<Physics::CharacterRequestBus>("CharacterControllerRequestBus", "Character Controller")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                ->Event("GetBasePosition", &Physics::CharacterRequests::GetBasePosition, "Get Base Position")
                ->Event("SetBasePosition", &Physics::CharacterRequests::SetBasePosition, "Set Base Position")
                ->Event("GetCenterPosition", &Physics::CharacterRequests::GetCenterPosition, "Get Center Position")
                ->Event("GetStepHeight", &Physics::CharacterRequests::GetStepHeight, "Get Step Height")
                ->Event("SetStepHeight", &Physics::CharacterRequests::SetStepHeight, "Set Step Height")
                ->Event("GetUpDirection", &Physics::CharacterRequests::GetUpDirection, "Get Up Direction")
                ->Event("GetSlopeLimitDegrees", &Physics::CharacterRequests::GetSlopeLimitDegrees, "Get Slope Limit (Degrees)")
                ->Event("SetSlopeLimitDegrees", &Physics::CharacterRequests::SetSlopeLimitDegrees, "Set Slope Limit (Degrees)")
                ->Event("GetMaximumSpeed", &Physics::CharacterRequests::GetMaximumSpeed, "Get Maximum Speed")
                ->Event("SetMaximumSpeed", &Physics::CharacterRequests::SetMaximumSpeed, "Set Maximum Speed")
                ->Event("GetVelocity", &Physics::CharacterRequests::GetVelocity, "Get Velocity")
                ->Event("AddVelocity", &Physics::CharacterRequests::AddVelocity, "Add Velocity")
                ;

            behaviorContext->EBus<CharacterControllerRequestBus>("PhysXCharacterControllerRequestBus",
                "Character Controller (PhysX specific)")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                ->Event("Resize", &CharacterControllerRequests::Resize)
                ->Event("GetHeight", &CharacterControllerRequests::GetHeight, "Get Height")
                ->Event("SetHeight", &CharacterControllerRequests::SetHeight, "Set Height")
                ->Event("GetRadius", &CharacterControllerRequests::GetRadius, "Get Radius")
                ->Event("SetRadius", &CharacterControllerRequests::SetRadius, "Set Radius")
                ->Event("GetHalfSideExtent", &CharacterControllerRequests::GetHalfSideExtent, "Get Half Side Extent")
                ->Event("SetHalfSideExtent", &CharacterControllerRequests::SetHalfSideExtent, "Set Half Side Extent")
                ->Event("GetHalfForwardExtent", &CharacterControllerRequests::GetHalfForwardExtent, "Get Half Forward Extent")
                ->Event("SetHalfForwardExtent", &CharacterControllerRequests::SetHalfForwardExtent, "Set Half Forward Extent")
                ;
        }
    }

    CharacterControllerComponent::CharacterControllerComponent() = default;

    CharacterControllerComponent::CharacterControllerComponent(AZStd::unique_ptr<Physics::CharacterConfiguration> characterConfig,
        AZStd::unique_ptr<Physics::ShapeConfiguration> shapeConfig)
        : m_characterConfig(AZStd::move(characterConfig))
        , m_shapeConfig(AZStd::move(shapeConfig))
    {
    }

    CharacterControllerComponent::~CharacterControllerComponent() = default;

    // AZ::Component
    void CharacterControllerComponent::Init()
    {
    }

    void CharacterControllerComponent::Activate()
    {
        EnablePhysics();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Physics::CharacterRequestBus::Handler::BusConnect(GetEntityId());
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(GetEntityId());
        Physics::WorldBodyRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterControllerComponent::Deactivate()
    {
        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        Physics::WorldBodyRequestBus::Handler::BusDisconnect();
        m_preSimulateHandler.Disconnect();
        CharacterControllerRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        Physics::CharacterRequestBus::Handler::BusDisconnect();
        Physics::Utils::DeferDelete(AZStd::move(m_controller));
    }

    AZ::Vector3 CharacterControllerComponent::GetBasePosition() const
    {
        return IsPhysicsEnabled() ? m_controller->GetBasePosition() : AZ::Vector3::CreateZero();
    }

    void CharacterControllerComponent::SetBasePosition(const AZ::Vector3& position)
    {
        if (IsPhysicsEnabled())
        {
            m_controller->SetBasePosition(position);
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, position);
        }
    }

    AZ::Vector3 CharacterControllerComponent::GetCenterPosition() const
    {
        return IsPhysicsEnabled() ? m_controller->GetCenterPosition() : AZ::Vector3::CreateZero();
    }

    float CharacterControllerComponent::GetStepHeight() const
    {
        return IsPhysicsEnabled() ? m_controller->GetStepHeight() : 0.0f;
    }

    void CharacterControllerComponent::SetStepHeight(float stepHeight)
    {
        if (IsPhysicsEnabled())
        {
            m_controller->SetStepHeight(stepHeight);
        }
    }

    AZ::Vector3 CharacterControllerComponent::GetUpDirection() const
    {
        return IsPhysicsEnabled() ? m_controller->GetUpDirection() : AZ::Vector3::CreateZero();
    }

    void CharacterControllerComponent::SetUpDirection([[maybe_unused]] const AZ::Vector3& upDirection)
    {
        AZ_Warning("PhysX Character Controller Component", false, "Setting up direction is not currently supported.");
    }

    float CharacterControllerComponent::GetSlopeLimitDegrees() const
    {
        return IsPhysicsEnabled() ? m_controller->GetSlopeLimitDegrees() : 0.0f;
    }

    void CharacterControllerComponent::SetSlopeLimitDegrees(float slopeLimitDegrees)
    {
        if (IsPhysicsEnabled())
        {
            m_controller->SetSlopeLimitDegrees(slopeLimitDegrees);
        }
    }

    float CharacterControllerComponent::GetMaximumSpeed() const
    {
        if (IsPhysicsEnabled())
        {
            return m_controller->GetMaximumSpeed();
        }

        return 0.0f;
    }

    void CharacterControllerComponent::SetMaximumSpeed(float maximumSpeed)
    {
        if (IsPhysicsEnabled())
        {
            m_controller->SetMaximumSpeed(maximumSpeed);
        }
    }

    AZ::Vector3 CharacterControllerComponent::GetVelocity() const
    {
        return IsPhysicsEnabled() ? m_controller->GetVelocity() : AZ::Vector3::CreateZero();
    }

    void CharacterControllerComponent::AddVelocity(const AZ::Vector3& velocity)
    {
        if (IsPhysicsEnabled())
        {
            m_controller->AddVelocity(velocity);
        }
    }

    Physics::Character* CharacterControllerComponent::GetCharacter()
    {
        return m_controller.get();
    }

    void CharacterControllerComponent::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }

        bool success = CreateController();
        if (success)
        {
            Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsEnabled);
        }
    }

    void CharacterControllerComponent::DisablePhysics()
    {
        if(IsPhysicsEnabled())
        {
            m_controller.reset();

            // do not disconnect from the CharacterRequestBus or the IsPresent function used to determine
            // if the CharacterControllerComponent is on this entity will not work

            if (CharacterControllerRequestBus::Handler::BusIsConnected())
            {
                CharacterControllerRequestBus::Handler::BusDisconnect();
            }

            Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsDisabled);
        }
    }

    bool CharacterControllerComponent::IsPhysicsEnabled() const
    {
        return m_controller && m_controller->GetWorld();
    }

    AZ::Aabb CharacterControllerComponent::GetAabb() const
    {
        if (m_controller)
        {
            return m_controller->GetAabb();
        }
        return AZ::Aabb::CreateNull();
    }

    Physics::WorldBody* CharacterControllerComponent::GetWorldBody()
    {
        return m_controller.get();
    }

    Physics::RayCastHit CharacterControllerComponent::RayCast(const Physics::RayCastRequest& request)
    {
        if (m_controller)
        {
            return m_controller->RayCast(request);
        }
        return Physics::RayCastHit();
    }

    // CharacterControllerRequestBus
    void CharacterControllerComponent::Resize(float height)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->Resize(height);
        }
    }

    float CharacterControllerComponent::GetHeight()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetHeight();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetHeight(float height)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetHeight(height);
        }
    }

    float CharacterControllerComponent::GetRadius()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetRadius();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetRadius(float radius)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetRadius(radius);
        }
    }

    float CharacterControllerComponent::GetHalfSideExtent()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetHalfSideExtent();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetHalfSideExtent(float halfSideExtent)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetHalfSideExtent(halfSideExtent);
        }
    }

    float CharacterControllerComponent::GetHalfForwardExtent()
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->GetHalfForwardExtent();
        }

        return 0.0f;
    }

    void CharacterControllerComponent::SetHalfForwardExtent(float halfForwardExtent)
    {
        if (auto characterController = static_cast<CharacterController*>(m_controller.get()))
        {
            return characterController->SetHalfForwardExtent(halfForwardExtent);
        }
    }

    // TransformNotificationBus
    void CharacterControllerComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }

        m_controller->SetBasePosition(world.GetTranslation());
    }
    
    void CharacterControllerComponent::SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag)
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }

        if (Physics::Utils::FilterTag(m_controller->GetColliderTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionLayer collisionLayer;
            Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, collisionLayer);
            if (success) 
            {
                m_controller->SetCollisionLayer(collisionLayer);
            }
        }
    }

    AZStd::string CharacterControllerComponent::GetCollisionLayerName()
    {
        AZStd::string layerName;
        if (!IsPhysicsEnabled())
        {
            return layerName;
        }

        Physics::CollisionRequestBus::BroadcastResult(layerName, &Physics::CollisionRequests::GetCollisionLayerName, m_controller->GetCollisionLayer());
        return layerName;
    }

    void CharacterControllerComponent::SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag)
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }

        if (Physics::Utils::FilterTag(m_controller->GetColliderTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionGroup collisionGroup;
            Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionGroupByName, groupName, collisionGroup);
            if (success)
            {
                m_controller->SetCollisionGroup(collisionGroup);
            }
        }
    }

    AZStd::string CharacterControllerComponent::GetCollisionGroupName()
    {
        AZStd::string groupName;
        if (!IsPhysicsEnabled())
        {
            return groupName;
        }
        
        Physics::CollisionRequestBus::BroadcastResult(groupName, &Physics::CollisionRequests::GetCollisionGroupName, m_controller->GetCollisionGroup());
        return groupName;
    }

    void CharacterControllerComponent::ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled)
    {
        if (!IsPhysicsEnabled())
        {
            return;
        }

        if (Physics::Utils::FilterTag(m_controller->GetColliderTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionLayer collisionLayer;
            Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, collisionLayer);
            if (success)
            {
                AzPhysics::CollisionLayer layer(layerName);
                AzPhysics::CollisionGroup group = m_controller->GetCollisionGroup();
                group.SetLayer(layer, enabled);
                m_controller->SetCollisionGroup(group);
            }
        }
    }

    void CharacterControllerComponent::OnPreSimulate(float deltaTime)
    {
        if (m_controller)
        {
            m_controller->ApplyRequestedVelocity(deltaTime);
            const AZ::Vector3 newPosition = GetBasePosition();
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, newPosition);
        }
    }

    bool CharacterControllerComponent::CreateController()
    {
        AZStd::shared_ptr<Physics::World> defaultWorld;
        Physics::DefaultWorldBus::BroadcastResult(defaultWorld, &Physics::DefaultWorldRequests::GetDefaultWorld);
        if (!defaultWorld)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to retrieve default world.");
            return false;
        }

        m_characterConfig->m_debugName = GetEntity()->GetName();
        m_characterConfig->m_entityId = GetEntityId();

        m_controller = Utils::Characters::CreateCharacterController(*m_characterConfig, *m_shapeConfig, *defaultWorld);
        if (!m_controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to create character controller.");
            return false;
        }

        AZ::Vector3 entityTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(entityTranslation, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
        // It's usually more convenient to control the foot position rather than the centre of the capsule, so
        // make the foot position coincide with the entity position.
        m_controller->SetBasePosition(entityTranslation);
        AttachColliders(*m_controller);
        CharacterControllerRequestBus::Handler::BusConnect(GetEntityId());

        m_preSimulateHandler = AzPhysics::SystemEvents::OnPresimulateEvent::Handler(
            [this](float deltaTime)
            {
                OnPreSimulate(deltaTime);
            }
        );

        if (auto* physXSystem = GetPhysXSystem())
        {
            physXSystem->RegisterPreSimulateEvent(m_preSimulateHandler);
        }

        return true;
    }

    void CharacterControllerComponent::AttachColliders(Physics::Character& character)
    {
        PhysX::ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [&character](PhysX::ColliderComponentRequests* handler)
        {
            for (auto& shape : handler->GetShapes())
            {
                character.AttachShape(shape);
            }
            return true;
        });
    }

} // namespace PhysX
