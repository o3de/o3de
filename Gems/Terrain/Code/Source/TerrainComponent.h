/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <GradientSignal/GradientSampler.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainConfig, "{F3C51D93-ECBF-4035-9CAE-1E667B53BD83}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool m_debugWireframeEnabled{ false };
        GradientSignal::GradientSampler m_gradientSampler;
    };

    class TerrainComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainComponent, "{6DBF9BB3-E748-4DA7-AFE9-4836C248FB21}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainComponent(const TerrainConfig& configuration);
        TerrainComponent() = default;
        ~TerrainComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const;

    private:
        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! AZ::TransformNotificationBus::Handler overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

        void BuildTerrainData();
        void DestroyTerrainData();

        void UpdateTerrainData(const AZ::Aabb& dirtyRegion);
        AZ::Aabb GetSurfaceAabb() const;
        SurfaceData::SurfaceTagVector GetSurfaceTags() const;


        TerrainConfig m_configuration;
        bool m_dirty;

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        AZ::Aabb m_terrainBounds = AZ::Aabb::CreateNull();
        AZStd::atomic_bool m_terrainBoundsIsValid{ false };
    };
}
