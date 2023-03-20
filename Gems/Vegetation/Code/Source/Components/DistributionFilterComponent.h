/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/DistributionFilterRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class DistributionFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(DistributionFilterConfig, AZ::SystemAllocator);
        AZ_RTTI(DistributionFilterConfig, "{7E304208-5FDF-4384-BC28-E7CDD2A15BEC}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        
        FilterStage m_filterStage = FilterStage::Default;
        float m_thresholdMin = 0.1f;
        float m_thresholdMax = 1.0f;
        GradientSignal::GradientSampler m_gradientSampler;

    private:
        AZStd::function<float(float, const GradientSignal::GradientSampleParams&)> GetFilterFunc();
        GradientSignal::GradientSampler* GetSampler();
    };

    inline constexpr AZ::TypeId DistributionFilterComponentTypeId{ "{7A1D2AB7-2F32-4CBE-B7F1-2C08D427BE50}" };

    /**
    * Component implementing VegetationFilterRequestBus that accepts/rejects based on noise generator passing threshold
    */      
    class DistributionFilterComponent
        : public AZ::Component
        , public FilterRequestBus::Handler
        , private DistributionFilterRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(DistributionFilterComponent, DistributionFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        DistributionFilterComponent(const DistributionFilterConfig& configuration);
        DistributionFilterComponent() = default;
        ~DistributionFilterComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // VegetationFilterRequestBus
        bool Evaluate(const InstanceData& instanceData) const override;
        FilterStage GetFilterStage() const override;
        void SetFilterStage(FilterStage filterStage) override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // DistributionFilterRequestBus
        float GetThresholdMin() const override;
        void SetThresholdMin(float thresholdMin) override;
        float GetThresholdMax() const override;
        void SetThresholdMax(float thresholdMax) override;
        GradientSignal::GradientSampler& GetGradientSampler() override;


    private:
        DistributionFilterConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
