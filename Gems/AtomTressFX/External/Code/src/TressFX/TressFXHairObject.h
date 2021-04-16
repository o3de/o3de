// ----------------------------------------------------------------------------
// Interface to strands of hair.  Both in terms of rendering, and the data 
// required for simulation.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef TRESSFXHAIROBJECT_H_
#define TRESSFXHAIROBJECT_H_

// AMD
#include "AMD_Types.h"

// TressFX
#include "TressFXAsset.h"
#include "TressFXConstantBuffers.h"
#include "AMD_Types.h"
#include "AMD_TressFX.h"
#include "TressFXCommon.h"
#include "TressFXSettings.h"
#include "EngineInterface.h"

// Contains the dynamic data
// which is used by 3 modules:
// simulation, sigend distance field, and rendering.
// Rendering uses current position and tangent as SRVs in VS.
class TressFXDynamicState
{
public:
    void CreateGPUResources(EI_Device* pDevice, int numVertices, int numStrands, const char * name, TressFXAsset* asset);
    void UploadGPUData(EI_CommandContext& commandContext, void* pos, void* tan, int numVertices);

    // UAV Barriers on buffers used in simulation.
    void UAVBarrier(EI_CommandContext& commandContext);

    // Transitions to move between simulation (UAV) and rendering (SRV).
    void TransitionSimToRendering(EI_CommandContext& commandContext);
    void TransitionRenderingToSim(EI_CommandContext& commandContext);

    EI_BindSet& GetSimBindSet() { return *m_pSimBindSets; }
    EI_BindSet& GetApplySDFBindSet() 
    { 
        return *m_pApplySDFBindSets; 
    }
    EI_BindSet& GetRenderBindSet() { return *m_pRenderBindSets; }

private:
    std::unique_ptr<EI_Resource> m_Positions;
    std::unique_ptr<EI_Resource> m_Tangents;
    std::unique_ptr<EI_Resource> m_PositionsPrev;
    std::unique_ptr<EI_Resource> m_PositionsPrevPrev;
    std::unique_ptr<EI_Resource> m_StrandLevelData;

    std::unique_ptr<EI_BindSet> m_pSimBindSets;
    std::unique_ptr<EI_BindSet> m_pApplySDFBindSets;  // updated by
    std::unique_ptr<EI_BindSet> m_pRenderBindSets;    // used for rendering.
};

struct TressFXStrandLevelData
{
    AMD::float4 skinningQuat;
    AMD::float4 vspQuat;
    AMD::float4 vspTranslation;
};

class TressFXHairObject : private TressFXNonCopyable
{
public:
    TressFXHairObject(TressFXAsset* asset,
        EI_Device*   pDevice,
        EI_CommandContext&  commandContext,
        const char *      name, int RenderIndex);

    // pBoneMatricesInWS constains array of column major bone matrices in world space.
    void UpdateBoneMatrices(const AMD::float4x4* pBoneMatricesInWS, int numBoneMatrices);

    void UpdateConstantBuffer(EI_CommandContext& commandContext);

    // update collision capsules
    void UpdateCapsuleCollisions();

    void UpdateSimulationParameters(const TressFXSimulationSettings* parameters, float timeStep);
    void UpdateRenderingParameters(const TressFXRenderingSettings* parameters, const int NodePoolSize, float timeStep, float Distance, bool ShadowUpdate = false);

    void ResetPositions() { m_SimCB[m_SimulationFrame%2]->g_ResetPositions = 1.0f; }

    // Rendering
    void DrawStrands(EI_CommandContext& commandContext,
                     EI_PSO&            pso,
                     EI_BindSet**		extraBindSets = nullptr,
                     uint32_t			numExtraBindSets = 0);

    TressFXDynamicState& GetDynamicState() { return m_DynamicState; }

    int GetNumTotalHairVertices() const { return m_NumTotalVertices; }
    int GetNumTotalHairStrands() const { return m_NumTotalStrands; }
    int GetNumVerticesPerStrand() const { return m_NumVerticesPerStrand; }
    int GetCPULocalShapeIterations() const { return  m_CPULocalShapeIterations; }
    int GetNumFollowHairsPerGuideHair() const { return m_NumFollowHairsPerGuideHair; }

    EI_BindSet* GetRenderLayoutBindSet() const { return m_pRenderLayoutBindSet.get(); }

    // Get hair asset info
    int GetNumTotalHairVertices() { return m_NumTotalVertices; }
    int GetNumTotalHairStrands() { return m_NumTotalStrands; }
    int GetNumVerticesPerStrand() { return m_NumVerticesPerStrand; }

    EI_BindSet * GetSimBindSet() { return m_pSimBindSet[m_SimulationFrame%2].get(); }
    void UpdatePerObjectRenderParams(EI_CommandContext& commandContext);
    void IncreaseSimulationFrame() { m_SimCB[m_SimulationFrame%2]->g_ResetPositions = 0.0f; m_SimulationFrame++; }

    void PopulateDrawStrandsBindSet(EI_Device* pDevice, TressFXRenderingSettings* pRenderSettings=nullptr);
private:
    // Turn raw data into GPU resources for rendering.
    void CreateRenderingGPUResources(EI_Device*   pDevice,
                                     TressFXAsset& asset,
                                     const char *      name);
    void UploadRenderingGPUResources(EI_CommandContext& commandContext, TressFXAsset& asset);


    // set wind parameters for simulation
    void SetWind(const Vector3& windDir, float windMag, int frame);

    // hair asset information
    int m_NumTotalVertices;
    int m_NumTotalStrands;
    int m_NumVerticesPerStrand;
    int m_CPULocalShapeIterations;
    int m_NumFollowHairsPerGuideHair;

    // frame counter for wind effect
    int m_SimulationFrame;

    // for parameter indexing
    int m_RenderIndex;

    // For LOD calculations
    float m_LODHairDensity = 1.0f;

    // simulation control params
    TressFXUniformBuffer<TressFXSimulationParams> m_SimCB[2];
    TressFXUniformBuffer<TressFXRenderParams> m_RenderCB;
    TressFXUniformBuffer<TressFXStrandParams> m_StrandCB;

    // position and tangent
    TressFXDynamicState m_DynamicState;

    std::unique_ptr<EI_Resource> m_InitialHairPositionsBuffer;
    std::unique_ptr<EI_Resource> m_HairRestLengthSRVBuffer;
    std::unique_ptr<EI_Resource> m_HairStrandTypeBuffer;
    std::unique_ptr<EI_Resource> m_FollowHairRootOffsetBuffer;
    std::unique_ptr<EI_Resource> m_BoneSkinningDataBuffer;

    // Textures
    std::unique_ptr<EI_Resource> m_BaseAlbedo = nullptr;
    std::unique_ptr<EI_Resource> m_StrandAlbedo = nullptr;

    // SRVs for rendering
    std::unique_ptr<EI_Resource> m_HairVertexRenderParams;
    std::unique_ptr<EI_Resource> m_HairTexCoords;


    std::unique_ptr<EI_BindSet> m_pRenderLayoutBindSet;

    // For simulation compute shader
    std::unique_ptr<EI_BindSet> m_pSimBindSet[2];

    // index buffer
    std::unique_ptr <EI_Resource> m_pIndexBuffer;
    AMD::uint32     m_TotalIndices;

};

#endif  // TRESSFXHAIROBJECT_H_
