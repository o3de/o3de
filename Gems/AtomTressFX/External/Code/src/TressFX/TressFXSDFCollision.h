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

#ifndef TRESSFXSDFCOLLIION_H_
#define TRESSFXSDFCOLLIION_H_

#include "TressFXCommon.h"
#include "TressFXHairObject.h"
#include "EngineInterface.h"
#include "TressFXLayouts.h"

class TressFXHairObject;
class TressFXAsset;
class TressFXSDFInputMeshInterface;

class TressFXSDFCollisionSystem
{
public:
    void Initialize(EI_Device* pDevice)
    {
        EI_BindLayout * layouts[] = { GetGenerateSDFLayout(), GetApplySDFLayout() };
        m_InitializeSignedDistanceFieldPSO = pDevice->CreateComputeShaderPSO("TressFXSDFCollision.hlsl", "InitializeSignedDistanceField", layouts, 2);
        m_ConstructSignedDistanceFieldPSO = pDevice->CreateComputeShaderPSO("TressFXSDFCollision.hlsl", "ConstructSignedDistanceField", layouts, 2);
        m_FinalizeSignedDistanceFieldPSO = pDevice->CreateComputeShaderPSO("TressFXSDFCollision.hlsl", "FinalizeSignedDistanceField", layouts, 2);
        m_CollideHairVerticesWithSdfPSO = pDevice->CreateComputeShaderPSO("TressFXSDFCollision.hlsl", "CollideHairVerticesWithSdf_forward", layouts, 2);
    }

    std::unique_ptr<EI_PSO> m_InitializeSignedDistanceFieldPSO;
    std::unique_ptr<EI_PSO> m_ConstructSignedDistanceFieldPSO;
    std::unique_ptr<EI_PSO> m_FinalizeSignedDistanceFieldPSO;
    std::unique_ptr<EI_PSO> m_CollideHairVerticesWithSdfPSO;
};

class TressFXSDFCollision : private TressFXNonCopyable
{
public:
    TressFXSDFCollision(
        EI_Device* pDevice,
        TressFXSDFInputMeshInterface* pCollMesh,
        const char * modelName,
        int numCellsInX,
        float collisionMargin);

    // Update and animate the collision mesh
    void Update(EI_CommandContext& commandContext, TressFXSDFCollisionSystem& system);

    // Run collision checking and response with hair
    void CollideWithHair(EI_CommandContext&          commandContext,
                         TressFXSDFCollisionSystem& system,
                         TressFXHairObject&         hairObject);

    float                            GetSDFCollisionMargin() const { return m_CollisionMargin; }


    const EI_Resource& GetSDFDataGPUBuffer() const { return *m_SignedDistanceFieldUAV; }
    EI_Resource&       GetSDFDataGPUBuffer() { return *m_SignedDistanceFieldUAV; }

    // Grid
    float                       GetGridCellSize() const { return m_CellSize; }
    Vector3           GetGridOrigin() const { return m_Origin; }
    void GetGridNumCells(int& x, int& y, int& z) const
    {
        x = m_NumCellsX;
        y = m_NumCellsY;
        z = m_NumCellsZ;
    }
    int GetGridNumTotalCells() const { return m_NumTotalCells; }

    TressFXSDFCollisionParams& GetConstantBufferData() { return m_ConstBuffer; }
private:
    TressFXSDFCollisionParams m_ConstBuffer;
    std::unique_ptr<EI_Resource> m_pConstantBufferResource;
    TressFXSDFInputMeshInterface* m_pInputCollisionMesh;

    // UAV
    std::unique_ptr<EI_Resource> m_SignedDistanceFieldUAV;

    std::unique_ptr<EI_BindSet> m_pSimBindSet;

    // SDF grid
    Vector3 m_Origin;
    float             m_CellSize;
    int               m_NumCellsX;
    int               m_NumCellsY;
    int               m_NumCellsZ;
    int               m_NumTotalCells;
    Vector3 m_Min;
    Vector3 m_Max;
    Vector3 m_PaddingBoundary;
    float             m_GridAllocationMutliplier;

    // number of cells in X axis
    int m_NumCellsInXAxis;

    // SDF collision margin.
    float m_CollisionMargin;

    void UpdateSDFGrid(const Vector3& tight_bbox_min,
                       const Vector3& tight_bbox_max);
};

#endif  // TRESSFXSDFCOLLIION_H_