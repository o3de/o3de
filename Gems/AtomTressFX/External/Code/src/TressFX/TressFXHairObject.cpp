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

#include "TressFXHairObject.h"

// TressFX
#include "Math/Matrix44.h"
#include "Math/Vector3D.h"
#include "TressFXAsset.h"
#include "TressFXLayouts.h"
#include "EngineInterface.h"

#include <memory.h>

using namespace AMD;

#define TRESSFX_MIN_VERTS_PER_STRAND_FOR_GPU_ITERATION 64

// Load simulation compute shader and create all buffers
// command context used to upload initial data.
TressFXHairObject::TressFXHairObject(TressFXAsset* asset,
    EI_Device*   pDevice,
    EI_CommandContext&  commandContext,
    const char *      name, int RenderIndex) :
    m_NumTotalVertices(0), 
    m_NumTotalStrands(0), 
    m_NumVerticesPerStrand(0), 
    m_CPULocalShapeIterations(0), 
    m_SimulationFrame(0), 
    m_RenderIndex(RenderIndex), 
    m_pRenderLayoutBindSet(nullptr)
{
    m_NumTotalVertices = asset->m_numTotalVertices;
    m_NumTotalStrands = asset->m_numTotalStrands;
    m_NumVerticesPerStrand = asset->m_numVerticesPerStrand;

    // Create buffers for simulation
    {
        m_DynamicState.CreateGPUResources(pDevice, m_NumTotalVertices, m_NumTotalStrands, name, asset);

        m_SimCB[0].CreateBufferResource("TressFXSimulationConstantBuffer");
        m_SimCB[1].CreateBufferResource("TressFXSimulationConstantBuffer");
        m_RenderCB.CreateBufferResource("TressFXRenderConstantBuffer");
        m_StrandCB.CreateBufferResource("TressFXStrandConstantBuffer");

        // initial hair positions
        m_InitialHairPositionsBuffer =
            pDevice->CreateBufferResource(
                sizeof(AMD::float4),
                m_NumTotalVertices,
                0,
                "InitialPosition"
            );

        // rest lengths
        m_HairRestLengthSRVBuffer =
            pDevice->CreateBufferResource(
                sizeof(float),
                m_NumTotalVertices,
                0,
                "RestLength");



        // strand types
        m_HairStrandTypeBuffer = pDevice->CreateBufferResource(
            sizeof(int),
            m_NumTotalStrands,
            0,
            "StrandType");

        // follow hair root offsets
        m_FollowHairRootOffsetBuffer =
            pDevice->CreateBufferResource(
                sizeof(AMD::float4),
                m_NumTotalStrands,
                0,
                "RootOffset");


        // bone skinning data
        m_BoneSkinningDataBuffer = pDevice->CreateBufferResource(sizeof(TressFXBoneSkinningData), m_NumTotalStrands, 0, "SkinningData");

    }

    // UPLOAD INITIAL DATA
    // UAVs must first be transitioned for copy dest, since they start with UAV state.
    // When done, we transition to appropriate state for start of first frame.

    m_DynamicState.UploadGPUData(
        commandContext, asset->m_positions.data(), asset->m_tangents.data(), m_NumTotalVertices);

    commandContext.UpdateBuffer(m_InitialHairPositionsBuffer.get(), asset->m_positions.data());
    commandContext.UpdateBuffer(m_HairRestLengthSRVBuffer.get(), asset->m_restLengths.data());
    commandContext.UpdateBuffer(m_HairStrandTypeBuffer.get(), asset->m_strandTypes.data());
    commandContext.UpdateBuffer(m_FollowHairRootOffsetBuffer.get(), asset->m_followRootOffsets.data());
    commandContext.UpdateBuffer(m_BoneSkinningDataBuffer.get(), asset->m_boneSkinningData.data());

    EI_Barrier copyBarriers[] =
    {
        { m_InitialHairPositionsBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_HairRestLengthSRVBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_HairStrandTypeBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_FollowHairRootOffsetBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_BoneSkinningDataBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_SRV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(copyBarriers), (EI_Barrier*)copyBarriers);

    m_NumVerticesPerStrand = asset->m_numVerticesPerStrand;
    m_NumFollowHairsPerGuideHair = asset->m_numFollowStrandsPerGuide;

    // Bind set
    for (int i = 0; i < 2; ++i)
    {
    EI_BindSetDescription bindSet =
    {
        {
            // SRVs
            m_InitialHairPositionsBuffer.get(),
            m_HairRestLengthSRVBuffer.get(),
            m_HairStrandTypeBuffer.get(),
            m_FollowHairRootOffsetBuffer.get(),
            m_BoneSkinningDataBuffer.get(),
            // CB
            m_SimCB[i].GetBufferResource()
        }
    };

    m_pSimBindSet[i] = pDevice->CreateBindSet(GetSimLayout(), bindSet);

    }

    // Set up with defaults.
    ResetPositions();

    // Rendering setup
    CreateRenderingGPUResources(pDevice, *asset, name);
    PopulateDrawStrandsBindSet(pDevice, nullptr);   // Null at the beginning
    UploadRenderingGPUResources(commandContext, *asset);

}

// TODO Move wind settings to Simulation Parameters or whatever.

// Wind is in a pyramid around the main wind direction.
// To add a random appearance, the shader will sample some direction
// within this cone based on the strand index.
// This function computes the vector for each edge of the pyramid.
static void SetWindCorner(Quaternion     rotFromXAxisToWindDir,
                          Vector3     rotAxis,
                          float                 angleToWideWindCone,
                          float                 wM,
                          AMD::float4& outVec)
{
    static const Vector3 XAxis(1.0f, 0, 0);
    Quaternion              rot(rotAxis, angleToWideWindCone);
    Vector3              newWindDir = rotFromXAxisToWindDir * rot * XAxis;
    outVec.x                                  = newWindDir.x * wM;
    outVec.y                                  = newWindDir.y * wM;
    outVec.z                                  = newWindDir.z * wM;
    outVec.w                                  = 0;  // unused.
}

const float MATH_PI2 = 3.14159265359f;
#define DEG_TO_RAD2(d) (d * MATH_PI2 / 180)


void TressFXHairObject::SetWind(const Vector3& windDir, float windMag, int frame)
{
    float wM = windMag * (pow(sin(frame * 0.01f), 2.0f) + 0.5f);

    Vector3 windDirN(windDir);
    windDirN.Normalize();

    Vector3 XAxis(1.0f, 0, 0);
    Vector3 xCrossW = XAxis.Cross(windDirN);

    Quaternion rotFromXAxisToWindDir;
    rotFromXAxisToWindDir.SetIdentity();

    float angle = asin(xCrossW.Length());

    if (angle > 0.001)
    {
        rotFromXAxisToWindDir.SetRotation(xCrossW.Normalize(), angle);
    }

    float angleToWideWindCone = DEG_TO_RAD2(40.f);

    SetWindCorner(rotFromXAxisToWindDir,
                  Vector3(0, 1.0, 0),
                  angleToWideWindCone,
                  wM,
                  m_SimCB[m_SimulationFrame % 2]->m_Wind);
    SetWindCorner(rotFromXAxisToWindDir,
                  Vector3(0, -1.0, 0),
                  angleToWideWindCone,
                  wM,
                  m_SimCB[m_SimulationFrame % 2]->m_Wind1);
    SetWindCorner(rotFromXAxisToWindDir,
                  Vector3(0, 0, 1.0),
                  angleToWideWindCone,
                  wM,
                  m_SimCB[m_SimulationFrame % 2]->m_Wind2);
    SetWindCorner(rotFromXAxisToWindDir,
                  Vector3(0, 0, -1.0),
                  angleToWideWindCone,
                  wM,
                  m_SimCB[m_SimulationFrame % 2]->m_Wind3);

    // fourth component unused. (used to store frame number, but no longer used).
}

void TressFXHairObject::UpdatePerObjectRenderParams(EI_CommandContext& commandContext)
{
    m_RenderCB.Update(commandContext);
    m_StrandCB.Update(commandContext);
}

void TressFXHairObject::DrawStrands(EI_CommandContext& commandContext,
                                    EI_PSO&            pso,
                                    EI_BindSet**       extraBindSets,
                                    uint32_t		   numExtraBindSets)
{
    // at some point, should probably pass these in to EI_BindAndDrawIndexedInstanced.
    static const uint32_t MaxSetsToBind = 10;	// Grow as needed
    EI_BindSet* sets[MaxSetsToBind];

    // First 2 sets are always the RenderLayout and PosTanCollection
    sets[0] = m_pRenderLayoutBindSet.get(); sets[1] = &m_DynamicState.GetRenderBindSet();

    for (uint32_t i = 0; i < numExtraBindSets; ++i)
        sets[2+i] = extraBindSets[i];

    commandContext.BindSets(&pso, 2 + numExtraBindSets, sets);
    AMD::uint32 nStrandCopies = 1;
    
    uint32_t NumPrimsToRender = (m_TotalIndices / 3);
    
    if (m_LODHairDensity != 1.f)
    {
        NumPrimsToRender = uint32_t(float(NumPrimsToRender) * m_LODHairDensity);

        // Calculate a new number of Primitives to draw. Keep it aligned to number of primitives per strand (i.e. don't cut strands in half or anything)
        uint32 NumPrimsPerStrand = (m_NumVerticesPerStrand - 1) * 2;
        uint32 RemainderPrims = NumPrimsToRender % NumPrimsPerStrand;

        NumPrimsToRender = (RemainderPrims > 0) ? NumPrimsToRender + NumPrimsPerStrand - RemainderPrims : NumPrimsToRender;

        // Force prims to be on (guide hair + its follow hairs boundary... no partial groupings)
        NumPrimsToRender = NumPrimsToRender - (NumPrimsToRender % (NumPrimsPerStrand * (m_NumFollowHairsPerGuideHair + 1)));
    }

    EI_IndexedDrawParams drawParams;
    drawParams.pIndexBuffer = m_pIndexBuffer.get();
    drawParams.numIndices = NumPrimsToRender * 3;
    drawParams.numInstances = nStrandCopies;

    commandContext.DrawIndexedInstanced(pso, drawParams);
}

void TressFXHairObject::CreateRenderingGPUResources(EI_Device*   pDevice,
                                                    TressFXAsset& asset,
                                                    const char *      name)
{
    // If rendering is seperated, we might be copying the asset count to the local member variable.
    // But since this is currently loaded after simulation right now, we'll just make sure
    // it's already set.
    TRESSFX_ASSERT(asset.m_numTotalStrands == m_NumTotalStrands);
    TRESSFX_ASSERT(asset.m_numTotalVertices == m_NumTotalVertices);
    m_TotalIndices = asset.GetNumHairTriangleIndices();  // asset.GetNumHairTriangleIndices();

    if (asset.m_strandUV.data())
    {
        m_HairTexCoords = pDevice->CreateBufferResource(
                                             2 * sizeof(AMD::real32),
                                             m_NumTotalStrands,
                                             0,
                                             "TexCoords");
    }

    m_HairVertexRenderParams = pDevice->CreateBufferResource(
                                                  sizeof(AMD::real32),
                                                  m_NumTotalVertices,
                                                  0,
                                                  "VertRenderParams");

    // TODO seperate creation from upload. Go through a real interface.
    m_pIndexBuffer = pDevice->CreateBufferResource( sizeof(AMD::int32), m_TotalIndices, EI_BF_INDEXBUFFER, name);
}

void TressFXHairObject::UploadRenderingGPUResources(EI_CommandContext&  commandContext,
                                                    TressFXAsset& asset)
{
    TRESSFX_ASSERT(asset.m_numTotalStrands == m_NumTotalStrands);
    TRESSFX_ASSERT(asset.m_numTotalVertices == m_NumTotalVertices);
    TRESSFX_ASSERT(m_TotalIndices == asset.GetNumHairTriangleIndices());
   
    if (asset.m_strandUV.data())
    {
        commandContext.UpdateBuffer(m_HairTexCoords.get(), asset.m_strandUV.data());
    }

    commandContext.UpdateBuffer(m_HairVertexRenderParams.get(), asset.m_thicknessCoeffs.data());
    commandContext.UpdateBuffer(m_pIndexBuffer.get(), asset.m_triangleIndices.data());

    EI_Barrier copyToVS[] = 
    {
        { m_HairTexCoords.get(), EI_STATE_COPY_DEST, EI_STATE_SRV},
        { m_HairVertexRenderParams.get(), EI_STATE_COPY_DEST, EI_STATE_SRV },
        { m_pIndexBuffer.get(), EI_STATE_COPY_DEST, EI_STATE_INDEX_BUFFER }
    };

    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(copyToVS), copyToVS);
   
}

