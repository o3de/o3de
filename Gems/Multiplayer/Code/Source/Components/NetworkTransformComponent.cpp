/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/EBus/IEventScheduler.h>
#include <AzFramework/Components/TransformComponent.h>

namespace Multiplayer
{
    void NetworkTransformComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkTransformComponent, NetworkTransformComponentBase>()
                ->Version(1);
        }
        NetworkTransformComponentBase::Reflect(context);

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<NetworkTransformComponent>("NetworkTransformComponent")
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")

                ->Method("IncrementResetCount", [](AZ::EntityId id)
                    {
                        const AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                        if (!entity)
                        {
                            AZ_Warning("Network Property", false, "NetworkTransformComponent IncrementResetCount failed. "
                                "The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str());
                        }

                        NetworkTransformComponent* networkComponent = entity->FindComponent<NetworkTransformComponent>();
                        if (!networkComponent)
                        {
                            AZ_Warning("Network Property", false, "NetworkTransformComponent IncrementResetCount failed. "
                                "Entity '%s' (id: %s) is missing NetworkTransformComponent, be sure to add NetworkTransformComponent to this entity.", entity->GetName().c_str(), id.ToString().c_str());
                        }
                        else
                        {
                            if (networkComponent->HasController())
                            {
                                static_cast<NetworkTransformComponentController*>(networkComponent->GetController())->ModifyResetCount()++;
                            }
                            else
                            {
                                AZ_Warning("Network Property", false, "NetworkTransformComponent IncrementResetCount failed. "
                                    "Entity '%s' (id: %s) does not have Authority or Autonomous role.", entity->GetName().c_str(), id.ToString().c_str());
                            }
                        }
                    });
        }
    }

    NetworkTransformComponent::NetworkTransformComponent()
        : m_entityPreRenderEventHandler([this](float deltaTime) { OnPreRender(deltaTime); })
        , m_entityCorrectionEventHandler([this]() { OnCorrection(); })
        , m_parentChangedEventHandler([this](NetEntityId parentId) { OnParentChanged(parentId); })
        , m_resetCountChangedEventHandler([this]([[maybe_unused]] uint8_t resetCount) { m_syncTransformImmediate = true; })
    {
        ;
    }

    void NetworkTransformComponent::OnInit()
    {
        ;
    }

    void NetworkTransformComponent::OnActivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        GetNetBindComponent()->AddEntityPreRenderEventHandler(m_entityPreRenderEventHandler);
        GetNetBindComponent()->AddEntityCorrectionEventHandler(m_entityCorrectionEventHandler);
        ParentEntityIdAddEvent(m_parentChangedEventHandler);
        ResetCountAddEvent(m_resetCountChangedEventHandler);

        if (!GetNetBindComponent()->IsNetEntityRoleAuthority())
        {
            OnParentChanged(GetParentEntityId());
        }
    }

    void NetworkTransformComponent::OnDeactivate([[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_resetCountChangedEventHandler.Disconnect();
        m_parentChangedEventHandler.Disconnect();
        m_entityCorrectionEventHandler.Disconnect();
        m_entityPreRenderEventHandler.Disconnect();
    }

    void NetworkTransformComponent::OnPreRender([[maybe_unused]] float deltaTime)
    {
        if (!HasController())
        {
            AZ::Transform blendTransform(GetTranslation(), GetRotation(), GetScale());

            if (!m_syncTransformImmediate)
            {
                const float blendFactor = GetMultiplayer()->GetCurrentBlendFactor();
                if (!AZ::IsClose(blendFactor, 1.0f))
                {
                    const AZ::Transform blendTransformPrevious(GetTranslationPrevious(), GetRotationPrevious(), GetScalePrevious());

                    if (!blendTransform.IsClose(blendTransformPrevious))
                    {
                        blendTransform.SetRotation(blendTransformPrevious.GetRotation().Slerp(blendTransform.GetRotation(), blendFactor));
                        blendTransform.SetTranslation(
                            blendTransformPrevious.GetTranslation().Lerp(blendTransform.GetTranslation(), blendFactor));
                        blendTransform.SetUniformScale(
                            AZ::Lerp(blendTransformPrevious.GetUniformScale(), blendTransform.GetUniformScale(), blendFactor));
                    }
                }
            }
            else
            {
                m_syncTransformImmediate = false;
            }

            AzFramework::TransformComponent* transformComponent = GetTransformComponent();
            if (GetParentEntityId() == InvalidNetEntityId)
            {
                if (!transformComponent->GetWorldTM().IsClose(blendTransform))
                {
                    transformComponent->SetWorldTM(blendTransform);
                }
            }
            else
            {
                if (!transformComponent->GetLocalTM().IsClose(blendTransform))
                {
                    transformComponent->SetLocalTM(blendTransform);
                }
            }
        }
    }

    void NetworkTransformComponent::OnCorrection()
    {
        // Snap to latest
        AZ::Transform targetTransform;
        targetTransform.SetRotation(GetRotation());
        targetTransform.SetTranslation(GetTranslation());
        targetTransform.SetUniformScale(GetScale());

        // Hard set the entities transform
        AzFramework::TransformComponent* transformComponent = GetTransformComponent();
        if (GetParentEntityId() == InvalidNetEntityId)
        {
            if (!transformComponent->GetWorldTM().IsClose(targetTransform))
            {
                transformComponent->SetWorldTM(targetTransform);
            }
        }
        else
        {
            if (!transformComponent->GetLocalTM().IsClose(targetTransform))
            {
                transformComponent->SetLocalTM(targetTransform);
            }
        }
    }

    void NetworkTransformComponent::OnParentChanged(NetEntityId parentId)
    {
        if (AZ::TransformInterface* transformComponent = GetEntity()->GetTransform())
        {
            const ConstNetworkEntityHandle parentEntityHandle = GetNetworkEntityManager()->GetEntity(parentId);
            if (parentEntityHandle.Exists())
            {
                if (const AZ::Entity* parentEntity = parentEntityHandle.GetEntity())
                {
                    transformComponent->SetParent(parentEntity->GetId());
                }
            }
            else
            {
                transformComponent->SetParent(AZ::EntityId());
            }
        }
    }

    NetworkTransformComponentController::NetworkTransformComponentController(NetworkTransformComponent& parent)
        : NetworkTransformComponentControllerBase(parent)
        , m_transformChangedHandler([this](const AZ::Transform& localTm, const AZ::Transform& worldTm) { OnTransformChangedEvent(localTm, worldTm); })
        , m_parentIdChangedHandler([this](AZ::EntityId oldParent, AZ::EntityId newParent) { OnParentIdChangedEvent(oldParent, newParent); })
    {
        ;
    }

    void NetworkTransformComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (AzFramework::TransformComponent* parentTransform = GetParent().GetTransformComponent())
        {
            parentTransform->BindTransformChangedEventHandler(m_transformChangedHandler);
            OnTransformChangedEvent(parentTransform->GetLocalTM(), parentTransform->GetWorldTM());

            parentTransform->BindParentChangedEventHandler(m_parentIdChangedHandler);
            OnParentIdChangedEvent(AZ::EntityId(), parentTransform->GetParentId());
        }
    }

    void NetworkTransformComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_parentIdChangedHandler.Disconnect();
        m_transformChangedHandler.Disconnect();
    }

    void NetworkTransformComponentController::OnTransformChangedEvent(const AZ::Transform& localTm, const AZ::Transform& worldTm)
    {
        const AZ::Transform& localOrWorld = GetParentEntityId() == InvalidNetEntityId ? worldTm : localTm;
        SetRotation(localOrWorld.GetRotation());
        SetTranslation(localOrWorld.GetTranslation());
        SetScale(localOrWorld.GetUniformScale());
    }

    void NetworkTransformComponentController::OnParentIdChangedEvent([[maybe_unused]] AZ::EntityId oldParent, AZ::EntityId newParent)
    {
        AZ::Entity* parentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newParent);
        if (parentEntity)
        {
            const ConstNetworkEntityHandle parentHandle(parentEntity, GetNetworkEntityTracker());
            if (parentHandle.Exists())
            {
                SetParentEntityId(parentHandle.GetNetEntityId());
            }
        }
    }

    void NetworkTransformComponentController::HandleMultiplayerTeleport(
        [[maybe_unused]] AzNetworking::IConnection* invokingConnection, const AZ::Vector3& teleportToPosition)
    {
        GetEntity()->GetTransform()->SetWorldTranslation(teleportToPosition);
        ModifyResetCount()++;
    }
}
