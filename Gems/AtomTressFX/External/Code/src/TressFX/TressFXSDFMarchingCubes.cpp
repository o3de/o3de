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

#include "MarchingCubesTables.h"
#include "TressFXSDFMarchingCubes.h"
#include "TressFXSDFCollision.h"
#include "EngineInterface.h"

TressFXSDFMarchingCubes::TressFXSDFMarchingCubes() 
    : m_MaxMarchingCubesVertices(128 * 1024)
    , m_MarchingCubesTriangleVerticesUAV(nullptr)
    , m_NumMarchingCubesVerticesUAV(nullptr)
    , m_MarchingCubesEdgeTableSRV(nullptr)
    , m_MarchingCubesTriangleTableSRV(nullptr)
{}

void TressFXSDFMarchingCubes::Initialize(const char * name, EI_Scene* gltfImplementation, EI_RenderTargetSet * renderPass)
{
    m_pGLTFImplementation = gltfImplementation;

    EI_Device* pDevice = GetDevice();

    // load a compute shader
    EI_BindLayout * layouts[] = { GetSDFMarchingCubesLayout() };
    m_pComputeEffectInitializeMCVertices = GetDevice()->CreateComputeShaderPSO("TressFXMarchingCubes.hlsl", "InitializeMCVertices", layouts, 1);
    m_pComputeEffectRunMarchingCubesOnSdf = GetDevice()->CreateComputeShaderPSO("TressFXMarchingCubes.hlsl", "RunMarchingCubesOnSdf", layouts, 1);

    EI_PSOParams psoParams;
    psoParams.depthTestEnable = true;
    psoParams.depthWriteEnable = true;
    psoParams.depthCompareOp = EI_CompareFunc::LessEqual;

    psoParams.colorBlendParams.colorBlendEnabled = false;
    psoParams.colorBlendParams.colorBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.colorSrcBlend = EI_BlendFactor::Zero;
    psoParams.colorBlendParams.colorDstBlend = EI_BlendFactor::SrcColor;
    psoParams.colorBlendParams.alphaBlendOp = EI_BlendOp::Add;
    psoParams.colorBlendParams.alphaSrcBlend = EI_BlendFactor::Zero;
    psoParams.colorBlendParams.alphaDstBlend = EI_BlendFactor::SrcAlpha;

    psoParams.layouts = layouts;
    psoParams.numLayouts = 1;
    psoParams.renderTargetSet = renderPass;
    m_pRenderEffect = GetDevice()->CreateGraphicsPSO("TressFXMarchingCubes.hlsl", "MarchingCubesVS", "TressFXMarchingCubes.hlsl", "MarchingCubesPS", psoParams);

    AMD::uint32 vertexBlockSize = sizeof(VertexData);
    const int         NUM_ELEMENTS_EDGE_TABLE = 256;
    const int         NUM_ELEMENTS_TRI_TABLE = 256 * 16;

    m_MarchingCubesTriangleVerticesUAV = pDevice->CreateBufferResource(vertexBlockSize, m_MaxMarchingCubesVertices, EI_BF_NEEDSUAV, "MCTriVerts");
    m_NumMarchingCubesVerticesUAV = pDevice->CreateBufferResource(sizeof(int), 1, EI_BF_NEEDSUAV, "NumMCVerts");
    m_MarchingCubesEdgeTableSRV = pDevice->CreateBufferResource(sizeof(int), NUM_ELEMENTS_EDGE_TABLE, 0, "MCEdgeTable");
    m_MarchingCubesTriangleTableSRV = pDevice->CreateBufferResource(sizeof(int), NUM_ELEMENTS_TRI_TABLE, 0, "MCTriTable");

    m_pUniformBuffer = pDevice->CreateBufferResource(sizeof(TressFXMarchingCubesUniformBuffer), 1, EI_BF_UNIFORMBUFFER, "ConstantBuffer_MC");
    //------------------
    // Initial data.
    //
    // Just need tables for generating marching cubes.
    // edge and triangle table should probably be combined at some point.
    //------------------

    EI_CommandContext& commandContext = pDevice->GetCurrentCommandContext();

    EI_Barrier uavToCopy[] =
    {
        { m_MarchingCubesTriangleVerticesUAV.get() , EI_STATE_UAV, EI_STATE_COPY_DEST},
        { m_NumMarchingCubesVerticesUAV.get(), EI_STATE_UAV, EI_STATE_COPY_DEST }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uavToCopy), uavToCopy);

    // write zero for the number of vertices.
    int zero = 0;
    commandContext.UpdateBuffer(m_NumMarchingCubesVerticesUAV.get(), &zero);

    // Fill in the edge table.
    commandContext.UpdateBuffer(m_MarchingCubesEdgeTableSRV.get(), (void*)MARCHING_CUBES_EDGE_TABLE);

    // Fill in the triangle table.
    commandContext.UpdateBuffer(m_MarchingCubesTriangleTableSRV.get(), (void*)MARCHING_CUBES_TRIANGLE_TABLE);

    EI_Barrier uploadDone[] =
    {
        { m_MarchingCubesTriangleVerticesUAV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_NumMarchingCubesVerticesUAV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_MarchingCubesEdgeTableSRV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_MarchingCubesTriangleTableSRV.get(), EI_STATE_COPY_DEST, EI_STATE_SRV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uploadDone), uploadDone);

    assert(m_pSDF);
    // Create bindet
    EI_Resource& sdfDataBuffer = m_pSDF->GetSDFDataGPUBuffer();
    EI_BindSetDescription desc =
    {
        { m_MarchingCubesEdgeTableSRV.get(), m_MarchingCubesTriangleTableSRV.get(), &sdfDataBuffer, m_MarchingCubesTriangleVerticesUAV.get(), m_NumMarchingCubesVerticesUAV.get(), m_pUniformBuffer.get() }
    };
    m_pBindSet = pDevice->CreateBindSet(GetSDFMarchingCubesLayout(), desc);
}

