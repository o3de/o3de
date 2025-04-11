/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Ebuses/PerlinGradientRequestBus.h>
#include <GradientSignal/PerlinImprovedNoise.h>

namespace AZ
{
    class Job;
}

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class PerlinGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PerlinGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(PerlinGradientConfig, "{A746CFD0-7288-42F4-837D-1CDE2EAA6923}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        int m_randomSeed = 1;
        int m_octave = 1;
        float m_amplitude = 1.0f;
        float m_frequency = 1.0f;
    };

    inline constexpr AZ::TypeId PerlinGradientComponentTypeId{ "{A293D617-C0F2-4D96-9DA0-791A5564878C}" };

    class PerlinGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private PerlinGradientRequestBus::Handler
        , private GradientTransformNotificationBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(PerlinGradientComponent, PerlinGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        PerlinGradientComponent(const PerlinGradientConfig& configuration);
        PerlinGradientComponent() = default;
        ~PerlinGradientComponent() = default;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // GradientRequestBus overrides...
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;

    protected:
        PerlinGradientConfig m_configuration;
        AZStd::unique_ptr<PerlinImprovedNoise> m_perlinImprovedNoise;
    private:
        GradientTransform m_gradientTransform;
        mutable AZStd::shared_mutex m_queryMutex;

        // GradientTransformNotificationBus overrides...
        void OnGradientTransformChanged(const GradientTransform& newTransform) override;

        // PerlinGradientRequestBus overrides...
        int GetRandomSeed() const override;
        void SetRandomSeed(int seed) override;

        int GetOctaves() const override;
        void SetOctaves(int octaves) override;

        float GetAmplitude() const override;
        void SetAmplitude(float amp) override;

        float GetFrequency() const override;
        void SetFrequency(float frequency) override;
    };
}
