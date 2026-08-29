// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <Components/${SanitizedCppName}ComponentController.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${SanitizedCppName}
{
    ${SanitizedCppName}ComponentController::${SanitizedCppName}ComponentController(const ${SanitizedCppName}ComponentConfig& config)
        : m_config(config)
    {
    }

    void ${SanitizedCppName}ComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}ComponentConfig>()
                ->Version(1)
                ->Field("SpeedMultiplier", &${SanitizedCppName}ComponentConfig::m_speedMultiplier)
                ->Field("IsEnabled", &${SanitizedCppName}ComponentConfig::m_isEnabled)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${SanitizedCppName}ComponentConfig>("${SanitizedCppName} Config", "Configuration parameters for gameplay logic.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &${SanitizedCppName}ComponentConfig::m_speedMultiplier, "Speed Multiplier", "Speed factor for gameplay movement.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &${SanitizedCppName}ComponentConfig::m_isEnabled, "Is Enabled", "Toggles component logic.")
                    ;
            }
        }
    }

    void ${SanitizedCppName}ComponentController::Reflect(AZ::ReflectContext* context)
    {
        ${SanitizedCppName}ComponentConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}ComponentController>()
                ->Version(1)
                ->Field("Config", &${SanitizedCppName}ComponentController::m_config)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<${SanitizedCppName}RequestBus>("${SanitizedCppName}RequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay/${SanitizedCppName}")
                ->Event("GetSpeedMultiplier", &${SanitizedCppName}RequestBus::Events::GetSpeedMultiplier)
                ->Event("SetSpeedMultiplier", &${SanitizedCppName}RequestBus::Events::SetSpeedMultiplier)
                ->Event("IsEnabled", &${SanitizedCppName}RequestBus::Events::IsEnabled)
                ->Event("SetEnabled", &${SanitizedCppName}RequestBus::Events::SetEnabled)
                ->Event("TriggerGameplayAction", &${SanitizedCppName}RequestBus::Events::TriggerGameplayAction)
                ;

            behaviorContext->EBus<${SanitizedCppName}NotificationBus>("${SanitizedCppName}NotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay/${SanitizedCppName}")
                ->Event("OnGameplayActionTriggered", &${SanitizedCppName}NotificationBus::Events::OnGameplayActionTriggered)
                ->Event("OnSpeedMultiplierChanged", &${SanitizedCppName}NotificationBus::Events::OnSpeedMultiplierChanged)
                ->Event("OnStateChanged", &${SanitizedCppName}NotificationBus::Events::OnStateChanged)
                ;
        }
    }

    void ${SanitizedCppName}ComponentController::Init()
    {
    }

    void ${SanitizedCppName}ComponentController::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        ${SanitizedCppName}RequestBus::Handler::BusConnect(m_entityId);

        if (m_config.m_isEnabled)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void ${SanitizedCppName}ComponentController::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect();
    }

    void ${SanitizedCppName}ComponentController::SetConfiguration(const ${SanitizedCppName}ComponentConfig& config)
    {
        m_config = config;
    }

    const ${SanitizedCppName}ComponentConfig& ${SanitizedCppName}ComponentController::GetConfiguration() const
    {
        return m_config;
    }

    float ${SanitizedCppName}ComponentController::GetSpeedMultiplier() const
    {
        return m_config.m_speedMultiplier;
    }

    void ${SanitizedCppName}ComponentController::SetSpeedMultiplier(float speed)
    {
        m_config.m_speedMultiplier = speed;
        ${SanitizedCppName}NotificationBus::Event(m_entityId, &${SanitizedCppName}NotificationBus::Events::OnSpeedMultiplierChanged, speed);
    }

    bool ${SanitizedCppName}ComponentController::IsEnabled() const
    {
        return m_config.m_isEnabled;
    }

    void ${SanitizedCppName}ComponentController::SetEnabled(bool enabled)
    {
        if (m_config.m_isEnabled != enabled)
        {
            m_config.m_isEnabled = enabled;
            if (m_config.m_isEnabled)
            {
                AZ::TickBus::Handler::BusConnect();
            }
            else
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
            ${SanitizedCppName}NotificationBus::Event(m_entityId, &${SanitizedCppName}NotificationBus::Events::OnStateChanged, enabled);
        }
    }

    void ${SanitizedCppName}ComponentController::TriggerGameplayAction()
    {
        ${SanitizedCppName}NotificationBus::Event(m_entityId, &${SanitizedCppName}NotificationBus::Events::OnGameplayActionTriggered);
    }

    void ${SanitizedCppName}ComponentController::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }
} // namespace ${SanitizedCppName}
