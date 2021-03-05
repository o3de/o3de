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

#include "GradientSignal_precompiled.h"
#include "PerlinGradientComponent.h"
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

namespace GradientSignal
{
    void PerlinGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PerlinGradientConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("randomSeed", &PerlinGradientConfig::m_randomSeed)
                ->Field("octave", &PerlinGradientConfig::m_octave)
                ->Field("amplitude", &PerlinGradientConfig::m_amplitude)
                ->Field("frequency", &PerlinGradientConfig::m_frequency)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<PerlinGradientConfig>(
                    "Perlin Noise Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PerlinGradientConfig::m_randomSeed, "Random Seed", "Using different seeds will cause the noise output to change")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<int>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100)
                    ->Attribute(AZ::Edit::Attributes::Step, 10)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PerlinGradientConfig::m_octave, "Octaves", "Number of recursions in the pattern generation, higher octaves refine the pattern")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 16)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 8)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PerlinGradientConfig::m_amplitude, "Amplitude", "Higher amplitudes widen the aperture of the highs (light) and lows (dark)")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 8.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PerlinGradientConfig::m_frequency, "Frequency", "Rescales coordinates based on a multiplied factor")
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 8.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.25) // Give the frequency a non-linear scale slider with higher precision at the low end
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PerlinGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("randomSeed", BehaviorValueProperty(&PerlinGradientConfig::m_randomSeed))
                ->Property("octave", BehaviorValueProperty(&PerlinGradientConfig::m_octave))
                ->Property("amplitude", BehaviorValueProperty(&PerlinGradientConfig::m_amplitude))
                ->Property("frequency", BehaviorValueProperty(&PerlinGradientConfig::m_frequency))
                ;
        }
    }

    void PerlinGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void PerlinGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void PerlinGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void PerlinGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        PerlinGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PerlinGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &PerlinGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("PerlinGradientComponentTypeId", BehaviorConstant(PerlinGradientComponentTypeId));

            behaviorContext->Class<PerlinGradientComponent>()->RequestBus("PerlinGradientRequestBus");

            behaviorContext->EBus<PerlinGradientRequestBus>("PerlinGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetRandomSeed", &PerlinGradientRequestBus::Events::GetRandomSeed)
                ->Event("SetRandomSeed", &PerlinGradientRequestBus::Events::SetRandomSeed)
                ->VirtualProperty("RandomSeed", "GetRandomSeed", "SetRandomSeed")
                ->Event("GetAmplitude", &PerlinGradientRequestBus::Events::GetAmplitude)
                ->Event("SetAmplitude", &PerlinGradientRequestBus::Events::SetAmplitude)
                ->VirtualProperty("Amplitude", "GetAmplitude", "SetAmplitude")
                ->Event("GetOctaves", &PerlinGradientRequestBus::Events::GetOctaves)
                ->Event("SetOctaves", &PerlinGradientRequestBus::Events::SetOctaves)
                ->VirtualProperty("Octaves", "GetOctaves", "SetOctaves")
                ->Event("GetFrequency", &PerlinGradientRequestBus::Events::GetFrequency)
                ->Event("SetFrequency", &PerlinGradientRequestBus::Events::SetFrequency)
                ->VirtualProperty("Frequency", "GetFrequency", "SetFrequency")
                ;
        }
    }

    PerlinGradientComponent::PerlinGradientComponent(const PerlinGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void PerlinGradientComponent::Activate()
    {
        m_perlinImprovedNoise.reset(aznew PerlinImprovedNoise(AZ::GetMax(m_configuration.m_randomSeed, 1)));
        GradientRequestBus::Handler::BusConnect(GetEntityId());
        PerlinGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PerlinGradientComponent::Deactivate()
    {
        m_perlinImprovedNoise.reset();
        GradientRequestBus::Handler::BusDisconnect();
        PerlinGradientRequestBus::Handler::BusDisconnect();
    }

    bool PerlinGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const PerlinGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool PerlinGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<PerlinGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float PerlinGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        if (m_perlinImprovedNoise)
        {
            AZ::Vector3 uvw = sampleParams.m_position;

            bool wasPointRejected = false;
            const bool shouldNormalizeOutput = false;
            GradientTransformRequestBus::Event(
                GetEntityId(), &GradientTransformRequestBus::Events::TransformPositionToUVW, sampleParams.m_position, uvw, shouldNormalizeOutput, wasPointRejected);

            if (!wasPointRejected)
            {
                return m_perlinImprovedNoise->GenerateOctaveNoise(uvw.GetX(), uvw.GetY(), uvw.GetZ(), m_configuration.m_octave, m_configuration.m_amplitude, m_configuration.m_frequency);
            }
        }

        return 0.0f;
    }

    int PerlinGradientComponent::GetRandomSeed() const
    {
        return m_configuration.m_randomSeed;
    }

    void PerlinGradientComponent::SetRandomSeed(int seed)
    {
        m_configuration.m_randomSeed = AZStd::GetMax(seed, 1);
        m_perlinImprovedNoise.release();
        m_perlinImprovedNoise.reset(aznew PerlinImprovedNoise(m_configuration.m_randomSeed));
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    int PerlinGradientComponent::GetOctaves() const
    {
        return m_configuration.m_octave;
    }

    void PerlinGradientComponent::SetOctaves(int octaves)
    {
        m_configuration.m_octave = octaves;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float PerlinGradientComponent::GetAmplitude() const
    {
        return m_configuration.m_amplitude;
    }

    void PerlinGradientComponent::SetAmplitude(float amp)
    {
        m_configuration.m_amplitude = amp;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float PerlinGradientComponent::GetFrequency() const
    {
        return m_configuration.m_frequency;
    }

    void PerlinGradientComponent::SetFrequency(float frequency)
    {
        m_configuration.m_frequency = frequency;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}