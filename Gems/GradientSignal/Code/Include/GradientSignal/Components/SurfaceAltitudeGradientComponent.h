/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/SurfaceAltitudeGradientRequestBus.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfacePointList.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class SurfaceAltitudeGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceAltitudeGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(SurfaceAltitudeGradientConfig, "{3CB05FC9-6E0F-435E-B420-F027B6716804}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZ::EntityId m_shapeEntityId;
        float m_altitudeMin = 0.0f;
        float m_altitudeMax = 128.0f;
        SurfaceData::SurfaceTagVector m_surfaceTagsToSample;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(AZStd::string tag);

    private:
        bool IsShapeValid() const;
    };

    inline constexpr AZ::TypeId SurfaceAltitudeGradientComponentTypeId{ "{76359FA6-AD40-4DF9-81C6-F63F2632B665}" };

    /**
    * Component implementing GradientRequestBus based on altitude
    */      
    class SurfaceAltitudeGradientComponent
        : public AZ::Component
        , public GradientRequestBus::Handler
        , private SurfaceAltitudeGradientRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private AZ::TickBus::Handler
        , private SurfaceData::SurfaceDataSystemNotificationBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceAltitudeGradientComponent, SurfaceAltitudeGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceAltitudeGradientComponent(const SurfaceAltitudeGradientConfig& configuration);
        SurfaceAltitudeGradientComponent() = default;
        ~SurfaceAltitudeGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataSystemNotificationBus
        void OnSurfaceChanged(
            const AZ::EntityId& entityId,
            const AZ::Aabb& oldBounds,
            const AZ::Aabb& newBounds,
            const SurfaceData::SurfaceTagSet& changedSurfaceTags) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void UpdateFromShape();

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // SurfaceAltituteGradientRequests
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId entityId) override;
        float GetAltitudeMin() const override;
        void SetAltitudeMin(float altitudeMin) override;
        float GetAltitudeMax() const override;
        void SetAltitudeMax(float altitudeMax) override;
        size_t GetNumTags() const override;
        AZ::Crc32 GetTag(int tagIndex) const override;
        void RemoveTag(int tagIndex) override;
        void AddTag(AZStd::string tag) override;

    private:
        mutable AZStd::shared_mutex m_queryMutex;
        SurfaceAltitudeGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        AZStd::atomic_bool m_dirty{ false };
        AZStd::atomic_bool m_surfaceDirty{ false };
    };
}
