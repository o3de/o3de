/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        AZStd::shared_ptr<Physics::ShapeConfiguration> shapeConfig)
        : m_characterConfig(AZStd::move(characterConfig))
        , m_shapeConfig(AZStd::move(shapeConfig))
    {
    }

    CharacterControllerComponent::~CharacterControllerComponent()
    {
        DisableController();
    }

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
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void CharacterControllerComponent::Deactivate()
    {
        DisableController();

        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        Physics::CharacterRequestBus::Handler::BusDisconnect();
    }

    AZ::Vector3 CharacterControllerComponent::GetBasePosition() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetBasePosition();
        }
        return AZ::Vector3::CreateZero();
    }

    void CharacterControllerComponent::SetBasePosition(const AZ::Vector3& position)
    {
        if (auto* controller = GetController())
        {
            controller->SetBasePosition(position);
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, position);
        }
    }

    AZ::Vector3 CharacterControllerComponent::GetCenterPosition() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetCenterPosition();
        }
        return AZ::Vector3::CreateZero();
    }

    float CharacterControllerComponent::GetStepHeight() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetStepHeight();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetStepHeight(float stepHeight)
    {
        if (auto* controller = GetController())
        {
            controller->SetStepHeight(stepHeight);
        }
    }

    AZ::Vector3 CharacterControllerComponent::GetUpDirection() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetUpDirection();
        }
        return AZ::Vector3::CreateZero();
    }

    void CharacterControllerComponent::SetUpDirection([[maybe_unused]] const AZ::Vector3& upDirection)
    {
        AZ_Warning("PhysX Character Controller Component", false, "Setting up direction is not currently supported.");
    }

    float CharacterControllerComponent::GetSlopeLimitDegrees() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetSlopeLimitDegrees();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetSlopeLimitDegrees(float slopeLimitDegrees)
    {
        if (auto* controller = GetController())
        {
            controller->SetSlopeLimitDegrees(slopeLimitDegrees);
        }
    }

    float CharacterControllerComponent::GetMaximumSpeed() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetMaximumSpeed();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetMaximumSpeed(float maximumSpeed)
    {
        if (auto* controller = GetController())
        {
            controller->SetMaximumSpeed(maximumSpeed);
        }
    }

    AZ::Vector3 CharacterControllerComponent::GetVelocity() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetVelocity();
        }
        return AZ::Vector3::CreateZero();
    }

    void CharacterControllerComponent::AddVelocity(const AZ::Vector3& velocity)
    {
        if (auto* controller = GetController())
        {
            controller->AddVelocity(velocity);
        }
    }

    Physics::Character* CharacterControllerComponent::GetCharacter()
    {
        return GetController();
    }

    void CharacterControllerComponent::EnablePhysics()
    {
        CreateController();
    }

    void CharacterControllerComponent::DisablePhysics()
    {
        DisableController();
    }

    bool CharacterControllerComponent::IsPhysicsEnabled() const
    {
        return GetControllerConst() != nullptr;
    }

    AZ::Aabb CharacterControllerComponent::GetAabb() const
    {
        if (auto* controller = GetControllerConst())
        {
            return controller->GetAabb();
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBody* CharacterControllerComponent::GetSimulatedBody()
    {
        return GetCharacter();
    }

    AzPhysics::SimulatedBodyHandle CharacterControllerComponent::GetSimulatedBodyHandle() const
    {
        return m_controllerBodyHandle;
    }

    AzPhysics::SceneQueryHit CharacterControllerComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (auto* controller = GetController())
        {
            return controller->RayCast(request);
        }

        return AzPhysics::SceneQueryHit();
    }

    // CharacterControllerRequestBus
    void CharacterControllerComponent::Resize(float height)
    {
        if (auto* controller = GetController())
        {
            controller->Resize(height);
        }
    }

    float CharacterControllerComponent::GetHeight()
    {
        if (auto* controller = GetController())
        {
            return controller->GetHeight();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetHeight(float height)
    {
        if (auto* controller = GetController())
        {
            controller->SetHeight(height);
        }
    }

    float CharacterControllerComponent::GetRadius()
    {
        if (auto* controller = GetController())
        {
            return controller->GetRadius();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetRadius(float radius)
    {
        if (auto* controller = GetController())
        {
            controller->SetRadius(radius);
        }
    }

    float CharacterControllerComponent::GetHalfSideExtent()
    {
        if (auto* controller = GetController())
        {
            return controller->GetHalfSideExtent();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetHalfSideExtent(float halfSideExtent)
    {
        if (auto* controller = GetController())
        {
            controller->SetHalfSideExtent(halfSideExtent);
        }
    }

    float CharacterControllerComponent::GetHalfForwardExtent()
    {
        if (auto* controller = GetController())
        {
            return controller->GetHalfForwardExtent();
        }
        return 0.0f;
    }

    void CharacterControllerComponent::SetHalfForwardExtent(float halfForwardExtent)
    {
        if (auto* controller = GetController())
        {
            controller->SetHalfForwardExtent(halfForwardExtent);
        }
    }

    // TransformNotificationBus
    void CharacterControllerComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        if (auto* controller = GetController())
        {
            controller->SetBasePosition(world.GetTranslation());
        }
    }
    
    void CharacterControllerComponent::SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag)
    {
        auto* controller = GetController();
        if (controller == nullptr)
        {
            return;
        }

        if (Physics::Utils::FilterTag(controller->GetColliderTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionLayer collisionLayer;
            Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, collisionLayer);
            if (success) 
            {
                controller->SetCollisionLayer(collisionLayer);
            }
        }
    }

    AZStd::string CharacterControllerComponent::GetCollisionLayerName()
    {
        AZStd::string layerName;
        auto* controller = GetControllerConst();
        if (controller == nullptr)
        {
            return layerName;
        }

        Physics::CollisionRequestBus::BroadcastResult(
            layerName, &Physics::CollisionRequests::GetCollisionLayerName, controller->GetCollisionLayer());
        return layerName;
    }

    void CharacterControllerComponent::SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag)
    {
        auto* controller = GetController();
        if (controller == nullptr)
        {
            return;
        }

        if (Physics::Utils::FilterTag(controller->GetColliderTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionGroup collisionGroup;
            Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionGroupByName, groupName, collisionGroup);
            if (success)
            {
                controller->SetCollisionGroup(collisionGroup);
            }
        }
    }

    AZStd::string CharacterControllerComponent::GetCollisionGroupName()
    {
        AZStd::string groupName;
        auto* controller = GetControllerConst();
        if (controller == nullptr)
        {
            return groupName;
        }

        Physics::CollisionRequestBus::BroadcastResult(
            groupName, &Physics::CollisionRequests::GetCollisionGroupName, controller->GetCollisionGroup());
        return groupName;
    }

    void CharacterControllerComponent::ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled)
    {
        auto* controller = GetController();
        if (controller == nullptr)
        {
            return;
        }

        if (Physics::Utils::FilterTag(controller->GetColliderTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionLayer collisionLayer;
            Physics::CollisionRequestBus::BroadcastResult(success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, collisionLayer);
            if (success)
            {
                AzPhysics::CollisionLayer layer(layerName);
                AzPhysics::CollisionGroup group = controller->GetCollisionGroup();
                group.SetLayer(layer, enabled);
                controller->SetCollisionGroup(group);
            }
        }
    }

    void CharacterControllerComponent::OnPreSimulate(float deltaTime)
    {
        if (auto* controller = GetController())
        {
            controller->ApplyRequestedVelocity(deltaTime);
            const AZ::Vector3 newPosition = controller->GetBasePosition();
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, newPosition);
        }
    }

    const PhysX::CharacterController* CharacterControllerComponent::GetControllerConst() const
    {
        if (m_controllerBodyHandle == AzPhysics::InvalidSimulatedBodyHandle || m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            return nullptr;
        }

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return azdynamic_cast<PhysX::CharacterController*>(
                sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_controllerBodyHandle));
        }
        return nullptr;
    }

    PhysX::CharacterController* CharacterControllerComponent::GetController()
    {
        return const_cast<PhysX::CharacterController*>(static_cast<const CharacterControllerComponent&>(*this).GetControllerConst());
    }

    void CharacterControllerComponent::CreateController()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }

        Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to retrieve default scene.");
            return;
        }

        m_characterConfig->m_debugName = GetEntity()->GetName();
        m_characterConfig->m_entityId = GetEntityId();
        m_characterConfig->m_shapeConfig = m_shapeConfig;
        // get all the collider shapes and add it to the config
        PhysX::ColliderComponentRequestBus::EnumerateHandlersId(GetEntityId(), [this](PhysX::ColliderComponentRequests* handler)
            {
                auto shapes = handler->GetShapes();
                m_characterConfig->m_colliders.insert(m_characterConfig->m_colliders.end(), shapes.begin(), shapes.end());
                return true;
            });

        // It's usually more convenient to control the foot position rather than the centre of the capsule, so
        // make the foot position coincide with the entity position.
        AZ::Vector3 entityTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(entityTranslation, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);
        m_characterConfig->m_position = entityTranslation;

        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (sceneInterface != nullptr)
        {
            m_controllerBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, m_characterConfig.get());
        }
        if (m_controllerBodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            AZ_Error("PhysX Character Controller Component", false, "Failed to create character controller.");
            return;
        }

        if (sceneInterface != nullptr)
        {
            // if the scene removes this controller body, we should also clean up our resources.
            m_onSimulatedBodyRemovedHandler = AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler(
                [this]([[maybe_unused]] AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle bodyHandle) {
                    if (bodyHandle == m_controllerBodyHandle)
                    {
                        DestroyController();
                    }
                });
            sceneInterface->RegisterSimulationBodyRemovedHandler(m_attachedSceneHandle, m_onSimulatedBodyRemovedHandler);
        }

        CharacterControllerRequestBus::Handler::BusConnect(GetEntityId());

        if (m_characterConfig->m_applyMoveOnPhysicsTick)
        {
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
        }
    }

    void CharacterControllerComponent::DisableController()
    {
        if (auto* controller = GetController())
        {
            controller->DisablePhysics();

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, controller->m_bodyHandle);
            }

            DestroyController();
        }
    }

    void CharacterControllerComponent::DestroyController()
    {
        m_controllerBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
        m_preSimulateHandler.Disconnect();
        m_onSimulatedBodyRemovedHandler.Disconnect();
        CharacterControllerRequestBus::Handler::BusDisconnect();
    }
} // namespace PhysX