void TressFXHairObject::PopulateDrawStrandsBindSet(EI_Device* pDevice, TressFXRenderingSettings* pRenderSettings/*=nullptr*/)
{
    if (pRenderSettings)
    {
        if (pRenderSettings->m_BaseAlbedoName != "<none>")
        {
            m_BaseAlbedo = GetDevice()->CreateResourceFromFile(pRenderSettings->m_BaseAlbedoName.c_str(), true);
        }
        if (pRenderSettings->m_StrandAlbedoName != "<none>")
        {
            m_StrandAlbedo = GetDevice()->CreateResourceFromFile(pRenderSettings->m_BaseAlbedoName.c_str(), true);
        }
    }
    EI_BindSetDescription bindSetDesc =
    {
        { 
          m_HairVertexRenderParams.get(),
          m_HairTexCoords.get(), 
          m_BaseAlbedo.get() ? m_BaseAlbedo.get() : pDevice->GetDefaultWhiteTexture(),
          m_RenderCB.GetBufferResource(),
          m_StrandCB.GetBufferResource(),
          m_StrandAlbedo.get() ? m_StrandAlbedo.get() : pDevice->GetDefaultWhiteTexture(),
        }
    };
    m_pRenderLayoutBindSet = pDevice->CreateBindSet(GetTressFXParamLayout(), bindSetDesc);
}

