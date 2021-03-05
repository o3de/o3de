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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
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
        AZ_CLASS_ALLOCATOR(PerlinGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(PerlinGradientConfig, "{A746CFD0-7288-42F4-837D-1CDE2EAA6923}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        int m_randomSeed = 1;
        int m_octave = 1;
        float m_amplitude = 1.0f;
        float m_frequency = 1.0f;
    };

    static const AZ::Uuid PerlinGradientComponentTypeId = "{A293D617-C0F2-4D96-9DA0-791A5564878C}";

    class PerlinGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private PerlinGradientRequestBus::Handler
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

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;

    private:
        PerlinGradientConfig m_configuration;
        AZStd::unique_ptr<PerlinImprovedNoise> m_perlinImprovedNoise;

        /////////////////////////////////////////////////////////////////////////
        //PerlinGradientRequest overrides
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