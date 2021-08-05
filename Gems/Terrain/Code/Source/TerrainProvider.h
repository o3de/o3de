/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
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

#include <TerrainBus.h>
#include <TerrainProviderBus.h>

class TerrainProvider
    : public Terrain::TerrainDataRequestBus::Handler
    , private Terrain::TerrainSystemServiceRequestBus::Handler
    , private AZ::TickBus::Handler
{
public:
    TerrainProvider();
    ~TerrainProvider();

    ///////////////////////////////////////////
    // TerrainDataRequestBus::Handler Impl
    AZ::Vector2 GetTerrainGridResolution() const override;
    AZ::Aabb GetTerrainAabb() const override;

    float GetHeightSynchronous(float x, float y) override;
    AZ::Vector3 GetNormalSynchronous(float x, float y) override;

    void GetHeight(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outPosition) override;
    void GetNormal(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outNormal) override;
    void GetSurfaceWeights(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfaceTagWeightMap& outSurfaceWeights) override;
    void GetSurfacePoint(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint) override;
    void ProcessHeightsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) override;
    void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) override;

    ///////////////////////////////////////////

    void SetWorldMin(AZ::Vector3 worldOrigin) { m_worldBounds.SetMin(worldOrigin); m_terrainVersionDirty = true; }
    void SetWorldMax(AZ::Vector3 worldBounds) { m_worldBounds.SetMax(worldBounds); m_terrainVersionDirty = true; }
    void SetHeightQueryResolution(AZ::Vector2 queryResolution) { m_heightQueryResolution = queryResolution;  m_terrainVersionDirty = true; }
    void SetDebugWireframe(bool wireframeEnabled) { m_debugWireframeEnabled = wireframeEnabled; }

    static constexpr float DefaultTerrainHeight = 32.0f;


    void RegisterArea(AZ::EntityId areaId) override;
    void UnregisterArea(AZ::EntityId areaId) override;
    void RefreshArea(AZ::EntityId areaId) override;

private:
    // AZ::TickBus::Handler overrides ...
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    bool m_terrainVersionDirty = true;
    bool m_terrainHeightDirty = false;
    AZ::Aabb m_dirtyRegion;

    AZ::Aabb m_worldBounds;
    AZ::Vector2 m_heightQueryResolution{ 1.0f };
    bool m_debugWireframeEnabled{ true };

    mutable AZStd::shared_mutex m_areaMutex;
    AZStd::unordered_map<AZ::EntityId, AZ::Aabb> m_registeredAreas;
};
