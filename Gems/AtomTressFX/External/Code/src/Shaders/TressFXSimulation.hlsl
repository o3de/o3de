
//---------------------------------------------------------------------------------------
// Shader code related to simulating hair strands in compute.
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

// This is code decoration for our sample.  
// Remove this and the EndHLSL at the end of this file 
// to turn this into valid HLSL

// StartHLSL TressFXSimulationCS

//--------------------------------------------------------------------------------------
// File: TresFXSimulation.hlsl
//
// Physics simulation of hair using compute shaders
//--------------------------------------------------------------------------------------

#define USE_MESH_BASED_HAIR_TRANSFORM 0

// Whether bones are specified by dual quaternion.
// This option is not currently functional.
#define TRESSFX_DQ 0

// Toggle capsule collisions
#define TRESSFX_COLLISION_CAPSULES 0

//constants that change frame to frame
[[vk::binding(13, 0)]] cbuffer tressfxSimParameters : register( b13, space0 )
{
    float4 g_Wind;
    float4 g_Wind1;
    float4 g_Wind2;
    float4 g_Wind3;

    float4 g_Shape;                  // damping, local stiffness, global stiffness, global range.
    float4 g_GravTimeTip;            // gravity maginitude (assumed to be in negative y direction.)
    int4   g_SimInts;                // Length iterations, local iterations, collision flag.
    int4   g_Counts;                 // num strands per thread group, num follow hairs per guid hair, num verts per strand.
    float4 g_VSP;                   // VSP parmeters

    float g_ResetPositions;
    float g_ClampPositionDelta;
    float g_pad1;
    float g_pad2;

#if TRESSFX_DQ
    float4 g_BoneSkinningDQ[AMD_TRESSFX_MAX_NUM_BONES * 2];
#else
    row_major float4x4 g_BoneSkinningMatrix[AMD_TRESSFX_MAX_NUM_BONES];
#endif

#if TRESSFX_COLLISION_CAPSULES
    float4 g_centerAndRadius0[TRESSFX_MAX_NUM_COLLISION_CAPSULES];
    float4 g_centerAndRadius1[TRESSFX_MAX_NUM_COLLISION_CAPSULES];
    int4 g_numCollisionCapsules;
#endif

}

#define g_NumOfStrandsPerThreadGroup      g_Counts.x
#define g_NumFollowHairsPerGuideHair      g_Counts.y
#define g_NumVerticesPerStrand            g_Counts.z

#define g_NumLocalShapeMatchingIterations g_SimInts.y

#define g_GravityMagnitude                g_GravTimeTip.x
#define g_TimeStep                        g_GravTimeTip.y
#define g_TipSeparationFactor             g_GravTimeTip.z


// We no longer support groups (indirection).
int GetStrandType(int globalThreadIndex)
{
    return 0;
}

float GetDamping(int strandType)
{
    // strand type unused.
    // In the future, we may create an array and use indirection.
    return g_Shape.x;
}

float GetLocalStiffness(int strandType)
{
    // strand type unused.
    // In the future, we may create an array and use indirection.
    return g_Shape.y;
}

float GetGlobalStiffness(int strandType)
{
    // strand type unused.
    // In the future, we may create an array and use indirection.
    return g_Shape.z;
}

float GetGlobalRange(int strandType)
{
    // strand type unused.
    // In the future, we may create an array and use indirection.
    return g_Shape.w;
}

float GetVelocityShockPropogation()
{
    return g_VSP.x;
}

float GetVSPAccelThreshold()
{
    return g_VSP.y;
}

int GetLocalConstraintIterations()
{
    return (int)g_SimInts.y;
}

int GetLengthConstraintIterations()
{
    return (int)g_SimInts.x;
}

struct Transforms
{
    row_major float4x4 tfm;
    float4 quat;
};

struct HairToTriangleMapping
{
    uint meshIndex;          // index to the mesh
    uint triangleIndex;      // index to triangle in the skinned mesh that contains this hair
    float3 barycentricCoord; // barycentric coordinate of the hair within the triangle
    uint reserved;           // for future use
};

struct BoneSkinningData
{
    float4 boneIndex;  // x, y, z and w component are four bone indices per strand
    float4 boneWeight; // x, y, z and w component are four bone weights per strand
};

struct StrandLevelData
{
    float4 skinningQuat;
    float4 vspQuat;
    float4 vspTranslation;
};

// UAVs
[[vk::binding(0, 1)]] RWStructuredBuffer<float4> g_HairVertexPositions : register(u0, space1);
[[vk::binding(1, 1)]] RWStructuredBuffer<float4> g_HairVertexPositionsPrev : register(u1, space1);
[[vk::binding(2, 1)]] RWStructuredBuffer<float4> g_HairVertexPositionsPrevPrev : register(u2, space1);
[[vk::binding(3, 1)]] RWStructuredBuffer<float4> g_HairVertexTangents : register(u3, space1);
[[vk::binding(4, 1)]] RWStructuredBuffer<StrandLevelData> g_StrandLevelData : register(u4, space1);

#if USE_MESH_BASED_HAIR_TRANSFORM == 1
RWStructuredBuffer<Transforms>  g_Transforms;
#endif

// SRVs
[[vk::binding(4, 0)]] StructuredBuffer<float4> g_InitialHairPositions : register(t4, space0);
[[vk::binding(5, 0)]] StructuredBuffer<float> g_HairRestLengthSRV : register(t5, space0);
[[vk::binding(6, 0)]] StructuredBuffer<float> g_HairStrandType : register(t6, space0);
[[vk::binding(7, 0)]] StructuredBuffer<float4> g_FollowHairRootOffset : register(t7, space0);
[[vk::binding(10, 0)]] StructuredBuffer<float4> g_MeshVertices : register(t10, space0);
[[vk::binding(11, 0)]] StructuredBuffer<float4> g_TransformedVerts : register(t11, space0);
[[vk::binding(12, 0)]] StructuredBuffer<BoneSkinningData> g_BoneSkinningData : register(t12, space0);

// If you change the value below, you must change it in TressFXAsset.h as well.
#ifndef THREAD_GROUP_SIZE
#define THREAD_GROUP_SIZE 64
#endif

