//---------------------------------------------------------------------------------------
// Shader code related to Marching Cubes
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


//StartHLSL TressFXMarchingCubesCS

#ifndef THREAD_GROUP_SIZE
#define THREAD_GROUP_SIZE 64
#endif

struct StandardVertex
{
    float4 position;
    float4 normal;
};

[[vk::binding(5, 0)]] cbuffer ConstBuffer_MC : register(b5, space0)
{
    // rendering
    float4x4 mMW;
    float4x4 mWVP;
    float4 cColor;
    float4 vLightDir;

    //simulation
    float4 g_Origin;
    float g_CellSize;
    int g_NumCellsX;
    int g_NumCellsY;
    int g_NumCellsZ;

    int g_MaxMarchingCubesVertices;
    float g_MarchingCubesIsolevel;
}

//Actually contains floats; make sure to use asfloat() when accessing. uint is used to allow atomics.
[[vk::binding(0, 0)]] RWStructuredBuffer<uint> g_MarchingCubesSignedDistanceField : register(u0, space0);

// UAV for MC
[[vk::binding(1, 0)]] RWStructuredBuffer<StandardVertex> g_MarchingCubesTriangleVertices : register(u1, space0);
[[vk::binding(2, 0)]] RWStructuredBuffer<int> g_NumMarchingCubesVertices : register(u2, space0);

// SRV for MC
[[vk::binding(3, 0)]] StructuredBuffer<int> g_MarchingCubesEdgeTable : register(t3, space0);
[[vk::binding(4, 0)]] StructuredBuffer<int> g_MarchingCubesTriangleTable : register(t4, space0);

int3 GetSdfCellPositionFromIndex(uint sdfCellIndex)
{
    uint cellsPerLine = (uint)g_NumCellsX;
    uint cellsPerPlane = (uint)(g_NumCellsX * g_NumCellsY);

    uint numPlanesZ = sdfCellIndex / cellsPerPlane;
    uint remainder = sdfCellIndex % cellsPerPlane;

    uint numLinesY = remainder / cellsPerLine;
    uint numCellsX = remainder % cellsPerLine;

    return int3((int)numCellsX, (int)numLinesY, (int)numPlanesZ);
}

float3 GetSdfCellPosition(int3 gridPosition)
{
    float3 cellCenter = float3(gridPosition.x, gridPosition.y, gridPosition.z) * g_CellSize;
    cellCenter += g_Origin.xyz;

    return cellCenter;
}

int GetSdfCellIndex(int3 gridPosition)
{
    int cellsPerLine = g_NumCellsX;
    int cellsPerPlane = g_NumCellsX * g_NumCellsY;

    return cellsPerPlane*gridPosition.z + cellsPerLine*gridPosition.y + gridPosition.x;
}

//Relative vertex positions:
//
//		   4-------5
//		  /|      /|
// 		 / |     / |
//		7-------6  |
//		|  0----|--1
//		| /     | / 
//		|/      |/  
//		3-------2   
struct MarchingCube
{
    float4 m_vertices[8];
    float m_scalars[8];
};

