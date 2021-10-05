//---------------------------------------------------------------------------------------
// Example code that encapsulates three related objects.
// 1.  The TressFXHairObject
// 2.  An interface to get the current set of bones in world space that drive the hair object.
// 3.  An interface to set up for drawing the strands, such as setting lighting parmeters, etc.
//
// Normally, you'd probably contain the TressFXObject in the engine wrapper, but we've arranged it this 
// way to focus on the important aspects of integration.
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

#include "HairStrands.h"
#include "TressFXSimulation.h"
#include "TressFXHairObject.h"
#include "TressFXAsset.h"
#include "EngineInterface.h"
#include <DirectXMath.h>
using namespace DirectX;

void HairStrands::TransitionSimToRendering(EI_CommandContext& context)
{
    m_pStrands->GetDynamicState().TransitionSimToRendering(context);
}

void HairStrands::TransitionRenderingToSim(EI_CommandContext& context)
{
    m_pStrands->GetDynamicState().TransitionRenderingToSim(context);
}

void HairStrands::UpdateBones(EI_CommandContext& context)
{
    std::vector<XMMATRIX> boneMatrices = m_pScene->GetWorldSpaceSkeletonMats(m_skinNumber);
    const AMD::float4x4*              pBoneMatricesInWS = (const AMD::float4x4*)&boneMatrices[0];

    // update bone matrices for bone skinning of first two vertices of hair strands
    m_pStrands->UpdateBoneMatrices(pBoneMatricesInWS, (int)boneMatrices.size());
}

HairStrands::HairStrands(
    EI_Scene *      scene,
    const char *      tfxFilePath,
    const char *      tfxboneFilePath,
    const char *      hairObjectName,
    int               numFollowHairsPerGuideHair,
    float             tipSeparationFactor,
    int               skinNumber, 
    int				  renderIndex
)
{
    EI_Device* pDevice = GetDevice();
    EI_CommandContext& uploadCommandContext = pDevice->GetCurrentCommandContext();
        
    TressFXAsset* asset = new TressFXAsset();
    size_t memOffset = 0;

    // Load *.tfx 
    FILE * fp = fopen(tfxFilePath, "rb");
    asset->LoadHairData(fp);
    fclose(fp);

    asset->GenerateFollowHairs(numFollowHairsPerGuideHair, tipSeparationFactor, 0.012f);
    asset->ProcessAsset();

    // Load *.tfxbone
    fp = fopen(tfxboneFilePath, "rb");
    asset->LoadBoneData(fp, skinNumber, scene);
    fclose(fp);

    TressFXHairObject* hairObject = new TressFXHairObject(asset, pDevice, uploadCommandContext, hairObjectName, renderIndex);

    m_pStrands.reset(hairObject);
    m_pScene = scene;
    m_skinNumber = skinNumber;

    delete asset;
}







