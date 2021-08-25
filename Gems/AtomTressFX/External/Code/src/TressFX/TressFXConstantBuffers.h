//---------------------------------------------------------------------------------------
// Constant buffer layouts.
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

#include <TressFX/TressFXCommon.h>
#include <TressFX/AMD_TressFX.h>

// Contains data typically passed through constant buffers.

namespace AMD
{
    struct TressFXViewParams
    {
        float4x4 mVP;
        float4 vEye;
        float4 vViewport;
        float4x4 mInvViewProj;
    };

    struct TressFXSimulationParams
    {
        float4 m_Wind;
        float4 m_Wind1;
        float4 m_Wind2;
        float4 m_Wind3;

        float4 m_Shape;  // damping, local stiffness, global stiffness, global range.
                         // float m_Damping;
                         // float m_StiffnessForLocalShapeMatching;
                         // float m_StiffnessForGlobalShapeMatching;
                         // float m_GlobalShapeMatchingEffectiveRange;

        float4 m_GravTimeTip;  // gravity, time step size,
                               // float m_GravityMagnitude;
                               // float m_TimeStep;
                               // float m_TipSeparationFactor;
                               // float m_velocityShockPropogation;

        sint4 m_SimInts;  // 4th component unused.
                          // int m_NumLengthConstraintIterations;
                          // int m_NumLocalShapeMatchingIterations;
                          // int m_bCollision;
                          // int m_CPULocalIterations;

        sint4 m_Counts;
        // int m_NumOfStrandsPerThreadGroup;
        // int m_NumFollowHairsPerGuideHair;
        // int m_NumVerticesPerStrand; // should be 2^n (n is integer and greater than 2) and less than
        // or equal to TRESSFX_SIM_THREAD_GROUP_SIZE. i.e. 8, 16, 32 or 64

        float4 m_VSP;

        float g_ResetPositions;
        float g_ClampPositionDelta;
        float g_pad1;
        float g_pad2;

        float4x4 m_BoneSkinningMatrix[AMD_TRESSFX_MAX_NUM_BONES];

#if TRESSFX_COLLISION_CAPSULES
        float4 m_centerAndRadius0[TRESSFX_MAX_NUM_COLLISION_CAPSULES];
        float4 m_centerAndRadius1[TRESSFX_MAX_NUM_COLLISION_CAPSULES];
        sint4 m_numCollisionCapsules;
#endif


        void SetDamping(float d) { m_Shape.x = d; }
        void SetLocalStiffness(float s) { m_Shape.y = s; }
        void SetGlobalStiffness(float s) { m_Shape.z = s; }
        void SetGlobalRange(float r) { m_Shape.w = r; }

        void SetGravity(float g) { m_GravTimeTip.x = g; }
        void SetTimeStep(float dt) { m_GravTimeTip.y = dt; }
        void SetTipSeperation(float ts) { m_GravTimeTip.z = ts; }

        void SetVelocityShockPropogation(float vsp) { m_VSP.x = vsp; }
        void SetVSPAccelThreshold(float vspAccelThreshold) { m_VSP.y = vspAccelThreshold; }

        void SetLengthIterations(int i) { m_SimInts.x = i; }
        void SetLocalIterations(int i) { m_SimInts.y = i; }
        void SetCollision(bool on) { m_SimInts.z = on ? 1 : 0; }

        void SetVerticesPerStrand(int n)
        {
            m_Counts.x = TRESSFX_SIM_THREAD_GROUP_SIZE / n;
            m_Counts.z = n;
        }
        void SetFollowHairsPerGuidHair(int n) { m_Counts.y = n; }
    };

    struct TressFXCapsuleCollisionConstantBuffer
    {
        float4 m_centerAndRadius[TRESSFX_MAX_NUM_COLLISION_CAPSULES];
        float4 m_centerAndRadiusSquared[TRESSFX_MAX_NUM_COLLISION_CAPSULES];
        int m_numCollisionCapsules;
    };

    struct TressFXSDFCollisionParams
    {
        float4      m_Origin;
        float       m_CellSize;
        int         m_NumCellsX;
        int         m_NumCellsY;
        int         m_NumCellsZ;
        int         m_MaxMarchingCubesVertices;
        float       m_MarchingCubesIsolevel;
        float       m_CollisionMargin;
        int         m_NumHairVerticesPerStrand;
        int         m_NumTotalHairVertices;
        float       pad1;
        float       pad2;
        float       pad3;
    };

