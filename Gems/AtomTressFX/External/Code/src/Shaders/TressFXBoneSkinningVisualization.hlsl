//---------------------------------------------------------------------------------------
// Shader code related to visualizing skining
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

[[vk::binding(0, 0)]] cbuffer boneSkinningVisualizationConstants {
	float4x4 mMW;
	float4x4 mWVP;
	float3 cColor;
	float3 vLightDir;
	float3 g_vEye;
};

struct VertexData
{
	float3 position;
	float3 normal;
};

[[vk::binding(1, 0)]] StructuredBuffer<VertexData> vertices;

struct VsOutput
{
	float4 vPositionSS : SV_POSITION;
	float3 vPos_ : vPos_;
	float3 vNormal_ : vNormal_;
};

VsOutput BoneSkinningVisualizationVS( uint vertexId : SV_VertexID )
{
	VsOutput output;

	output.vNormal_ = mul(vertices[vertexId].normal.xyz, (float3x3)mMW);
	float4 inputVertexPos = float4(vertices[vertexId].position.xyz, 1.0);

	float4 vPos = mul(mMW, float4(inputVertexPos.xyz, 1.0f));
	output.vPos_ = inputVertexPos.xyz;
	output.vPositionSS = mul(mWVP, vPos);
	return output;
}


struct PSInput
{
	float4 vPositionSS : SV_POSITION;
	float3 vPos_ : vPos_;
	float3 vNormal_ : vNormal_;
};

struct PSOutput
{
	float4 vColor : SV_TARGET;
};

PSOutput BoneSkinningVisualizationPS(PSInput input)
{
	PSOutput output = (PSOutput)0;
	float4 vDiffuse = float4(cColor.xyz, 1.0);
	float fAmbient = 0.2;
	float3 vLightDir1 = float3(-1., 0., -1.);

	//float fLighting = saturate(dot(normalize(vLightDir), input.vNormal_));
	float fLighting = saturate(dot(normalize(vLightDir), input.vNormal_)) + 0.7*saturate(dot(normalize(vLightDir1), input.vNormal_));
	fLighting = max(fLighting, fAmbient);

	output.vColor = vDiffuse * fLighting;
	return output;
}