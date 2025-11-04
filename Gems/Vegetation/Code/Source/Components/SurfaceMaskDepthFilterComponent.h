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
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfacePointList.h>
#include <Vegetation/Ebuses/SurfaceMaskDepthFilterRequestBus.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    /**
    * Configures the component to enforce depth rules for a vegetation region
    */
    static constexpr float s_defaultLowerSurfaceDistance = -1000.0f;
    static constexpr float s_defaultUpperSurfaceDistance = 1000.0f;

    class SurfaceMaskDepthFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceMaskDepthFilterConfig, AZ::SystemAllocator);
        AZ_RTTI(SurfaceMaskDepthFilterConfig, "{5F0CD700-EC2B-468D-B708-F6EEA7782C46}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        FilterStage m_filterStage = FilterStage::Default;
        bool m_allowOverrides = false;
        float m_lowerDistance = s_defaultLowerSurfaceDistance;
        float m_upperDistance = s_defaultUpperSurfaceDistance;
        SurfaceData::SurfaceTagVector m_depthComparisonTags;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(AZStd::string tag);
    };

    inline constexpr AZ::TypeId SurfaceMaskDepthFilterComponentTypeId{ "{A54BB0B2-8B30-4583-B44E-EFA17173040B}" };

    /**
    * This component filters based on the depth between two surface masks (using labels)
    */
    class SurfaceMaskDepthFilterComponent
        : public AZ::Component
        , protected FilterRequestBus::Handler
        , private SurfaceMaskDepthFilterRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceMaskDepthFilterComponent, SurfaceMaskDepthFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceMaskDepthFilterComponent(const SurfaceMaskDepthFilterConfig& configuration);
        SurfaceMaskDepthFilterComponent() = default;
        ~SurfaceMaskDepthFilterComponent() = default;

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
        // SurfaceMaskDepthFilterRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        float GetLowerDistance() const override;
        void SetLowerDistance(float lowerDistance) override;
        float GetUpperDistance() const override;
        void SetUpperDistance(float upperDistance) override;
        size_t GetNumTags() const override;
        AZ::Crc32 GetTag(int tagIndex) const override;
        void RemoveTag(int tagIndex) override;
        void AddTag(AZStd::string tag) override;


    private:
        void ExecutePerDescriptor(const InstanceData& instanceData, bool& result);
        void ExecuteForAllDescriptors(const InstanceData& instanceData, bool& result);

        SurfaceMaskDepthFilterConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;

        //point vector reserved for reuse
        mutable SurfaceData::SurfacePointList m_points;
    };
}