void TressFXHairObject::UpdateBoneMatrices(const AMD::float4x4* pBoneMatricesInWS, int numBoneMatrices)
{
    int numMatrices = min(numBoneMatrices, AMD_TRESSFX_MAX_NUM_BONES);
    for (int i = 0; i < numMatrices; ++i)
    {
        m_SimCB[m_SimulationFrame%2]->m_BoneSkinningMatrix[i] = pBoneMatricesInWS[i];
    }
}

void TressFXHairObject::UpdateConstantBuffer(EI_CommandContext& commandContext)
{
    m_SimCB[m_SimulationFrame%2].Update(commandContext);
}

void TressFXHairObject::UpdateSimulationParameters(const TressFXSimulationSettings* settings, float timeStep)
{
    m_SimCB[m_SimulationFrame%2]->SetVelocityShockPropogation(settings->m_vspCoeff);
    m_SimCB[m_SimulationFrame%2]->SetVSPAccelThreshold(settings->m_vspAccelThreshold);
    m_SimCB[m_SimulationFrame%2]->SetDamping(settings->m_damping);
    m_SimCB[m_SimulationFrame%2]->SetLocalStiffness(settings->m_localConstraintStiffness);
    m_SimCB[m_SimulationFrame%2]->SetGlobalStiffness(settings->m_globalConstraintStiffness);
    m_SimCB[m_SimulationFrame%2]->SetGlobalRange(settings->m_globalConstraintsRange);
    m_SimCB[m_SimulationFrame%2]->SetLocalStiffness(settings->m_localConstraintStiffness);
    m_SimCB[m_SimulationFrame%2]->SetGravity(settings->m_gravityMagnitude);
    m_SimCB[m_SimulationFrame%2]->SetTimeStep(timeStep);
    m_SimCB[m_SimulationFrame%2]->SetCollision(false);
    m_SimCB[m_SimulationFrame%2]->SetVerticesPerStrand(m_NumVerticesPerStrand);
    m_SimCB[m_SimulationFrame%2]->SetFollowHairsPerGuidHair(m_NumFollowHairsPerGuideHair);
    m_SimCB[m_SimulationFrame%2]->SetTipSeperation(settings->m_tipSeparation);

    // use 1.0 for now, this needs to be maxVelocity * timestep
    m_SimCB[m_SimulationFrame % 2]->g_ClampPositionDelta = 20.0f;

    // Right now, we do all local contraint iterations on the CPU.
    // It's actually a bit faster to

    if (m_NumVerticesPerStrand >= TRESSFX_MIN_VERTS_PER_STRAND_FOR_GPU_ITERATION)
    {
        m_SimCB[m_SimulationFrame % 2]->SetLocalIterations((int)settings->m_localConstraintsIterations);
        m_CPULocalShapeIterations = 1;
    }
    else
    {
        m_SimCB[m_SimulationFrame % 2]->SetLocalIterations(1);
        m_CPULocalShapeIterations = (int)settings->m_localConstraintsIterations;
    }

    m_SimCB[m_SimulationFrame % 2]->SetLengthIterations((int)settings->m_lengthConstraintsIterations);

    // Set wind parameters
    Vector3 windDir(
        settings->m_windDirection[0], settings->m_windDirection[1], settings->m_windDirection[2]);
    float windMag = settings->m_windMagnitude;
    SetWind(windDir, windMag, m_SimulationFrame);


#if TRESSFX_COLLISION_CAPSULES
    m_SimCB.m_numCollisionCapsules.x = 0;

    // Below is an example showing how to pass capsule collision objects. 
    /*
    mSimCB.m_numCollisionCapsules.x = 1;
    mSimCB.m_centerAndRadius0[0] = { 0, 0.f, 0.f, 50.f };
    mSimCB.m_centerAndRadius1[0] = { 0, 100.f, 0, 10.f };
    */
#endif
    // make sure we start of with a correct pose
    if (m_SimulationFrame < 2)
        ResetPositions();

    // Bone matrix set elsewhere. It is not dependent on the settings passed in here.
}

