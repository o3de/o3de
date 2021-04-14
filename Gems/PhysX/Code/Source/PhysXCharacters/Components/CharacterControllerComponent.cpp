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

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>
#include <PhysX/ColliderComponentBus.h>
#include <Scene/PhysXScene.h>
#include <System/PhysXSystem.h>
#include <AzFramework/Physics/Utils.h>
#include <PhysXCharacters/API/CharacterUtils.h>

#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
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
        CreateController();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        Physics::CharacterRequestBus::Handler::BusConnect(GetEntityId());
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(GetEntityId());
        Physics::WorldBodyRequestBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterControllerComponent::Deactivate()
    {
        DestroyController();

        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        Physics::WorldBodyRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        Physics::CharacterRequestBus::Handler::BusDisconnect();
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
        CreateController();
    }

    void CharacterControllerComponent::DisablePhysics()
    {
        DestroyController();
    }

    bool CharacterControllerComponent::IsPhysicsEnabled() const
    {
        return m_controller != nullptr;
    }

    AZ::Aabb CharacterControllerComponent::GetAabb() const
    {
        if (m_controller)
        {
            return m_controller->GetAabb();
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBody* CharacterControllerComponent::GetWorldBody()
    {
        return m_controller.get();
    }

    AzPhysics::SceneQueryHit CharacterControllerComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_controller)
        {
            return m_controller->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    // CharacterControllerRequestBus
    void CharacterControllerComponent::Resize(float height)
    {
        return m_controller->Resize(height);
    }

    float CharacterControllerComponent::GetHeight()
    {
        return m_controller->GetHeight();
    }

    void CharacterControllerComponent::SetHeight(float height)
    {
        return m_controller->SetHeight(height);
    }

    float CharacterControllerComponent::GetRadius()
    {
        return m_controller->GetRadius();
    }

    void CharacterControllerComponent::SetRadius(float radius)
    {
        return m_controller->SetRadius(radius);
    }

    float CharacterControllerComponent::GetHalfSideExtent()
    {
        return m_controller->GetHalfSideExtent();
    }

    void CharacterControllerComponent::SetHalfSideExtent(float halfSideExtent)
    {
        return m_controller->SetHalfSideExtent(halfSideExtent);
    }

    float CharacterControllerComponent::GetHalfForwardExtent()
    {
        return m_controller->GetHalfForwardExtent();
    }

    void CharacterControllerComponent::SetHalfForwardExtent(float halfForwardExtent)
    {
        return m_controller->SetHalfForwardExtent(halfForwardExtent);
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

    void CharacterControllerComponent::CreateController()
    {
        if (m_controller)
        {
            return;
        }

        AzPhysics::SceneHandle defaultSceneHandle = AzPhysics::InvalidSceneHandle;
        Physics::DefaultWorldBus::BroadcastResult(defaultSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        if (defaultSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to retrieve default scene.");
            return;
        }

        m_characterConfig->m_debugName = GetEntity()->GetName();
        m_characterConfig->m_entityId = GetEntityId();

        m_controller = Utils::Characters::CreateCharacterController(*m_characterConfig, *m_shapeConfig, defaultSceneHandle);
        if (!m_controller)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to create character controller.");
            return;
        }
        m_controller->EnablePhysics(*m_characterConfig);

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

        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsEnabled);
    }

    void CharacterControllerComponent::DestroyController()
    {
        if (!m_controller)
        {
            return;
        }

        m_controller->DisablePhysics();

        // The character is first removed from the scene, and then its deletion is deferred.
        // This ensures trigger exit events are raised correctly on deleted objects.
        {
            auto* scene = azdynamic_cast<PhysX::PhysXScene*>(m_controller->GetScene());
            AZ_Assert(scene, "Invalid PhysX scene");
            scene->DeferDelete(AZStd::move(m_controller));
            m_controller.reset();
        }

        m_preSimulateHandler.Disconnect();

        CharacterControllerRequestBus::Handler::BusDisconnect();

        Physics::WorldBodyNotificationBus::Event(GetEntityId(), &Physics::WorldBodyNotifications::OnPhysicsDisabled);
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
