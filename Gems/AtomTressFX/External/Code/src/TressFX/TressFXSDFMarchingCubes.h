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

#ifndef TRESSFXSDFMARCHINGCUBES_H_
#define TRESSFXSDFMARCHINGCUBES_H_

#include "TressFXCommon.h"
#include "EngineInterface.h"

class TressFXSDFCollision;
class EI_Scene;

struct TressFXMarchingCubesUniformBuffer {
    AMD::float4x4 mMW;
    AMD::float4x4 mMWP;
    AMD::float4 cColor;
    AMD::float4 vLightDir;
    AMD::float4 g_Origin;
    float g_CellSize;
    int g_NumCellsX;
    int g_NumCellsY;
    int g_NumCellsZ;
    int g_MaxMarchingCubesVertices;
    float g_MarchingCubesIsolevel;
};

class TressFXSDFMarchingCubes : private TressFXNonCopyable
{
public:
    TressFXSDFMarchingCubes();

    void Initialize(const char * name, EI_Scene* gltfImplementation, EI_RenderTargetSet * renderTargetSet);

    // Draw the SDF using marching cubes for debug purpose
    void Draw();

    // Draw the grid
    void DrawGrid();

    // Update mesh by running marching cubes
    void Update(EI_CommandContext& commandContext);

    void SetSDF(TressFXSDFCollision* SDF)
    {
        assert(SDF);
        m_pSDF = SDF;
    }

    // Setting the SDF ISO level for drawing.
    void SetSDFIsoLevel(float isoLevel) { m_SDFIsoLevel = isoLevel; }

private:
    // SDF grid
    AMD::float3 m_Origin;
    float     m_CellSize;
    int       m_NumCellsX;
    int       m_NumCellsY;
    int       m_NumCellsZ;
    int       m_NumTotalCells;

    EI_Scene* m_pGLTFImplementation;

    TressFXSDFCollision* m_pSDF;

    TressFXMarchingCubesUniformBuffer m_uniformBufferData;
    std::unique_ptr<EI_Resource> m_pUniformBuffer;

    std::unique_ptr<EI_BindSet> m_pBindSet;

    // For drawing lines
    // SuBatchLineRenderer m_LineRenderer;

    struct VertexData
    {
        float position[4];
        float normal[4];
    };

    // SDF ISO level. This value will be multiplied with the cell size before be passed to the
    // compute shader.
    float m_SDFIsoLevel;

    const int m_MaxMarchingCubesVertices;
    int       m_NumMCVertices;

    std::unique_ptr<EI_Resource> m_MarchingCubesTriangleVerticesUAV;
    std::unique_ptr<EI_Resource> m_NumMarchingCubesVerticesUAV;

    std::unique_ptr<EI_Resource> m_MarchingCubesEdgeTableSRV;
    std::unique_ptr<EI_Resource> m_MarchingCubesTriangleTableSRV;

    // compute shader
    std::unique_ptr<EI_PSO> m_pComputeEffectInitializeMCVertices;
    std::unique_ptr<EI_PSO> m_pComputeEffectRunMarchingCubesOnSdf;

    // shader for rendering
    std::unique_ptr<EI_PSO> m_pRenderEffect;
};

#endif  // TRESSFXSDFMARCHINGCUBES_H_