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
    }

    void NetworkTransformComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void NetworkTransformComponent::OnRotationChangedEvent(const AZ::Quaternion& rotation)
    {
        AZ::Transform worldTm = GetTransformComponent()->GetWorldTM();
        worldTm.SetRotation(rotation);
        GetTransformComponent()->SetWorldTM(worldTm);
    }

    void NetworkTransformComponent::OnTranslationChangedEvent(const AZ::Vector3& translation)
    {
        AZ::Transform worldTm = GetTransformComponent()->GetWorldTM();
        worldTm.SetTranslation(translation);
        GetTransformComponent()->SetWorldTM(worldTm);
    }

    void NetworkTransformComponent::OnScaleChangedEvent(float scale)
    {
        AZ::Transform worldTm = GetTransformComponent()->GetWorldTM();
        worldTm.SetUniformScale(scale);
        GetTransformComponent()->SetWorldTM(worldTm);
    }


    NetworkTransformComponentController::NetworkTransformComponentController(NetworkTransformComponent& parent)
        : NetworkTransformComponentControllerBase(parent)
        , m_transformChangedHandler([this](const AZ::Transform&, const AZ::Transform& worldTm) { OnTransformChangedEvent(worldTm); })
    {
        ;
    }

    void NetworkTransformComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        GetParent().GetTransformComponent()->BindTransformChangedEventHandler(m_transformChangedHandler);
        OnTransformChangedEvent(GetParent().GetTransformComponent()->GetWorldTM());
    }

    void NetworkTransformComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void NetworkTransformComponentController::OnTransformChangedEvent(const AZ::Transform& worldTm)
    {
        if (IsAuthority())
        {
            SetRotation(worldTm.GetRotation());
            SetTranslation(worldTm.GetTranslation());
            SetScale(worldTm.GetUniformScale());
        }
    }
}
