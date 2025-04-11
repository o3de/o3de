/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Vegetation/Descriptor.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/DistanceBetweenFilterRequestBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/InstanceData.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class DistanceBetweenFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(DistanceBetweenFilterConfig, AZ::SystemAllocator);
        AZ_RTTI(DistanceBetweenFilterConfig, "{8CD110EE-95FA-4B26-B10E-95079BE4CB11}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_allowOverrides = false;
        BoundMode m_boundMode = BoundMode::Radius;
        float m_radiusMin = 0.0f;
    private:
        bool IsRadiusReadOnly() const;
    };

    inline constexpr AZ::TypeId DistanceBetweenFilterComponentTypeId{ "{B1F3B6E1-A3C4-44EE-B70B-D69013073E82}" };

    /**
    * Component implementing VegetationFilterRequestBus that accepts/rejects based on distance between instances
    */
    class DistanceBetweenFilterComponent
        : public AZ::Component
        , public FilterRequestBus::Handler
        , private DistanceBetweenFilterRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(DistanceBetweenFilterComponent, DistanceBetweenFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        DistanceBetweenFilterComponent(const DistanceBetweenFilterConfig& configuration);
        DistanceBetweenFilterComponent() = default;
        ~DistanceBetweenFilterComponent() = default;

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
        // DistanceBetweenFilterRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        BoundMode GetBoundMode() const override;
        void SetBoundMode(BoundMode boundMode) override;
        float GetRadiusMin() const override;
        void SetRadiusMin(float radiusMin) override;

    private:
        AZ::Aabb GetInstanceBounds(const InstanceData& instanceData) const;
        DistanceBetweenFilterConfig m_configuration;
    };
}