groupshared float4 sharedPos[THREAD_GROUP_SIZE];
groupshared float4 sharedTangent[THREAD_GROUP_SIZE];
groupshared float  sharedLength[THREAD_GROUP_SIZE];

//--------------------------------------------------------------------------------------
//
//  Helper Functions for the main simulation shaders
//
//--------------------------------------------------------------------------------------
bool IsMovable(float4 particle)
{
    if ( particle.w > 0 )
        return true;
    return false;
}

float2 ConstraintMultiplier(float4 particle0, float4 particle1)
{
    if (IsMovable(particle0))
    {
        if (IsMovable(particle1))
            return float2(0.5, 0.5);
        else
            return float2(1, 0);
    }
    else
    {
        if (IsMovable(particle1))
            return float2(0, 1);
        else
            return float2(0, 0);
    }
}

float4 MakeQuaternion(float angle_radian, float3 axis)
{
    // create quaternion using angle and rotation axis
    float4 quaternion;
    float halfAngle = 0.5f * angle_radian;
    float sinHalf = sin(halfAngle);

    quaternion.w = cos(halfAngle);
    quaternion.xyz = sinHalf * axis.xyz;

    return quaternion;
}

// Makes a quaternion from a 4x4 column major rigid transform matrix. Rigid transform means that rotational 3x3 sub matrix is orthonormal. 
// Note that this function does not check the orthonormality. 
float4 MakeQuaternion(column_major float4x4 m)
{
    float4 q;
    float trace = m[0][0] + m[1][1] + m[2][2];

    if (trace > 0.0f)
    {
        float r = sqrt(trace + 1.0f);
        q.w = 0.5 * r;
        r = 0.5 / r;
        q.x = (m[1][2] - m[2][1])*r;
        q.y = (m[2][0] - m[0][2])*r;
        q.z = (m[0][1] - m[1][0])*r;
    }
    else
    {
        int i = 0, j = 1, k = 2;

        if (m[1][1] > m[0][0])
        {
            i = 1; j = 2; k = 0;
        }
        if (m[2][2] > m[i][i])
        {
            i = 2; j = 0; k = 1;
        }

        float r = sqrt(m[i][i] - m[j][j] - m[k][k] + 1.0f);

        float qq[4];

        qq[i] = 0.5f * r;
        r = 0.5f / r;
        q.w = (m[j][k] - m[k][j])*r;
        qq[j] = (m[j][i] + m[i][j])*r;
        qq[k] = (m[k][i] + m[i][k])*r;

        q.x = qq[0]; q.y = qq[1]; q.z = qq[2];
    }

    return q;
}

float4 InverseQuaternion(float4 q)
{
    float lengthSqr = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;

    if ( lengthSqr < 0.001 )
        return float4(0, 0, 0, 1.0f);

    q.x = -q.x / lengthSqr;
    q.y = -q.y / lengthSqr;
    q.z = -q.z / lengthSqr;
    q.w = q.w / lengthSqr;

    return q;
}

float3 MultQuaternionAndVector(float4 q, float3 v)
{
    float3 uv, uuv;
    float3 qvec = float3(q.x, q.y, q.z);
    uv = cross(qvec, v);
    uuv = cross(qvec, uv);
    uv *= (2.0f * q.w);
    uuv *= 2.0f;

    return v + uv + uuv;
}

float4 MultQuaternionAndQuaternion(float4 qA, float4 qB)
{
    float4 q;

    q.w = qA.w * qB.w - qA.x * qB.x - qA.y * qB.y - qA.z * qB.z;
    q.x = qA.w * qB.x + qA.x * qB.w + qA.y * qB.z - qA.z * qB.y;
    q.y = qA.w * qB.y + qA.y * qB.w + qA.z * qB.x - qA.x * qB.z;
    q.z = qA.w * qB.z + qA.z * qB.w + qA.x * qB.y - qA.y * qB.x;

    return q;
}

float4 NormalizeQuaternion(float4 q)
{
    float4 qq = q;
    float n = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;

    if (n < 1e-10f)
    {
        qq.w = 1;
        return qq;
    }

    qq *= 1.0f / sqrt(n);
    return qq;
}

// Compute a quaternion which rotates u to v. u and v must be unit vector. 
float4 QuatFromTwoUnitVectors(float3 u, float3 v)
{
    float r = 1.f + dot(u, v);
    float3 n;

    // if u and v are parallel
    if (r < 1e-7)
    {
        r = 0.0f;
        n = abs(u.x) > abs(u.z) ? float3(-u.y, u.x, 0.f) : float3(0.f, -u.z, u.y);
    }
    else
    {
        n = cross(u, v);  
    }

    float4 q = float4(n.x, n.y, n.z, r);
    return NormalizeQuaternion(q);
}

// Make the inpute 4x4 matrix be identity
float4x4 MakeIdentity()
{
    float4x4 m;
    m._m00 = 1;   m._m01 = 0;   m._m02 = 0;   m._m03 = 0;
    m._m10 = 0;   m._m11 = 1;   m._m12 = 0;   m._m13 = 0;
    m._m20 = 0;   m._m21 = 0;   m._m22 = 1;   m._m23 = 0;
    m._m30 = 0;   m._m31 = 0;   m._m32 = 0;   m._m33 = 1;

    return m;
}

void ApplyDistanceConstraint(inout float4 pos0, inout float4 pos1, float targetDistance, float stiffness = 1.0)
{
    float3 delta = pos1.xyz - pos0.xyz;
    float distance = max(length(delta), 1e-7);
    float stretching = 1 - targetDistance / distance;
    delta = stretching * delta;
    float2 multiplier = ConstraintMultiplier(pos0, pos1);

    pos0.xyz += multiplier[0] * delta * stiffness;
    pos1.xyz -= multiplier[1] * delta * stiffness;
}

void CalcIndicesInVertexLevelTotal(uint local_id, uint group_id, inout uint globalStrandIndex, inout uint localStrandIndex, inout uint globalVertexIndex, inout uint localVertexIndex, inout uint numVerticesInTheStrand, inout uint indexForSharedMem, inout uint strandType)
{
    indexForSharedMem = local_id;
    numVerticesInTheStrand = (THREAD_GROUP_SIZE / g_NumOfStrandsPerThreadGroup);

    localStrandIndex = local_id % g_NumOfStrandsPerThreadGroup;
    globalStrandIndex = group_id * g_NumOfStrandsPerThreadGroup + localStrandIndex;
    localVertexIndex = (local_id - localStrandIndex) / g_NumOfStrandsPerThreadGroup;

    strandType = GetStrandType(globalStrandIndex);
    globalVertexIndex = globalStrandIndex * numVerticesInTheStrand + localVertexIndex;
}

