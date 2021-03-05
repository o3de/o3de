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
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/SurfaceSlopeFilterRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class SurfaceSlopeFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceSlopeFilterConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceSlopeFilterConfig, "{6CEBAF3A-2A5C-4508-A351-9613E32CF63F}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        FilterStage m_filterStage = FilterStage::Default;
        bool m_allowOverrides = false;
        float m_slopeMin = 0.0f;
        float m_slopeMax = 180.0f;
    };

    static const AZ::Uuid SurfaceSlopeFilterComponentTypeId = "{2938AA64-9B84-4B18-A90F-25798A255B8C}";

    /**
    * Component implementing VegetationFilterRequestBus that accepts/rejects based on slope
    */      
    class SurfaceSlopeFilterComponent
        : public AZ::Component
        , public FilterRequestBus::Handler
        , private SurfaceSlopeFilterRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceSlopeFilterComponent, SurfaceSlopeFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceSlopeFilterComponent(const SurfaceSlopeFilterConfig& configuration);
        SurfaceSlopeFilterComponent() = default;
        ~SurfaceSlopeFilterComponent() = default;

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
        // SurfaceSlopeFilterRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        float GetSlopeMin() const override;
        void SetSlopeMin(float slopeMin) override;
        float GetSlopeMax() const override;
        void SetSlopeMax(float slopeMax) override;

    private:
        SurfaceSlopeFilterConfig m_configuration;
    };
}