/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ShapeAreaFalloffGradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class ShapeAreaFalloffGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeAreaFalloffGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(ShapeAreaFalloffGradientConfig, "{8FB7C786-D8A7-41C4-A703-020020EB4A4F}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_shapeEntityId;
        float m_falloffWidth = 1.0f;
        FalloffType m_falloffType = FalloffType::Outer;
        bool m_is3dFalloff = false;
    };

    inline constexpr AZ::TypeId ShapeAreaFalloffGradientComponentTypeId{ "{F32A108B-7612-4AC2-B436-96DDDCE9E70B}" };

    /**
    * calculates a gradient value based on distance from a shapes surface
    */      
    class ShapeAreaFalloffGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private ShapeAreaFalloffGradientRequestBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private AZ::EntityBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ShapeAreaFalloffGradientComponent, ShapeAreaFalloffGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ShapeAreaFalloffGradientComponent(const ShapeAreaFalloffGradientConfig& configuration);
        ShapeAreaFalloffGradientComponent() = default;
        ~ShapeAreaFalloffGradientComponent() = default;

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

    protected:
        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        ////////////////////////////////////////////////////////////////////////
        // LmbrCentral::ShapeComponentNotificationsBus
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        //////////////////////////////////////////////////////////////////////////
        // ShapeAreaFalloffGradientRequestBus
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId entityId) override;

        float GetFalloffWidth() const override;
        void SetFalloffWidth(float falloffWidth) override;

        FalloffType GetFalloffType() const override;
        void SetFalloffType(FalloffType type) override;

        bool Get3dFalloff() const override;
        void Set3dFalloff(bool is3dFalloff) override;

        void CacheShapeBounds();

        void NotifyRegionChanged(const AZ::Aabb& region);

    private:
        ShapeAreaFalloffGradientConfig m_configuration;
        mutable AZStd::shared_mutex m_queryMutex;
        AZ::Vector3 m_cachedShapeCenter;
        AZ::Aabb m_cachedShapeBounds;
    };
}
