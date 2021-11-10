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
    }

    NetworkTransformComponent::NetworkTransformComponent()
        : m_entityPreRenderEventHandler([this](float deltaTime) { OnPreRender(deltaTime); })
        , m_entityCorrectionEventHandler([this]() { OnCorrection(); })
        , m_parentChangedEventHandler([this](NetEntityId parentId) { OnParentChanged(parentId); })
    {
        ;
    }

    void NetworkTransformComponent::OnInit()
    {
        ;
    }

    void NetworkTransformComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        GetNetBindComponent()->AddEntityPreRenderEventHandler(m_entityPreRenderEventHandler);
        GetNetBindComponent()->AddEntityCorrectionEventHandler(m_entityCorrectionEventHandler);
        ParentEntityIdAddEvent(m_parentChangedEventHandler);

        if (!HasController())
        {
            OnParentChanged(GetParentEntityId());
        }
    }

    void NetworkTransformComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void NetworkTransformComponent::OnPreRender([[maybe_unused]] float deltaTime)
    {
        if (!HasController())
        {
            AZ::Transform blendTransform;
            blendTransform.SetRotation(GetRotation());
            blendTransform.SetTranslation(GetTranslation());
            blendTransform.SetUniformScale(GetScale());

            const float blendFactor = GetMultiplayer()->GetCurrentBlendFactor();
            if (!AZ::IsClose(blendFactor, 1.0f))
            {
                AZ::Transform blendTransformPrevious;
                blendTransformPrevious.SetRotation(GetRotationPrevious());
                blendTransformPrevious.SetTranslation(GetTranslationPrevious());
                blendTransformPrevious.SetUniformScale(GetScalePrevious());

                if (!blendTransform.IsClose(blendTransformPrevious))
                {
                    blendTransform.SetRotation(blendTransformPrevious.GetRotation().Slerp(blendTransform.GetRotation(), blendFactor));
                    blendTransform.SetTranslation(
                        blendTransformPrevious.GetTranslation().Lerp(blendTransform.GetTranslation(), blendFactor));
                    blendTransform.SetUniformScale(
                        AZ::Lerp(blendTransformPrevious.GetUniformScale(), blendTransform.GetUniformScale(), blendFactor));
                }
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
        ;
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
}
