/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>

#include <SurfaceData/SurfaceDataTypes.h>


namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainSurfaceMaterialsListConfig : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceMaterialsListConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceMaterialsListConfig, "{68A1CB1B-C835-4C3A-8D1C-08692E07711A}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

         //AZStd::unorderd_map<AZ::EntityId> m_gradientEntities;
        AZStd::vector<SurfaceData::SurfaceTag> m_tags;
    };

    class TerrainSurfaceMaterialsListComponent
        : public AZ::Component
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainSurfaceMaterialsListComponent, "{93CF3938-FBC3-4E55-B825-27BA94A5CD35}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainSurfaceMaterialsListComponent(const TerrainSurfaceMaterialsListConfig& configuration);
        TerrainSurfaceMaterialsListComponent() = default;
        ~TerrainSurfaceMaterialsListComponent() = default;

        //void GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        //void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Terrain::TerrainDataNotificationBus
       // void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

    private:
        TerrainSurfaceMaterialsListConfig m_configuration;

        /* void RefreshMinMaxHeights();

         float m_cachedMinWorldHeight{ 0.0f };
         float m_cachedMaxWorldHeight{ 0.0f };
         AZ::Vector2 m_cachedHeightQueryResolution{ 1.0f, 1.0f };
         AZ::Aabb m_cachedShapeBounds;

         LmbrCentral::DependencyMonitor m_dependencyMonitor;*/
    };
} // namespace Terrain
