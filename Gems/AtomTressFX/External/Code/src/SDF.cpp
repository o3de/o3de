
///---------------------------------------------------------------------------------------
// Example code for managing objects for signed-distance fields (SDFs)
//
// This includes the TressFXSDFCollision objects.  Associated with each is a system
// for skinning the model on the GPU (since that is input to TressFXSDFCollision) and 
// visualizing the SDFs using marching cubes.  The GPU skinning and marching cubes 
// systems could be packaged as library code as well, but are not there yet.
//
// The skinned meshes are loaded through this interface as well.
//-------------------------------------------------------------------------------------
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

#include "SDF.h"

#include "TressFXBoneSkinning.h"
#include "TressFXSDFMarchingCubes.h"
#include "TressFXSDFCollision.h"
#include "EngineInterface.h"

CollisionMesh::CollisionMesh(
    EI_Scene * gltfImplementation,
    EI_RenderTargetSet * renderPass,
    const char *      name,
    const char *      tfxmeshFilePath,
    int               numCellsInXAxis,
    float             SDFCollMargin,
    int               skinNumber,
    const char *      followBone)
{
    m_pBoneSkinning.reset(new TressFXBoneSkinning);
    m_pSDFMachingCubes.reset(new TressFXSDFMarchingCubes);

    EI_Device* pDevice = GetDevice();
    EI_CommandContext& uploadCommandContext = GetDevice()->GetCurrentCommandContext();

    m_pBoneSkinning->LoadTressFXCollisionMeshData(gltfImplementation, tfxmeshFilePath, skinNumber, followBone);
    m_pBoneSkinning->Initialize(renderPass, pDevice, uploadCommandContext, name);

    m_pCollisionMesh.reset(new TressFXSDFCollision(pDevice, m_pBoneSkinning.get(), name, numCellsInXAxis, SDFCollMargin));
#if ENABLE_MARCHING_CUBES
    m_pSDFMachingCubes->SetSDF(m_pCollisionMesh.get());
    m_pSDFMachingCubes->SetSDFIsoLevel(m_pCollisionMesh->GetSDFCollisionMargin());
    m_pSDFMachingCubes->Initialize(name, gltfImplementation, renderPass);
#endif
}

void CollisionMesh::SkinTheMesh(EI_CommandContext& context, double fTime)
{
    m_pBoneSkinning->Update(context, fTime);
}

void CollisionMesh::AccumulateSDF(EI_CommandContext& context, TressFXSDFCollisionSystem& sdfCollisionSystem)
{
    m_pCollisionMesh->Update(context, sdfCollisionSystem);
}

void CollisionMesh::ApplySDF(EI_CommandContext& context, TressFXSDFCollisionSystem& sdfCollisionSystem, TressFXHairObject* strands)
{
    m_pCollisionMesh->CollideWithHair(context, sdfCollisionSystem, *strands);
}

void CollisionMesh::GenerateIsoSurface(EI_CommandContext& context)
{
    m_pSDFMachingCubes->Update(context);
}
void CollisionMesh::DrawIsoSurface(EI_CommandContext& context)
{
    (void)context;
    m_pSDFMachingCubes->Draw();
}

void CollisionMesh::DrawMesh(EI_CommandContext& context)
{
    m_pBoneSkinning->DrawMesh(context);
}