void CalcIndicesInVertexLevelMaster(uint local_id, uint group_id, inout uint globalStrandIndex, inout uint localStrandIndex, inout uint globalVertexIndex, inout uint localVertexIndex, inout uint numVerticesInTheStrand, inout uint indexForSharedMem, inout uint strandType)
{
    indexForSharedMem = local_id;
    numVerticesInTheStrand = (THREAD_GROUP_SIZE / g_NumOfStrandsPerThreadGroup);

    localStrandIndex = local_id % g_NumOfStrandsPerThreadGroup;
    globalStrandIndex = group_id * g_NumOfStrandsPerThreadGroup + localStrandIndex;
    globalStrandIndex *= (g_NumFollowHairsPerGuideHair+1);
    localVertexIndex = (local_id - localStrandIndex) / g_NumOfStrandsPerThreadGroup;

    strandType = GetStrandType(globalStrandIndex);
    globalVertexIndex = globalStrandIndex * numVerticesInTheStrand + localVertexIndex;
}

void CalcIndicesInStrandLevelTotal(uint local_id, uint group_id, inout uint globalStrandIndex, inout uint numVerticesInTheStrand, inout uint globalRootVertexIndex, inout uint strandType)
{
    globalStrandIndex = THREAD_GROUP_SIZE*group_id + local_id;
    numVerticesInTheStrand = (THREAD_GROUP_SIZE / g_NumOfStrandsPerThreadGroup);
    strandType = GetStrandType(globalStrandIndex);
    globalRootVertexIndex = globalStrandIndex * numVerticesInTheStrand;
}

void CalcIndicesInStrandLevelMaster(uint local_id, uint group_id, inout uint globalStrandIndex, inout uint numVerticesInTheStrand, inout uint globalRootVertexIndex, inout uint strandType)
{
    globalStrandIndex = THREAD_GROUP_SIZE*group_id + local_id;
    globalStrandIndex *= (g_NumFollowHairsPerGuideHair+1);
    numVerticesInTheStrand = (THREAD_GROUP_SIZE / g_NumOfStrandsPerThreadGroup);
    strandType = GetStrandType(globalStrandIndex);
    globalRootVertexIndex = globalStrandIndex * numVerticesInTheStrand;
}

//--------------------------------------------------------------------------------------
//
//  Integrate
//
//  Uses Verlet integration to calculate the new position for the current time step
//
//--------------------------------------------------------------------------------------
float3 Integrate(float3 curPosition, float3 oldPosition, float3 initialPos, float dampingCoeff = 1.0f)
{
    float3 force = g_GravityMagnitude * float3(0, -1.0f, 0);
    // float decay = exp(-g_TimeStep/decayTime)
    float decay = exp(-dampingCoeff * g_TimeStep * 60.0f);
    return curPosition + decay * (curPosition - oldPosition) + force * g_TimeStep * g_TimeStep;
}


struct CollisionCapsule
{
    float4 p0; // xyz = position of capsule 0, w = radius 0
    float4 p1; // xyz = position of capsule 1, w = radius 1
};

//--------------------------------------------------------------------------------------
//
//  CapsuleCollision
//
//  Moves the position based on collision with capsule
//
//--------------------------------------------------------------------------------------
bool CapsuleCollision(float4 curPosition, float4 oldPosition, inout float3 newPosition, CollisionCapsule cc, float friction = 0.4f)
{
    const float radius0 = cc.p0.w;
    const float radius1 = cc.p1.w;
    newPosition = curPosition.xyz;

    if ( !IsMovable(curPosition) )
        return false;

    float3 segment = cc.p1.xyz - cc.p0.xyz;
    float3 delta0 = curPosition.xyz - cc.p0.xyz;
    float3 delta1 = cc.p1.xyz - curPosition.xyz;

    float dist0 = dot(delta0, segment);
    float dist1 = dot(delta1, segment);

    // colliding with sphere 1
    if (dist0 < 0.f )
    {
        if ( dot(delta0, delta0) < radius0 * radius0)
        {
            float3 n = normalize(delta0);
            newPosition = radius0 * n + cc.p0.xyz;
            return true;
        }

        return false;
    }

    // colliding with sphere 2
    if (dist1 < 0.f )
    {
        if ( dot(delta1, delta1) < radius1 * radius1)
        {
            float3 n = normalize(-delta1);
            newPosition = radius1 * n + cc.p1.xyz;
            return true;
        }

        return false;
    }

    // colliding with middle cylinder
    float3 x = (dist0 * cc.p1.xyz + dist1 * cc.p0.xyz) / (dist0 + dist1);
    float3 delta = curPosition.xyz - x;

    float radius_at_x = (dist0 * radius1 + dist1 * radius0) / (dist0 + dist1);

    if ( dot(delta, delta) < radius_at_x * radius_at_x)
    {
        float3 n = normalize(delta);
        float3 vec = curPosition.xyz - oldPosition.xyz;
        float3 segN = normalize(segment);
        float3 vecTangent = dot(vec, segN) * segN;
        float3 vecNormal = vec - vecTangent;
        newPosition = oldPosition.xyz + friction * vecTangent + (vecNormal + radius_at_x * n - delta);
        return true;
    }

    return false;
}

