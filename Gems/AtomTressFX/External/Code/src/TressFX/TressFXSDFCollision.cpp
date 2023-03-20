// ----------------------------------------------------------------------------
// This wraps a single signed distance field, used for TressFX collision.
// it provides the interface to generate the signed distance field, as well
// as apply that signed distance field to a TressFX simulation.
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


// TressFX
#include "TressFXAsset.h"
#include "TressFXHairObject.h"
#include "TressFXLayouts.h"
#include "TressFXSDFCollision.h"
#include "TressFXSDFInputMeshInterface.h"

#include <limits.h>

TressFXSDFCollision::TressFXSDFCollision(
    EI_Device* pDevice,
    TressFXSDFInputMeshInterface* pCollMesh,
    const char * modelName,
    int numCellsInX,
    float collisionMargin)
    : m_CollisionMargin(collisionMargin)
    , m_NumCellsInXAxis(numCellsInX)
    , m_GridAllocationMutliplier(1.4f)
    , m_NumTotalCells(INT_MAX)
    , m_pSimBindSet(nullptr)

{
    m_pInputCollisionMesh = pCollMesh;

    // initialize SDF grid using the associated model's bounding box
    Vector3 bmin, bmax;
    m_pInputCollisionMesh->GetInitialBoundingBox(bmin, bmax);
    m_CellSize = (bmax.x - bmin.x) / m_NumCellsInXAxis;
    int numExtraPaddingCells = (int)(0.8f * (float)m_NumCellsInXAxis);
    m_PaddingBoundary = numExtraPaddingCells * m_CellSize, numExtraPaddingCells * m_CellSize,
        numExtraPaddingCells * m_CellSize;

    UpdateSDFGrid(bmin, bmax);

    bmin -= m_PaddingBoundary;
    bmax += m_PaddingBoundary;

    m_NumCellsX = (int)((bmax.x - bmin.x) / m_CellSize);
    m_NumCellsY = (int)((bmax.y - bmin.y) / m_CellSize);
    m_NumCellsZ = (int)((bmax.z - bmin.z) / m_CellSize);
    m_NumTotalCells =
        TressFXMin(m_NumTotalCells,
        (int)(m_GridAllocationMutliplier * m_NumCellsX * m_NumCellsY * m_NumCellsZ));

    // UAV
    m_SignedDistanceFieldUAV =
        pDevice->CreateBufferResource(sizeof(int), m_NumTotalCells, EI_BF_NEEDSUAV, "SDF");

    // constant buffer
    m_pConstantBufferResource = pDevice->CreateBufferResource(sizeof(TressFXSDFCollisionParams), 1, EI_BF_UNIFORMBUFFER, "TressFXSDFCollisionConstantBuffer");

    // Bind set
    EI_BindSetDescription bindSet =
    {
        { &m_pInputCollisionMesh->GetTrimeshVertexIndicesBuffer(), m_SignedDistanceFieldUAV.get(), &m_pInputCollisionMesh->GetMeshBuffer(), m_pConstantBufferResource.get() }
    };
    m_pSimBindSet = pDevice->CreateBindSet(GetGenerateSDFLayout(), bindSet);
}

void TressFXSDFCollision::UpdateSDFGrid(const Vector3& tight_bbox_min,
                                        const Vector3& tight_bbox_max)
{
    Vector3 bmin = tight_bbox_min - m_PaddingBoundary;
    m_Origin               = bmin;
}

