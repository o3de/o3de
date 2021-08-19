
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

#ifndef THREAD_GROUP_SIZE
#define THREAD_GROUP_SIZE 64
#endif

#define INITIAL_DISTANCE 1e10f
#define MARGIN g_CellSize
#define GRID_MARGIN int3(1, 1, 1)

struct StandardVertex
{
    float4 position;
    float4 normal;
};

[[vk::binding(3, 0)]] cbuffer ConstBuffer_SDF : register(b3, space0)
{
    //row_major float4x4 g_ModelTransformForHead;
    //row_major float4x4 g_ModelInvTransformForHead;

    float4 g_Origin;
    float g_CellSize;
    int g_NumCellsX;
    int g_NumCellsY;
    int g_NumCellsZ;
    int g_MaxMarchingCubesVertices;
    float g_MarchingCubesIsolevel;
    float g_CollisionMargin;
    int g_NumHairVerticesPerStrand;
    int g_NumTotalHairVertices;
    float pad1;
    float pad2;
    float pad3;
}

// bindsets: 0 -> GenerateSDFLayout
//           1 -> ApplySDFLayout
//           2 -> 

//Actually contains floats; make sure to use asfloat() when accessing. uint is used to allow atomics.
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> g_SignedDistanceField : register(u1, space0);
[[vk::binding(0, 1)]] RWStructuredBuffer<float4> g_HairVertices : register(u0, space1);
[[vk::binding(1, 1)]] RWStructuredBuffer<float4> g_PrevHairVertices : register(u1, space1);

//Triangle input to SDF builder
// [[vk::binding(0, 0)]] StructuredBuffer<StandardVertex> g_TrimeshVertices;
[[vk::binding(0, 0)]] StructuredBuffer<uint> g_TrimeshVertexIndices : register(t0, space0);
[[vk::binding(2, 0)]] RWStructuredBuffer<StandardVertex> collMeshVertexPositions : register(u2, space0);

//When building the SDF we want to find the lowest distance at each SDF cell. In order to allow multiple threads to write to the same
//cells, it is necessary to use atomics. However, there is no support for atomics with 32-bit floats so we convert the float into unsigned int
//and use atomic_min() / InterlockedMin() as a workaround.
//
//When used with atomic_min, both FloatFlip2() and FloatFlip3() will store the float with the lowest magnitude.
//The difference is that FloatFlip2() will preper negative values ( InterlockedMin( FloatFlip2(1.0), FloatFlip2(-1.0) ) == -1.0 ),
//while FloatFlip3() prefers positive values (  InterlockedMin( FloatFlip3(1.0), FloatFlip3(-1.0) ) == 1.0 ).
//Using FloatFlip3() seems to result in a SDF with higher quality compared to FloatFlip2().
uint FloatFlip2(float fl)
{
    uint f = asuint(fl);
    return (f << 1) | (f >> 31 ^ 0x00000001);		//Rotate sign bit to least significant and Flip sign bit so that (0 == negative)
}
uint IFloatFlip2(uint f2)
{
    return (f2 >> 1) | (f2 << 31 ^ 0x80000000);
}
uint FloatFlip3(float fl)
{
    uint f = asuint(fl);
    return (f << 1) | (f >> 31);		//Rotate sign bit to least significant
}
uint IFloatFlip3(uint f2)
{
    return (f2 >> 1) | (f2 << 31);
}