float3 VertexLerp(float isolevel, float scalar1, float scalar2, float4 p1, float4 p2)
{
    //Given 2 points p1, p2 with associated values scalar1, scalar2,
    //we want the linearly interpolated position of a point in between p1 and p2 with a value equal to isolevel.
    //Isolevel should be between scalar1 and scalar2.
    //
    //p = p1 + (p2 - p1) * (isolevel - scalar1) / (scalar2 - scalar1)

    float interp = (isolevel - scalar1) / (scalar2 - scalar1);
    return (p1 + (p2 - p1) * interp).xyz;
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void InitializeMCVertices(uint GIndex : SV_GroupIndex,
                            uint3 GId : SV_GroupID,
                            uint3 DTid : SV_DispatchThreadID)
{
    int index = GId.x * THREAD_GROUP_SIZE + GIndex;
    
    if (index < g_MaxMarchingCubesVertices)
    {
        StandardVertex v;
        v.position = float4(0, 0, 0, 0);
        v.normal = float4(0, 0, 0, 0);

        g_MarchingCubesTriangleVertices[index] = v;
    }

    if (index == 0)
        g_NumMarchingCubesVertices[0] = 0;
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void RunMarchingCubesOnSdf(uint GIndex : SV_GroupIndex,
                    uint3 GId : SV_GroupID,
                    uint3 DTid : SV_DispatchThreadID)
{
    int numSdfCells = g_NumCellsX * g_NumCellsY * g_NumCellsZ;

    int sdfCellIndex = GId.x * THREAD_GROUP_SIZE + GIndex;
    
    if(sdfCellIndex >= numSdfCells) 
        return;

    int3 gridPosition = GetSdfCellPositionFromIndex( (uint)sdfCellIndex );
    
    if( !(0 <= gridPosition.x && gridPosition.x < g_NumCellsX - 1)
     || !(0 <= gridPosition.y && gridPosition.y < g_NumCellsY - 1)
     || !(0 <= gridPosition.z && gridPosition.z < g_NumCellsZ - 1) ) return;
    
    int3 offset[8];
    offset[0] = int3(0, 0, 0);
    offset[1] = int3(1, 0, 0);
    offset[2] = int3(1, 0, 1);
    offset[3] = int3(0, 0, 1);
    offset[4] = int3(0, 1, 0);
    offset[5] = int3(1, 1, 0);
    offset[6] = int3(1, 1, 1);
    offset[7] = int3(0, 1, 1);
    
    int3 cellCoordinates[8];

    for(int i = 0; i < 8; ++i) 
        cellCoordinates[i] = gridPosition + offset[i];

    MarchingCube C;

    for(int j = 0; j < 8; ++j) 
        C.m_vertices[j].xyz = GetSdfCellPosition(cellCoordinates[j]);

    for(int k = 0; k < 8; ++k) 
    {
        int sdfIndex = GetSdfCellIndex(cellCoordinates[k]);
        float dist = asfloat(g_MarchingCubesSignedDistanceField[sdfIndex]);
        
        C.m_scalars[k] = dist;
    }


    //appendTrianglesMarchingCubes(C);
    {
        //Compare floats at vertices 0-7 with g_MarchingCubesIsolevel 
        //to determine which of the 256 possible configurations is present
        uint cubeIndex = 0;
        if( C.m_scalars[0] < g_MarchingCubesIsolevel ) cubeIndex |= 1;
        if( C.m_scalars[1] < g_MarchingCubesIsolevel ) cubeIndex |= 2;
        if( C.m_scalars[2] < g_MarchingCubesIsolevel ) cubeIndex |= 4;
        if( C.m_scalars[3] < g_MarchingCubesIsolevel ) cubeIndex |= 8;
        if( C.m_scalars[4] < g_MarchingCubesIsolevel ) cubeIndex |= 16;
        if( C.m_scalars[5] < g_MarchingCubesIsolevel ) cubeIndex |= 32;
        if( C.m_scalars[6] < g_MarchingCubesIsolevel ) cubeIndex |= 64;
        if( C.m_scalars[7] < g_MarchingCubesIsolevel ) cubeIndex |= 128;
        
        if( !g_MarchingCubesEdgeTable[cubeIndex] ) 
            return;		//All vertices are above or below isolevel
        
        //Generate vertices for edges 0-11; interpolate between the edge's vertices
        float3 vertices[12];
        if( g_MarchingCubesEdgeTable[cubeIndex] & 1    ) vertices[0]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[0], C.m_scalars[1], C.m_vertices[0], C.m_vertices[1]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 2    ) vertices[1]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[1], C.m_scalars[2], C.m_vertices[1], C.m_vertices[2]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 4    ) vertices[2]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[2], C.m_scalars[3], C.m_vertices[2], C.m_vertices[3]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 8    ) vertices[3]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[3], C.m_scalars[0], C.m_vertices[3], C.m_vertices[0]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 16   ) vertices[4]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[4], C.m_scalars[5], C.m_vertices[4], C.m_vertices[5]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 32   ) vertices[5]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[5], C.m_scalars[6], C.m_vertices[5], C.m_vertices[6]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 64   ) vertices[6]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[6], C.m_scalars[7], C.m_vertices[6], C.m_vertices[7]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 128  ) vertices[7]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[7], C.m_scalars[4], C.m_vertices[7], C.m_vertices[4]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 256  ) vertices[8]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[0], C.m_scalars[4], C.m_vertices[0], C.m_vertices[4]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 512  ) vertices[9]  = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[1], C.m_scalars[5], C.m_vertices[1], C.m_vertices[5]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 1024 ) vertices[10] = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[2], C.m_scalars[6], C.m_vertices[2], C.m_vertices[6]);
        if( g_MarchingCubesEdgeTable[cubeIndex] & 2048 ) vertices[11] = VertexLerp(g_MarchingCubesIsolevel, C.m_scalars[3], C.m_scalars[7], C.m_vertices[3], C.m_vertices[7]);
        
        //Store triangles
        uint numVerticesFromThisCube = 0;

        for(int i = 0; i < 16 && g_MarchingCubesTriangleTable[cubeIndex * 16 + i] != -1; ++i) 
            ++numVerticesFromThisCube;
        
        int vertexIndexOffset;
        InterlockedAdd(g_NumMarchingCubesVertices[0], numVerticesFromThisCube, vertexIndexOffset);

        if(vertexIndexOffset + numVerticesFromThisCube < g_MaxMarchingCubesVertices)
        {
            uint numTriangles = numVerticesFromThisCube / 3;
            
            for(uint tri = 0; tri < numTriangles; ++tri)
            {
                uint offset0 = tri * 3 + 0;
                uint offset1 = tri * 3 + 1;
                uint offset2 = tri * 3 + 2;
                
                StandardVertex v0;
                StandardVertex v1;
                StandardVertex v2;

                v0.position = float4(vertices[ g_MarchingCubesTriangleTable[cubeIndex * 16 + offset0] ], 0);
                v1.position = float4(vertices[ g_MarchingCubesTriangleTable[cubeIndex * 16 + offset1] ], 0);
                v2.position = float4(vertices[ g_MarchingCubesTriangleTable[cubeIndex * 16 + offset2] ], 0);
                
                float3 normal = normalize( cross(v1.position.xyz - v0.position.xyz, v2.position.xyz - v0.position.xyz) );
                
                v0.normal = float4(normal.xyz, 0);
                v1.normal = float4(normal.xyz, 0);
                v2.normal = float4(normal.xyz, 0);
                
                uint index0 = vertexIndexOffset + offset0;
                uint index1 = vertexIndexOffset + offset1;
                uint index2 = vertexIndexOffset + offset2;
                
                g_MarchingCubesTriangleVertices[index0] = v0;
                g_MarchingCubesTriangleVertices[index1] = v1;
                g_MarchingCubesTriangleVertices[index2] = v2;
            }
        }
    }

}

