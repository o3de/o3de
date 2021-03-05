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
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/RandomGradientRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class PerlinImprovedNoise;

    class RandomGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RandomGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(RandomGradientConfig, "{A435F06D-A148-4B5F-897D-39996495B6F4}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZ::u32 m_randomSeed = 13;
    };

    static const AZ::Uuid RandomGradientComponentTypeId = "{8B7E5121-41B0-4EF9-96A9-04953EC69754}";

    class RandomGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private RandomGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(RandomGradientComponent, RandomGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        RandomGradientComponent(const RandomGradientConfig& configuration);
        RandomGradientComponent() = default;
        ~RandomGradientComponent() = default;

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
        RandomGradientConfig m_configuration;

        /////////////////////////////////////////////////////////////////////////
        // RandomGradientRequest overrides
        int GetRandomSeed() const override;
        void SetRandomSeed(int seed) override;
    };
}