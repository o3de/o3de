//---------------------------------------------------------------------------------------
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

#pragma once

#define ENABLE_MARCHING_CUBES 1

#include "AMD_TressFX.h"
#include "TressFXSDFMarchingCubes.h"
#include "TressFXBoneSkinning.h"

class TressFXSDFCollisionSystem;
class TressFXSDFCollision;
class EI_Scene;
class TressFXHairObject;

class CollisionMesh
{
public:
    CollisionMesh(
        EI_Scene* gltfImplementation,
        EI_RenderTargetSet* renderPass,
        const char* name,
        const char* tfxmeshFilePath,
        int               numCellsInXAxis,
        float             SDFCollMargin,
        int               skinNumber,
        const char* followBone);

    void SkinTheMesh(EI_CommandContext& context, double fTime);
    void AccumulateSDF(EI_CommandContext& context, TressFXSDFCollisionSystem& sdfCollisionSystem);
    void ApplySDF(EI_CommandContext& context, TressFXSDFCollisionSystem& sdfCollisionSystem, TressFXHairObject* strands);
    void GenerateIsoSurface(EI_CommandContext& context);
    void DrawIsoSurface(EI_CommandContext& context);
    void DrawMesh(EI_CommandContext& context);
private:
    std::unique_ptr<TressFXSDFCollision> m_pCollisionMesh;
    // Adi: this structure is required to for computing the per-frame SDF
    std::unique_ptr<TressFXBoneSkinning> m_pBoneSkinning;
    std::unique_ptr<TressFXSDFMarchingCubes> m_pSDFMachingCubes;
};