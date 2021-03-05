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
#include "OccluderAreaComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <MathConversion.h>

namespace Visibility
{
    void OccluderAreaComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides)
    {
        provides.push_back(AZ_CRC("OccluderAreaService"));
    }
    void OccluderAreaComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires)
    {
        requires.push_back(AZ_CRC("TransformService"));
    }

    void OccluderAreaConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OccluderAreaConfiguration>()
                ->Version(1)
                ->Field("DisplayFilled", &OccluderAreaConfiguration::m_displayFilled)
                ->Field("CullDistRatio", &OccluderAreaConfiguration::m_cullDistRatio)
                ->Field("UseInIndoors", &OccluderAreaConfiguration::m_useInIndoors)
                ->Field("DoubleSide", &OccluderAreaConfiguration::m_doubleSide)
                ->Field("vertices", &OccluderAreaConfiguration::m_vertices)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<OccluderAreaRequestBus>("OccluderAreaRequestBus")
                ->Event("GetDisplayFilled", &OccluderAreaRequestBus::Events::GetDisplayFilled)
                ->VirtualProperty("DisplayFilled", "GetDisplayFilled", nullptr)

                ->Event("GetCullDistRatio", &OccluderAreaRequestBus::Events::GetCullDistRatio)
                ->VirtualProperty("CullDistRatio", "GetCullDistRatio", nullptr)

                ->Event("GetUseInIndoors", &OccluderAreaRequestBus::Events::GetUseInIndoors)
                ->VirtualProperty("UseInIndoors", "GetUseInIndoors", nullptr)

                ->Event("GetDoubleSide", &OccluderAreaRequestBus::Events::GetDoubleSide)
                ->VirtualProperty("DoubleSide", "GetDoubleSide", nullptr)
                ;

            behaviorContext->Class<OccluderAreaComponent>()->RequestBus("OccluderAreaRequestBus");
        }
    }

    void OccluderAreaComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<OccluderAreaComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_config", &OccluderAreaComponent::m_config)
                ;
        }

        OccluderAreaConfiguration::Reflect(context);
    }

    OccluderAreaComponent::OccluderAreaComponent(const OccluderAreaConfiguration& params)
        : m_config(params)
    {
    }

    void OccluderAreaComponent::Activate()
    {
        OccluderAreaRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void OccluderAreaComponent::Deactivate()
    {
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        OccluderAreaRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    bool OccluderAreaComponent::GetDisplayFilled()
    {
        return m_config.m_displayFilled;
    }

    float OccluderAreaComponent::GetCullDistRatio()
    {
        return m_config.m_cullDistRatio;
    }

    bool OccluderAreaComponent::GetUseInIndoors()
    {
        return m_config.m_useInIndoors;
    }

    bool OccluderAreaComponent::GetDoubleSide()
    {
        return m_config.m_doubleSide;
    }

} //namespace Visibility