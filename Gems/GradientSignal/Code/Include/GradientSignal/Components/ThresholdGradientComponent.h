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
#include <GradientSignal/Ebuses/ThresholdGradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class ThresholdGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ThresholdGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(ThresholdGradientConfig, "{E9E2D5B3-66F1-494D-91D2-1E83D36A1AC1}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        GradientSampler m_gradientSampler;
        float m_threshold = 0.5f;
    };

    inline constexpr AZ::TypeId ThresholdGradientComponentTypeId{ "{CCE70521-E2D8-4304-B748-1E37A6DC57BF}" };

    /**
    * calculates a gradient value by converting values from another gradient to 0 or 1
    */      
    class ThresholdGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private ThresholdGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ThresholdGradientComponent, ThresholdGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ThresholdGradientComponent(const ThresholdGradientConfig& configuration);
        ThresholdGradientComponent() = default;
        ~ThresholdGradientComponent() = default;

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
        // ThresholdGradientRequestBus
        float GetThreshold() const override;
        void SetThreshold(float threshold) override;

        GradientSampler& GetGradientSampler() override;

    private:
        ThresholdGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        mutable AZStd::shared_mutex m_queryMutex;
    };
}
