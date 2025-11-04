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
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Ebuses/RandomGradientRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class RandomGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RandomGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(RandomGradientConfig, "{A435F06D-A148-4B5F-897D-39996495B6F4}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZ::u32 m_randomSeed = 13;
    };

    inline constexpr AZ::TypeId RandomGradientComponentTypeId{ "{8B7E5121-41B0-4EF9-96A9-04953EC69754}" };

    class RandomGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private RandomGradientRequestBus::Handler
        , private GradientTransformNotificationBus::Handler
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

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // GradientRequestBus overrides...
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;

    private:
        RandomGradientConfig m_configuration;
        GradientTransform m_gradientTransform;
        mutable AZStd::shared_mutex m_queryMutex;

        // GradientTransformNotificationBus overrides...
        void OnGradientTransformChanged(const GradientTransform& newTransform) override;

        // RandomGradientRequestBus overrides...
        int GetRandomSeed() const override;
        void SetRandomSeed(int seed) override;

        float GetRandomValue(const AZ::Vector3& position, AZStd::size_t seed) const;
    };
}