void TressFXHairObject::UpdateRenderingParameters(const TressFXRenderingSettings* parameters, const int NodePoolSize, float timeStep, float Distance, bool ShadowUpdate /*= false*/)
{
    // Update Render Parameters
    m_RenderCB->FiberRadius = parameters->m_FiberRadius;	// Don't modify radius by LOD multiplier as this one is used to calculate shadowing and that calculation should remain unaffected
    
    m_RenderCB->ShadowAlpha = parameters->m_HairShadowAlpha;
    m_RenderCB->FiberSpacing = parameters->m_HairFiberSpacing;

    m_RenderCB->HairKs2 = parameters->m_HairKSpec2;
    m_RenderCB->HairEx2 = parameters->m_HairSpecExp2;
    m_RenderCB->MatKValue = { 0.f, parameters->m_HairKDiffuse, parameters->m_HairKSpec1, parameters->m_HairSpecExp1 }; // no ambient

    // Marschner lighting model parameters
    m_RenderCB->Roughness = parameters->m_HairRoughness;
    m_RenderCB->CuticleTilt = parameters->m_HairCuticleTilt;

    m_RenderCB->MaxShadowFibers = parameters->m_HairMaxShadowFibers;
    
    // Update Strand Parameters (per hair object)
    m_StrandCB->MatBaseColor = parameters->m_HairMatBaseColor;
    m_StrandCB->MatTipColor = parameters->m_HairMatTipColor;
    m_StrandCB->TipPercentage = parameters->m_TipPercentage;
    m_StrandCB->StrandUVTilingFactor = parameters->m_StrandUVTilingFactor;
    m_StrandCB->FiberRatio = parameters->m_FiberRatio;

    // Reset LOD hair density for the frame
    m_LODHairDensity = 1.f;

    float FiberRadius = parameters->m_FiberRadius;
    if (parameters->m_EnableHairLOD)
    {
        float MinLODDist = ShadowUpdate? min(parameters->m_ShadowLODStartDistance, parameters->m_ShadowLODEndDistance) : min(parameters->m_LODStartDistance, parameters->m_LODEndDistance);
        float MaxLODDist = ShadowUpdate? max(parameters->m_ShadowLODStartDistance, parameters->m_ShadowLODEndDistance) : max(parameters->m_LODStartDistance, parameters->m_LODEndDistance);

        if (Distance > MinLODDist)
        {
            float DistanceRatio = min((Distance - MinLODDist) / max(MaxLODDist - MinLODDist, 0.00001f), 1.f);

            // Lerp: x + s(y-x)
            float MaxLODFiberRadius = FiberRadius * (ShadowUpdate? parameters->m_ShadowLODWidthMultiplier : parameters->m_LODWidthMultiplier);
            FiberRadius = FiberRadius + (DistanceRatio * (MaxLODFiberRadius - FiberRadius));

            // Lerp: x + s(y-x)
            m_LODHairDensity = 1.f + (DistanceRatio * ((ShadowUpdate ? parameters->m_ShadowLODPercent : parameters->m_LODPercent) - 1.f));
        }
    }

    m_StrandCB->FiberRadius = FiberRadius;

    m_StrandCB->NumVerticesPerStrand = m_NumVerticesPerStrand;  // Always constant
    m_StrandCB->EnableThinTip = parameters->m_EnableThinTip;
    m_StrandCB->NodePoolSize = NodePoolSize;
    m_StrandCB->RenderParamsIndex = m_RenderIndex;				// Always constant

    m_StrandCB->EnableStrandUV = parameters->m_EnableStrandUV;
    m_StrandCB->EnableStrandTangent = parameters->m_EnableStrandTangent;
}

