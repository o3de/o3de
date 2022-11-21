/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetworkHitVolumesComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/SystemBus.h>
#include <MCore/Source/AzCoreConversions.h>
#include <Integration/ActorComponentBus.h>

namespace Multiplayer
{
    AZ_CVAR(bool, bg_DrawArticulatedHitVolumes, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables debug draw of articulated hit volumes");
    AZ_CVAR(float, bg_DrawDebugHitVolumeLifetime, 0.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "The lifetime for hit volume draw-debug shapes");

    AZ_CVAR(float, bg_RewindPositionTolerance, 0.0001f, nullptr, AZ::ConsoleFunctorFlags::Null, "Don't sync the physx entity if the square of delta position is less than this value");
    AZ_CVAR(float, bg_RewindOrientationTolerance, 0.001f, nullptr, AZ::ConsoleFunctorFlags::Null, "Don't sync the physx entity if the square of delta orientation is less than this value");

    NetworkHitVolumesComponent::AnimatedHitVolume::AnimatedHitVolume
    (
        AzNetworking::ConnectionId connectionId,
        Physics::CharacterRequests* character,
        const char* hitVolumeName,
        const Physics::ColliderConfiguration* colliderConfig,
        const Physics::ShapeConfiguration* shapeConfig,
        const uint32_t jointIndex
    )
        : m_colliderConfig(colliderConfig)
        , m_shapeConfig(shapeConfig)
        , m_jointIndex(jointIndex)
    {
        m_transform.SetOwningConnectionId(connectionId);

        m_colliderOffSetTransform = AZ::Transform::CreateFromQuaternionAndTranslation(m_colliderConfig->m_rotation, m_colliderConfig->m_position);

        if (m_colliderConfig->m_isExclusive)
        {
            Physics::SystemRequestBus::BroadcastResult(m_physicsShape, &Physics::SystemRequests::CreateShape, *m_colliderConfig, *m_shapeConfig);
        }
        else
        {
            Physics::ColliderConfiguration colliderConfiguration = *m_colliderConfig;
            colliderConfiguration.m_isExclusive = true;
            colliderConfiguration.m_isSimulated = false;
            colliderConfiguration.m_isInSceneQueries = true;
            Physics::SystemRequestBus::BroadcastResult(m_physicsShape, &Physics::SystemRequests::CreateShape, colliderConfiguration, *m_shapeConfig);
        }

        if (m_physicsShape)
        {
            m_physicsShape->SetName(hitVolumeName);
            character->GetCharacter()->AttachShape(m_physicsShape);
        }
    }

    void NetworkHitVolumesComponent::AnimatedHitVolume::UpdateTransform(const AZ::Transform& transform)
    {
        m_transform = transform;
        m_physicsShape->SetLocalPose(transform.GetTranslation(), transform.GetRotation());
    }

    void NetworkHitVolumesComponent::AnimatedHitVolume::SyncToCurrentTransform()
    {
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

        const AZ::Transform  physicsTransform = AZ::Transform::CreateFromQuaternionAndTranslation(m_physicsShape->GetLocalPose().second, m_physicsShape->GetLocalPose().first);

        // Don't call SetLocalPose unless the transforms are actually different
        const AZ::Vector3 positionDelta = physicsTransform.GetTranslation() - rewoundTransform.GetTranslation();
        const AZ::Quaternion orientationDelta = physicsTransform.GetRotation() - rewoundTransform.GetRotation();

        if ((positionDelta.GetLengthSq() >= bg_RewindPositionTolerance) || (orientationDelta.GetLengthSq() >= bg_RewindOrientationTolerance))
        {
            m_physicsShape->SetLocalPose(rewoundTransform.GetTranslation(), rewoundTransform.GetRotation());
        }
    }

    void NetworkHitVolumesComponent::NetworkHitVolumesComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkHitVolumesComponent, NetworkHitVolumesComponentBase>()
                ->Version(1);
        }
        NetworkHitVolumesComponentBase::Reflect(context);
    }

    NetworkHitVolumesComponent::NetworkHitVolumesComponent()
        : m_syncRewindHandler([this]() { OnSyncRewind(); })
        , m_preRenderHandler([this](float deltaTime) { OnPreRender(deltaTime); })
        , m_transformChangedHandler([this](const AZ::Transform&, const AZ::Transform& worldTm) { OnTransformUpdate(worldTm); })
    {
        ;
    }

    void NetworkHitVolumesComponent::OnInit()
    {
        ;
    }

    void NetworkHitVolumesComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusConnect(GetEntityId());
        GetNetBindComponent()->AddEntitySyncRewindEventHandler(m_syncRewindHandler);
        GetNetBindComponent()->AddEntityPreRenderEventHandler(m_preRenderHandler);
        m_physicsCharacter = Physics::CharacterRequestBus::FindFirstHandler(GetEntityId());
        GetTransformComponent()->BindTransformChangedEventHandler(m_transformChangedHandler);
        OnTransformUpdate(GetTransformComponent()->GetWorldTM());
    }

    void NetworkHitVolumesComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_syncRewindHandler.Disconnect();
        m_preRenderHandler.Disconnect();
        m_transformChangedHandler.Disconnect();
        DestroyHitVolumes();
        EMotionFX::Integration::ActorComponentNotificationBus::Handler::BusDisconnect();
    }

    void NetworkHitVolumesComponent::OnPreRender([[maybe_unused]] float deltaTime)
    {
        if (m_animatedHitVolumes.empty())
        {
            CreateHitVolumes();
        }

        AZ::Vector3 position, scale;
        AZ::Quaternion rotation;
        for (AnimatedHitVolume& hitVolume : m_animatedHitVolumes)
        {
            m_actorComponent->GetJointTransformComponents(hitVolume.m_jointIndex, EMotionFX::Integration::Space::ModelSpace, position, rotation, scale);
            hitVolume.UpdateTransform(AZ::Transform::CreateFromQuaternionAndTranslation(rotation, position) * hitVolume.m_colliderOffSetTransform);
        }
    }

    void NetworkHitVolumesComponent::OnTransformUpdate([[maybe_unused]] const AZ::Transform& transform)
    {
        OnSyncRewind();
    }

    void NetworkHitVolumesComponent::OnSyncRewind()
    {
        if (m_physicsCharacter && m_physicsCharacter->GetCharacter())
        {
            uint32_t frameId = static_cast<uint32_t>(Multiplayer::GetNetworkTime()->GetHostFrameId());
            m_physicsCharacter->GetCharacter()->SetFrameId(frameId);
        }

        for (AnimatedHitVolume& hitVolume : m_animatedHitVolumes)
        {
            hitVolume.SyncToCurrentTransform();
        }
    }

    void NetworkHitVolumesComponent::CreateHitVolumes()
    {
        if (m_physicsCharacter == nullptr || m_actorComponent == nullptr)
        {
            return;
        }

        const Physics::AnimationConfiguration* physicsConfig = m_actorComponent->GetPhysicsConfig();
        if (physicsConfig == nullptr)
        {
            return;
        }

        m_hitDetectionConfig = &physicsConfig->m_hitDetectionConfig;
        const AzNetworking::ConnectionId owningConnectionId = GetNetBindComponent()->GetOwningConnectionId();

        m_animatedHitVolumes.reserve(m_hitDetectionConfig->m_nodes.size());
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : m_hitDetectionConfig->m_nodes)
        {
            const AZStd::size_t jointIndex = m_actorComponent->GetJointIndexByName(nodeConfig.m_name.c_str());
            if (jointIndex == EMotionFX::Integration::ActorComponentRequests::s_invalidJointIndex)
            {
                continue;
            }

            for (const AzPhysics::ShapeColliderPair& coliderPair : nodeConfig.m_shapes)
            {
                const Physics::ColliderConfiguration* colliderConfig = coliderPair.first.get();
                Physics::ShapeConfiguration* shapeConfig = coliderPair.second.get();
                m_animatedHitVolumes.emplace_back(owningConnectionId, m_physicsCharacter, nodeConfig.m_name.c_str(), colliderConfig, shapeConfig, aznumeric_cast<uint32_t>(jointIndex));
            }
        }
    }

    void NetworkHitVolumesComponent::DestroyHitVolumes()
    {
        m_animatedHitVolumes.clear();
    }

    void NetworkHitVolumesComponent::OnActorInstanceCreated([[maybe_unused]] EMotionFX::ActorInstance* actorInstance)
    {
        m_actorComponent = EMotionFX::Integration::ActorComponentRequestBus::FindFirstHandler(GetEntity()->GetId());
    }

    void NetworkHitVolumesComponent::OnActorInstanceDestroyed([[maybe_unused]] EMotionFX::ActorInstance* actorInstance)
    {
        m_actorComponent = nullptr;
    }
}
