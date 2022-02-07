/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Material/MaterialAssignment.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <TerrainRenderer/TerrainAreaMaterialRequestBus.h>


namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    struct TerrainSurfaceMaterial
    {
        AZ_CLASS_ALLOCATOR(TerrainSurfaceMaterial, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceMaterial, "{C96BCB4D-D3D6-4346-AF42-AD1B18D52070}");

        virtual ~TerrainSurfaceMaterial() = default;

        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
        AZ::Data::Instance<AZ::RPI::Material> m_materialInstance;

        SurfaceData::SurfaceTag m_surfaceTag;
        SurfaceData::SurfaceTag m_previousTag;
        AZ::Data::AssetId m_activeMaterialAssetId;
        AZ::RPI::Material::ChangeId m_previousChangeId = AZ::RPI::Material::DEFAULT_CHANGE_ID;

        bool m_active = false;
    };

    struct TerrainSurfaceMaterialMapping final : public TerrainSurfaceMaterial
    {
        AZ_CLASS_ALLOCATOR(TerrainSurfaceMaterialMapping, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceMaterialMapping, "{37D2A586-CDDD-4FB7-A7D6-0B4CC575AB8C}", TerrainSurfaceMaterial);
        static void Reflect(AZ::ReflectContext* context);
        
        SurfaceData::SurfaceTag m_surfaceTag;
        SurfaceData::SurfaceTag m_previousTag;
    };
    
    struct DefaultTerrainSurfaceMaterial final : public TerrainSurfaceMaterial
    {
        AZ_CLASS_ALLOCATOR(DefaultTerrainSurfaceMaterial, AZ::SystemAllocator, 0);
        AZ_RTTI(DefaultTerrainSurfaceMaterial, "{EAE129EF-1A69-4A30-8D79-D2D9500E3237}", TerrainSurfaceMaterial);
        static void Reflect(AZ::ReflectContext* context);

        // This struct exists solely so it can have a separate edit context layout
        // from TerrainSurfaceMaterialMapping.
    };

    class TerrainSurfaceMaterialsListConfig : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceMaterialsListConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceMaterialsListConfig, "{68A1CB1B-C835-4C3A-8D1C-08692E07711A}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        DefaultTerrainSurfaceMaterial m_defaultSurfaceMaterial; // Surface tag ignored for default
        AZStd::vector<TerrainSurfaceMaterialMapping> m_surfaceMaterials;
    };

    class TerrainSurfaceMaterialsListComponent
        : public AZ::Component
        , private TerrainAreaMaterialRequestBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
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

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    private:
        void HandleMaterialStateChanges();
        int CountMaterialIdInstances(AZ::Data::AssetId id) const;

        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        //////////////////////////////////////////////////////////////////////////
        // TerrainAreaMaterialRequestBus
        const AZ::Aabb& GetTerrainSurfaceMaterialRegion() const override;
        const AZStd::vector<TerrainSurfaceMaterialMapping>& GetSurfaceMaterialMappings() const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        TerrainSurfaceMaterialsListConfig m_configuration;

        AZ::Aabb m_cachedAabb;
    };
} // namespace Terrain
