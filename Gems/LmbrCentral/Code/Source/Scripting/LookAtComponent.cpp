/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentral_precompiled.h"
#include "LookAtComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LmbrCentral
{

    //////////////////////////////////////////////////////////////////////////
    class BehaviorLookAtComponentNotificationBusHandler : public LookAtComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorLookAtComponentNotificationBusHandler, "{2C171B89-CE6A-4C53-A286-0E1236A61FA0}", AZ::SystemAllocator,
            OnTargetChanged);

        // Sent when the light is turned on.
        void OnTargetChanged(AZ::EntityId entityId) override
        {
            Call(FN_OnTargetChanged, entityId);
        }
    };


    //=========================================================================
    void LookAtComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LookAtComponent, AZ::Component>()
                ->Version(1)
                ->Field("Target", &LookAtComponent::m_targetId)
                ->Field("ForwardAxis", &LookAtComponent::m_forwardAxis)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LookAtComponentRequestBus>("LookAt", "LookAtRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay")
                ->Event("SetTarget", &LookAtComponentRequestBus::Events::SetTarget, "Set Target", { { { "Target", "The entity to look at" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the entity to look at")
                ->Event("SetTargetPosition", &LookAtComponentRequestBus::Events::SetTargetPosition, "Set Target Position", { { { "Position", "The position to look at" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets the target position to look at.")
                ->Event("SetAxis", &LookAtComponentRequestBus::Events::SetAxis, "Set Axis", { { { "Axis", "The forward axis to use as reference" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Specify the forward axis to use as reference for the look at")
                ;

            behaviorContext->EBus<LookAtComponentNotificationBus>("LookAtNotification", "LookAtComponentNotificationBus", "Notifications for the Look At Component")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay")
                ->Handler<BehaviorLookAtComponentNotificationBusHandler>();
        }
    }

    //=========================================================================
    void LookAtComponent::Activate()
    {
        LookAtComponentRequestBus::Handler::BusConnect(GetEntityId());

        if (m_targetId.IsValid())
        {
            AZ::EntityBus::Handler::BusConnect(m_targetId);
        }
    }

    //=========================================================================
    void LookAtComponent::Deactivate()
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();

        LookAtComponentRequestBus::Handler::BusDisconnect();

    }

    //=========================================================================
    void LookAtComponent::OnEntityActivated(const AZ::EntityId& /*entityId*/)
    {
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_targetId);
    }

    //=========================================================================
    void LookAtComponent::OnEntityDeactivated(const AZ::EntityId& /*entityId*/)
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
    }

    void LookAtComponent::SetTarget(AZ::EntityId targetEntity)
    {
        if (m_targetId.IsValid())
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
        }

        m_targetPosition = AZ::Vector3(0, 0, 0);
        m_targetId = targetEntity;

        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_targetId);

        RecalculateTransform();

        LookAtComponentNotificationBus::Broadcast(&LookAtComponentNotifications::OnTargetChanged, m_targetId);
    }

    void LookAtComponent::SetTargetPosition(const AZ::Vector3& targetPosition)
    {
        if (m_targetId.IsValid())
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
        }

        m_targetId.SetInvalid();

        m_targetPosition = targetPosition;

        RecalculateTransform();

        LookAtComponentNotificationBus::Broadcast(&LookAtComponentNotifications::OnTargetChanged, m_targetId);
    }

    void LookAtComponent::SetAxis(AZ::Transform::Axis axis)
    {
        m_forwardAxis = axis;

        RecalculateTransform();
    }

    //=========================================================================
    void LookAtComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        // See corresponding function in EditorLookAtComponent for comment.
        AZ::TickBus::Handler::BusConnect();
    }

    //=========================================================================
    void LookAtComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        RecalculateTransform();
        AZ::TickBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void LookAtComponent::RecalculateTransform()
    {
        AZ::Vector3 targetPosition = m_targetPosition;

        if (m_targetId.IsValid())
        {
            AZ::Transform targetTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(targetTM, m_targetId, &AZ::TransformBus::Events::GetWorldTM);

            targetPosition = targetTM.GetTranslation();
        }

        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(GetEntityId());
        {
            AZ::Transform currentTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(currentTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            AZ::Transform lookAtTransform = AZ::Transform::CreateLookAt(
                currentTM.GetTranslation(),
                targetPosition,
                m_forwardAxis
                );

            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformInterface::SetWorldTM, lookAtTransform);
        }
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
    }

} // namespace LmbrCentral
