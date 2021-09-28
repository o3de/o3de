/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>

namespace Terrain
{
    class TerrainSurfaceDataSystemConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceDataSystemConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceDataSystemConfig, "{2B93F5E5-5346-47A1-9C4D-EFBC6BDF468F}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
    };

    /**
    * The system component to serve for the game side queries for surface values
    */
    class TerrainSurfaceDataSystemComponent
        : public AZ::Component
        , private SurfaceData::SurfaceDataProviderRequestBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
        friend class EditorTerrainSurfaceDataSystemComponent;
        TerrainSurfaceDataSystemComponent(const TerrainSurfaceDataSystemConfig&);

    public:
        TerrainSurfaceDataSystemComponent();

        //////////////////////////////////////////////////////////////////////////
        // Component static
        AZ_COMPONENT(TerrainSurfaceDataSystemComponent, "{0C821DA4-6DB1-4860-BE25-CB57B3E3F4D4}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // Component
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Terrain::TerrainDataNotificationBus
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

    private:
        void UpdateTerrainData(const AZ::Aabb& dirtyRegion);
        AZ::Aabb GetSurfaceAabb() const;
        SurfaceData::SurfaceTagVector GetSurfaceTags() const;
        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

        TerrainSurfaceDataSystemConfig m_configuration;

        AZ::Aabb m_terrainBounds = AZ::Aabb::CreateNull();
        AZStd::atomic_bool m_terrainBoundsIsValid{ false };
    };
}