// Positions and tangents are handled in the following order, from the point of view of each
// buffer.
//
// Positions updated from previous sim output by copy. (COPY_DEST)
// Simulate updates mPositions and mPositionsPrev (UAVs)  also updates mTangents (this might move)
// SDF updates mPositions and mPositionsPrev (UAVs)
// Render with mPositions and mTangnets (PS SRVs)

void TressFXDynamicState::CreateGPUResources(EI_Device* pDevice, int numVertices, int numStrands, const char * name, TressFXAsset* asset)
{
    m_PositionsPrev = pDevice->CreateBufferResource(sizeof(AMD::float4), numVertices, EI_BF_NEEDSUAV, "PosPrev");
    m_PositionsPrevPrev = pDevice->CreateBufferResource(sizeof(AMD::float4), numVertices, EI_BF_NEEDSUAV, "PosPrevPrev");
    m_Positions = pDevice->CreateBufferResource(sizeof(AMD::float4), numVertices, EI_BF_NEEDSUAV, "Pos");
    m_Tangents = pDevice->CreateBufferResource(sizeof(AMD::float4), numVertices, EI_BF_NEEDSUAV, "Tan");
    m_StrandLevelData = pDevice->CreateBufferResource(sizeof(TressFXStrandLevelData), numStrands, EI_BF_NEEDSUAV, "StrandLevelData");

    EI_BindSetDescription bindSet;

    bindSet = {
        {m_Positions.get(), m_PositionsPrev.get(), m_PositionsPrevPrev.get(), m_Tangents.get(), m_StrandLevelData.get() }
    };
    m_pSimBindSets = pDevice->CreateBindSet(GetSimPosTanLayout(), bindSet);

    bindSet = {
        {m_Positions.get(), m_PositionsPrev.get()}
    };
    m_pApplySDFBindSets = pDevice->CreateBindSet(GetApplySDFLayout(), bindSet);

    bindSet = {
        {m_Positions.get(), m_Tangents.get()}
    };
    m_pRenderBindSets = pDevice->CreateBindSet(GetRenderPosTanLayout(), bindSet);
}