void TressFXSDFCollision::Update(EI_CommandContext&          commandContext,
                                 TressFXSDFCollisionSystem& system)
{
    if (!m_pInputCollisionMesh)
        return;

    EI_Marker marker(commandContext, "SDFUpdate");

    // Update the grid data based on the current bounding box
    Vector3 min, max;
    m_pInputCollisionMesh->GetBoundingBox(min, max);
    UpdateSDFGrid(min, max);

    // Set the constant buffer parameters
    m_ConstBuffer.m_Origin.x  = m_Origin.x;
    m_ConstBuffer.m_Origin.y  = m_Origin.y;
    m_ConstBuffer.m_Origin.z  = m_Origin.z;
    m_ConstBuffer.m_Origin.w  = 0;
    m_ConstBuffer.m_CellSize  = m_CellSize;
    m_ConstBuffer.m_NumCellsX = m_NumCellsX;
    m_ConstBuffer.m_NumCellsY = m_NumCellsY;
    m_ConstBuffer.m_NumCellsZ = m_NumCellsZ;

    commandContext.UpdateBuffer(m_pConstantBufferResource.get(), &m_ConstBuffer);

    // Binding UAVs, SRVs and CBs

    // Run InitializeSignedDistanceField. One thread per one cell.
    {
        int numDispatchSize =
            (int)ceil((float)m_NumTotalCells / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
        commandContext.BindPSO(system.m_InitializeSignedDistanceFieldPSO.get());
        EI_BindSet* bindSets[] = { m_pSimBindSet.get() };
        commandContext.BindSets(system.m_InitializeSignedDistanceFieldPSO.get(), 1, bindSets);
        commandContext.Dispatch(numDispatchSize);
        GetDevice()->GetTimeStamp("InitializeSignedDistanceField");
    }


    EI_Barrier uavMeshAndSDF[] = 
    {
        { & (m_pInputCollisionMesh->GetMeshBuffer()), EI_STATE_UAV, EI_STATE_UAV },
        { m_SignedDistanceFieldUAV.get(), EI_STATE_UAV, EI_STATE_UAV }
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uavMeshAndSDF), uavMeshAndSDF);

    // Run ConstructSignedDistanceField. One thread per each triangle
    {
        int numDispatchSize = (int)ceil((float)m_pInputCollisionMesh->GetNumMeshTriangle() /
            (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
        commandContext.BindPSO(system.m_ConstructSignedDistanceFieldPSO.get());
        EI_BindSet* bindSets[] = { m_pSimBindSet.get() };
        commandContext.BindSets(system.m_ConstructSignedDistanceFieldPSO.get(), 1, bindSets);
        commandContext.Dispatch(numDispatchSize);
        GetDevice()->GetTimeStamp("ConstructSignedDistanceField");
    }

    // State transition for DX12
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uavMeshAndSDF), uavMeshAndSDF);

    // Run FinalizeSignedDistanceField. One thread per each triangle
    {
        int numDispatchSize =
            (int)ceil((float)m_NumTotalCells / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
        commandContext.BindPSO(system.m_FinalizeSignedDistanceFieldPSO.get());
        EI_BindSet* bindSets[] = { m_pSimBindSet.get() };
        commandContext.BindSets(system.m_FinalizeSignedDistanceFieldPSO.get(), 1, bindSets);
        commandContext.Dispatch(numDispatchSize);
        GetDevice()->GetTimeStamp("FinalizeSignedDistanceField");
    }

    // State transition for DX12
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uavMeshAndSDF), uavMeshAndSDF);
}

void TressFXSDFCollision::CollideWithHair(EI_CommandContext&          commandContext,
                                          TressFXSDFCollisionSystem& system,
                                          TressFXHairObject&         hairObject)
{
    if (!m_pInputCollisionMesh)
        return;

    EI_Marker marker(commandContext, "CollideWithHair");

    int numTotalHairVertices = hairObject.GetNumTotalHairVertices();

    // Get vertex buffers from the hair object.
    TressFXDynamicState& state = hairObject.GetDynamicState();

    // Set the constant buffer parameters
    m_ConstBuffer.m_Origin.x                 = m_Origin.x;
    m_ConstBuffer.m_Origin.y                 = m_Origin.y;
    m_ConstBuffer.m_Origin.z                 = m_Origin.z;
    m_ConstBuffer.m_Origin.w                 = 0;
    m_ConstBuffer.m_CellSize                 = m_CellSize;
    m_ConstBuffer.m_NumCellsX                = m_NumCellsX;
    m_ConstBuffer.m_NumCellsY                = m_NumCellsY;
    m_ConstBuffer.m_NumCellsZ                = m_NumCellsZ;
    m_ConstBuffer.m_CollisionMargin          = m_CollisionMargin * m_CellSize;
    m_ConstBuffer.m_NumTotalHairVertices     = hairObject.GetNumTotalHairVertices();
    m_ConstBuffer.m_NumHairVerticesPerStrand = hairObject.GetNumVerticesPerStrand();

    commandContext.UpdateBuffer(m_pConstantBufferResource.get(), &m_ConstBuffer);


    // Run CollideHairVerticesWithSdf. One thread per one hair vertex.
    {
        int numDispatchSize =
            (int)ceil((float)numTotalHairVertices / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
        commandContext.BindPSO(system.m_CollideHairVerticesWithSdfPSO.get());
        EI_BindSet * bindSets[] = { m_pSimBindSet.get(), &state.GetApplySDFBindSet() };
        commandContext.BindSets(system.m_CollideHairVerticesWithSdfPSO.get(), 2, bindSets );
        commandContext.Dispatch(numDispatchSize);
        GetDevice()->GetTimeStamp("CollideHairVerticesWithSdf");
    }

    // State transition for DX12
    state.UAVBarrier(commandContext);

    //EI_SB_Transition(commandContext, m_SignedDistanceFieldUAV, EI_STATE_UAV, EI_STATE_UAV);
    EI_Barrier uavSDF[] = 
    {
        { m_SignedDistanceFieldUAV.get(), EI_STATE_UAV, EI_STATE_UAV }
    };
    commandContext.SubmitBarrier(1, uavSDF);
}