void TressFXSDFMarchingCubes::Draw()
{
    //----------------------------
    // Draw marching cubes mesh
    //----------------------------
    m_uniformBufferData.cColor = { 1.0f, 1.0f, 0.0f };
    m_uniformBufferData.vLightDir = { 1.0f, 1.0f, 1.0f };
    m_uniformBufferData.mMW = m_pGLTFImplementation->GetMV();
    m_uniformBufferData.mMWP = m_pGLTFImplementation->GetMVP();

 
    EI_CommandContext& context = GetDevice()->GetCurrentCommandContext();

    EI_BindSet* bindSets[] = { m_pBindSet.get() };
    context.BindSets(m_pRenderEffect.get(), 1, bindSets);
    EI_DrawParams drawParams;
    drawParams.numVertices = m_MaxMarchingCubesVertices;
    drawParams.numInstances = 1;
    context.DrawInstanced(*m_pRenderEffect, drawParams);
}

void TressFXSDFMarchingCubes::DrawGrid()
{
    /*
    // Render SDF grid
    m_LineRenderer.SetColor(SuVector4(0.f, 0.f, 1.f, 0));

    for (int j = 0; j <= m_NumCellsY; ++j)
    {
        for (int k = 0; k <= m_NumCellsZ; ++k)
        {
            SuVector3 p0 = m_Origin + SuVector3(0, j * m_CellSize, k * m_CellSize);
            SuVector3 p1 = p0 + SuVector3(m_NumCellsX * m_CellSize, 0, 0);

            m_LineRenderer.DrawLine((SuPoint3&)p0, (SuPoint3&)p1);
        }

        for (int i = 0; i <= m_NumCellsX; ++i)
        {
            SuVector3 p0 = m_Origin + SuVector3(i * m_CellSize, j * m_CellSize, 0);
            SuVector3 p1 = p0 + SuVector3(0, 0, m_NumCellsZ * m_CellSize);

            m_LineRenderer.DrawLine((SuPoint3&)p0, (SuPoint3&)p1);
        }
    }
    */
}

void TressFXSDFMarchingCubes::Update(EI_CommandContext& commandContext)
{
    assert(m_pSDF);

    m_Origin = { m_pSDF->GetGridOrigin().x, m_pSDF->GetGridOrigin().y, m_pSDF->GetGridOrigin().z };
    m_CellSize = m_pSDF->GetGridCellSize();
    m_pSDF->GetGridNumCells(m_NumCellsX, m_NumCellsY, m_NumCellsZ);
    m_NumTotalCells = m_pSDF->GetGridNumTotalCells();

    EI_Barrier barrier1[] =
    {
        { m_MarchingCubesTriangleVerticesUAV.get(), EI_STATE_SRV, EI_STATE_UAV },
        { m_NumMarchingCubesVerticesUAV.get(), EI_STATE_SRV, EI_STATE_UAV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(barrier1), barrier1);

    //-------------------------------
    // RunMarchingCubesOnSdf
    //-------------------------------
    {
        // Set the constant buffer parameters
        TressFXMarchingCubesUniformBuffer& constBuffer = m_uniformBufferData;
        constBuffer.g_MaxMarchingCubesVertices = m_MaxMarchingCubesVertices;
        constBuffer.g_MarchingCubesIsolevel = m_CellSize * m_SDFIsoLevel;

        // Set the constant buffer parameters
        constBuffer.g_Origin = { m_Origin.x, m_Origin.y, m_Origin.z, 0.0f };
        constBuffer.g_CellSize = m_CellSize;
        constBuffer.g_NumCellsX = m_NumCellsX;
        constBuffer.g_NumCellsY = m_NumCellsY;
        constBuffer.g_NumCellsZ = m_NumCellsZ;

        commandContext.UpdateBuffer(m_pUniformBuffer.get(), &constBuffer);

        EI_Barrier copyToResource = { m_pUniformBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_CONSTANT_BUFFER };
        commandContext.SubmitBarrier(1, &copyToResource);

        EI_BindSet* bindSets = { m_pBindSet.get() };
        commandContext.BindSets(m_pComputeEffectInitializeMCVertices.get(), 1, &bindSets);

        // Run InitializeMCVertices. One thread per each cell
        {
            int numDispatchSize = (int)ceil((float)m_MaxMarchingCubesVertices /
                (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
            commandContext.BindPSO(m_pComputeEffectInitializeMCVertices.get());
            commandContext.Dispatch(numDispatchSize);
        }

        commandContext.BindSets(m_pComputeEffectRunMarchingCubesOnSdf.get(), 1, &bindSets);

        // Run RunMarchingCubesOnSdf. One thread per each cell
        {
            int numDispatchSize =
                (int)ceil((float)m_NumTotalCells / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
            commandContext.BindPSO(m_pComputeEffectRunMarchingCubesOnSdf.get());
            commandContext.Dispatch(numDispatchSize);
        }
        EI_Resource& sdfDataBuffer = m_pSDF->GetSDFDataGPUBuffer();
    }
    EI_Barrier barrier[] =
    {
        { m_MarchingCubesTriangleVerticesUAV.get(), EI_STATE_UAV, EI_STATE_SRV },
        { m_NumMarchingCubesVerticesUAV.get(), EI_STATE_UAV, EI_STATE_SRV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(barrier), barrier);
}