void TressFXDynamicState::UploadGPUData(EI_CommandContext& commandContext, void* pos, void* tan, int numVertices)
{
    TRESSFX_ASSERT(m_Positions != nullptr);
    TRESSFX_ASSERT(m_Tangents != nullptr);
    TRESSFX_ASSERT(m_PositionsPrev != nullptr);
    TRESSFX_ASSERT(m_PositionsPrevPrev != nullptr);
    
    EI_Barrier uavToUpload[] = 
    {
        { m_Positions.get(), EI_STATE_UAV, EI_STATE_COPY_DEST },
        { m_Tangents.get(), EI_STATE_UAV, EI_STATE_COPY_DEST },
        { m_PositionsPrev.get(), EI_STATE_UAV, EI_STATE_COPY_DEST },
        { m_PositionsPrevPrev.get(), EI_STATE_UAV, EI_STATE_COPY_DEST }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uavToUpload), uavToUpload);

 
    commandContext.UpdateBuffer(m_Positions.get(), pos);
    commandContext.UpdateBuffer(m_Tangents.get(), tan);
    commandContext.UpdateBuffer(m_PositionsPrev.get(), pos);
    commandContext.UpdateBuffer(m_PositionsPrevPrev.get(), pos);

    EI_Barrier uploadToUAV[] =
    {
        { m_Positions.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
        { m_Tangents.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
        { m_PositionsPrev.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
        { m_PositionsPrevPrev.get(), EI_STATE_COPY_DEST, EI_STATE_UAV },
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uploadToUAV), uploadToUAV);
}

void TressFXDynamicState::TransitionSimToRendering(EI_CommandContext& commandContext)
{
    TRESSFX_ASSERT(m_Positions != nullptr);
    TRESSFX_ASSERT(m_Tangents != nullptr);

    EI_Barrier simToRender[] = 
    {
        { m_Positions.get(), EI_STATE_UAV, EI_STATE_SRV },
        { m_Tangents.get(), EI_STATE_UAV, EI_STATE_SRV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(simToRender), simToRender);
}

void TressFXDynamicState::TransitionRenderingToSim(EI_CommandContext& commandContext)
{
    TRESSFX_ASSERT(m_Positions != nullptr);
    TRESSFX_ASSERT(m_Tangents != nullptr);

    EI_Barrier renderToSim[] =
    {
        { m_Positions.get(), EI_STATE_SRV, EI_STATE_UAV },
        { m_Tangents.get(), EI_STATE_SRV, EI_STATE_UAV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(renderToSim), renderToSim);
}

void TressFXDynamicState::UAVBarrier(EI_CommandContext& commandContext)
{
    TRESSFX_ASSERT(m_Positions != nullptr);
    TRESSFX_ASSERT(m_PositionsPrev != nullptr);
    TRESSFX_ASSERT(m_PositionsPrevPrev != nullptr);

    EI_Barrier uav[] = 
    {
        { m_Positions.get(), EI_STATE_UAV, EI_STATE_UAV },
        { m_PositionsPrev.get(), EI_STATE_UAV, EI_STATE_UAV },
        { m_PositionsPrevPrev.get(), EI_STATE_UAV, EI_STATE_UAV }
    };
    commandContext.SubmitBarrier(AMD_ARRAY_SIZE(uav), uav);

    // Assuming tangent is only written by one kernel, so will get caught with transition to SRV
    // for rendering.
}
