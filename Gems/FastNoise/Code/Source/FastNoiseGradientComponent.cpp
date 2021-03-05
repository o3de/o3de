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

#include "FastNoise_precompiled.h"

#include "FastNoiseGradientComponent.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <External/FastNoise/FastNoise.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

namespace FastNoiseGem
{

    AZ::u32 FastNoiseGradientConfig::GetCellularParameterVisibility() const
    {
        return m_noiseType == FastNoise::NoiseType::Cellular ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 FastNoiseGradientConfig::GetFractalParameterVisbility() const
    {
        switch (m_noiseType)
        {
        case FastNoise::NoiseType::CubicFractal:
        case FastNoise::NoiseType::PerlinFractal:
        case FastNoise::NoiseType::SimplexFractal:
        case FastNoise::NoiseType::ValueFractal:
            return AZ::Edit::PropertyVisibility::Show;
        }
        return AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 FastNoiseGradientConfig::GetFrequencyParameterVisbility() const
    {
        return m_noiseType != FastNoise::NoiseType::WhiteNoise ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::u32 FastNoiseGradientConfig::GetInterpParameterVisibility() const
    {
        switch (m_noiseType)
        {
        case FastNoise::NoiseType::Value:
        case FastNoise::NoiseType::ValueFractal:
        case FastNoise::NoiseType::Perlin:
        case FastNoise::NoiseType::PerlinFractal:
            return AZ::Edit::PropertyVisibility::Show;
        }
        return AZ::Edit::PropertyVisibility::Hide;
    }

    void FastNoiseGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FastNoiseGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("NoiseType", &FastNoiseGradientConfig::m_noiseType)
                ->Field("Seed", &FastNoiseGradientConfig::m_seed)
                ->Field("Frequency", &FastNoiseGradientConfig::m_frequency)
                ->Field("Octaves", &FastNoiseGradientConfig::m_octaves)
                ->Field("Lacunarity", &FastNoiseGradientConfig::m_lacunarity)
                ->Field("Gain", &FastNoiseGradientConfig::m_gain)
                ->Field("Interp", &FastNoiseGradientConfig::m_interp)
                ->Field("FractalType", &FastNoiseGradientConfig::m_fractalType)
                ->Field("CellularDistanceFunction", &FastNoiseGradientConfig::m_cellularDistanceFunction)
                ->Field("CellularReturnType", &FastNoiseGradientConfig::m_cellularReturnType)
                ->Field("CellularJitter", &FastNoiseGradientConfig::m_cellularJitter)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<FastNoiseGradientConfig>(
                    "FastNoise Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FastNoiseGradientConfig::m_seed, "Random Seed", "Using different seeds will cause the noise output to change")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<int>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100)
                    ->Attribute(AZ::Edit::Attributes::Step, 10)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FastNoiseGradientConfig::m_noiseType, "Noise Type", "Sets the type of noise generator used")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<int>::min())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<int>::max())
                    ->EnumAttribute(FastNoise::NoiseType::Value, "Value")
                    ->EnumAttribute(FastNoise::NoiseType::ValueFractal, "Value Fractal")
                    ->EnumAttribute(FastNoise::NoiseType::Perlin, "Perlin")
                    ->EnumAttribute(FastNoise::NoiseType::PerlinFractal, "Perlin Fractal")
                    ->EnumAttribute(FastNoise::NoiseType::Simplex, "Simplex")
                    ->EnumAttribute(FastNoise::NoiseType::SimplexFractal, "Simplex Fractal")
                    ->EnumAttribute(FastNoise::NoiseType::Cellular, "Cellular")
                    ->EnumAttribute(FastNoise::NoiseType::WhiteNoise, "White Noise")
                    ->EnumAttribute(FastNoise::NoiseType::Cubic, "Cubic")
                    ->EnumAttribute(FastNoise::NoiseType::CubicFractal, "Cubic Fractal")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FastNoiseGradientConfig::m_frequency, "Frequency", "Higher frequencies are more coarse")
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 8.0f)
                    ->Attribute(AZ::Edit::Attributes::SliderCurveMidpoint, 0.25) // Give the frequency a non-linear scale slider with higher precision at the low end
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetFrequencyParameterVisbility)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FastNoiseGradientConfig::m_octaves, "Octaves", "Number of recursions in the pattern generation, higher octaves refine the pattern")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 8)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetFractalParameterVisbility)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FastNoiseGradientConfig::m_lacunarity, "Lacunarity", "The frequency multiplier between each octave")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 5.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetFractalParameterVisbility)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FastNoiseGradientConfig::m_gain, "Gain", "The relative strength of noise from each layer when compared to the last")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 5.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetFractalParameterVisbility)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FastNoiseGradientConfig::m_cellularDistanceFunction, "Distance Function", "Sets the distance function used to calculate the cell for a given point")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetCellularParameterVisibility)
                    ->EnumAttribute(FastNoise::CellularDistanceFunction::Euclidean, "Euclidean")
                    ->EnumAttribute(FastNoise::CellularDistanceFunction::Manhattan, "Manhattan")
                    ->EnumAttribute(FastNoise::CellularDistanceFunction::Natural, "Natural")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FastNoiseGradientConfig::m_cellularReturnType, "Return Type", "Alters the value type the cellular function returns from its calculation")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetCellularParameterVisibility)
                    ->EnumAttribute(FastNoise::CellularReturnType::CellValue, "CellValue")
                    ->EnumAttribute(FastNoise::CellularReturnType::Distance, "Distance")
                    ->EnumAttribute(FastNoise::CellularReturnType::Distance2, "Distance2")
                    ->EnumAttribute(FastNoise::CellularReturnType::Distance2Add, "Distance2Add")
                    ->EnumAttribute(FastNoise::CellularReturnType::Distance2Sub, "Distance2Sub")
                    ->EnumAttribute(FastNoise::CellularReturnType::Distance2Mul, "Distance2Mul")
                    ->EnumAttribute(FastNoise::CellularReturnType::Distance2Div, "Distance2Div")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &FastNoiseGradientConfig::m_cellularJitter, "Jitter", "Sets the maximum distance a cellular point can move from its grid position")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 10.f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetCellularParameterVisibility)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "FastNoise Advanced Settings")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FastNoiseGradientConfig::m_interp, "Interpolation", "Changes the interpolation method used to smooth between noise values")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetInterpParameterVisibility)
                    ->EnumAttribute(FastNoise::Interp::Linear, "Linear")
                    ->EnumAttribute(FastNoise::Interp::Hermite, "Hermite")
                    ->EnumAttribute(FastNoise::Interp::Quintic, "Quintic")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FastNoiseGradientConfig::m_fractalType, "Fractal Type", "Sets how the fractal is combined")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &FastNoiseGradientConfig::GetFractalParameterVisbility)
                    ->EnumAttribute(FastNoise::FractalType::FBM, "FBM")
                    ->EnumAttribute(FastNoise::FractalType::Billow, "Billow")
                    ->EnumAttribute(FastNoise::FractalType::RigidMulti, "Rigid Multi")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<FastNoiseGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("randomSeed", BehaviorValueProperty(&FastNoiseGradientConfig::m_seed))
                ->Property("frequency", BehaviorValueProperty(&FastNoiseGradientConfig::m_frequency))
                ->Property("octaves", BehaviorValueProperty(&FastNoiseGradientConfig::m_octaves))
                ->Property("lacunarity", BehaviorValueProperty(&FastNoiseGradientConfig::m_lacunarity))
                ->Property("gain", BehaviorValueProperty(&FastNoiseGradientConfig::m_gain))
                ->Property("noiseType",
                    [](FastNoiseGradientConfig* config) { return (int&)(config->m_noiseType); },
                    [](FastNoiseGradientConfig* config, const int& i) { config->m_noiseType = (FastNoise::NoiseType)i; })
                ->Property("interpolation",
                    [](FastNoiseGradientConfig* config) { return (int&)(config->m_interp); },
                    [](FastNoiseGradientConfig* config, const int& i) { config->m_interp = (FastNoise::Interp)i; })
                ->Property("fractalType",
                    [](FastNoiseGradientConfig* config) { return (int&)(config->m_fractalType); },
                    [](FastNoiseGradientConfig* config, const int& i) { config->m_fractalType = (FastNoise::FractalType)i; })
                ;
        }
    }

    void FastNoiseGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void FastNoiseGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void FastNoiseGradientComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
    }

    void FastNoiseGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        FastNoiseGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<FastNoiseGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &FastNoiseGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("FastNoiseGradientComponentTypeId", BehaviorConstant(FastNoiseGradientComponentTypeId));

            behaviorContext->Class<FastNoiseGradientComponent>()->RequestBus("FastNoiseGradientRequestBus");

            behaviorContext->EBus<FastNoiseGradientRequestBus>("FastNoiseGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetRandomSeed", &FastNoiseGradientRequestBus::Events::GetRandomSeed)
                ->Event("SetRandomSeed", &FastNoiseGradientRequestBus::Events::SetRandomSeed)
                ->VirtualProperty("RandomSeed", "GetRandomSeed", "SetRandomSeed")
                ->Event("GetFrequency", &FastNoiseGradientRequestBus::Events::GetFrequency)
                ->Event("SetFrequency", &FastNoiseGradientRequestBus::Events::SetFrequency)
                ->VirtualProperty("Frequency", "GetFrequency", "SetFrequency")
                ->Event("GetInterpolation", &FastNoiseGradientRequestBus::Events::GetInterpolation)
                ->Event("SetInterpolation", &FastNoiseGradientRequestBus::Events::SetInterpolation)
                ->VirtualProperty("Interpolation", "GetInterpolation", "SetInterpolation")
                ->Event("GetNoiseType", &FastNoiseGradientRequestBus::Events::GetNoiseType)
                ->Event("SetNoiseType", &FastNoiseGradientRequestBus::Events::SetNoiseType)
                ->VirtualProperty("NoiseType", "GetNoiseType", "SetNoiseType")
                ->Event("GetOctaves", &FastNoiseGradientRequestBus::Events::GetOctaves)
                ->Event("SetOctaves", &FastNoiseGradientRequestBus::Events::SetOctaves)
                ->VirtualProperty("Octaves", "GetOctaves", "SetOctaves")
                ->Event("GetLacunarity", &FastNoiseGradientRequestBus::Events::GetLacunarity)
                ->Event("SetLacunarity", &FastNoiseGradientRequestBus::Events::SetLacunarity)
                ->VirtualProperty("Lacunarity", "GetLacunarity", "SetLacunarity")
                ->Event("GetGain", &FastNoiseGradientRequestBus::Events::GetGain)
                ->Event("SetGain", &FastNoiseGradientRequestBus::Events::SetGain)
                ->VirtualProperty("Gain", "GetGain", "SetGain")
                ->Event("GetFractalType", &FastNoiseGradientRequestBus::Events::GetFractalType)
                ->Event("SetFractalType", &FastNoiseGradientRequestBus::Events::SetFractalType)
                ->VirtualProperty("FractalType", "GetFractalType", "SetFractalType")
                ;
        }
    }

    FastNoiseGradientComponent::FastNoiseGradientComponent(const FastNoiseGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void FastNoiseGradientComponent::Activate()
    {
        // Some platforms require random seeds to be > 0.  Clamp to a positive range to ensure we're always safe.
        m_generator.SetSeed(AZ::GetMax(m_configuration.m_seed, 1));
        m_generator.SetFrequency(m_configuration.m_frequency);
        m_generator.SetInterp(m_configuration.m_interp);
        m_generator.SetNoiseType(m_configuration.m_noiseType);

        m_generator.SetFractalOctaves(m_configuration.m_octaves);
        m_generator.SetFractalLacunarity(m_configuration.m_lacunarity);
        m_generator.SetFractalGain(m_configuration.m_gain);
        m_generator.SetFractalType(m_configuration.m_fractalType);

        m_generator.SetCellularDistanceFunction(m_configuration.m_cellularDistanceFunction);
        m_generator.SetCellularReturnType(m_configuration.m_cellularReturnType);
        m_generator.SetCellularJitter(m_configuration.m_cellularJitter);

        GradientSignal::GradientRequestBus::Handler::BusConnect(GetEntityId());
        FastNoiseGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void FastNoiseGradientComponent::Deactivate()
    {
        GradientSignal::GradientRequestBus::Handler::BusDisconnect();
        FastNoiseGradientRequestBus::Handler::BusDisconnect();
    }

    bool FastNoiseGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const FastNoiseGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool FastNoiseGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<FastNoiseGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float FastNoiseGradientComponent::GetValue(const GradientSignal::GradientSampleParams& sampleParams) const
    {
        AZ::Vector3 uvw = sampleParams.m_position;

        bool wasPointRejected = false;
        const bool shouldNormalizeOutput = false;
        GradientSignal::GradientTransformRequestBus::Event(
            GetEntityId(), &GradientSignal::GradientTransformRequestBus::Events::TransformPositionToUVW, sampleParams.m_position, uvw, shouldNormalizeOutput, wasPointRejected);

        if (!wasPointRejected)
        {
            // Generator returns a range between [-1, 1], map that to [0, 1]
            return AZ::GetClamp((m_generator.GetNoise(uvw.GetX(), uvw.GetY(), uvw.GetZ()) + 1.0f) / 2.0f, 0.0f, 1.0f);
        }

        return 0.0f;
    }

    template <typename TValueType, TValueType FastNoiseGradientConfig::*TConfigMember, void (FastNoise::*TMethod)(TValueType)>
    void FastNoiseGradientComponent::SetConfigValue(TValueType value)
    {
        m_configuration.*TConfigMember = value;
        ((&m_generator)->*TMethod)(value);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    int FastNoiseGradientComponent::GetRandomSeed() const
    {
        return m_configuration.m_seed;
    }

    void FastNoiseGradientComponent::SetRandomSeed(int seed)
    {
        // Some platforms require random seeds to be > 0.  Clamp to a positive range to ensure we're always safe.
        SetConfigValue<int, &FastNoiseGradientConfig::m_seed, &FastNoise::SetSeed>(AZ::GetMax(seed, 1));
    }

    float FastNoiseGradientComponent::GetFrequency() const
    {
        return m_configuration.m_frequency;
    }
    
    void FastNoiseGradientComponent::SetFrequency(float freq)
    {
        SetConfigValue<float, &FastNoiseGradientConfig::m_frequency, &FastNoise::SetFrequency>(freq);
    }

    FastNoise::Interp FastNoiseGradientComponent::GetInterpolation() const
    {
        return m_configuration.m_interp;
    }

    void FastNoiseGradientComponent::SetInterpolation(FastNoise::Interp interp)
    {
        SetConfigValue<FastNoise::Interp, &FastNoiseGradientConfig::m_interp, &FastNoise::SetInterp>(interp);
    }

    FastNoise::NoiseType FastNoiseGradientComponent::GetNoiseType() const
    {
        return m_configuration.m_noiseType;
    }

    void FastNoiseGradientComponent::SetNoiseType(FastNoise::NoiseType type)
    {
        SetConfigValue<FastNoise::NoiseType, &FastNoiseGradientConfig::m_noiseType, &FastNoise::SetNoiseType>(type);
    }

    int FastNoiseGradientComponent::GetOctaves() const
    {
        return m_configuration.m_octaves;
    }

    void FastNoiseGradientComponent::SetOctaves(int octaves)
    {
        SetConfigValue<int, &FastNoiseGradientConfig::m_octaves, &FastNoise::SetFractalOctaves>(octaves);
    }

    float FastNoiseGradientComponent::GetLacunarity() const
    {
        return m_configuration.m_lacunarity;
    }

    void FastNoiseGradientComponent::SetLacunarity(float lacunarity)
    {
        SetConfigValue<float, &FastNoiseGradientConfig::m_lacunarity, &FastNoise::SetFractalLacunarity>(lacunarity);
    }

    float FastNoiseGradientComponent::GetGain() const
    {
        return m_configuration.m_gain;
    }

    void FastNoiseGradientComponent::SetGain(float gain)
    {
        SetConfigValue<float, &FastNoiseGradientConfig::m_gain, &FastNoise::SetFractalGain>(gain);
    }

    FastNoise::FractalType FastNoiseGradientComponent::GetFractalType() const
    {
        return m_configuration.m_fractalType;
    }

    void FastNoiseGradientComponent::SetFractalType(FastNoise::FractalType type)
    {
        SetConfigValue<FastNoise::FractalType, &FastNoiseGradientConfig::m_fractalType, &FastNoise::SetFractalType>(type);
    }

} // namespace FastNoiseGem