float3 ApplyVertexBoneSkinning(float3 vertexPos, BoneSkinningData skinningData, inout float4 bone_quat)
{
    float3 newVertexPos;

#if TRESSFX_DQ
    {
        // weighted rotation part of dual quaternion
        float4 nq = g_BoneSkinningDQ[skinningData.boneIndex.x * 2] * skinningData.boneWeight.x + g_BoneSkinningDQ[skinningData.boneIndex.y * 2] * skinningData.boneWeight.y +
            g_BoneSkinningDQ[skinningData.boneIndex.z * 2] * skinningData.boneWeight.z + g_BoneSkinningDQ[skinningData.boneIndex.w * 2] * skinningData.boneWeight.w;

        // weighted tranlation part of dual quaternion
        float4 dq = g_BoneSkinningDQ[skinningData.boneIndex.x * 2 + 1] * skinningData.boneWeight.x + g_BoneSkinningDQ[skinningData.boneIndex.y * 2 + 1] * skinningData.boneWeight.y +
            g_BoneSkinningDQ[skinningData.boneIndex.z * 2 + 1] * skinningData.boneWeight.z + g_BoneSkinningDQ[skinningData.boneIndex.w * 2 + 1] * skinningData.boneWeight.w;

        float len = rsqrt(dot(nq, nq));
        nq *= len;
        dq *= len;

        bone_quat = nq;

        //convert translation part of dual quaternion to translation vector:
        float3 translation = (nq.w*dq.xyz - dq.w*nq.xyz + cross(nq.xyz, dq.xyz)) * 2;

        newVertexPos = MultQuaternionAndVector(nq, vertexPos) + translation.xyz;
    }
#else
    {
        // Interpolate world space bone matrices using weights. 
        row_major float4x4 bone_matrix = g_BoneSkinningMatrix[skinningData.boneIndex[0]] * skinningData.boneWeight[0];
        float weight_sum = skinningData.boneWeight[0];

        for (int i = 1; i < 4; i++)
        {
            if (skinningData.boneWeight[i] > 0)
            {
                bone_matrix += g_BoneSkinningMatrix[skinningData.boneIndex[i]] * skinningData.boneWeight[i];
                weight_sum += skinningData.boneWeight[i];
            }
        }

        bone_matrix /= weight_sum;
        bone_quat = MakeQuaternion(bone_matrix);

        newVertexPos = mul(float4(vertexPos, 1), bone_matrix).xyz;
    }
#endif

    return newVertexPos;
}

//--------------------------------------------------------------------------------------
//
//  UpdateFinalVertexPositions
//
//  Updates the  hair vertex positions based on the physics simulation
//
//--------------------------------------------------------------------------------------
void UpdateFinalVertexPositions(float4 oldPosition, float4 newPosition, int globalVertexIndex, int localVertexIndex, int numVerticesInTheStrand)
{
    g_HairVertexPositionsPrevPrev[globalVertexIndex] = g_HairVertexPositionsPrev[globalVertexIndex];
    g_HairVertexPositionsPrev[globalVertexIndex] = oldPosition;
    g_HairVertexPositions[globalVertexIndex] = newPosition;
}
            

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void TestShader(uint GIndex : SV_GroupIndex,
    uint3 GId : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID)
{
    // Copy data into shared memory
    g_HairVertexPositions[0] = g_InitialHairPositions[0]; // rest position

}

//------------------------------------------------------------------------------
// Adi: this method will update the hair vertices according to the bone matrices 
// and will not apply any physics interaction and response.
// Use for testing of a single pass.
//------------------------------------------------------------------------------
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void SkinHairVerticesTest(uint GIndex : SV_GroupIndex,
    uint3 GId : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType);

    // Copy data into shared memory
    float4 initialPos = g_InitialHairPositions[globalVertexIndex]; // rest position
    BoneSkinningData skinningData = g_BoneSkinningData[globalStrandIndex];
    float4 bone_quat;
    float4 currentPos = float4(ApplyVertexBoneSkinning(initialPos.xyz, skinningData, bone_quat), 1.0);  // Apply bone skinning to initial position
                                                                   
//    UpdateFinalVertexPositions( g_HairVertexPositions[globalVertexIndex], currentPos, globalVertexIndex, localVertexIndex, numVerticesInTheStrand);
    g_HairVertexPositions[globalVertexIndex] = currentPos;
//    UpdateFinalVertexPositions( currentPos, currentPos, globalVertexIndex, localVertexIndex, numVerticesInTheStrand);

    GroupMemoryBarrierWithGroupSync();

    //-------------------
    // Compute tangent
    //-------------------
    // If this is the last vertex in the strand, we can't get tangent from subtracting from the next vertex, need to use last vertex to current
    sharedPos[indexForSharedMem] = g_HairVertexPositions[globalVertexIndex];
    sharedTangent[indexForSharedMem] = g_HairVertexTangents[globalVertexIndex];
    uint numOfStrandsPerThreadGroup = g_NumOfStrandsPerThreadGroup;
    uint indexForTangent = (localVertexIndex == numVerticesInTheStrand - 1) ? indexForSharedMem - numOfStrandsPerThreadGroup : indexForSharedMem;
    float3 tangent = sharedPos[indexForTangent + numOfStrandsPerThreadGroup].xyz - sharedPos[indexForTangent].xyz;
    g_HairVertexTangents[globalVertexIndex].xyz = normalize(tangent);

    //-------------------
    // Compute follow hair
    //-------------------
    // Adi - the following barrier might not be required 
 //   GroupMemoryBarrierWithGroupSync();

    for ( uint i = 0; i < g_NumFollowHairsPerGuideHair; i++ )
    {
        int globalFollowVertexIndex = globalVertexIndex + numVerticesInTheStrand * (i + 1);
        int globalFollowStrandIndex = globalStrandIndex + i + 1;
        float factor = g_TipSeparationFactor*((float)localVertexIndex / (float)numVerticesInTheStrand) + 1.0f;
        float3 followPos = sharedPos[indexForSharedMem].xyz + factor*g_FollowHairRootOffset[globalFollowStrandIndex].xyz;
        g_HairVertexPositions[globalFollowVertexIndex].xyz = followPos;
        g_HairVertexTangents[globalFollowVertexIndex] = sharedTangent[indexForSharedMem];
    }

    return;
}