//EndHLSL

// rendering stuff ( from oSDF.sufx )

struct VsOutput
{
    float4 vPositionSS : SV_POSITION;
    float3 vPos_ : vPos_;
    float3 vNormal_ : vNormal_;
};

VsOutput MarchingCubesVS(uint vertexId : SV_VertexID)
{
    VsOutput output;

    output.vNormal_ = mul(g_MarchingCubesTriangleVertices[vertexId].normal.xyz, (float3x3)mMW);
    float4 inputVertexPos = float4(g_MarchingCubesTriangleVertices[vertexId].position.xyz, 1.0);

    output.vPositionSS = mul(mWVP, float4(inputVertexPos.xyz, 1.0f));
    output.vPos_ = inputVertexPos.xyz;
    return output;
}

struct PSOutput
{
    float4 vColor : SV_TARGET;
};

PSOutput MarchingCubesPS(VsOutput input)
{
    PSOutput output = (PSOutput)0;
    float4 vDiffuse = float4(cColor.xyz, 1.0);
    float fAmbient = 0.2;
    float3 vLightDir1 = float3(-1., 0., -1.);

    //float fLighting = saturate(dot(normalize(vLightDir), input.vNormal_));
    float fLighting = saturate(dot(normalize(vLightDir), input.vNormal_)) + 0.7 * saturate(dot(normalize(vLightDir1), input.vNormal_));
    fLighting = max(fLighting, fAmbient);

    output.vColor = vDiffuse * fLighting;
    return output;
}
