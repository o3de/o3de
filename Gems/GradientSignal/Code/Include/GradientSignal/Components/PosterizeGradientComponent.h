/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/PosterizeGradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class PosterizeGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PosterizeGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(PosterizeGradientConfig, "{4AFDFD7F-384A-41DF-900C-9B25A4AA8D1E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        enum class ModeType : AZ::u8
        {
            Ceiling = 0,
            Floor,
            Round,
            Ps,
        };
        ModeType m_mode = ModeType::Ps;
        AZ::s32 m_bands = 3;
        GradientSampler m_gradientSampler;
    };

    inline constexpr AZ::TypeId PosterizeGradientComponentTypeId{ "{BDA78E8D-DEEE-477B-B1FD-11F9930322AA}" };

    /**
    * calculates a gradient value by converting values from another gradient to another's range
    */      
    class PosterizeGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private PosterizeGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(PosterizeGradientComponent, PosterizeGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        PosterizeGradientComponent(const PosterizeGradientConfig& configuration);
        PosterizeGradientComponent() = default;
        ~PosterizeGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;
        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // PosterizeGradientRequestBus
        AZ::s32 GetBands() const override;
        void SetBands(AZ::s32 bands) override;
        AZ::u8 GetModeType() const override;
        void SetModeType(AZ::u8 modeType) override;

        GradientSampler& GetGradientSampler() override;

    private:

        static float PosterizeValue(float input, float bands, PosterizeGradientConfig::ModeType mode)
        {
            const float clampedInput = AZ::GetClamp(input, 0.0f, 1.0f);
            float output = 0.0f;

            // "quantize" the input down to a number that goes from 0 to (bands-1)
            const float band = AZ::GetMin(floorf(clampedInput * bands), bands - 1.0f);

            // Given our quantized band, produce the right output for that band range.
            switch (mode)
            {
            default:
            case PosterizeGradientConfig::ModeType::Floor:
                // Floor:  the output range should be the lowest value of each band, or (0 to bands-1) / bands
                output = (band + 0.0f) / bands;
                break;
            case PosterizeGradientConfig::ModeType::Round:
                // Round:  the output range should be the midpoint of each band, or (0.5 to bands-0.5) / bands
                output = (band + 0.5f) / bands;
                break;
            case PosterizeGradientConfig::ModeType::Ceiling:
                // Ceiling:  the output range should be the highest value of each band, or (1 to bands) / bands
                output = (band + 1.0f) / bands;
                break;
            case PosterizeGradientConfig::ModeType::Ps:
                // Ps:  the output range should be equally distributed from 0-1, or (0 to bands-1) / (bands-1)
                output = band / (bands - 1.0f);
                break;
            }
            return AZ::GetMin(output, 1.0f);
        }

        PosterizeGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        mutable AZStd::shared_mutex m_queryMutex;
    };
}