//--------------------------------------------------------------------------------------
//
//  IntegrationAndGlobalShapeConstraints
//
//  Compute shader to simulate the gravitational force with integration and to maintain the
//  global shape constraints.
//
// One thread computes one vertex.
//
//--------------------------------------------------------------------------------------
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void IntegrationAndGlobalShapeConstraints(uint GIndex : SV_GroupIndex,
                  uint3 GId : SV_GroupID,
                  uint3 DTid : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType);

    // Copy data into shared memory
    float4 initialPos = g_InitialHairPositions[globalVertexIndex]; // rest position

    // Apply bone skinning to initial position
    BoneSkinningData skinningData = g_BoneSkinningData[globalStrandIndex];
    float4 bone_quat;
    initialPos.xyz = ApplyVertexBoneSkinning(initialPos.xyz, skinningData, bone_quat);

    // position when this step starts. In other words, a position from the last step.
    float4 currentPos = sharedPos[indexForSharedMem] = g_HairVertexPositions[globalVertexIndex];

    GroupMemoryBarrierWithGroupSync();

    // Integrate
    float dampingCoeff = GetDamping(strandType);
    float4 oldPos;
    oldPos = g_HairVertexPositionsPrev[globalVertexIndex];

    // reset if we got teleported
    if (g_ResetPositions != 0.0f)
    {
        currentPos = initialPos;
        oldPos = initialPos;
    }

    if ( IsMovable(currentPos) )
        sharedPos[indexForSharedMem].xyz = Integrate(currentPos.xyz, oldPos.xyz, initialPos.xyz, dampingCoeff);
    else
        sharedPos[indexForSharedMem] = initialPos;
        
    // Global Shape Constraints
    float stiffnessForGlobalShapeMatching = GetGlobalStiffness(strandType);
    float globalShapeMatchingEffectiveRange = GetGlobalRange(strandType);

    if ( stiffnessForGlobalShapeMatching > 0 && globalShapeMatchingEffectiveRange )
    {
        if ( IsMovable(sharedPos[indexForSharedMem]) )
        {
            if ( (float)localVertexIndex < globalShapeMatchingEffectiveRange * (float)numVerticesInTheStrand )
            {
                float factor = stiffnessForGlobalShapeMatching;
                float3 del = factor * (initialPos - sharedPos[indexForSharedMem]).xyz;
                sharedPos[indexForSharedMem].xyz += del;
            }
        }
    }

    // update global position buffers
    UpdateFinalVertexPositions(currentPos, sharedPos[indexForSharedMem], globalVertexIndex, localVertexIndex, numVerticesInTheStrand);
}

//--------------------------------------------------------------------------------------
//
//  Calculate Strand Level Data
//
//  Propagate velocity shock resulted by attached based mesh
//
// One thread computes one strand.
//
//--------------------------------------------------------------------------------------
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CalculateStrandLevelData(uint GIndex : SV_GroupIndex,
                              uint3 GId : SV_GroupID,
                              uint3 DTid : SV_DispatchThreadID)
{
    uint local_id, group_id, globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType;
    CalcIndicesInStrandLevelMaster(GIndex, GId.x, globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType);

    float4 pos_old_old[2]; // previous previous positions for vertex 0 (root) and vertex 1.
    float4 pos_old[2]; // previous positions for vertex 0 (root) and vertex 1.
    float4 pos_new[2]; // current positions for vertex 0 (root) and vertex 1.

    pos_old_old[0] = g_HairVertexPositionsPrevPrev[globalRootVertexIndex];
    pos_old_old[1] = g_HairVertexPositionsPrevPrev[globalRootVertexIndex + 1];

    pos_old[0] = g_HairVertexPositionsPrev[globalRootVertexIndex];
    pos_old[1] = g_HairVertexPositionsPrev[globalRootVertexIndex + 1];
    
    pos_new[0] = g_HairVertexPositions[globalRootVertexIndex];
    pos_new[1] = g_HairVertexPositions[globalRootVertexIndex + 1];

    float3 u = normalize(pos_old[1].xyz - pos_old[0].xyz);
    float3 v = normalize(pos_new[1].xyz - pos_new[0].xyz);

    // Compute rotation and translation which transform pos_old to pos_new. 
    // Since the first two vertices are immovable, we can assume that there is no scaling during tranform. 
    float4 rot = QuatFromTwoUnitVectors(u, v);
    float3 trans = pos_new[0].xyz - MultQuaternionAndVector(rot, pos_old[0].xyz);

    float vspCoeff = GetVelocityShockPropogation();
    float restLength0 = g_HairRestLengthSRV[globalRootVertexIndex];
    float vspAccelThreshold  = GetVSPAccelThreshold();

    // Increate the VSP coefficient by checking pseudo-acceleration to handle over-stretching when the character moves very fast 
    float accel = length(pos_new[1] - 2.0 * pos_old[1] + pos_old_old[1]);

    if (accel > vspAccelThreshold) // TODO: expose this value?
        vspCoeff = 1.0f;
    g_StrandLevelData[globalStrandIndex].vspQuat = rot;
    g_StrandLevelData[globalStrandIndex].vspTranslation = float4(trans, vspCoeff);

    // skinning

    // Copy data into shared memory
    float4 initialPos = g_InitialHairPositions[globalRootVertexIndex]; // rest position

    // Apply bone skinning to initial position
    BoneSkinningData skinningData = g_BoneSkinningData[globalStrandIndex];
    float4 bone_quat;
    initialPos.xyz = ApplyVertexBoneSkinning(initialPos.xyz, skinningData, bone_quat);
    g_StrandLevelData[globalStrandIndex].skinningQuat = bone_quat;
}

//--------------------------------------------------------------------------------------
//
//  VelocityShockPropagation
//
//  Propagate velocity shock resulted by attached based mesh
//
// One thread computes one strand.
//
//--------------------------------------------------------------------------------------
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void VelocityShockPropagation(uint GIndex : SV_GroupIndex,
    uint3 GId : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType);

    if (localVertexIndex < 2)
        return;

    float4 vspQuat = g_StrandLevelData[globalStrandIndex].vspQuat;
    float4 vspTrans = g_StrandLevelData[globalStrandIndex].vspTranslation;
    float vspCoeff = vspTrans.w;

    float4 pos_new_n = g_HairVertexPositions[globalVertexIndex];
    float4 pos_old_n = g_HairVertexPositionsPrev[globalVertexIndex];

    pos_new_n.xyz = (1.f - vspCoeff) * pos_new_n.xyz + vspCoeff * (MultQuaternionAndVector(vspQuat, pos_new_n.xyz) + vspTrans.xyz);
    pos_old_n.xyz = (1.f - vspCoeff) * pos_old_n.xyz + vspCoeff * (MultQuaternionAndVector(vspQuat, pos_old_n.xyz) + vspTrans.xyz);

    g_HairVertexPositions[globalVertexIndex].xyz = pos_new_n.xyz;
    g_HairVertexPositionsPrev[globalVertexIndex].xyz = pos_old_n.xyz;
}

