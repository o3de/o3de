/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Aabb.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainSystem/TerrainSystemBus.h>

namespace Terrain
{
    class TerrainSystem
        : public AzFramework::Terrain::TerrainDataRequestBus::Handler
        , private Terrain::TerrainSystemServiceRequestBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        TerrainSystem();
        ~TerrainSystem();

        ///////////////////////////////////////////
        // TerrainSystemServiceRequestBus::Handler Impl

        void SetWorldMin(AZ::Vector3 worldOrigin) override;
        void SetWorldMax(AZ::Vector3 worldBounds) override;
        void SetHeightQueryResolution(AZ::Vector2 queryResolution) override;
        void SetDebugWireframe(bool wireframeEnabled) override;

        void Activate() override;
        void Deactivate() override;

        void RegisterArea(AZ::EntityId areaId) override;
        void UnregisterArea(AZ::EntityId areaId) override;
        void RefreshArea(AZ::EntityId areaId) override;

        ///////////////////////////////////////////
        // TerrainDataRequestBus::Handler Impl
        AZ::Vector2 GetTerrainGridResolution() const override;
        AZ::Aabb GetTerrainAabb() const override;

        //! Returns terrains height in meters at location x,y.
        //! @terrainExistsPtr: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will become false,
        //!                  otherwise *terrainExistsPtr will become true.
        float GetHeight(AZ::Vector3 position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        float GetHeightFromFloats(float x, float y, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        //! Given an XY coordinate, return the max surface type and weight.
        //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will be set to false,
        //!                  otherwise *terrainExistsPtr will be set to true.
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(
            AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(
            float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        //! Convenience function for  low level systems that can't do a reverse lookup from Crc to string. Everyone else should use
        //! GetMaxSurfaceWeight or GetMaxSurfaceWeightFromFloats. Not available in the behavior context. Returns nullptr if the position is
        //! inside a hole or outside of the terrain boundaries.
        const char* GetMaxSurfaceName(
            AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

        //! Returns true if there's a hole at location x,y.
        //! Also returns true if there's no terrain data at location x,y.
        bool GetIsHoleFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR) const override;

        // Given an XY coordinate, return the surface normal.
        //! @terrainExists: Can be nullptr. If != nullptr then, if there's no terrain at location x,y or location x,y is inside a terrain
        //! HOLE then *terrainExistsPtr will be set to false,
        //!                  otherwise *terrainExistsPtr will be set to true.
        AZ::Vector3 GetNormal(
            AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
        AZ::Vector3 GetNormalFromFloats(
            float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

    private:
        float GetHeightSynchronous(float x, float y) const;
        AZ::Vector3 GetNormalSynchronous(float x, float y) const;

        AZ::EntityId TerrainSystem::FindBestLayerSpawnerAt(const AZ::u32 x, const AZ::u32 y) const;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void SystemActivate();
        void SystemDeactivate();

        struct TerrainSystemSettings
        {
            AZ::Aabb m_worldBounds;
            AZ::Vector2 m_heightQueryResolution{ 1.0f };
            bool m_debugWireframeEnabled{ false };
            bool m_systemActive{ false };
        };

        TerrainSystemSettings m_currentSettings;
        TerrainSystemSettings m_requestedSettings;

        bool m_terrainSettingsDirty = true;
        bool m_terrainHeightDirty = false;
        AZ::Aabb m_dirtyRegion;

        mutable AZStd::shared_mutex m_areaMutex;
        AZStd::unordered_map<AZ::EntityId, AZ::Aabb> m_registeredAreas;
    };
} // namespace Terrain
