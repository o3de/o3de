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

#include "Visibility_precompiled.h"
#include "PortalComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace Visibility
{
    void PortalComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ::Crc32("PortalService"));
    }

    void PortalComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ::Crc32("TransformService"));
    }

    void PortalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PortalConfiguration>()
                ->Version(1)
                ->Field("Height", &PortalConfiguration::m_height)
                ->Field("DisplayFilled", &PortalConfiguration::m_displayFilled)
                ->Field("AffectedBySun", &PortalConfiguration::m_affectedBySun)
                ->Field("ViewDistRatio", &PortalConfiguration::m_viewDistRatio)
                ->Field("SkyOnly", &PortalConfiguration::m_skyOnly)
                ->Field("OceanIsVisible", &PortalConfiguration::m_oceanIsVisible)
                ->Field("UseDeepness", &PortalConfiguration::m_useDeepness)
                ->Field("DoubleSide", &PortalConfiguration::m_doubleSide)
                ->Field("LightBlending", &PortalConfiguration::m_lightBlending)
                ->Field("LightBlendValue", &PortalConfiguration::m_lightBlendValue)
                ->Field("vertices", &PortalConfiguration::m_vertices)
                ;
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PortalRequestBus>("PortalRequestBus")
                ->Event("GetHeight", &PortalRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", nullptr)

                ->Event("GetDisplayFilled", &PortalRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", nullptr)

                ->Event("GetAffectedBySun", &PortalRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", nullptr)

                ->Event("GetViewDistRatio", &PortalRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", nullptr)

                ->Event("GetSkyOnly", &PortalRequestBus::Events::GetSkyOnly)
                ->VirtualProperty("SkyOnly", "GetSkyOnly", nullptr)

                ->Event("GetOceanIsVisible", &PortalRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", nullptr)

                ->Event("GetUseDeepness", &PortalRequestBus::Events::GetUseDeepness)
                ->VirtualProperty("UseDeepness", "GetUseDeepness", nullptr)

                ->Event("GetDoubleSide", &PortalRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", nullptr)

                ->Event("GetLightBlending", &PortalRequestBus::Events::GetLightBlending)
                ->VirtualProperty("LightBlending", "GetLightBlending", nullptr)

                ->Event("GetLightBlendValue", &PortalRequestBus::Events::GetLightBlendValue)
                ->VirtualProperty("LightBlendValue", "GetLightBlendValue", nullptr)
                ;

            behaviorContext->Class<PortalComponent>()->RequestBus("PortalRequestBus");
        }
    }

    bool PortalConfiguration::VersionConverter(
        [[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Remove IgnoreSkyColor
        // - Remove IgnoreGI
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("IgnoreSkyColor"));
            classElement.RemoveElementByName(AZ_CRC("IgnoreGI"));
        }

        return true;
    }

    void PortalComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PortalComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &PortalComponent::m_config)
                ;
        }

        PortalConfiguration::Reflect(context);
    }

    PortalComponent::PortalComponent(const PortalConfiguration& params)
        : m_config(params)
    {
    }

    void PortalComponent::Activate()
    {
        PortalRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PortalComponent::Deactivate()
    {
        PortalRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    float PortalComponent::GetHeight()
    {
        return m_config.m_height;
    }

    bool PortalComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    bool PortalComponent::GetAffectedBySun()
    {
        return m_config.m_affectedBySun;
    }

    float PortalComponent::GetViewDistRatio()
    {
        return m_config.m_viewDistRatio;
    }

    bool PortalComponent::GetSkyOnly()
    {
        return m_config.m_skyOnly;
    }

    bool PortalComponent::GetOceanIsVisible()
    {
        return m_config.m_oceanIsVisible;
    }

    bool PortalComponent::GetUseDeepness()
    {
        return m_config.m_useDeepness;
    }

    bool PortalComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

    bool PortalComponent::GetLightBlending()
    {
        return m_config.m_lightBlending;
    }

    float PortalComponent::GetLightBlendValue()
    {
        return m_config.m_lightBlendValue;
    }

} //namespace Visibility