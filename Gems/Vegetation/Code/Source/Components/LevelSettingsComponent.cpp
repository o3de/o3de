/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LevelSettingsComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>

namespace Vegetation
{
    //////////////////////////////////////////////////////////////////////////
    // LevelSettingsConfig

    void LevelSettingsConfig::Reflect(AZ::ReflectContext* context)
    {
        InstanceSystemConfig::Reflect(context);
        AreaSystemConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LevelSettingsConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("AreaSystemConfig", &LevelSettingsConfig::m_areaSystemConfig)
                ->Field("InstanceSystemConfig", &LevelSettingsConfig::m_instanceSystemConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LevelSettingsConfig>(
                    "Vegetation System Settings", "The vegetation system settings for this level/map.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &LevelSettingsConfig::m_areaSystemConfig, "Area System Settings", "Area management settings.")
                    ->DataElement(0, &LevelSettingsConfig::m_instanceSystemConfig, "Instance System Settings", "Instance management settings.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LevelSettingsConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("areaSystemConfig", BehaviorValueProperty(&LevelSettingsConfig::m_areaSystemConfig))
                ->Property("instanceSystemConfig", BehaviorValueProperty(&LevelSettingsConfig::m_instanceSystemConfig))
                ;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // LevelSettingsComponent

    void LevelSettingsComponent::Reflect(AZ::ReflectContext* context)
    {
        LevelSettingsConfig::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<LevelSettingsComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &LevelSettingsComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("LevelSettingsComponentTypeId", BehaviorConstant(LevelSettingsComponentTypeId));

            behaviorContext->Class<LevelSettingsComponent>()->RequestBus("LevelSettingsRequestBus");

            behaviorContext->EBus<LevelSettingsRequestBus>("LevelSettingsRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAreaSystemConfig", &LevelSettingsRequestBus::Events::GetAreaSystemConfig)
                ->Event("GetInstanceSystemConfig", &LevelSettingsRequestBus::Events::GetInstanceSystemConfig)
                ;
        }
    }

    void LevelSettingsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationLevelSettingsService"));
    }

    void LevelSettingsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationLevelSettingsService"));
    }

    LevelSettingsComponent::LevelSettingsComponent(const LevelSettingsConfig& configuration)
        : m_configuration(configuration)
    {
    }

    LevelSettingsComponent::~LevelSettingsComponent()
    {
    }

    void LevelSettingsComponent::Init()
    {
    }

    void LevelSettingsComponent::Activate()
    {
        SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::GetSystemConfig, &m_previousAreaSystemConfig);
        SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::GetSystemConfig, &m_previousInstanceSystemConfig);

        LevelSettingsRequestBus::Handler::BusConnect(GetEntityId());

        m_active = true;

        UpdateSystemConfig();
    }

    void LevelSettingsComponent::Deactivate()
    {
        SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::UpdateSystemConfig, &m_previousAreaSystemConfig);
        SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::UpdateSystemConfig, &m_previousInstanceSystemConfig);
        LevelSettingsRequestBus::Handler::BusDisconnect();
        m_active = false;
    }
    
    bool LevelSettingsComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const LevelSettingsConfig*>(baseConfig))
        {
            m_configuration = *config;
            UpdateSystemConfig();
            return true;
        }
        return false;
    }

    bool LevelSettingsComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<LevelSettingsConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void LevelSettingsComponent::UpdateSystemConfig()
    {
        // Need to check if active to avoid writing our data to the component while inactive, which will cause our save of previous
        // configs to be incorrect in Activate
        if (m_active)
        {
            SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::UpdateSystemConfig, &m_configuration.m_areaSystemConfig);
            SystemConfigurationRequestBus::Broadcast(&SystemConfigurationRequestBus::Events::UpdateSystemConfig, &m_configuration.m_instanceSystemConfig);
        }
    }

    AreaSystemConfig* LevelSettingsComponent::GetAreaSystemConfig()
    {
        return &(m_configuration.m_areaSystemConfig);
    }

    InstanceSystemConfig* LevelSettingsComponent::GetInstanceSystemConfig()
    {
        return &(m_configuration.m_instanceSystemConfig);
    }
} // namespace Vegetation
