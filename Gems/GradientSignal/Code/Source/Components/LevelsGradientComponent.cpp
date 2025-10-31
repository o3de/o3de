/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/LevelsGradientComponent.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    void LevelsGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LevelsGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("InputMid", &LevelsGradientConfig::m_inputMid)
                ->Field("InputMin", &LevelsGradientConfig::m_inputMin)
                ->Field("InputMax", &LevelsGradientConfig::m_inputMax)
                ->Field("OutputMin", &LevelsGradientConfig::m_outputMin)
                ->Field("OutputMax", &LevelsGradientConfig::m_outputMax)
                ->Field("Gradient", &LevelsGradientConfig::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<LevelsGradientConfig>(
                    "Levels Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LevelsGradientConfig::m_inputMid, "Input Mid", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LevelsGradientConfig::m_inputMin, "Input Min", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LevelsGradientConfig::m_inputMax, "Input Max", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LevelsGradientConfig::m_outputMin, "Output Min", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &LevelsGradientConfig::m_outputMax, "Output Max", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &LevelsGradientConfig::m_gradientSampler, "Gradient", "Input gradient whose values will be transformed in relation to threshold.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<LevelsGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("inputMid", BehaviorValueProperty(&LevelsGradientConfig::m_inputMid))
                ->Property("inputMin", BehaviorValueProperty(&LevelsGradientConfig::m_inputMin))
                ->Property("inputMax", BehaviorValueProperty(&LevelsGradientConfig::m_inputMax))
                ->Property("outputMin", BehaviorValueProperty(&LevelsGradientConfig::m_outputMin))
                ->Property("outputMax", BehaviorValueProperty(&LevelsGradientConfig::m_outputMax))
                ->Property("gradientSampler", BehaviorValueProperty(&LevelsGradientConfig::m_gradientSampler))
                ;
        }
    }

    void LevelsGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void LevelsGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void LevelsGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void LevelsGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        LevelsGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<LevelsGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &LevelsGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("LevelsGradientComponentTypeId", BehaviorConstant(LevelsGradientComponentTypeId));

            behaviorContext->Class<LevelsGradientComponent>()->RequestBus("LevelsGradientRequestBus");

            behaviorContext->EBus<LevelsGradientRequestBus>("LevelsGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetInputMin", &LevelsGradientRequestBus::Events::GetInputMin)
                ->Event("SetInputMin", &LevelsGradientRequestBus::Events::SetInputMin)
                ->VirtualProperty("InputMin", "GetInputMin", "SetInputMin")
                ->Event("GetInputMid", &LevelsGradientRequestBus::Events::GetInputMid)
                ->Event("SetInputMid", &LevelsGradientRequestBus::Events::SetInputMid)
                ->VirtualProperty("InputMid", "GetInputMid", "SetInputMid")
                ->Event("GetInputMax", &LevelsGradientRequestBus::Events::GetInputMax)
                ->Event("SetInputMax", &LevelsGradientRequestBus::Events::SetInputMax)
                ->VirtualProperty("InputMax", "GetInputMax", "SetInputMax")
                ->Event("GetOutputMin", &LevelsGradientRequestBus::Events::GetOutputMin)
                ->Event("SetOutputMin", &LevelsGradientRequestBus::Events::SetOutputMin)
                ->VirtualProperty("OutputMin", "GetOutputMin", "SetOutputMin")
                ->Event("GetOutputMax", &LevelsGradientRequestBus::Events::GetOutputMax)
                ->Event("SetOutputMax", &LevelsGradientRequestBus::Events::SetOutputMax)
                ->VirtualProperty("PatternType", "GetOutputMax", "SetOutputMax")
                ->Event("GetGradientSampler", &LevelsGradientRequestBus::Events::GetGradientSampler)
                ;
        }
    }

    LevelsGradientComponent::LevelsGradientComponent(const LevelsGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void LevelsGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);
        LevelsGradientRequestBus::Handler::BusConnect(GetEntityId());

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void LevelsGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        LevelsGradientRequestBus::Handler::BusDisconnect();
    }

    bool LevelsGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const LevelsGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool LevelsGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<LevelsGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float LevelsGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        float output = 0.0f;

        output = GetLevels(
            m_configuration.m_gradientSampler.GetValue(sampleParams),
            m_configuration.m_inputMid,
            m_configuration.m_inputMin,
            m_configuration.m_inputMax,
            m_configuration.m_outputMin,
            m_configuration.m_outputMax);

        return output;
    }

    void LevelsGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        m_configuration.m_gradientSampler.GetValues(positions, outValues);

        GetLevels(outValues, 
                m_configuration.m_inputMid, m_configuration.m_inputMin, m_configuration.m_inputMax,
                m_configuration.m_outputMin, m_configuration.m_outputMax);
    }

    bool LevelsGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    float LevelsGradientComponent::GetInputMin() const
    {
        return m_configuration.m_inputMin;
    }

    void LevelsGradientComponent::SetInputMin(float value)
    {
        bool valueChanged = false;

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            if (m_configuration.m_inputMin != value)
            {
                m_configuration.m_inputMin = value;
                valueChanged = true;
            }
        }

        if (valueChanged)
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    float LevelsGradientComponent::GetInputMid() const
    {
        return m_configuration.m_inputMid;
    }

    void LevelsGradientComponent::SetInputMid(float value)
    {
        bool valueChanged = false;

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            if (m_configuration.m_inputMid != value)
            {
                m_configuration.m_inputMid = value;
                valueChanged = true;
            }
        }

        if (valueChanged)
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    float LevelsGradientComponent::GetInputMax() const
    {
        return m_configuration.m_inputMax;
    }

    void LevelsGradientComponent::SetInputMax(float value)
    {
        bool valueChanged = false;

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            if (m_configuration.m_inputMax != value)
            {
                m_configuration.m_inputMax = value;
                valueChanged = true;
            }
        }

        if (valueChanged)
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    float LevelsGradientComponent::GetOutputMin() const
    {
        return m_configuration.m_outputMin;
    }

    void LevelsGradientComponent::SetOutputMin(float value)
    {
        bool valueChanged = false;

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            if (m_configuration.m_outputMin != value)
            {
                m_configuration.m_outputMin = value;
                valueChanged = true;
            }
        }

        if (valueChanged)
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    float LevelsGradientComponent::GetOutputMax() const
    {
        return m_configuration.m_outputMin;
    }

    void LevelsGradientComponent::SetOutputMax(float value)
    {
        bool valueChanged = false;

        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            if (m_configuration.m_outputMax != value)
            {
                m_configuration.m_outputMax = value;
                valueChanged = true;
            }
        }

        if (valueChanged)
        {
            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    GradientSampler& LevelsGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
