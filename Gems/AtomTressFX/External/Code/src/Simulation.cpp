//---------------------------------------------------------------------------------------
// Example code for managing simulation
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

#include "Simulation.h"

#include "AMD_TressFX.h"

#include "TressFXLayouts.h"
#include "TressFXSimulation.h"

#include "HairStrands.h"
#include "SDF.h"

#define DEFINE_TO_STRING(x) #x
#define DEFINE_VALUE_TO_STRING(x) DEFINE_TO_STRING(x)

Simulation::Simulation()
    : mSimulation()
    , mSDFCollisionSystem()
{
    EI_Device* pDevice = GetDevice();

   // mSimulation = new TressFXSimulation();
    mSimulation.Initialize(pDevice);
    mSDFCollisionSystem.Initialize(pDevice);
}

void Simulation::StartSimulation(
        double fTime,
        SimulationContext& ctx,
        bool bUpdateCollMesh,
        bool bSDFCollisionResponse,
        bool bAsync
    )
{
    // When we are done submitting sim commands, we will restore this as the default command list.
    // TODO Maybe pass this explicitly, rather than setting as default and retrieving from there.
    EI_CommandContext* renderContext = &GetDevice()->GetCurrentCommandContext();


    // When running async, we are getting a command list for submission on the async compute queue.
    // We are only accumulating commands for submission now.  There will be a wait for actual
    // submission after this.
    EI_CommandContext* simContext = renderContext;
    if (bAsync)
    {
        simContext = &GetDevice()->GetComputeCommandContext();
    }

    if (bUpdateCollMesh)
    {
        // Updates the skinned version of the mesh,
        // which is input to SDF.
        // We are using a compute-based skinning system here, 
        // which is not part of the TressFX library.
        for (size_t i = 0; i < ctx.collisionMeshes.size(); i++)
            ctx.collisionMeshes[i]->SkinTheMesh(*simContext, fTime);
            
        UpdateCollisionMesh(*simContext, ctx);
    }

    RunSimulation(*simContext, ctx);

    if (bSDFCollisionResponse)
        RunCollision(*simContext, ctx);


    for (size_t i = 0; i < ctx.hairStrands.size(); i++)
        ctx.hairStrands[i]->TransitionSimToRendering(*renderContext);


}

void Simulation::WaitOnSimulation()
{
    GetDevice()->WaitForCompute();
}

void Simulation::UpdateCollisionMesh(EI_CommandContext& simContext, SimulationContext& ctx)
{
    for (size_t i = 0; i < ctx.collisionMeshes.size(); i++)
        ctx.collisionMeshes[i]->AccumulateSDF(simContext, mSDFCollisionSystem);
}

void Simulation::RunCollision(EI_CommandContext& simContext, SimulationContext& ctx)
{
    // We will apply every collision mesh to every set of strands.
    // This is of course not necessary in general - a BB check, for example, 
    // could check for overlaps first.
        for (size_t i = 0; i < ctx.hairStrands.size(); i++)
        {
            TressFXHairObject* pHair = ctx.hairStrands[i]->GetTressFXHandle();

            for (size_t j = 0; j < ctx.collisionMeshes.size(); j++)
                ctx.collisionMeshes[j]->ApplySDF(simContext, mSDFCollisionSystem, pHair);
        }
}

void Simulation::RunSimulation(EI_CommandContext& simContext, SimulationContext& ctx)
{
    std::vector<TressFXHairObject*> hairObjects(ctx.hairStrands.size());
    // Adi: the following part is required for both simulation and render since it updates 
    // the skinning matrices
    for (size_t i = 0; i < ctx.hairStrands.size(); i++)
    {
        // update bone matrices for bone skinning of first two vertices of hair strands
        ctx.hairStrands[i]->UpdateBones(simContext);
        hairObjects[i] = ctx.hairStrands[i]->GetTressFXHandle();
    }

// Adi: use the skin method only for initial testing with single compute pass
#define _SKIN_HAIR_NO_PHYSICS_
#ifdef _SKIN_HAIR_NO_PHYSICS_
    mSimulation.UpdateHairSkinning(simContext, hairObjects);
#else
    // Adi: this part involves the physics simulation: gravity, collisions and response and
    // runs the compute shaders for that. 
    // Since it also contains the initial skinning of the hair and the final stage of adding 
    // follow hair, it must be replaced (as per the above) if we want to skip the simulation 
    // part for now.
    // Run simulation
    mSimulation.Simulate(simContext, hairObjects);
#endif
}