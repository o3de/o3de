/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetworkRigidBodyComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>

namespace Multiplayer
{
    AZ_CVAR_EXTERNED(float, bg_RewindPositionTolerance);
    AZ_CVAR_EXTERNED(float, bg_RewindOrientationTolerance);

    void NetworkRigidBodyComponent::NetworkRigidBodyComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkRigidBodyComponent, NetworkRigidBodyComponentBase>()->Version(1);
        }
        NetworkRigidBodyComponentBase::Reflect(context);
    }

    void NetworkRigidBodyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        NetworkRigidBodyComponentBase::GetRequiredServices(required);
        required.push_back(AZ_CRC_CE("PhysicsDynamicRigidBodyService"));
    }

    NetworkRigidBodyComponent::NetworkRigidBodyComponent()
        : m_syncRewindHandler([this](){ OnSyncRewind(); })
        , m_transformChangedHandler([this]([[maybe_unused]] const AZ::Transform& localTm, const AZ::Transform& worldTm){ OnTransformUpdate(worldTm); })
    {
    }

    void NetworkRigidBodyComponent::OnInit()
    {
    }

    void NetworkRigidBodyComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        // During activation the simulated bodies are not created yet.
        // Connect to RigidBodyNotificationBus to listen when it's enabled after creation.
        Physics::RigidBodyNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void NetworkRigidBodyComponent::OnPhysicsEnabled(const AZ::EntityId& entityId)
    {
        Physics::RigidBodyNotificationBus::Handler::BusDisconnect();

        m_physicsRigidBodyComponent = Physics::RigidBodyRequestBus::FindFirstHandler(entityId);
        AZ_Assert(m_physicsRigidBodyComponent, "Physics Rigid Body is required on entity %s", GetEntity()->GetName().c_str());

        // By default we're kinematic unless there is a controller, in which case the controller will handle it.
        if (!HasController())
        {
            m_physicsRigidBodyComponent->SetKinematic(true);
        }

        NetworkRigidBodyRequestBus::Handler::BusConnect(GetEntityId());

        GetNetBindComponent()->AddEntitySyncRewindEventHandler(m_syncRewindHandler);
        GetEntity()->GetTransform()->BindTransformChangedEventHandler(m_transformChangedHandler);
    }

    void NetworkRigidBodyComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        Physics::RigidBodyNotificationBus::Handler::BusDisconnect();
        NetworkRigidBodyRequestBus::Handler::BusDisconnect();

        m_physicsRigidBodyComponent = nullptr;
    }

    void NetworkRigidBodyComponent::OnTransformUpdate(const AZ::Transform& worldTm)
    {
        m_transform = worldTm;

        if (!HasController())
        {
            m_physicsRigidBodyComponent->SetKinematicTarget(worldTm);
        }
    }

    void NetworkRigidBodyComponent::OnSyncRewind()
    {
        uint32_t frameId = static_cast<uint32_t>(Multiplayer::GetNetworkTime()->GetHostFrameId());

        if (AzPhysics::RigidBody* rigidBody = m_physicsRigidBodyComponent->GetRigidBody())
        {
            rigidBody->SetFrameId(frameId);

            AZ::Transform rewoundTransform;
            const AZ::Transform& targetTransform = m_transform.Get();
            const float blendFactor = Multiplayer::GetNetworkTime()->GetHostBlendFactor();
            if (blendFactor < 1.f)
            {
                // If a blend factor was supplied, interpolate the transform appropriately
                const AZ::Transform& previousTransform = m_transform.GetPrevious();
                rewoundTransform.SetRotation(previousTransform.GetRotation().Slerp(targetTransform.GetRotation(), blendFactor));
                rewoundTransform.SetTranslation(previousTransform.GetTranslation().Lerp(targetTransform.GetTranslation(), blendFactor));
                rewoundTransform.SetUniformScale(AZ::Lerp(previousTransform.GetUniformScale(), targetTransform.GetUniformScale(), blendFactor));
            }
            else
            {
                rewoundTransform = m_transform.Get();
            }
            const AZ::Transform& physicsTransform = rigidBody->GetTransform();

            // Don't call SetLocalPose unless the transforms are actually different
            const AZ::Vector3 positionDelta = physicsTransform.GetTranslation() - rewoundTransform.GetTranslation();
            const AZ::Quaternion orientationDelta = physicsTransform.GetRotation() - rewoundTransform.GetRotation();

            if ((positionDelta.GetLengthSq() >= bg_RewindPositionTolerance) ||
                (orientationDelta.GetLengthSq() >= bg_RewindOrientationTolerance))
            {
                rigidBody->SetTransform(rewoundTransform);
            }
        }
    }

    NetworkRigidBodyComponentController::NetworkRigidBodyComponentController(NetworkRigidBodyComponent& parent)
        : NetworkRigidBodyComponentControllerBase(parent)
#if AZ_TRAIT_SERVER
        , m_transformChangedHandler([this](const AZ::Transform&, const AZ::Transform&) { OnTransformUpdate(); })
#endif
    {
        ;
    }

    void NetworkRigidBodyComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        // During activation the simulated bodies are not created yet.
        // Connect to RigidBodyNotificationBus to listen when it's enabled after creation.
        Physics::RigidBodyNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void NetworkRigidBodyComponentController::OnPhysicsEnabled(const AZ::EntityId& entityId)
    {
        Physics::RigidBodyNotificationBus::Handler::BusDisconnect();

        m_physicsRigidBodyComponent = Physics::RigidBodyRequestBus::FindFirstHandler(entityId);
        AZ_Assert(m_physicsRigidBodyComponent, "Physics Rigid Body is required on entity %s", GetEntity()->GetName().c_str());

        m_physicsRigidBodyComponent->SetKinematic(false);

#if AZ_TRAIT_SERVER
        if (IsNetEntityRoleAuthority())
        {
            if (AzPhysics::RigidBody* rigidBody = m_physicsRigidBodyComponent->GetRigidBody())
            {
                rigidBody->SetLinearVelocity(GetLinearVelocity());
                rigidBody->SetAngularVelocity(GetAngularVelocity());
                GetEntity()->GetTransform()->BindTransformChangedEventHandler(m_transformChangedHandler);
            }
        }
#endif
    }

    void NetworkRigidBodyComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
#if AZ_TRAIT_SERVER
        m_transformChangedHandler.Disconnect();
#endif
        if (m_physicsRigidBodyComponent)
        {
            m_physicsRigidBodyComponent->SetKinematic(true);
            m_physicsRigidBodyComponent = nullptr;
        }
    }

#if AZ_TRAIT_SERVER
    void NetworkRigidBodyComponentController::HandleSendApplyImpulse
    (
        [[maybe_unused]] AzNetworking::IConnection* invokingConnection,
        const AZ::Vector3& impulse,
        const AZ::Vector3& worldPoint
    )
    {
        if (AzPhysics::RigidBody* rigidBody = m_physicsRigidBodyComponent->GetRigidBody())
        {
            rigidBody->ApplyLinearImpulseAtWorldPoint(impulse, worldPoint);
        }
    }

    void NetworkRigidBodyComponentController::OnTransformUpdate()
    {
        if (AzPhysics::RigidBody* rigidBody = m_physicsRigidBodyComponent->GetRigidBody())
        {
            SetLinearVelocity(rigidBody->GetLinearVelocity());
            SetAngularVelocity(rigidBody->GetAngularVelocity());
        }
    }
#endif
} // namespace Multiplayer
