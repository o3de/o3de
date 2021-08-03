/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>

#include "platform.h"
#include "ISystem.h"
#include "IRenderer.h"
#include "I3DEngine.h"
#include "ITerrain.h"
#include "MathConversion.h"

#include <Terrain/Bus/TerrainBus.h>
#include <Terrain/Bus/WorldMaterialRequestsBus.h>

#include <Terrain/Bus/TerrainProviderBus.h>

#include <Terrain/Bus/TerrainRendererBus.h>

#include <CrySystemBus.h>

/*
    TerrainProvider is largely invoked from the rendering thread inside Terrain::CRETerrain::mfDraw()
*/

class TerrainProvider
    : public Terrain::TerrainProviderRequestBus::Handler
    , public Terrain::TerrainDataRequestBus::Handler
    , private Terrain::TerrainProviderNotificationBus::Handler
    , private Terrain::TerrainSystemServiceRequestBus::Handler
{
public:
    TerrainProvider();
    ~TerrainProvider();

    ///////////////////////////////////////////
    // TerrainProviderRequestBus::Handler Impl
    AZ::Aabb GetWorldBounds() override;
    AZ::Vector3 GetRegionSize() override;
    void GetRegionIndex(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, int& regionIndexX, int& regionIndexY) override;
    float GetHeightAtIndexedPosition(int ix, int iy) override;
    float GetHeightAtWorldPosition(float fx, float fy) override;
    unsigned char GetSurfaceTypeAtIndexedPosition(int ix, int iy) override;

    ///////////////////////////////////////////
    // TerrainProviderNotificationBus::Handler Impl
    void SynchronizeSettings(Terrain::CRETerrainContext* context) override;

    ///////////////////////////////////////////
    // TerrainDataRequestBus::Handler Impl
    float GetHeightmapCellSize() override;

    float GetHeightSynchronous(float x, float y) override;
    AZ::Vector3 GetNormalSynchronous(float x, float y) override;

    void GetHeight(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outPosition) override;
    void GetNormal(const AZ::Vector3& inPosition, Sampler sampleFilter, AZ::Vector3& outNormal) override;
    void GetSurfaceWeights(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfaceTagWeightMap& outSurfaceWeights) override;
    void GetSurfacePoint(const AZ::Vector3& inPosition, Sampler sampleFilter, SurfaceData::SurfacePoint& outSurfacePoint) override;
    void ProcessHeightsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) override;
    void ProcessSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, Sampler sampleFilter, SurfacePointRegionFillCallback perPositionCallback, TerrainDataReadyCallback onComplete = nullptr) override;

    CShader* GetTerrainHeightGeneratorShader() const override;
    CShader* GetTerrainMaterialCompositingShader() const override;
    ///////////////////////////////////////////

    void SetWorldMin(AZ::Vector3 worldOrigin) { m_worldBounds.SetMin(worldOrigin); m_terrainVersionDirty = true; }
    void SetWorldMax(AZ::Vector3 worldBounds) { m_worldBounds.SetMax(worldBounds); m_terrainVersionDirty = true; }
    void SetRegionBounds(AZ::Vector3 regionBounds) { m_regionBounds = regionBounds; m_terrainVersionDirty = true; }
    void SetHeightmapCellSize(float cellSize) { m_heightmapCellSize = cellSize;  m_terrainVersionDirty = true; }
    void SetMaterialName(AZStd::string materialName) { m_worldMaterialName = materialName;  m_terrainVersionDirty = true; }

    static constexpr float DefaultTerrainHeight = 32.0f;


    void RegisterArea(AZ::EntityId areaId) override;
    void UnregisterArea(AZ::EntityId areaId) override;
    void RefreshArea(AZ::EntityId areaId) override;

private:
    // m_context is available to the other members chained during a call to executeRendering()
    Terrain::CRETerrainContext* m_context = nullptr;

    bool m_terrainVersionDirty = true;
    bool m_terrainHeightDirty = false;
    AZ::Aabb m_dirtyRegion;

    // Cached pointers to I3DEngine and ITerrain - the calls to retrieve them use EBuses that expect to be used from the main thread,
    // but our access is from multiple threads, so we retrieve and cache them on the main thread.
    ITerrain* m_terrain = nullptr;
    I3DEngine* m_3dEngine = nullptr;


    AZ::Aabb m_worldBounds;
    AZ::Vector3 m_regionBounds{ 2048.0f, 2048.0f, 2048.0f };
    float m_heightmapCellSize = { 1.0f };
    AZStd::string m_worldMaterialName;
    CShader* m_terrainHeightGenerator = nullptr;
    CShader* m_terrainMaterialCompositing = nullptr;

    mutable AZStd::shared_mutex m_areaMutex;
    AZStd::unordered_map<AZ::EntityId, AZ::Aabb> m_registeredAreas;
};
