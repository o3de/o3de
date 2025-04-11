/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <AzCore/Component/Component.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/SurfaceMaskFilterRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfaceDataTagEnumeratorRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    /**
    * Configuration of the vegetation surface acceptance filter
    */
    class SurfaceMaskFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceMaskFilterConfig, AZ::SystemAllocator);
        AZ_RTTI(SurfaceMaskFilterConfig, "{5B085DA7-CDC9-47C7-B2DB-BA5DD5AA2FB5}", AZ::ComponentConfig);
        SurfaceMaskFilterConfig();
        static void Reflect(AZ::ReflectContext* context);
        FilterStage m_filterStage = FilterStage::Default;
        bool m_allowOverrides = false;
        SurfaceData::SurfaceTagVector m_inclusiveSurfaceMasks;
        float m_inclusiveWeightMin = 0.1f;
        float m_inclusiveWeightMax = 1.0f;
        SurfaceData::SurfaceTagVector m_exclusiveSurfaceMasks;
        float m_exclusiveWeightMin = 0.1f;
        float m_exclusiveWeightMax = 1.0f;

        size_t GetNumInclusiveTags() const;
        AZ::Crc32 GetInclusiveTag(int tagIndex) const;
        void RemoveInclusiveTag(int tagIndex);
        void AddInclusiveTag(AZStd::string tag);

        size_t GetNumExclusiveTags() const;
        AZ::Crc32 GetExclusiveTag(int tagIndex) const;
        void RemoveExclusiveTag(int tagIndex);
        void AddExclusiveTag(AZStd::string tag);
    };

    inline constexpr AZ::TypeId SurfaceMaskFilterComponentTypeId{ "{62AAAD68-DF4F-4551-8F78-2C72CEF79ED6}" };

    /**
    * Accepts the placement of vegetation based on surface tags and/or depth between two surface tags
    */
    class SurfaceMaskFilterComponent
        : public AZ::Component
        , protected FilterRequestBus::Handler
        , private SurfaceMaskFilterRequestBus::Handler
        , private SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceMaskFilterComponent, SurfaceMaskFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceMaskFilterComponent(const SurfaceMaskFilterConfig& configuration);
        SurfaceMaskFilterComponent() = default;
        ~SurfaceMaskFilterComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataTagEnumeratorRequestBus
        void GetInclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags, bool& includeAll) const override;
        void GetExclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags) const override;

        //////////////////////////////////////////////////////////////////////////
        // VegetationFilterRequestBus
        bool Evaluate(const InstanceData& instanceData) const override;
        FilterStage GetFilterStage() const override;
        void SetFilterStage(FilterStage filterStage) override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // SurfaceMaskFilterRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        size_t GetNumInclusiveTags() const override;
        AZ::Crc32 GetInclusiveTag(int tagIndex) const override;
        void RemoveInclusiveTag(int tagIndex) override;
        void AddInclusiveTag(AZStd::string tag) override;
        size_t GetNumExclusiveTags() const override;
        AZ::Crc32 GetExclusiveTag(int tagIndex) const override;
        void RemoveExclusiveTag(int tagIndex) override;
        void AddExclusiveTag(AZStd::string tag) override;
        float GetInclusiveWeightMin() const override;
        void SetInclusiveWeightMin(float value) override;
        float GetInclusiveWeightMax() const override;
        void SetInclusiveWeightMax(float value) override;
        float GetExclusiveWeightMin() const override;
        void SetExclusiveWeightMin(float value) override;
        float GetExclusiveWeightMax() const override;
        void SetExclusiveWeightMax(float value) override;

    private:
        SurfaceMaskFilterConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
