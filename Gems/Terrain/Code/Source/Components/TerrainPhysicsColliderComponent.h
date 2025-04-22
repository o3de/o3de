/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
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
    class EditorSurfaceTagListProvider;

    static const uint8_t InvalidSurfaceTagIndex = 0xFF;

    struct TerrainPhysicsSurfaceMaterialMapping final
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsSurfaceMaterialMapping, AZ::SystemAllocator);
        AZ_TYPE_INFO(TerrainPhysicsSurfaceMaterialMapping, "{A88B5289-DFCD-4564-8395-E2177DFE5B18}");
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> BuildSelectableTagList() const;
        void SetTagListProvider(const EditorSurfaceTagListProvider* tagListProvider);

        SurfaceData::SurfaceTag m_surfaceTag;
        AZ::Data::Asset<Physics::MaterialAsset> m_materialAsset;

    private:
        const EditorSurfaceTagListProvider* m_tagListProvider = nullptr;
    };

    class TerrainPhysicsColliderConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsColliderConfig, AZ::SystemAllocator);
        AZ_TYPE_INFO(TerrainPhysicsColliderConfig, "{E9EADB8F-C3A5-4B9C-A62D-2DBC86B4CE59}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<Physics::MaterialAsset> m_defaultMaterialAsset;
        AZStd::vector<TerrainPhysicsSurfaceMaterialMapping> m_surfaceMaterialMappings;
    };


    class TerrainPhysicsColliderComponent
        : public AZ::Component
        , public Physics::HeightfieldProviderRequestsBus::Handler
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
        , protected AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        friend class EditorTerrainPhysicsColliderComponent;
        AZ_COMPONENT(TerrainPhysicsColliderComponent, "{33C20287-1D37-44D0-96A0-2C3766E23624}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainPhysicsColliderComponent(const TerrainPhysicsColliderConfig& configuration);
        TerrainPhysicsColliderComponent();
        ~TerrainPhysicsColliderComponent() = default;

        // HeightfieldProviderRequestsBus
        AZ::Vector2 GetHeightfieldGridSpacing() const override;
        void GetHeightfieldGridSize(size_t& numColumns, size_t& numRows) const override;
        AZ::u64 GetHeightfieldGridColumns() const override;
        AZ::u64 GetHeightfieldGridRows() const override;
        void GetHeightfieldHeightBounds(float& minHeightBounds, float& maxHeightBounds) const override;
        float GetHeightfieldMinHeight() const override;
        float GetHeightfieldMaxHeight() const override;
        AZ::Aabb GetHeightfieldAabb() const override;
        AZ::Transform GetHeightfieldTransform() const override;
        AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> GetMaterialList() const override;
        AZStd::vector<float> GetHeights() const override;
        AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const override;

        void GetHeightfieldIndicesFromRegion(
            const AZ::Aabb& region, size_t& startColumn, size_t& startRow, size_t& numColumns, size_t& numRows) const override;

        //! Updates the list of heights and materials within the region.
        void UpdateHeightsAndMaterials(
            const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
            size_t startColumn, size_t startRow, size_t numColumns, size_t numRows) const override;

        void UpdateHeightsAndMaterialsAsync(
            const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
            const Physics::UpdateHeightfieldCompleteFunction& updateHeightsCompleteCallback,
            size_t startColumn,
            size_t startRow,
            size_t numColumns,
            size_t numRows) const override;

        void UpdateConfiguration(const TerrainPhysicsColliderConfig& newConfiguration);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        uint8_t GetMaterialIndex(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset, const AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>>& materialList) const;
        AZ::Data::Asset<Physics::MaterialAsset> FindMaterialAssetForSurfaceTag(const SurfaceData::SurfaceTag tag) const;

        void GenerateHeightsInBounds(AZStd::vector<float>& heights) const;

        void NotifyListenersOfHeightfieldDataChange(
            Physics::HeightfieldProviderNotifications::HeightfieldChangeMask heightfieldChangeMask,
            const AZ::Aabb& dirtyRegion);

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        void OnTerrainDataCreateEnd() override;
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        void CalculateHeightfieldRegion();
        void BuildSurfaceTagToMaterialIndexLookup();

    private:
        // The default material will always be the first material in the material list.
        static inline constexpr uint8_t DefaultMaterialIndex = 0;

        TerrainPhysicsColliderConfig m_configuration;
        AZStd::atomic_bool m_terrainDataActive = false;
        AzFramework::Terrain::TerrainQueryRegion m_heightfieldRegion;
        AZStd::unordered_map<SurfaceData::SurfaceTag, uint8_t> m_surfaceTagToMaterialIndexLookup;

        // Protect state reads from happening in parallel with state writes.
        mutable AZStd::shared_mutex m_stateMutex;
    };
}
