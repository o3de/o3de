/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
            OnTargetChanged, OnEnabledChanged);

        // Sent when the target entityId changes
        void OnTargetChanged(AZ::EntityId entityId) override
        {
            Call(FN_OnTargetChanged, entityId);
        }

        // Sent when Look At is enabled or disabled
        void OnEnabledChanged(bool enabled) override
        {
            Call(FN_OnEnabledChanged, enabled);
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
                ->Field("Strength", &LookAtComponent::m_strength)
                ->Field("FixatePitch", &LookAtComponent::m_fixatePitch)
                ->Field("FixateRoll", &LookAtComponent::m_fixateRoll)
                ->Field("FixateYaw", &LookAtComponent::m_fixateYaw)
                ->Field("Enabled", &LookAtComponent::m_enabled)
                ->Field("ApplyLookAtTransform", &LookAtComponent::m_applyLookAtTransform)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LookAtComponentRequestBus>("LookAt", "LookAtRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay")
                ->Event("GetTarget", &LookAtComponentRequestBus::Events::GetTarget, "Get Target")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get the entity being looked at")
                ->Event("SetTarget", &LookAtComponentRequestBus::Events::SetTarget, "Set Target", { { { "Target", "The entity to look at" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the entity to look at")
                ->Event("GetTargetPosition", &LookAtComponentRequestBus::Events::GetTargetPosition, "Get Target Position")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get the target position being looked at.")
                ->Event("SetTargetPosition", &LookAtComponentRequestBus::Events::SetTargetPosition, "Set Target Position", { { { "Position", "The position to look at" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the target position to look at.")
                ->Event("GetLookAtTransform", &LookAtComponentRequestBus::Events::GetLookAtTransform, "Get Look At Transform")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get the target Look At transform.")
                ->Event("GetAxis", &LookAtComponentRequestBus::Events::GetAxis, "Get Axis")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get the forward axis being used as reference for the look at")
                ->Event("SetAxis", &LookAtComponentRequestBus::Events::SetAxis, "Set Axis", { { { "Axis", "The forward axis to use as reference" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the forward axis to use as reference for the look at")
                ->Event("GetStrength", &LookAtComponentRequestBus::Events::GetStrength, "Get Strength")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get the strength / quickness of the look at rotation")
                ->Event("SetStrength", &LookAtComponentRequestBus::Events::SetStrength, "Set Strength", { { { "Strength", "Determines how quickly the rotation is performed and how much it resists changes in the rotation" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the strength / quickness of the look at rotation")
                ->Event("GetFixatePitch", &LookAtComponentRequestBus::Events::GetFixatePitch, "Get Fixate Pitch")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get whether the pitch is fixated")
                ->Event("SetFixatePitch", &LookAtComponentRequestBus::Events::SetFixatePitch, "Set Fixate Pitch", { { { "Fixate Pitch", "Whether the pitch is fixated towards the Look At entity / point" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set whether the pitch is fixated")
                ->Event("GetFixateRoll", &LookAtComponentRequestBus::Events::GetFixateRoll, "Get Fixate Roll")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get whether the roll is fixated")
                ->Event("SetFixateRoll", &LookAtComponentRequestBus::Events::SetFixateRoll, "Set Fixate Roll", { { { "Fixate Roll", "Whether the roll is fixated towards the Look At entity / point" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set whether the roll is fixated")
                ->Event("GetFixateYaw", &LookAtComponentRequestBus::Events::GetFixateYaw, "Get Fixate Yaw")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get whether the yaw is fixated")
                ->Event("SetFixateYaw", &LookAtComponentRequestBus::Events::SetFixateYaw, "Set Fixate Yaw", { { { "Fixate Yaw", "Whether the yaw is fixated towards the Look At entity / point" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set whether the yaw is fixated")
                ->Event("GetEnabled", &LookAtComponentRequestBus::Events::GetEnabled, "Get Enabled")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get whether the Look At component is enabled or disabled")
                ->Event("SetEnabled", &LookAtComponentRequestBus::Events::SetEnabled, "Set Enabled", { { { "Enabled", "Whether the Look At component is enabled or disabled" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set whether the Look At component is enabled or disabled")
                ->Event("GetApplyLookAtTransform", &LookAtComponentRequestBus::Events::GetApplyLookAtTransform, "Get Apply Look At Transform")
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Get whether the Look At transform is applied")
                ->Event("SetApplyLookAtTransform", &LookAtComponentRequestBus::Events::SetApplyLookAtTransform, "Set Apply Look At Transform", { { { "Enabled", "Whether the Look At transform is applied when calculated" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set whether the Look At transform is applied when the component is enabled")
                ;
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

    AZ::EntityId LookAtComponent::GetTarget() const
    {
        return m_targetId;
    }

    void LookAtComponent::SetTarget(const AZ::EntityId& targetEntity)
    {
        if (m_targetId.IsValid())
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
        }

        m_targetPosition = AZ::Vector3(0, 0, 0);
        m_targetId = targetEntity;

        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_targetId);

        RecalculateTransform(0.f);

        LookAtComponentNotificationBus::Broadcast(&LookAtComponentNotifications::OnTargetChanged, m_targetId);
    }

    AZ::Vector3 LookAtComponent::GetTargetPosition() const
    {
        return m_targetPosition;
    }

    void LookAtComponent::SetTargetPosition(const AZ::Vector3& targetPosition)
    {
        if (m_targetId.IsValid())
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
        }

        m_targetId.SetInvalid();

        m_targetPosition = targetPosition;

        RecalculateTransform(0.f);

        LookAtComponentNotificationBus::Broadcast(&LookAtComponentNotifications::OnTargetChanged, m_targetId);
    }

    AZ::Transform LookAtComponent::GetLookAtTransform() const
    {
        return m_lookAtTransform;
    }

    AZ::Transform::Axis LookAtComponent::GetAxis() const
    {
        return m_forwardAxis;
    }

    void LookAtComponent::SetAxis(const AZ::Transform::Axis axis)
    {
        m_forwardAxis = axis;

        RecalculateTransform(0.f);
    }

    float LookAtComponent::GetStrength() const
    {
        return m_strength;
    }

    void LookAtComponent::SetStrength(const float strength)
    {
        m_strength = AZ::GetClamp(strength, 0.f, 1.f);
    }

    bool LookAtComponent::GetFixatePitch() const
    {
        return m_fixatePitch;
    }

    void LookAtComponent::SetFixatePitch(const bool fixatePitch)
    {
        m_fixatePitch = fixatePitch;
    }

    bool LookAtComponent::GetFixateRoll() const
    {
        return m_fixateRoll;
    }

    void LookAtComponent::SetFixateRoll(const bool fixateRoll)
    {
        m_fixateRoll = fixateRoll;
    }

    bool LookAtComponent::GetFixateYaw() const
    {
        return m_fixateYaw;
    }

    void LookAtComponent::SetFixateYaw(const bool fixateYaw)
    {
        m_fixateYaw = fixateYaw;
    }

    bool LookAtComponent::GetEnabled() const
    {
        return m_enabled;
    }

    void LookAtComponent::SetEnabled(const bool enabled)
    {
        if (m_enabled != enabled)
        {
            LookAtComponentNotificationBus::Broadcast(&LookAtComponentNotifications::OnEnabledChanged, m_enabled);
        }
        m_enabled = enabled;
        AZ::TransformBus::EventResult(m_lastDiscreteTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        if (m_enabled)
        {
            RecalculateTransform(0.f);
        }
    }

    bool LookAtComponent::GetApplyLookAtTransform() const
    {
        return m_applyLookAtTransform;
    }

    void LookAtComponent::SetApplyLookAtTransform(const bool applyLookAtTransform)
    {
        m_applyLookAtTransform = applyLookAtTransform;
        if (m_applyLookAtTransform)
        {
            RecalculateTransform(0.f);
        }
    }

    //=========================================================================
    void LookAtComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        // See corresponding function in EditorLookAtComponent for comment.
        AZ::TickBus::Handler::BusConnect();
    }

    //=========================================================================
    void LookAtComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        RecalculateTransform(deltaTime);
        AZ::TickBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void LookAtComponent::RecalculateTransform(const float deltaTime)
    {
        if (!m_enabled)
        {
            return;
        }

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

            m_lookAtTransform = AZ::Transform::CreateLookAt(
                currentTM.GetTranslation(),
                targetPosition,
                m_forwardAxis
                );
            m_lookAtTransform.SetUniformScale(currentTM.GetUniformScale());

            // Only apply the strength / slerp when it's not at it's not maxed out to 1
            if (m_strength < 1.f)
            {
                // Normalize the strength / slerp calculation to 30FPS
                constexpr float normalizedFrameRate = 30.f;

                if (deltaTime < 1.f / normalizedFrameRate)
                {
                    m_accumulatedDeltaTime += deltaTime;
                    if (m_accumulatedDeltaTime >= 1.f / normalizedFrameRate)
                    {
                        m_lastDiscreteTM = currentTM;
                        m_accumulatedDeltaTime -= 1.f / normalizedFrameRate;
                    }
                }
                else
                {
                    // When the frame rate is below 30FPS, simply use deltaTime
                    m_lastDiscreteTM = currentTM;
                    m_accumulatedDeltaTime = deltaTime;
                }

                const AZ::Quaternion targetRotation = m_lastDiscreteTM.GetRotation().Slerp(m_lookAtTransform.GetRotation(), m_accumulatedDeltaTime * normalizedFrameRate * m_strength);

                m_lookAtTransform.SetRotation(targetRotation);
            }

            if (!m_fixatePitch)
            {
                m_lookAtTransform.SetFromEulerRadians(AZ::Vector3(currentTM.GetRotation().GetEulerRadians().GetX(), m_lookAtTransform.GetRotation().GetEulerRadians().GetY(), m_lookAtTransform.GetRotation().GetEulerRadians().GetZ()));
            }
            if (!m_fixateRoll)
            {
                m_lookAtTransform.SetFromEulerRadians(AZ::Vector3(m_lookAtTransform.GetRotation().GetEulerRadians().GetX(), currentTM.GetRotation().GetEulerRadians().GetY(), m_lookAtTransform.GetRotation().GetEulerRadians().GetZ()));
            }
            if (!m_fixateYaw)
            {
                m_lookAtTransform.SetFromEulerRadians(AZ::Vector3(m_lookAtTransform.GetRotation().GetEulerRadians().GetX(), m_lookAtTransform.GetRotation().GetEulerRadians().GetY(), currentTM.GetRotation().GetEulerRadians().GetZ()));
            }

            m_lookAtTransform.SetTranslation(currentTM.GetTranslation());

            // Only set the transform when set to do so
            if (m_applyLookAtTransform)
            {
                AZ::TransformBus::Event(GetEntityId(), &AZ::TransformInterface::SetWorldTM, m_lookAtTransform);
            }
        }
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
    }

} // namespace LmbrCentral
