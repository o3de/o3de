/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Physics/Material.h>
#include <SurfaceData/SurfaceTag.h>
#include <TerrainSystem/TerrainSystemBus.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    static const uint8_t InvalidSurfaceTagIndex = 0xFF;

    struct TerrainPhysicsSurfaceMaterialMapping final
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsSurfaceMaterialMapping, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainPhysicsSurfaceMaterialMapping, "{A88B5289-DFCD-4564-8395-E2177DFE5B18}");
        static void Reflect(AZ::ReflectContext* context);

        SurfaceData::SurfaceTag m_surfaceTag;
        Physics::MaterialId m_materialId;
    };

    class TerrainPhysicsColliderConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsColliderConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainPhysicsColliderConfig, "{E9EADB8F-C3A5-4B9C-A62D-2DBC86B4CE59}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<TerrainPhysicsSurfaceMaterialMapping> m_surfaceMaterialMappings;
    };


    class TerrainPhysicsColliderComponent
        : public AZ::Component
        , public Physics::HeightfieldProviderRequestsBus::Handler
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
        , protected AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainPhysicsColliderComponent, "{33C20287-1D37-44D0-96A0-2C3766E23624}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainPhysicsColliderComponent(const TerrainPhysicsColliderConfig& configuration);
        TerrainPhysicsColliderComponent();
        ~TerrainPhysicsColliderComponent() = default;

        // HeightfieldProviderRequestsBus
        AZ::Vector2 GetHeightfieldGridSpacing() const override;
        void GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const override;
        int32_t GetHeightfieldGridColumns() const override;
        int32_t GetHeightfieldGridRows() const override;
        void GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const override;
        float GetHeightfieldMinHeight() const override;
        float GetHeightfieldMaxHeight() const override;
        AZ::Aabb GetHeightfieldAabb() const override;
        AZ::Transform GetHeightfieldTransform() const override;
        AZStd::vector<Physics::MaterialId> GetMaterialList() const override;
        AZStd::vector<float> GetHeights() const override;
        AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        uint8_t GetMaterialIdIndex(const Physics::MaterialId& materialId, const AZStd::vector<Physics::MaterialId>& materialList) const;
        Physics::MaterialId FindMaterialIdForSurfaceTag(const SurfaceData::SurfaceTag tag) const;

        void GenerateHeightsInBounds(AZStd::vector<float>& heights) const;
        void GenerateHeightsAndMaterialsInBounds(AZStd::vector<Physics::HeightMaterialPoint>& heightMaterials) const;

        void NotifyListenersOfHeightfieldDataChange();

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        void OnTerrainDataCreateEnd() override;
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

    private:
        TerrainPhysicsColliderConfig m_configuration;
    };
}