    // If you change this, you MUST also change TressFXParameters in HairRenderingSrgs.azsli
    struct TressFXRenderParams // TressFXParameters
    {
        // General information
        float       FiberRadius = 0.0021f;

        // For deep approximated shadow lookup
        float       ShadowAlpha = 0.35f;
        float       FiberSpacing = 0.4f;

        // Original TressFX Kajiya lighting model parameters
        float       HairKs2 = 0.072f;
        float       HairEx2 = 11.80f;
        float3      fPadding0;

        float4      MatKValue = {{{0.f, 0.07f, 0.0017f, 14.40f}}};  // KAmbient, KDiffuse, KSpec1, Exp1

        int         MaxShadowFibers = 50;

        // Marschner lighting model parameters 
        float       Roughness;
        float       CuticleTilt;  // tile angle in radians

        float       fPadding1;
    };

    // If you change this, you MUST also change TressFXStrandParameters in HairRenderingSrgs.azsli
    struct TressFXStrandParams // TressFXStrandParameters
    {
        // For lighting/shading
        float4      MatBaseColor = {{{1.f, 1.f, 1.f, 0.63f}}};
        float4      MatTipColor = {{{0.5f, 0.5f, 1.f, 0.63f}}};

        // General information
        float       TipPercentage = 0.5f;
        float       StrandUVTilingFactor = 1.f;
        float       FiberRatio = 0.463f;
        float       FiberRadius = 0.0021f;

        int         NumVerticesPerStrand = 32;
        int         EnableThinTip = 0;

        // For PPLL
        int         NodePoolSize = 0;
        int         RenderParamsIndex = 0;

        // Other params
        int         EnableStrandUV = 0;
        int         EnableStrandTangent = 0;
        sint2       iPadding1;
    };

    //--------------------------------------------------------------------------
    //! This structure is used as the per object material.
    //! It is filled per hair object and set in an array that represents the
    //!  materials of all hair objects in the scene.
    //! The per draw raster pass will write per pixel data with index of the
    //!  material so that the resolve pass can use it a lookup table to shade
    //!  the pixel per the hair object it belongs to.
    struct ShadeParams
    {
        // General information
        float       FiberRadius = 0.002f;
        // For deep approximated shadow lookup
        float       ShadowAlpha = 0.35f;
        float       FiberSpacing = 0.4f;

        // Original TressFX Kajiya lighting model parameters
        float       HairEx2 = 11.80f;
        float4      MatKValue = {{{0.f, 0.07f, 0.0017f, 14.40f}}};   // KAmbient, KDiffuse, KSpec1, Exp1
        float       HairKs2 = 0.072f;

        // Marschner lighting model parameters 
        float       Roughness;
        float       CuticleTilt;  // tile angle in radians

        float       fPadding0;
    };

    //! Hair objects material array - passed as a constant structure used by
    //!  the resolve pass to shader hair pixels given the index via the PPLL.
    struct TressFXShadeParams
    {
        ShadeParams HairShadeParams[AMD_TRESSFX_MAX_HAIR_GROUP_RENDER];
    };
    //--------------------------------------------------------------------------


    // If you change this, you MUST also change TressFXLightParameters in TressFXRendering.hlsl
#define AMD_TRESSFX_MAX_LIGHTS 10

    struct LightParams
    {
        float  LightIntensity = 1.f;                   // fLightIntensity in Sushi
        float  LightOuterConeCos = 0.70710678f;        // vLightConeAngles.x in Sushi
        float  LightInnerConeCos = 0.70710678f;        // vLightConeAngles.y in Sushi
        float  LightRange = 100.f;                     // vLightConeAngles.z in Sushi

        float4 LightPositionWS = {{{0.f, 0.f, 0.f, 0.f}}};// vLightPosWS in Sushi
        float4 LightDirWS = {{{0.f, -1.f, 0.f, 0.f}}};    // vLightDirWS in Sushi
        float4 LightColor = {{{1.f, 1.f, 1.f, 0.f}}};     // vLightColor in Sushi

        float4x4 ShadowProjection;
        float4   ShadowParams = {{{0.0007f, 0.f, 0.f, 0.f}}};

        int     LightType = 3;                              // vLightParams.x in Sushi
        int     ShadowMapIndex = -1;
        int     ShadowMapSize = 2048;
        int     Padding;
    };

    struct TressFXLightParams
    {
        int         NumLights;
        int			UseDepthApproximation;
        sint2       Padding;
        LightParams LightInfo[AMD_TRESSFX_MAX_LIGHTS];
    };

} // namespace AMD

