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
#include <Vegetation/Ebuses/SurfaceAltitudeFilterRequestBus.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class SurfaceAltitudeFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceAltitudeFilterConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceAltitudeFilterConfig, "{BB3C3018-66B1-4BAD-AD27-F385BA015C69}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        FilterStage m_filterStage = FilterStage::Default;
        bool m_allowOverrides = false;
        AZ::EntityId m_shapeEntityId;
        float m_altitudeMin = 0.0f;
        float m_altitudeMax = 128.0f;
    private:
        bool IsShapeValid() const;
    };

    static const AZ::Uuid SurfaceAltitudeFilterComponentTypeId = "{A32681E7-61BE-40CA-93D8-A1CD6E76B2EB}";

    /**
    * Component implementing VegetationFilterRequestBus that accepts/rejects based on altitude
    */      
    class SurfaceAltitudeFilterComponent
        : public AZ::Component
        , public FilterRequestBus::Handler
        , private SurfaceAltitudeFilterRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceAltitudeFilterComponent, SurfaceAltitudeFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceAltitudeFilterComponent(const SurfaceAltitudeFilterConfig& configuration);
        SurfaceAltitudeFilterComponent() = default;
        ~SurfaceAltitudeFilterComponent() = default;

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
        // SurfaceAltitudeFilterRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId shapeEntityId) override;
        float GetAltitudeMin() const override;
        void SetAltitudeMin(float altitudeMin) override;
        float GetAltitudeMax() const override;
        void SetAltitudeMax(float altitudeMax) override;

    private:
        SurfaceAltitudeFilterConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}