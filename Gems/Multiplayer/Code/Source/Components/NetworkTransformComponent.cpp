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
    void NetworkTransformComponent::NetworkTransformComponent::Reflect(AZ::ReflectContext* context)
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
        : m_rotationEventHandler([this](const AZ::Quaternion& rotation) { OnRotationChangedEvent(rotation); })
        , m_translationEventHandler([this](const AZ::Vector3& translation) { OnTranslationChangedEvent(translation); })
        , m_scaleEventHandler([this](float scale) { OnScaleChangedEvent(scale); })
        , m_resetCountEventHandler([this](const uint8_t&) { OnResetCountChangedEvent(); })
        , m_parentIdChangedEventHandler([this](NetEntityId newParent) { OnParentIdChangedEvent(newParent); })
        , m_entityPreRenderEventHandler([this](float deltaTime, float blendFactor) { OnPreRender(deltaTime, blendFactor); })
        , m_entityCorrectionEventHandler([this]() { OnCorrection(); })
    {
        ;
    }

    void NetworkTransformComponent::OnInit()
    {
        ;
    }

    void NetworkTransformComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        RotationAddEvent(m_rotationEventHandler);
        TranslationAddEvent(m_translationEventHandler);
        ScaleAddEvent(m_scaleEventHandler);
        ResetCountAddEvent(m_resetCountEventHandler);
        ParentEntityIdAddEvent(m_parentIdChangedEventHandler);
        if (GetNetBindComponent())
        {
            GetNetBindComponent()->AddEntityPreRenderEventHandler(m_entityPreRenderEventHandler);
            GetNetBindComponent()->AddEntityCorrectionEventHandler(m_entityCorrectionEventHandler);
        }

        // When coming into relevance, reset all blending factors so we don't interpolate to our start position
        OnResetCountChangedEvent();
    }

    void NetworkTransformComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void NetworkTransformComponent::OnRotationChangedEvent(const AZ::Quaternion& rotation)
    {
        m_previousTransform.SetRotation(m_targetTransform.GetRotation());
        m_targetTransform.SetRotation(rotation);
        UpdateTargetHostFrameId();
    }

    void NetworkTransformComponent::OnTranslationChangedEvent(const AZ::Vector3& translation)
    {
        m_previousTransform.SetTranslation(m_targetTransform.GetTranslation());
        m_targetTransform.SetTranslation(translation);
        UpdateTargetHostFrameId();
    }

    void NetworkTransformComponent::OnScaleChangedEvent(float scale)
    {
        m_previousTransform.SetUniformScale(m_targetTransform.GetUniformScale());
        m_targetTransform.SetUniformScale(scale);
        UpdateTargetHostFrameId();
    }

    void NetworkTransformComponent::OnResetCountChangedEvent()
    {
        OnParentIdChangedEvent(GetParentEntityId());

        m_targetTransform.SetRotation(GetRotation());
        m_targetTransform.SetTranslation(GetTranslation());
        m_targetTransform.SetUniformScale(GetScale());
        m_previousTransform = m_targetTransform;
    }

    void NetworkTransformComponent::OnParentIdChangedEvent([[maybe_unused]] NetEntityId newParent)
    {
        const ConstNetworkEntityHandle rootHandle = GetNetworkEntityManager()->GetEntity(newParent);
        if (rootHandle.Exists())
        {
            const AZ::EntityId parentEntityId = rootHandle.GetEntity()->GetId();
            if (AzFramework::TransformComponent* transformComponent = GetEntity()->FindComponent<AzFramework::TransformComponent>())
            {
                if (transformComponent->GetParentId() != parentEntityId)
                {
                    transformComponent->SetParent(parentEntityId);
                }
            }
        }
    }

    void NetworkTransformComponent::UpdateTargetHostFrameId()
    {
        const HostFrameId currentHostFrameId = Multiplayer::GetNetworkTime()->GetHostFrameId();
        if (currentHostFrameId > m_targetHostFrameId)
        {
            m_targetHostFrameId = currentHostFrameId;
        }
    }

    void NetworkTransformComponent::OnPreRender([[maybe_unused]] float deltaTime, float blendFactor)
    {
        if (!HasController())
        {
            AZ::Transform blendTransform;
            if (Multiplayer::GetNetworkTime() && Multiplayer::GetNetworkTime()->GetHostFrameId() > m_targetHostFrameId)
            {
                m_previousTransform = m_targetTransform;
                blendTransform = m_targetTransform;
            }
            else
            {
                blendTransform.SetRotation(m_previousTransform.GetRotation().Slerp(m_targetTransform.GetRotation(), blendFactor));
                blendTransform.SetTranslation(m_previousTransform.GetTranslation().Lerp(m_targetTransform.GetTranslation(), blendFactor));
                blendTransform.SetUniformScale(AZ::Lerp(m_previousTransform.GetUniformScale(), m_targetTransform.GetUniformScale(), blendFactor));
            }

            if (!GetTransformComponent()->GetWorldTM().IsClose(blendTransform))
            {
                GetTransformComponent()->SetWorldTM(blendTransform);
            }
        }
    }

    void NetworkTransformComponent::OnCorrection()
    {
        // Snap to latest
        OnResetCountChangedEvent();

        // Hard set the entities transform
        if (!GetTransformComponent()->GetWorldTM().IsClose(m_targetTransform))
        {
            GetTransformComponent()->SetWorldTM(m_targetTransform);
        }
    }


    NetworkTransformComponentController::NetworkTransformComponentController(NetworkTransformComponent& parent)
        : NetworkTransformComponentControllerBase(parent)
        , m_transformChangedHandler([this](const AZ::Transform&, const AZ::Transform& worldTm) { OnTransformChangedEvent(worldTm); })
        , m_parentIdChangedHandler([this](AZ::EntityId oldParent, AZ::EntityId newParent) { OnParentIdChangedEvent(oldParent, newParent); })
    {
        ;
    }

    void NetworkTransformComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        GetParent().GetTransformComponent()->BindTransformChangedEventHandler(m_transformChangedHandler);
        OnTransformChangedEvent(GetParent().GetTransformComponent()->GetWorldTM());

        GetParent().GetTransformComponent()->BindParentChangedEventHandler(m_parentIdChangedHandler);
        OnParentIdChangedEvent(AZ::EntityId(), GetParent().GetTransformComponent()->GetParentId());
    }

    void NetworkTransformComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void NetworkTransformComponentController::OnTransformChangedEvent(const AZ::Transform& worldTm)
    {
        SetRotation(worldTm.GetRotation());
        SetTranslation(worldTm.GetTranslation());
        SetScale(worldTm.GetUniformScale());
    }

    void NetworkTransformComponentController::OnParentIdChangedEvent([[maybe_unused]] AZ::EntityId oldParent, AZ::EntityId newParent)
    {
        AZ::Entity* parentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newParent);
        if (parentEntity)
        {
            const ConstNetworkEntityHandle parentHandle(parentEntity, GetNetworkEntityTracker());
            SetParentEntityId(parentHandle.GetNetEntityId());
        }
    }
}