//--------------------------------------------------------------------------------------
//
//  LocalShapeConstraints
//
//  Compute shader to maintain the local shape constraints.
//
// One thread computes one strand.
//
//--------------------------------------------------------------------------------------
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void LocalShapeConstraints(uint GIndex : SV_GroupIndex,
    uint3 GId : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID)
{
    uint local_id, group_id, globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType;
    CalcIndicesInStrandLevelMaster(GIndex, GId.x, globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType);

    // stiffness for local shape constraints
    float stiffnessForLocalShapeMatching = GetLocalStiffness(strandType);

    //1.0 for stiffness makes things unstable sometimes.
    stiffnessForLocalShapeMatching = 0.5f*min(stiffnessForLocalShapeMatching, 0.95f);

    //--------------------------------------------
    // Local shape constraint for bending/twisting
    //--------------------------------------------
    {
        float4 boneQuat = g_StrandLevelData[globalStrandIndex].skinningQuat;

        // vertex 1 through n-1
        for (uint localVertexIndex = 1; localVertexIndex < numVerticesInTheStrand - 1; localVertexIndex++)
        {
            uint globalVertexIndex = globalRootVertexIndex + localVertexIndex;

            float4 pos = g_HairVertexPositions[globalVertexIndex];
            float4 pos_plus_one = g_HairVertexPositions[globalVertexIndex + 1];
            float4 pos_minus_one = g_HairVertexPositions[globalVertexIndex - 1];

            float3 bindPos = MultQuaternionAndVector(boneQuat, g_InitialHairPositions[globalVertexIndex].xyz);
            float3 bindPos_plus_one = MultQuaternionAndVector(boneQuat, g_InitialHairPositions[globalVertexIndex + 1].xyz);
            float3 bindPos_minus_one = MultQuaternionAndVector(boneQuat, g_InitialHairPositions[globalVertexIndex - 1].xyz);

            float3 lastVec = pos.xyz - pos_minus_one.xyz;

            float4 invBone = InverseQuaternion(boneQuat);
            float3 vecBindPose = bindPos_plus_one - bindPos;
            float3 lastVecBindPose = bindPos - bindPos_minus_one;
            float4 rotGlobal = QuatFromTwoUnitVectors(normalize(lastVecBindPose), normalize(lastVec));

            float3 orgPos_i_plus_1_InGlobalFrame = MultQuaternionAndVector(rotGlobal, vecBindPose) + pos.xyz;
            float3 del = stiffnessForLocalShapeMatching * (orgPos_i_plus_1_InGlobalFrame - pos_plus_one.xyz);

            if (IsMovable(pos))
                pos.xyz -= del.xyz;

            if (IsMovable(pos_plus_one))
                pos_plus_one.xyz += del.xyz;


            g_HairVertexPositions[globalVertexIndex].xyz = pos.xyz;
            g_HairVertexPositions[globalVertexIndex + 1].xyz = pos_plus_one.xyz;
        }
    }

    return;
}

// Resolve hair vs capsule collisions. To use this, set TRESSFX_COLLISION_CAPSULES to 1 in both hlsl and cpp sides. 
bool ResolveCapsuleCollisions(inout float4 curPosition, float4 oldPos, float friction = 0.4f)
{
    bool bAnyColDetected = false;

#if TRESSFX_COLLISION_CAPSULES
    if (g_numCollisionCapsules.x > 0)
    {        
        float3 newPos;

        for (int i = 0; i < g_numCollisionCapsules.x; i++)
        {
            float3 center0 = g_centerAndRadius0[i].xyz;
            float3 center1 = g_centerAndRadius1[i].xyz;

            CollisionCapsule cc;
            cc.p0.xyz = center0;
            cc.p0.w = g_centerAndRadius0[i].w;
            cc.p1.xyz = center1;
            cc.p1.w = g_centerAndRadius1[i].w;

            bool bColDetected = CapsuleCollision(curPosition, oldPos, newPos, cc, friction);

            if (bColDetected)
                curPosition.xyz = newPos;

            bAnyColDetected = bColDetected ? true : bAnyColDetected;
        }
    }
#endif

    return bAnyColDetected;
}

