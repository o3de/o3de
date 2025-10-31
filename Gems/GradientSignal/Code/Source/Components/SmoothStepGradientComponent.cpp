/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/SmoothStepGradientComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{

    static bool SmoothStepGradientConfigUpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // From v0 to v1, The smooth step parameters were moved into a SmoothStep subclass.  This reads the old parameters
        // into the subclass, removes the old parameters, then write out the subclass.
        if (classElement.GetVersion() == 0)
        {
            SmoothStep convertedSmoothStep;

            if (classElement.GetChildData(AZ_CRC_CE("FalloffRange"), convertedSmoothStep.m_falloffRange))
            {
                classElement.RemoveElementByName(AZ_CRC_CE("FalloffRange"));
            }
            if (classElement.GetChildData(AZ_CRC_CE("FalloffStrength"), convertedSmoothStep.m_falloffStrength))
            {
                classElement.RemoveElementByName(AZ_CRC_CE("FalloffStrength"));
            }
            if (classElement.GetChildData(AZ_CRC_CE("FalloffMidpoint"), convertedSmoothStep.m_falloffMidpoint))
            {
                classElement.RemoveElementByName(AZ_CRC_CE("FalloffMidpoint"));
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
                ->Version(1, &SmoothStepGradientConfigUpdateVersion)
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
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void SmoothStepGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
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
        SmoothStepGradientRequestBus::Handler::BusConnect(GetEntityId());
        SmoothStepRequestBus::Handler::BusConnect(GetEntityId());

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SmoothStepGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
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
        AZStd::shared_lock lock(m_queryMutex);

        const float value = m_configuration.m_gradientSampler.GetValue(sampleParams);
        return m_configuration.m_smoothStep.GetSmoothedValue(value);
    }

    void SmoothStepGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        m_configuration.m_gradientSampler.GetValues(positions, outValues);
        m_configuration.m_smoothStep.GetSmoothedValues(outValues);
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
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_smoothStep.m_falloffRange = range;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SmoothStepGradientComponent::GetFallOffStrength() const
    {
        return m_configuration.m_smoothStep.m_falloffStrength;
    }

    void SmoothStepGradientComponent::SetFallOffStrength(float strength)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_smoothStep.m_falloffStrength = strength;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SmoothStepGradientComponent::GetFallOffMidpoint() const
    {
        return m_configuration.m_smoothStep.m_falloffMidpoint;
    }

    void SmoothStepGradientComponent::SetFallOffMidpoint(float midpoint)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_smoothStep.m_falloffMidpoint = midpoint;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSampler& SmoothStepGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
