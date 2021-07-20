/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "SmoothStepGradientComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{

    bool SmoothStepGradientConfig::UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // From v0 to v1, The smooth step parameters were moved into a SmoothStep subclass.  This reads the old parameters 
        // into the subclass, removes the old parameters, then write out the subclass.
        if (classElement.GetVersion() == 0)
        {
            SmoothStep convertedSmoothStep;

            if (classElement.GetChildData(AZ_CRC("FalloffRange", 0x36c95aea), convertedSmoothStep.m_falloffRange))
            {
                classElement.RemoveElementByName(AZ_CRC("FalloffRange", 0x36c95aea));
            }
            if (classElement.GetChildData(AZ_CRC("FalloffStrength", 0x9ce36ed3), convertedSmoothStep.m_falloffStrength))
            {
                classElement.RemoveElementByName(AZ_CRC("FalloffStrength", 0x9ce36ed3));
            }
            if (classElement.GetChildData(AZ_CRC("FalloffMidpoint", 0x985aa54b), convertedSmoothStep.m_falloffMidpoint))
            {
                classElement.RemoveElementByName(AZ_CRC("FalloffMidpoint", 0x985aa54b));
            }

            classElement.AddElementWithData(context, "SmoothStep", convertedSmoothStep);
        }
        return true;
    }

    void SmoothStepGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SmoothStepGradientConfig, AZ::ComponentConfig>()
                ->Version(1, &SmoothStepGradientConfig::UpdateVersion)
                ->Field("SmoothStep", &SmoothStepGradientConfig::m_smoothStep)
                ->Field("Gradient", &SmoothStepGradientConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SmoothStepGradientConfig>(
                    "Smooth Step Gradient", "Smooth Step Gradient")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SmoothStepGradientConfig::m_smoothStep, "Smooth Step", "Parameters for controlling the smooth-step curve.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &SmoothStepGradientConfig::m_gradientSampler, "Gradient", "Input gradient whose values will be transformed.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SmoothStepGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("smoothStep", BehaviorValueProperty(&SmoothStepGradientConfig::m_smoothStep))
                ->Property("gradientSampler", BehaviorValueProperty(&SmoothStepGradientConfig::m_gradientSampler))
                ;
        }
    }

    void SmoothStepGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void SmoothStepGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void SmoothStepGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void SmoothStepGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        SmoothStepGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SmoothStepGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SmoothStepGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SmoothStepGradientComponentTypeId", BehaviorConstant(SmoothStepGradientComponentTypeId));

            behaviorContext->Class<SmoothStepGradientComponent>()
                ->RequestBus("SmoothStepGradientRequestBus")
                ->RequestBus("SmoothStepRequestBus")
                ;

            behaviorContext->EBus<SmoothStepGradientRequestBus>("SmoothStepGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetGradientSampler", &SmoothStepGradientRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    SmoothStepGradientComponent::SmoothStepGradientComponent(const SmoothStepGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SmoothStepGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        SmoothStepGradientRequestBus::Handler::BusConnect(GetEntityId());
        SmoothStepRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SmoothStepGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
        SmoothStepGradientRequestBus::Handler::BusDisconnect();
        SmoothStepRequestBus::Handler::BusDisconnect();
    }

    bool SmoothStepGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SmoothStepGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SmoothStepGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SmoothStepGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float SmoothStepGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float output = 0.0f;

        const float value = AZ::GetClamp(m_configuration.m_gradientSampler.GetValue(sampleParams), 0.0f, 1.0f);
        output = m_configuration.m_smoothStep.GetSmoothedValue(value);

        return output;
    }

    bool SmoothStepGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    float SmoothStepGradientComponent::GetFallOffRange() const
    {
        return m_configuration.m_smoothStep.m_falloffRange;
    }

    void SmoothStepGradientComponent::SetFallOffRange(float range)
    {
        m_configuration.m_smoothStep.m_falloffRange = range;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SmoothStepGradientComponent::GetFallOffStrength() const
    {
        return m_configuration.m_smoothStep.m_falloffStrength;
    }

    void SmoothStepGradientComponent::SetFallOffStrength(float strength)
    {
        m_configuration.m_smoothStep.m_falloffStrength = strength;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SmoothStepGradientComponent::GetFallOffMidpoint() const
    {
        return m_configuration.m_smoothStep.m_falloffMidpoint;
    }

    void SmoothStepGradientComponent::SetFallOffMidpoint(float midpoint)
    {
        m_configuration.m_smoothStep.m_falloffMidpoint = midpoint;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSampler& SmoothStepGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
