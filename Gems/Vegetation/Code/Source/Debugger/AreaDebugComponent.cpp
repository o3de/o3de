/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debugger/AreaDebugComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <AzCore/Debug/Profiler.h>

namespace Vegetation
{
    namespace AreaDebugUtil
    {
        static bool UpdateVersion([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("PropagateDebug"));
                classElement.RemoveElementByName(AZ_CRC_CE("InheritDebug"));
            }
            return true;
        }
    }

    void AreaDebugConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaDebugConfig, AZ::ComponentConfig>()
                ->Version(1, &AreaDebugUtil::UpdateVersion)
                ->Field("DebugColor", &AreaDebugConfig::m_debugColor)
                ->Field("CubeSize", &AreaDebugConfig::m_debugCubeSize)
                ->Field("HideInDebug", &AreaDebugConfig::m_hideDebug)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaDebugConfig>(
                    "Vegetation Layer Debugger Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Color, &AreaDebugConfig::m_debugColor, "Debug Visualization Color", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaDebugConfig::m_debugCubeSize, "Debug Visualization Cube Size", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AreaDebugConfig::m_hideDebug, "Hide created instance in the Debug Visualization", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaDebugConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("DebugColor", BehaviorValueProperty(&AreaDebugConfig::m_debugColor))
                ->Property("DebugCubeSize", BehaviorValueProperty(&AreaDebugConfig::m_debugCubeSize))
                ->Property("HideInDebug", BehaviorValueProperty(&AreaDebugConfig::m_hideDebug))
                ;
        }
    }

    void AreaDebugComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaDebugService"));
    }

    void AreaDebugComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaDebugService"));
    }

    void AreaDebugComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void AreaDebugComponent::Reflect(AZ::ReflectContext* context)
    {
        AreaDebugConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaDebugComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AreaDebugComponent::m_configuration)
                ;
        }
    }

    AreaDebugComponent::AreaDebugComponent(const AreaDebugConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AreaDebugComponent::Activate()
    {
        ResetBlendedDebugDisplayData();
        AreaDebugBus::Handler::BusConnect(GetEntityId());
    }

    void AreaDebugComponent::Deactivate()
    {
        AreaDebugBus::Handler::BusDisconnect();
    }

    bool AreaDebugComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AreaDebugConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaDebugComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AreaDebugConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    AreaDebugDisplayData AreaDebugComponent::GetBaseDebugDisplayData() const
    {
        AreaDebugDisplayData data;
        data.m_instanceColor = m_configuration.m_debugColor;
        data.m_instanceSize = m_configuration.m_debugCubeSize;
        data.m_instanceRender = !m_configuration.m_hideDebug;
        return data;
    }

    void AreaDebugComponent::ResetBlendedDebugDisplayData()
    {
        m_hasBlendedDebugDisplayData = false;
        m_blendedDebugDisplayData = AreaDebugDisplayData();
    }

    void AreaDebugComponent::AddBlendedDebugDisplayData(const AreaDebugDisplayData& data)
    {
        m_hasBlendedDebugDisplayData = true;

        //do not render if any render flag is disabled
        m_blendedDebugDisplayData.m_instanceRender = m_blendedDebugDisplayData.m_instanceRender && data.m_instanceRender;

        //performing a multiply/modulate color blend.
        m_blendedDebugDisplayData.m_instanceColor *= data.m_instanceColor;

        //setting size to the last size added
        m_blendedDebugDisplayData.m_instanceSize = data.m_instanceSize;
    }

    AreaDebugDisplayData AreaDebugComponent::GetBlendedDebugDisplayData() const
    {
        return m_hasBlendedDebugDisplayData ? m_blendedDebugDisplayData : GetBaseDebugDisplayData();
    }
}
