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
#include "VisAreaComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>

namespace Visibility
{
    void VisAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ::Crc32("VisAreaService"));
    }

    void VisAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ::Crc32("TransformService"));
    }

    void VisAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VisAreaConfiguration>()
                ->Version(2, &VersionConverter)
                ->Field("m_Height", &VisAreaConfiguration::m_height)
                ->Field("m_DisplayFilled", &VisAreaConfiguration::m_displayFilled)
                ->Field("m_AffectedBySun", &VisAreaConfiguration::m_affectedBySun)
                ->Field("m_ViewDistRatio", &VisAreaConfiguration::m_viewDistRatio)
                ->Field("m_OceanIsVisible", &VisAreaConfiguration::m_oceanIsVisible)
                ->Field("m_vertexContainer", &VisAreaConfiguration::m_vertexContainer)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<VisAreaComponentRequestBus>("VisAreaComponentRequestBus")
                ->Event("GetHeight", &VisAreaComponentRequestBus::Events::GetHeight)
                ->VirtualProperty("Height", "GetHeight", nullptr)

                ->Event("GetDisplayFilled", &VisAreaComponentRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", nullptr)

                ->Event("GetAffectedBySun", &VisAreaComponentRequestBus::Events::GetAffectedBySun)
                ->VirtualProperty("AffectedBySun", "GetAffectedBySun", nullptr)

                ->Event("GetViewDistRatio", &VisAreaComponentRequestBus::Events::GetViewDistRatio)
                ->VirtualProperty("ViewDistRatio", "GetViewDistRatio", nullptr)

                ->Event("GetOceanIsVisible", &VisAreaComponentRequestBus::Events::GetOceanIsVisible)
                ->VirtualProperty("OceanIsVisible", "GetOceanIsVisible", nullptr)
                ;

            behaviorContext->Class<VisAreaComponent>()->RequestBus("VisAreaComponentRequestBus");
        }
    }

    bool VisAreaConfiguration::VersionConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Remove IgnoreSkyColor
        // - Remove IgnoreGI
        // - Remove SkyOnly
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("IgnoreSkyColor"));
            classElement.RemoveElementByName(AZ_CRC("IgnoreGI"));
            classElement.RemoveElementByName(AZ_CRC("SkyOnly"));
        }

        return true;
    }

    void VisAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VisAreaComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &VisAreaComponent::m_config)
                ;
        }

        VisAreaConfiguration::Reflect(context);
    }

    VisAreaComponent::VisAreaComponent(const VisAreaConfiguration& params)
        : m_config(params)
    {
    }
    
    void VisAreaComponent::Activate()
    {
        VisAreaComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void VisAreaComponent::Deactivate()
    {
        VisAreaComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    float VisAreaComponent::GetHeight()
    {
        return m_config.m_height;
    }

    bool VisAreaComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    bool VisAreaComponent::GetAffectedBySun()
    {
        return m_config.m_affectedBySun;
    }

    float VisAreaComponent::GetViewDistRatio()
    {
        return m_config.m_viewDistRatio;
    }

    bool VisAreaComponent::GetOceanIsVisible()
    {
        return m_config.m_oceanIsVisible;
    }
} //namespace Visibility