//--------------------------------------------------------------------------------------
//
//  LengthConstriantsWindAndCollision
//
//  Compute shader to move the vertex position based on wind, maintain the lenght constraints
//  and handles collisions.
//
// One thread computes one vertex.
//
//--------------------------------------------------------------------------------------
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void LengthConstriantsWindAndCollision(uint GIndex : SV_GroupIndex,
                  uint3 GId : SV_GroupID,
                  uint3 DTid : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType);

    uint numOfStrandsPerThreadGroup = g_NumOfStrandsPerThreadGroup;

    //------------------------------
    // Copy data into shared memory
    //------------------------------
    sharedPos[indexForSharedMem] = g_HairVertexPositions[globalVertexIndex];
    sharedLength[indexForSharedMem] = g_HairRestLengthSRV[globalVertexIndex];
    GroupMemoryBarrierWithGroupSync();

    //------------
    // Wind
    //------------
    if ( g_Wind.x != 0 || g_Wind.y != 0 || g_Wind.z != 0 )
    {
        float4 force = float4(0, 0, 0, 0);

        float frame = g_Wind.w;

        if ( localVertexIndex >= 2 && localVertexIndex < numVerticesInTheStrand-1 )
        {
            // combining four winds.
            float a = ((float)(globalStrandIndex % 20))/20.0f;
            float3  w = a*g_Wind.xyz + (1.0f-a)*g_Wind1.xyz + a*g_Wind2.xyz + (1.0f-a)*g_Wind3.xyz;

            uint sharedIndex = localVertexIndex * numOfStrandsPerThreadGroup + localStrandIndex;

            float3 v = sharedPos[sharedIndex].xyz - sharedPos[sharedIndex+numOfStrandsPerThreadGroup].xyz;
            float3 force = -cross(cross(v, w), v);
            sharedPos[sharedIndex].xyz += force*g_TimeStep*g_TimeStep;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    //----------------------------
    // Enforce length constraints
    //----------------------------
    uint a = floor(numVerticesInTheStrand/2.0f);
    uint b = floor((numVerticesInTheStrand-1)/2.0f);

    int nLengthContraintIterations = GetLengthConstraintIterations();

    for ( int iterationE=0; iterationE < nLengthContraintIterations; iterationE++ )
    {
        uint sharedIndex = 2*localVertexIndex * numOfStrandsPerThreadGroup + localStrandIndex;

        if( localVertexIndex < a )
            ApplyDistanceConstraint(sharedPos[sharedIndex], sharedPos[sharedIndex+numOfStrandsPerThreadGroup], sharedLength[sharedIndex].x);

        GroupMemoryBarrierWithGroupSync();

        if( localVertexIndex < b )
            ApplyDistanceConstraint(sharedPos[sharedIndex+numOfStrandsPerThreadGroup], sharedPos[sharedIndex+numOfStrandsPerThreadGroup*2], sharedLength[sharedIndex+numOfStrandsPerThreadGroup].x);

        GroupMemoryBarrierWithGroupSync();
    }

    //------------------------------------------
    // Collision handling with capsule objects
    //------------------------------------------
    float4 oldPos = g_HairVertexPositionsPrev[globalVertexIndex];

    bool bAnyColDetected = ResolveCapsuleCollisions(sharedPos[indexForSharedMem], oldPos);
    GroupMemoryBarrierWithGroupSync();

    //-------------------
    // Compute tangent
    //-------------------
    // If this is the last vertex in the strand, we can't get tangent from subtracting from the next vertex, need to use last vertex to current
    uint indexForTangent = (localVertexIndex == numVerticesInTheStrand - 1) ? indexForSharedMem - numOfStrandsPerThreadGroup : indexForSharedMem;
    float3 tangent = sharedPos[indexForTangent + numOfStrandsPerThreadGroup].xyz - sharedPos[indexForTangent].xyz;
    g_HairVertexTangents[globalVertexIndex].xyz = normalize(tangent);

    //---------------------------------------
    // clamp velocities, rewrite history
    //---------------------------------------
    float3 positionDelta = sharedPos[indexForSharedMem].xyz - oldPos;
    float speedSqr = dot(positionDelta, positionDelta);
    if (speedSqr > g_ClampPositionDelta * g_ClampPositionDelta) {
        positionDelta *= g_ClampPositionDelta * g_ClampPositionDelta / speedSqr;
        g_HairVertexPositionsPrev[globalVertexIndex].xyz = sharedPos[indexForSharedMem].xyz - positionDelta;
    }

    //---------------------------------------
    // update global position buffers
    //---------------------------------------
    g_HairVertexPositions[globalVertexIndex] = sharedPos[indexForSharedMem];

    if (bAnyColDetected)
        g_HairVertexPositionsPrev[globalVertexIndex] = sharedPos[indexForSharedMem];

    return;
}

// One thread computes one vertex.
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void UpdateFollowHairVertices(uint GIndex : SV_GroupIndex,
                              uint3 GId : SV_GroupID,
                              uint3 DTid : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType);

    sharedPos[indexForSharedMem] = g_HairVertexPositions[globalVertexIndex];
    sharedTangent[indexForSharedMem] = g_HairVertexTangents[globalVertexIndex];
    GroupMemoryBarrierWithGroupSync();

    for ( uint i = 0; i < g_NumFollowHairsPerGuideHair; i++ )
    {
        int globalFollowVertexIndex = globalVertexIndex + numVerticesInTheStrand * (i + 1);
        int globalFollowStrandIndex = globalStrandIndex + i + 1;
        float factor = g_TipSeparationFactor*((float)localVertexIndex / (float)numVerticesInTheStrand) + 1.0f;
        float3 followPos = sharedPos[indexForSharedMem].xyz + factor*g_FollowHairRootOffset[globalFollowStrandIndex].xyz;
        g_HairVertexPositions[globalFollowVertexIndex].xyz = followPos;
        g_HairVertexTangents[globalFollowVertexIndex] = sharedTangent[indexForSharedMem];
    }

    return;
}

// One thread computes one vertex.
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void PrepareFollowHairBeforeTurningIntoGuide(uint GIndex : SV_GroupIndex,
                                             uint3 GId : SV_GroupID,
                                             uint3 DTid : SV_DispatchThreadID)
{
    uint globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType;
    CalcIndicesInVertexLevelMaster(GIndex, GId.x, globalStrandIndex, localStrandIndex, globalVertexIndex, localVertexIndex, numVerticesInTheStrand, indexForSharedMem, strandType);

    sharedPos[indexForSharedMem] = g_HairVertexPositions[globalVertexIndex];
    GroupMemoryBarrierWithGroupSync();

    for ( uint i = 0; i < g_NumFollowHairsPerGuideHair; i++ )
    {
        int globalFollowVertexIndex = globalVertexIndex + numVerticesInTheStrand * (i + 1);
        g_HairVertexPositionsPrev[globalFollowVertexIndex].xyz = g_HairVertexPositions[globalFollowVertexIndex].xyz;
    }

    return;
}

