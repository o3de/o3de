/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

struct VertexOutput
{
    float4 m_position : SV_Position;
};

// Single triangle to fill entire clip space.
static float2 positions[3] = 
{
    {-1.0, -1.0},
    {3.0, -1.0},
    {-1.0, 3.0}
};

VertexOutput MainVS(uint vertexId:SV_VertexID)
{
    VertexOutput output;
    output.m_position = float4(positions[vertexId], 1.0, 1.0);
    return output;
}
