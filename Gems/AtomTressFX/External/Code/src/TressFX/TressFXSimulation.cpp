// ----------------------------------------------------------------------------
// Invokes simulation compute shaders.
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

#include "AMD_TressFX.h"
#include "TressFXSimulation.h"

// TressFX
#include "TressFXLayouts.h"
#include "TressFXHairObject.h"
#include "EngineInterface.h"

void TressFXSimulation::Initialize(EI_Device* pDevice)
{
    EI_BindLayout * layouts[] = { GetSimLayout(), GetSimPosTanLayout() };
    mVelocityShockPropagationPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "VelocityShockPropagation", layouts, 2);
    mIntegrationAndGlobalShapeConstraintsPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "IntegrationAndGlobalShapeConstraints", layouts, 2);
    mCalculateStrandLevelDataPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "CalculateStrandLevelData", layouts, 2);
    mLocalShapeConstraintsPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "LocalShapeConstraints", layouts, 2);
    mLengthConstriantsWindAndCollisionPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "LengthConstriantsWindAndCollision", layouts, 2);
    mUpdateFollowHairVerticesPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "UpdateFollowHairVertices", layouts, 2);

    // Adi - skins the hair vertices and follow hair, avoids any simulation
    mSkinHairVerticesTestPSO = pDevice->CreateComputeShaderPSO("TressFXSimulation.hlsl", "SkinHairVerticesOnly", layouts, 2);
}

enum DispatchLevel
{
    DISPATCHLEVEL_VERTEX,
    DISPATCHLEVEL_STRAND
};

void DispatchComputeShader(EI_CommandContext& ctx, EI_PSO * pso, DispatchLevel level, std::vector<TressFXHairObject*>& hairObjects, const bool iterate = false)
{
    ctx.BindPSO(pso);
    for (int i = 0; i < hairObjects.size(); ++i)
    {
        int numGroups = (int)((float)hairObjects[i]->GetNumTotalHairVertices() / (float)TRESSFX_SIM_THREAD_GROUP_SIZE);
        if (level == DISPATCHLEVEL_STRAND)
        {
            numGroups = (int)(((float)(hairObjects[i]->GetNumTotalHairStrands()) / (float)TRESSFX_SIM_THREAD_GROUP_SIZE));
        }
        EI_BindSet* bindSets[] = { hairObjects[i]->GetSimBindSet(), &hairObjects[i]->GetDynamicState().GetSimBindSet() };
        ctx.BindSets(pso, 2, bindSets);

        int iterations = 1;
        if (iterate)
        {
            iterations = hairObjects[i]->GetCPULocalShapeIterations();
        }
        for (int j = 0; j < iterations; ++j)
        {
            ctx.Dispatch(numGroups);
        }
        hairObjects[i]->GetDynamicState().UAVBarrier(ctx);
    }
}

// Adi: this method handles the skinning of the hair and update the follow hair.
// It avoid any physics and simulation response and should be used for initial integration testing
void TressFXSimulation::UpdateHairSkinning(EI_CommandContext& commandContext, std::vector<TressFXHairObject*>& hairObjects)
{
    // Adi: binding the m_SimCB buffers (matrices, wind parameters..) to the GPU 
    for (int i = 0; i < hairObjects.size(); ++i)
    {
        hairObjects[i]->UpdateConstantBuffer(commandContext);
    }

    // Adi: only skin hair vertices without any physics. 
    DispatchComputeShader(commandContext, mSkinHairVerticesTestPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
    GetDevice()->GetTimeStamp("SkinHairVerticesTestPSO");
    

    // UpdateFollowHairVertices - This part is embedded in the single pass shader
//    DispatchComputeShader(commandContext, mUpdateFollowHairVerticesPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
//    GetDevice()->GetTimeStamp("UpdateFollowHairVertices");

    // Adi: make sure the dual buffers are updated properly - advance the current frame 
    for (int i = 0; i < hairObjects.size(); ++i)
    {
        hairObjects[i]->IncreaseSimulationFrame();
    }
}

void TressFXSimulation::Simulate(EI_CommandContext& commandContext, std::vector<TressFXHairObject*>& hairObjects)
{
    // Binding the bones' matrices
    for (int i = 0; i < hairObjects.size(); ++i)
    {
        hairObjects[i]->UpdateConstantBuffer(commandContext);
    }

    // IntegrationAndGlobalShapeConstraints
    DispatchComputeShader(commandContext, mIntegrationAndGlobalShapeConstraintsPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
    GetDevice()->GetTimeStamp("IntegrationAndGlobalShapeContraints");

    // Calculate Strand Level Data
    DispatchComputeShader(commandContext, mCalculateStrandLevelDataPSO.get(), DISPATCHLEVEL_STRAND, hairObjects);
    GetDevice()->GetTimeStamp("CalculateStrandLevelData");

    // VelocityShockPropagation
    DispatchComputeShader(commandContext, mVelocityShockPropagationPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
    GetDevice()->GetTimeStamp("VelocityShockPropagation");

    DispatchComputeShader(commandContext, mLocalShapeConstraintsPSO.get(), DISPATCHLEVEL_STRAND, hairObjects, true);
    GetDevice()->GetTimeStamp("LocalShapeConstraints");

    // LengthConstriantsWindAndCollision
    DispatchComputeShader(commandContext, mLengthConstriantsWindAndCollisionPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
    GetDevice()->GetTimeStamp("LengthConstriantsWindAndCollision");

    // UpdateFollowHairVertices
    DispatchComputeShader(commandContext, mUpdateFollowHairVerticesPSO.get(), DISPATCHLEVEL_VERTEX, hairObjects);
    GetDevice()->GetTimeStamp("UpdateFollowHairVertices");

    for (int i = 0; i < hairObjects.size(); ++i)
    {
        hairObjects[i]->IncreaseSimulationFrame();
    }
}
