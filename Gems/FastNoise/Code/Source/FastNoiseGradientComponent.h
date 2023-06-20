/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <FastNoise/Ebuses/FastNoiseGradientRequestBus.h>

#include <External/FastNoise/FastNoise.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(FastNoise::Interp, "{E1D450B5-CE30-450F-BD29-382DA64A469B}");
    AZ_TYPE_INFO_SPECIALIZE(FastNoise::NoiseType, "{0B54F0FB-F5D2-49DF-B5F8-30B209978D59}");
    AZ_TYPE_INFO_SPECIALIZE(FastNoise::FractalType, "{AAC4BD68-217B-4247-A1CE-E5E98B15956F}");
    AZ_TYPE_INFO_SPECIALIZE(FastNoise::CellularDistanceFunction, "{761E3584-FACD-4355-BAD6-DA4D2DAFFD8C}");
    AZ_TYPE_INFO_SPECIALIZE(FastNoise::CellularReturnType, "{31CDBAC6-882C-4330-8C68-4039FE4D1A48}");
}

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace FastNoiseGem
{
    class FastNoiseGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(FastNoiseGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(FastNoiseGradientConfig, "{831C1F11-5898-4FBF-B4CF-92B757A907A8}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 GetCellularParameterVisibility() const;
        AZ::u32 GetFractalParameterVisbility() const;
        AZ::u32 GetFrequencyParameterVisbility() const;
        AZ::u32 GetInterpParameterVisibility() const;

        bool operator==(const FastNoiseGradientConfig& rhs) const;

        int m_seed = 1;
        float m_frequency = 1.f;
        FastNoise::Interp m_interp = FastNoise::Interp::Quintic;
        FastNoise::NoiseType m_noiseType = FastNoise::NoiseType::PerlinFractal;

        int m_octaves = 4;
        float m_lacunarity = 2.f;
        float m_gain = 0.5;
        FastNoise::FractalType m_fractalType = FastNoise::FractalType::FBM;

        FastNoise::CellularDistanceFunction m_cellularDistanceFunction = FastNoise::CellularDistanceFunction::Euclidean;
        FastNoise::CellularReturnType m_cellularReturnType = FastNoise::CellularReturnType::CellValue;
        float m_cellularJitter = 0.45f;
    };

    inline constexpr AZ::TypeId FastNoiseGradientComponentTypeId{ "{81449CDF-D6DE-46DA-A50C-576B0B921311}" };

    class FastNoiseGradientComponent
        : public AZ::Component
        , private GradientSignal::GradientRequestBus::Handler
        , private FastNoiseGradientRequestBus::Handler
        , private GradientSignal::GradientTransformNotificationBus::Handler
    {
    public:
        friend class EditorFastNoiseGradientComponent;
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(FastNoiseGradientComponent, FastNoiseGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        FastNoiseGradientComponent(const FastNoiseGradientConfig& configuration);
        FastNoiseGradientComponent() = default;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // GradientRequestBus overrides...
        float GetValue(const GradientSignal::GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;

    protected:
        FastNoiseGradientConfig m_configuration;
        FastNoise m_generator;
        GradientSignal::GradientTransform m_gradientTransform;
        mutable AZStd::shared_mutex m_queryMutex;

        // GradientTransformNotificationBus overrides...
        void OnGradientTransformChanged(const GradientSignal::GradientTransform& newTransform) override;

        // FastNoiseGradientRequest overrides...
        int GetRandomSeed() const override;
        void SetRandomSeed(int seed) override;

        float GetFrequency() const override;
        void SetFrequency(float freq) override;

        FastNoise::Interp GetInterpolation() const override;
        void SetInterpolation(FastNoise::Interp interp) override;

        FastNoise::NoiseType GetNoiseType() const override;
        void SetNoiseType(FastNoise::NoiseType type) override;

        int GetOctaves() const override;
        void SetOctaves(int octaves) override;

        float GetLacunarity() const override;
        void SetLacunarity(float lacunarity) override;

        float GetGain() const override;
        void SetGain(float gain) override;

        FastNoise::FractalType GetFractalType() const override;
        void SetFractalType(FastNoise::FractalType type) override;

        template <typename TValueType, TValueType FastNoiseGradientConfig::*TConfigMember, void (FastNoise::*TMethod)(TValueType)>
        void SetConfigValue(TValueType value);
    };
} //namespace FastNoiseGem