// Get SDF cell index coordinates (x, y and z) from a point position in world space
int3 GetSdfCoordinates(float3 positionInWorld)
{
    float3 sdfPosition = (positionInWorld - g_Origin.xyz) / g_CellSize;
    
    int3 result;
    result.x = (int)sdfPosition.x;
    result.y = (int)sdfPosition.y;
    result.z = (int)sdfPosition.z;
    
    return result;
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

float DistancePointToEdge2(float3 p, float3 x0, float3 x1, out float3 pointOnEdge)
{
    float3 x10 = x1 - x0;
    
    float t = dot(x1 - p, x10) / dot(x10, x10);
    t = max( 0.0f, min(t, 1.0f) );
    
    pointOnEdge = (t*x0 + (1.0f - t)*x1);
    
    float3 a = p - pointOnEdge;
    float d = length(a);
    float3 n = a / (d + 1e-30f);

    return d;
}

float DistancePointToEdge(float3 p, float3 x0, float3 x1, out float3 n)
{
    float3 x10 = x1 - x0;

    float t = dot(x1 - p, x10) / dot(x10, x10);
    t = max(0.0f, min(t, 1.0f));

    float3 a = p - (t*x0 + (1.0f - t)*x1);
    float d = length(a);
    n = a / (d + 1e-30f);

    return d;
}

// Check if p is in the positive or negative side of triangle (x0, x1, x2)
// Positive side is where the normal vector of triangle ( (x1-x0) x (x2-x0) ) is pointing to.
float SignedDistancePointToTriangle(float3 p, float3 x0, float3 x1, float3 x2)
{
    float d = 0;
    float3 x02 = x0 - x2;
    float l0 = length(x02) + 1e-30f;
    x02 = x02 / l0;
    float3 x12 = x1 - x2;
    float l1 = dot(x12, x02);
    x12 = x12 - l1*x02;
    float l2 = length(x12) + 1e-30f;
    x12 = x12 / l2;
    float3 px2 = p - x2;

    float b = dot(x12, px2) / l2;
    float a = (dot(x02, px2) - l1*b) / l0;
    float c = 1 - a - b;

    // normal vector of triangle. Don't need to normalize this yet.
    float3 nTri = cross((x1 - x0), (x2 - x0));
    float3 n;

    float tol = 1e-8f;

    if (a >= -tol && b >= -tol && c >= -tol)
    {
        n = p - (a*x0 + b*x1 + c*x2);
        d = length(n);

        float3 n1 = n / d;
        float3 n2 = nTri / (length(nTri) + 1e-30f);		// if d == 0

        n = (d > 0) ? n1 : n2;
    }
    else
    {
        float3 n_12;
        float3 n_02;
        d = DistancePointToEdge(p, x0, x1, n);

        float d12 = DistancePointToEdge(p, x1, x2, n_12);
        float d02 = DistancePointToEdge(p, x0, x2, n_02);

        d = min(d, d12);
        d = min(d, d02);

        n = (d == d12) ? n_12 : n;
        n = (d == d02) ? n_02 : n;
    }

    d = (dot(p - x0, nTri) < 0.f) ? -d : d;

    return d;
}


float SignedDistancePointToTriangle2(float3 p, float3 x0, float3 x1, float3 x2,
                                    float3 vertexNormal0, float3 vertexNormal1, float3 vertexNormal2,
                                    float3 edgeNormal01, float3 edgeNormal12, float3 edgeNormal20)
{
    float d = 0;
    float3 x02 = x0 - x2;
    float l0 = length(x02) + 1e-30f;
    x02 = x02 / l0;
    float3 x12 = x1 - x2;
    float l1 = dot(x12, x02);
    x12 = x12 - l1*x02;
    float l2 = length(x12) + 1e-30f;
    x12 = x12 / l2;
    float3 px2 = p - x2;

    float b = dot(x12, px2) / l2;
    float a = ( dot(x02, px2) - l1*b ) / l0;
    float c = 1 - a - b;

    // normal vector of triangle. Don't need to normalize this yet.
    float3 nTri = cross( (x1 - x0), (x2 - x0) );
    float3 n;

    float tol = 1e-8f;

    //Check if triangle is in the Voronoi face region
    if ( a >= -tol && b >= -tol && c >= -tol )
    {
        n = p - (a*x0 + b*x1 + c*x2);
        d = length(n);

        float3 n1 = n / d;
        float3 n2 = nTri / ( length(nTri) + 1e-30f );		// if d == 0
        
        n = (d > 0) ? n1 : n2;
        d = ( dot(p - x0, nTri) < 0.f ) ? -d : d;
    }
    else
    {
        //Otherwise find the nearest edge/vertex
        float3 normals[6];
        normals[0] = vertexNormal0;
        normals[1] = vertexNormal1;
        normals[2] = vertexNormal2;
        normals[3] = edgeNormal01;
        normals[4] = edgeNormal12;
        normals[5] = edgeNormal20;
        
        float3 nearestPoint[6];
        nearestPoint[0] = x0;
        nearestPoint[1] = x1;
        nearestPoint[2] = x2;
        
        float distances[6];
        distances[0] = length(p - x0);
        distances[1] = length(p - x1);
        distances[2] = length(p - x2);
        distances[3] = DistancePointToEdge2(p, x0, x1, nearestPoint[3]);
        distances[4] = DistancePointToEdge2(p, x1, x2, nearestPoint[4]);
        distances[5] = DistancePointToEdge2(p, x0, x2, nearestPoint[5]);
        
        float minDistance = distances[0];
        for(int i = 1; i < 6; ++i) minDistance = min(minDistance, distances[i]);
        
        float3 pointOnPlane = nearestPoint[0];
        float3 normal = normals[0];
        
        for(int j = 1; j < 6; ++j)
        {
            int isMin = (minDistance == distances[j]);
            
            pointOnPlane = (isMin) ? nearestPoint[j] : pointOnPlane;
            normal = (isMin) ? normals[j] : normal;
        }
        
        d = ( dot(p - pointOnPlane, normal) < 0.f ) ? -minDistance : minDistance;
    }


    return d;
}

// One thread for each cell. 
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void InitializeSignedDistanceField(uint GIndex : SV_GroupIndex,
                    uint3 GId : SV_GroupID,
                    uint3 DTid : SV_DispatchThreadID)
{
    int numSdfCells = g_NumCellsX * g_NumCellsY * g_NumCellsZ;

    int sdfCellIndex = GId.x * THREAD_GROUP_SIZE + GIndex;
    if(sdfCellIndex >= numSdfCells) return;
    
    g_SignedDistanceField[sdfCellIndex] = FloatFlip3(INITIAL_DISTANCE);
}

int3 GetLocalCellPositionFromIndex(uint localCellIndex, int3 cellsPerDimensionLocal)
{
    uint cellsPerLine = (uint)cellsPerDimensionLocal.x;
    uint cellsPerPlane = (uint)(cellsPerDimensionLocal.x * cellsPerDimensionLocal.y);

    uint numPlanesZ = localCellIndex / cellsPerPlane;
    uint remainder = localCellIndex % cellsPerPlane;
    
    uint numLinesY = remainder / cellsPerLine;
    uint numCellsX = remainder % cellsPerLine;
    
    return int3((int)numCellsX, (int)numLinesY, (int)numPlanesZ);
}

// One thread per each triangle
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void ConstructSignedDistanceField(uint GIndex : SV_GroupIndex,
    uint3 GId : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID)
{
    int triangleIndex = GId.x * THREAD_GROUP_SIZE + GIndex;

    uint numTriangleIndices, stride;
    g_TrimeshVertexIndices.GetDimensions(numTriangleIndices, stride);
    uint numTriangles = numTriangleIndices / 3;
        
    if (triangleIndex >= (int)numTriangles) 
        return;

    uint index0 = g_TrimeshVertexIndices[triangleIndex * 3 + 0];
    uint index1 = g_TrimeshVertexIndices[triangleIndex * 3 + 1];
    uint index2 = g_TrimeshVertexIndices[triangleIndex * 3 + 2];

    float3 tri0 = collMeshVertexPositions[index0].position;
    float3 tri1 = collMeshVertexPositions[index1].position;
    float3 tri2 = collMeshVertexPositions[index2].position;
        
    float3 aabbMin = min(tri0, min(tri1, tri2)) - float3(MARGIN, MARGIN, MARGIN);
    float3 aabbMax = max(tri0, max(tri1, tri2)) + float3(MARGIN, MARGIN, MARGIN);

    int3 gridMin = GetSdfCoordinates(aabbMin) - GRID_MARGIN;
    int3 gridMax = GetSdfCoordinates(aabbMax) + GRID_MARGIN;

    gridMin.x = max(0, min(gridMin.x, g_NumCellsX - 1));
    gridMin.y = max(0, min(gridMin.y, g_NumCellsY - 1));
    gridMin.z = max(0, min(gridMin.z, g_NumCellsZ - 1));

    gridMax.x = max(0, min(gridMax.x, g_NumCellsX - 1));
    gridMax.y = max(0, min(gridMax.y, g_NumCellsY - 1));
    gridMax.z = max(0, min(gridMax.z, g_NumCellsZ - 1));

    

    //for (int z = gridMin.z; z <= gridMax.z; ++z)
    //	for (int y = gridMin.y; y <= gridMax.y; ++y)
    //		for (int x = gridMin.x; x <= gridMax.x; ++x)
    //		{
    //			int3 gridCellCoordinate = int3(x, y, z);

    //			int gridCellIndex = GetSdfCellIndex(gridCellCoordinate);
    //			float3 cellPosition = GetSdfCellPosition(gridCellCoordinate);

    //			float distance = SignedDistancePointToTriangle(cellPosition, tri0, tri1, tri2);
    //			//distance -= MARGIN;

    //			uint distanceAsUint = FloatFlip3(distance);
    //			InterlockedMin(g_SignedDistanceField[gridCellIndex], distanceAsUint);
    //		}


    for (int z = gridMin.z; z <= gridMax.z; ++z)
        for (int y = gridMin.y; y <= gridMax.y; ++y)
            for (int x = gridMin.x; x <= gridMax.x; ++x)
            {
                int3 gridCellCoordinate = int3(x, y, z);
                int gridCellIndex = GetSdfCellIndex(gridCellCoordinate);
                float3 cellPosition = GetSdfCellPosition(gridCellCoordinate);

                float distance = SignedDistancePointToTriangle(cellPosition, tri0, tri1, tri2);
                //distance -= MARGIN;
                uint distanceAsUint = FloatFlip3(distance);
                InterlockedMin(g_SignedDistanceField[gridCellIndex], distanceAsUint);
            }
}

// One thread per each cell. 
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void FinalizeSignedDistanceField(uint GIndex : SV_GroupIndex,
    uint3 GId : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID)
{
    int numSdfCells = g_NumCellsX * g_NumCellsY * g_NumCellsZ;

    int sdfCellIndex = GId.x * THREAD_GROUP_SIZE + GIndex;
    if (sdfCellIndex >= numSdfCells) return;

    uint distance = g_SignedDistanceField[sdfCellIndex];
    g_SignedDistanceField[sdfCellIndex] = IFloatFlip3(distance);
}

float LinearInterpolate(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

//    bilinear interpolation
//
//         p    :  1-p
//     c------------+----d
//     |            |    |
//     |            |    |
//     |       1-q  |    |
//     |            |    |
//     |            x    |
//     |            |    |
//     |         q  |    |
//     a------------+----b
//         p    :  1-p
//
//    x = BilinearInterpolate(a, b, c, d, p, q)
//      = LinearInterpolate(LinearInterpolate(a, b, p), LinearInterpolate(c, d, p), q)
float BilinearInterpolate(float a, float b, float c, float d, float p, float q)
{
    return LinearInterpolate( LinearInterpolate(a, b, p), LinearInterpolate(c, d, p), q );
}

//    trilinear interpolation
//
//                      c        p            1-p    d
//                       ------------------+----------
//                      /|                 |        /|
//                     /                   |       / |
//                    /  |                 |1-q   /  |
//                   /                     |     /   |
//                  /    |                 |    /    |
//               g ------------------+---------- h   |
//                 |     |           |     |   |     |
//                 |                 |     +   |     |
//                 |     |           |   r/|   |     |
//                 |                 |   / |q  |     |
//                 |     |           |  x  |   |     |
//                 |   a - - - - - - | / - + - |- - -| b
//                 |    /            |/1-r     |     /
//                 |                 +         |    /
//                 |  /              |         |   /
//                 |                 |         |  /
//                 |/                |         | /
//                 ------------------+----------
//              e                            f
//
//		x = TrilinearInterpolate(a, b, c, d, e, f, g, h, p, q, r)
//		  = LinearInterpolate(BilinearInterpolate(a, b, c, d, p, q), BilinearInterpolate(e, f, g, h, p, q), r)
float TrilinearInterpolate(float a, float b, float c, float d, float e, float f, float g, float h, float p, float q, float r)
{
    return LinearInterpolate(BilinearInterpolate(a, b, c, d, p, q), BilinearInterpolate(e, f, g, h, p, q), r);
}

// Get signed distance at the position in world space
float GetSignedDistance(float3 positionInWorld)
{
    int3 gridCoords = GetSdfCoordinates(positionInWorld);
    
    if( !(0 <= gridCoords.x && gridCoords.x < g_NumCellsX - 2)
     || !(0 <= gridCoords.y && gridCoords.y < g_NumCellsY - 2)
     || !(0 <= gridCoords.z && gridCoords.z < g_NumCellsZ - 2) ) 
        return INITIAL_DISTANCE;
    
    int sdfIndices[8];
    {
        int index = GetSdfCellIndex(gridCoords);
        for(int i = 0; i < 8; ++i) sdfIndices[i] = index;
        
        int x = 1;
        int y = g_NumCellsX;
        int z = g_NumCellsY * g_NumCellsX;
        
        sdfIndices[1] += x;
        sdfIndices[2] += y;
        sdfIndices[3] += y + x;
        
        sdfIndices[4] += z;
        sdfIndices[5] += z + x;
        sdfIndices[6] += z + y;
        sdfIndices[7] += z + y + x;
    }
    
    float distances[8];

    for(int j = 0; j < 8; ++j)
    {
        int sdfCellIndex = sdfIndices[j];
        float dist = asfloat(g_SignedDistanceField[sdfCellIndex]);

        if(dist == INITIAL_DISTANCE) 
            return INITIAL_DISTANCE;
        
        distances[j] = dist;
    }
    
    float distance_000 = distances[0];	// X,  Y,  Z
    float distance_100 = distances[1];	//+X,  Y,  Z
    float distance_010 = distances[2];	// X, +Y,  Z
    float distance_110 = distances[3];	//+X, +Y,  Z
    float distance_001 = distances[4];	// X,  Y, +Z
    float distance_101 = distances[5];	//+X,  Y, +Z
    float distance_011 = distances[6];	// X, +Y, +Z
    float distance_111 = distances[7];	//+X, +Y, +Z
    
    float3 cellPosition = GetSdfCellPosition(gridCoords);
    float3 interp = (positionInWorld - cellPosition) / g_CellSize;
    return TrilinearInterpolate(distance_000, distance_100, distance_010, distance_110,
                                distance_001, distance_101, distance_011, distance_111,
                                interp.x, interp.y, interp.z);
}

//SDF-Hair collision using forward differences only
// One thread per one hair vertex
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CollideHairVerticesWithSdf_forward(uint GIndex : SV_GroupIndex,
                    uint3 GId : SV_GroupID,
                    uint3 DTid : SV_DispatchThreadID)
{
    int hairVertexGlobalIndex = GId.x * THREAD_GROUP_SIZE + GIndex;
    
    if(hairVertexGlobalIndex >= g_NumTotalHairVertices)
        return;
    
    int hairVertexLocalIndex = hairVertexGlobalIndex % g_NumHairVerticesPerStrand;

    // We don't run collision check on the first two vertices in the strand. They are fixed on the skin mesh. 
    if (hairVertexLocalIndex == 0 || hairVertexLocalIndex == 1)
        return;

    float4 hairVertex = g_HairVertices[hairVertexGlobalIndex];
    float4 vertexInSdfLocalSpace = hairVertex;
    
    float distance = GetSignedDistance(vertexInSdfLocalSpace.xyz);
    
    // early exit if the distance is larger than collision margin
    if(distance > g_CollisionMargin) 
        return;
    
    // small displacement. 
    float h = 0.1f * g_CellSize;

    float3 sdfGradient;
    {
        //Compute gradient using forward difference
        float3 offset[3];
        offset[0] = float3(1, 0, 0);
        offset[1] = float3(0, 1, 0);
        offset[2] = float3(0, 0, 1);
        
        float3 neighborCellPositions[3];

        for(int i = 0; i < 3; ++i) 
            neighborCellPositions[i] = vertexInSdfLocalSpace.xyz + offset[i] * h;
        
        //Use trilinear interpolation to get distances
        float neighborCellDistances[3];

        for(int j = 0; j < 3; ++j) 
            neighborCellDistances[j] = GetSignedDistance(neighborCellPositions[j]);
        
        float3 forwardDistances;
        forwardDistances.x = neighborCellDistances[0];
        forwardDistances.y = neighborCellDistances[1];
        forwardDistances.z = neighborCellDistances[2];
        
        sdfGradient = ( forwardDistances - float3(distance, distance, distance) ) / h;
    }
    
    //Project hair vertex out of SDF
    float3 normal = normalize(sdfGradient);
    
    if(distance < g_CollisionMargin)
    {
        float3 projectedVertex = hairVertex.xyz + normal * (g_CollisionMargin - distance);
        g_HairVertices[hairVertexGlobalIndex].xyz = projectedVertex;
        g_PrevHairVertices[hairVertexGlobalIndex].xyz = projectedVertex;
    }
}

//Faster SDF-Hair collision mixing forward and backward differences
// One thread per one hair vertex
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CollideHairVerticesWithSdf(uint GIndex : SV_GroupIndex,
                    uint3 GId : SV_GroupID,
                    uint3 DTid : SV_DispatchThreadID)
{
    //When using forward differences to compute the SDF gradient,
    //we need to use trilinear interpolation to look up 4 points in the SDF.
    //In the worst case, this involves reading the distances in 4 different cubes (reading 32 floats from the SDF).
    //In the ideal case, only 1 cube needs to be read (reading 8 floats from the SDF).
    //The number of distance lookups varies depending on whether the 4 points cross cell boundaries.
    //
    //If we assume that (h < g_CellSize), where h is the distance between the points used for finite difference,
    //we can mix forwards and backwards differences to ensure that the points never cross cell boundaries (always read 8 floats).
    //By default we use forward differences, and switch to backwards differences for each dimension that crosses a cell boundary.
    //
    //This is much faster than using forward differences only, but it could also be less stable.
    //In particular, it has the effect of making the gradient less accurate. The amount of inaccuracy is 
    //proportional to the size of h, so it is recommended keep h as low as possible.
    
    int hairVertexIndex = GId.x * THREAD_GROUP_SIZE + GIndex;
    
    uint numHairVertices, stride;
    g_HairVertices.GetDimensions(numHairVertices, stride);

    if(hairVertexIndex >= (int)numHairVertices) 
        return;
    
    float4 hairVertex = g_HairVertices[hairVertexIndex];
    float4 vertexInSdfLocalSpace = hairVertex;

    int3 gridCoords = GetSdfCoordinates(vertexInSdfLocalSpace.xyz);
    
    if( !(0 <= gridCoords.x && gridCoords.x < g_NumCellsX - 2)
     || !(0 <= gridCoords.y && gridCoords.y < g_NumCellsY - 2)
     || !(0 <= gridCoords.z && gridCoords.z < g_NumCellsZ - 2) ) 
        return;
    
    //
    float distances[8];
    {
        int sdfIndices[8];
        {
            int index = GetSdfCellIndex(gridCoords);
            for(int i = 0; i < 8; ++i) sdfIndices[i] = index;
            
            int x = 1;
            int y = g_NumCellsX;
            int z = g_NumCellsY * g_NumCellsX;
            
            sdfIndices[1] += x;
            sdfIndices[2] += y;
            sdfIndices[3] += y + x;
            
            sdfIndices[4] += z;
            sdfIndices[5] += z + x;
            sdfIndices[6] += z + y;
            sdfIndices[7] += z + y + x;
        }
        
        for(int j = 0; j < 8; ++j)
        {
            int sdfCellIndex = sdfIndices[j];
            float dist = asfloat(g_SignedDistanceField[sdfCellIndex]);
            if(dist == INITIAL_DISTANCE) return;
            
            distances[j] = dist;
        }
    }
    
    float distance_000 = distances[0];	// X,  Y,  Z
    float distance_100 = distances[1];	//+X,  Y,  Z
    float distance_010 = distances[2];	// X, +Y,  Z
    float distance_110 = distances[3];	//+X, +Y,  Z
    float distance_001 = distances[4];	// X,  Y, +Z
    float distance_101 = distances[5];	//+X,  Y, +Z
    float distance_011 = distances[6];	// X, +Y, +Z
    float distance_111 = distances[7];	//+X, +Y, +Z
    
    float3 cellPosition = GetSdfCellPosition(gridCoords);
    float3 interp = (vertexInSdfLocalSpace.xyz - cellPosition) / g_CellSize;
    float distanceAtVertex = TrilinearInterpolate(distance_000, distance_100, distance_010, distance_110,
                                                  distance_001, distance_101, distance_011, distance_111,
                                                  interp.x, interp.y, interp.z);
    
    //Compute gradient with finite differences
    float3 sdfGradient;
    {
        float h = 0.1f * g_CellSize;
        float3 direction;
        direction.x = (interp.x + h < 1.0f) ? 1.0f : -1.0f;
        direction.y = (interp.y + h < 1.0f) ? 1.0f : -1.0f;
        direction.z = (interp.z + h < 1.0f) ? 1.0f : -1.0f;
        
        float3 neighborDistances;
        neighborDistances.x = TrilinearInterpolate(distance_000, distance_100, distance_010, distance_110,
                                                    distance_001, distance_101, distance_011, distance_111,
                                                    interp.x + h * direction.x, interp.y, interp.z);
        neighborDistances.y = TrilinearInterpolate(distance_000, distance_100, distance_010, distance_110,
                                                    distance_001, distance_101, distance_011, distance_111,
                                                    interp.x, interp.y + h * direction.y, interp.z);
        neighborDistances.z = TrilinearInterpolate(distance_000, distance_100, distance_010, distance_110,
                                                    distance_001, distance_101, distance_011, distance_111,
                                                    interp.x, interp.y, interp.z + h * direction.z);
        
        sdfGradient = ( direction * ( neighborDistances - float3(distanceAtVertex, distanceAtVertex, distanceAtVertex) ) ) / h;
    }
    
    //Project hair vertex out of SDF
    float3 normal = normalize(sdfGradient);
    
    if(distanceAtVertex < g_CollisionMargin)
    {
        float3 projectedVertex = hairVertex.xyz + normal * (g_CollisionMargin - distanceAtVertex);
        g_HairVertices[hairVertexIndex].xyz = projectedVertex;
        g_PrevHairVertices[hairVertexIndex].xyz = projectedVertex;
    }
}

// EndHLSL
