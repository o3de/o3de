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
#include <AzCore/Component/EntityId.h>
#include <Vegetation/Ebuses/FilterRequestBus.h>
#include <Vegetation/Ebuses/ShapeIntersectionFilterRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class ShapeIntersectionFilterConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeIntersectionFilterConfig, AZ::SystemAllocator);
        AZ_RTTI(ShapeIntersectionFilterConfig, "{B88C9D87-8609-4EAB-82D6-92DFEF006629}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        FilterStage m_filterStage = FilterStage::Default;
        AZ::EntityId m_shapeEntityId;
    };

    inline constexpr AZ::TypeId ShapeIntersectionFilterComponentTypeId{ "{BA6C09DC-16B2-4550-8115-4882A40A622C}" };

    /**
    * Component implementing VegetationFilterRequestBus that accepts/rejects vegetation based on shape intersection
    */      
    class ShapeIntersectionFilterComponent
        : public AZ::Component
        , public FilterRequestBus::Handler
        , private ShapeIntersectionFilterRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ShapeIntersectionFilterComponent, ShapeIntersectionFilterComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ShapeIntersectionFilterComponent(const ShapeIntersectionFilterConfig& configuration);
        ShapeIntersectionFilterComponent() = default;
        ~ShapeIntersectionFilterComponent() = default;

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
        // ShapeIntersectionFilterRequestBus
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId shapeEntityId) override;

    private:
        ShapeIntersectionFilterConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;

        void SetupDependencyMonitor();
    };
}