//--------------------------------------------------------------------------------------
//
//  GenerateTransforms
//
//  This was the method of fur in TressFX 3.x.  It may no longer work, and is not
//  currently supported.  We're including it, because it should be possible to 
//  get it working again without a whole lot of trouble, assuming the need arises.
//
//  In this TressFX 3.x method, each strand has a triangle index assigned on export from
//  Maya.  It derives its transform from the post-skinned triangle.  The advantage of this 
//  approach is that it can work independently of skinning method, blendshapes, etc. 
//
//  It relies on the triangle index being correct at runtime, though, which can be tricky.
//  The engine would need to either maintain the original Maya triangles, or a mapping from
//  the original Maya triangles to the runtime set.
//
//  It's also a bit more expensive, given that it must calculate a transform from the triangle.
//
//--------------------------------------------------------------------------------------
#if USE_MESH_BASED_HAIR_TRANSFORM == 1
    [numthreads(THREAD_GROUP_SIZE, 1, 1)]
    void GenerateTransforms(uint GIndex : SV_GroupIndex,
                      uint3 GId : SV_GroupID,
                      uint3 DTid : SV_DispatchThreadID)
    {
        uint local_id, group_id, globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType;
        CalcIndicesInStrandLevelMaster(GIndex, GId.x, globalStrandIndex, numVerticesInTheStrand, globalRootVertexIndex, strandType);

        // get the index for the mesh triangle
        uint triangleIndex = g_HairToMeshMapping[globalStrandIndex].triangleIndex * 3;

        // get the barycentric coordinate for this hair strand
        float a = g_HairToMeshMapping[globalStrandIndex].barycentricCoord[0];
        float b = g_HairToMeshMapping[globalStrandIndex].barycentricCoord[1];
        float c = g_HairToMeshMapping[globalStrandIndex].barycentricCoord[2];

        // get the un-transformed triangle
        float3 vert1 = g_MeshVertices[triangleIndex].xyz;
        float3 vert2 = g_MeshVertices[triangleIndex+1].xyz;
        float3 vert3 = g_MeshVertices[triangleIndex+2].xyz;

        // get the transfomed (skinned) triangle
        float3 skinnedVert1 = g_TransformedVerts[triangleIndex].xyz;
        float3 skinnedVert2 = g_TransformedVerts[triangleIndex+1].xyz;
        float3 skinnedVert3 = g_TransformedVerts[triangleIndex+2].xyz;

        // calculate original hair position for the strand using the barycentric coordinate
        //float3 pos = mul(a, vert1) + mul (b, vert2) + mul(c, vert3);

        // calculate the new hair position for the strand using the barycentric coordinate
        float3 tfmPos = mul(a, skinnedVert1) + mul (b, skinnedVert2) + mul(c, skinnedVert3);
        float3 initialPos = g_InitialHairPositions[globalRootVertexIndex].xyz;

        //-------------------------------------------------
        // Calculate transformation matrix for the hair
        //-------------------------------------------------

        // create a coordinate system from the untransformed triangle
        // Note: this part only needs to be done once. We could pre-calculate it
        // for every hair and save it in a buffer.
        float3 normal;
        float3 tangent = normalize(vert1 - vert3);
        float3 tangent2 = vert2 - vert3;
        normal = normalize(cross(tangent, tangent2));
        float3 binormal = normalize(cross(normal, tangent));

        row_major float4x4  triangleMtx;
        triangleMtx._m00 = tangent.x;   triangleMtx._m01 = tangent.y;   triangleMtx._m02 = tangent.z;   triangleMtx._m03 = 0;
        triangleMtx._m10 = normal.x;    triangleMtx._m11 = normal.y;    triangleMtx._m12 = normal.z;    triangleMtx._m13 = 0;
        triangleMtx._m20 = binormal.x;  triangleMtx._m21 = binormal.y;  triangleMtx._m22 = binormal.z;  triangleMtx._m23 = 0;
        triangleMtx._m30 = 0;           triangleMtx._m31 = 0;           triangleMtx._m32 = 0;           triangleMtx._m33 = 1;

        // create a coordinate system from the transformed triangle
        tangent = normalize(skinnedVert1 - skinnedVert3);
        tangent2 = skinnedVert2 - skinnedVert3;
        normal = normalize(cross(tangent, tangent2));
        binormal = normalize(cross(normal, tangent));

        row_major float4x4  tfmTriangleMtx;
        tfmTriangleMtx._m00 = tangent.x;   tfmTriangleMtx._m01 = tangent.y;   tfmTriangleMtx._m02 = tangent.z;   tfmTriangleMtx._m03 = 0;
        tfmTriangleMtx._m10 = normal.x;    tfmTriangleMtx._m11 = normal.y;    tfmTriangleMtx._m12 = normal.z;    tfmTriangleMtx._m13 = 0;
        tfmTriangleMtx._m20 = binormal.x;  tfmTriangleMtx._m21 = binormal.y;  tfmTriangleMtx._m22 = binormal.z;  tfmTriangleMtx._m23 = 0;
        tfmTriangleMtx._m30 = 0;           tfmTriangleMtx._m31 = 0;           tfmTriangleMtx._m32 = 0;           tfmTriangleMtx._m33 = 1;

        // Find the rotation transformation from the untransformed triangle to the transformed triangle
        // rotation = inverse(triangleMtx) x tfmTriangleMtx = transpose(triangleMtx) x tfmTriangleMtx, since triangelMtx is orthonormal
        row_major float4x4  rotationMtx =  mul(transpose(triangleMtx), tfmTriangleMtx);

        // translation matrix from hair to origin since we want to rotate the hair at it's root
        row_major float4x4  translationMtx;
        translationMtx._m00 = 1;                translationMtx._m01 = 0;                translationMtx._m02 = 0;                translationMtx._m03 = 0;
        translationMtx._m10 = 0;                translationMtx._m11 = 1;                translationMtx._m12 = 0;                translationMtx._m13 = 0;
        translationMtx._m20 = 0;                translationMtx._m21 = 0;                translationMtx._m22 = 1;                translationMtx._m23 = 0;
        translationMtx._m30 = -initialPos.x;    translationMtx._m31 = -initialPos.y;    translationMtx._m32 = -initialPos.z;    translationMtx._m33 = 1;

        // final rotation matrix
        rotationMtx = mul(translationMtx, rotationMtx);

        // translate back to the final position (as determined by the skinned mesh position)
        translationMtx._m30 = tfmPos.x;    translationMtx._m31 = tfmPos.y;    translationMtx._m32 = tfmPos.z;    translationMtx._m33 = 1;

        // combine the rotation and translation
        row_major float4x4  tfmMtx = mul(rotationMtx, translationMtx);

        // apply the global transformation for the model
        //tfmMtx = mul(tfmMtx, g_ModelTransformForHead);

       // calculate the quaternion from the matrix
        float4 quaternion;
        quaternion.w = sqrt(1 + tfmMtx._m00 + tfmMtx._m11 + tfmMtx._m22) / 2;
        quaternion.x = (tfmMtx._m21 - tfmMtx._m12)/( 4 * quaternion.w);
        quaternion.y = (tfmMtx._m02 - tfmMtx._m20)/( 4 * quaternion.w);
        quaternion.z = (tfmMtx._m10 - tfmMtx._m01)/( 4 * quaternion.w);

        g_Transforms[globalStrandIndex].tfm = tfmMtx;
        g_Transforms[globalStrandIndex].quat = quaternion;
        return;
    }
#endif
// EndHLSL
