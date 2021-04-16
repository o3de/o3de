// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "glTFHelpers.h"

namespace CAULDRON_DX12
{
    DXGI_FORMAT GetFormat(const std::string &str, int id)
    {
        if (str == "SCALAR")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_R8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32_FLOAT; //(FLOAT)
            }
        }
        else if (str == "VEC2")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_R8G8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8G8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16G16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16G16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32_FLOAT; //(FLOAT)
            }
        }
        else if (str == "VEC3")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_UNKNOWN; //(BYTE)
            case 5121: return DXGI_FORMAT_UNKNOWN; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_UNKNOWN; //(SHORT)2
            case 5123: return DXGI_FORMAT_UNKNOWN; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32B32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32B32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32B32_FLOAT; //(FLOAT)
            }
        }
        else if (str == "VEC4")
        {
            switch (id)
            {
            case 5120: return DXGI_FORMAT_R8G8B8A8_SINT; //(BYTE)
            case 5121: return DXGI_FORMAT_R8G8B8A8_UINT; //(UNSIGNED_BYTE)1
            case 5122: return DXGI_FORMAT_R16G16B16A16_SINT; //(SHORT)2
            case 5123: return DXGI_FORMAT_R16G16B16A16_UINT; //(UNSIGNED_SHORT)2
            case 5124: return DXGI_FORMAT_R32G32B32A32_SINT; //(SIGNED_INT)4
            case 5125: return DXGI_FORMAT_R32G32B32A32_UINT; //(UNSIGNED_INT)4
            case 5126: return DXGI_FORMAT_R32G32B32A32_FLOAT; //(FLOAT)
            }
        }

        return DXGI_FORMAT_UNKNOWN;
    }

    void CreateSamplerForPBR(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    };

    void CreateSamplerForBrdfLut(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    };

    void CreateSamplerForShadowMap(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    };

    void CreateSamplerForShadowBuffer(uint32_t samplerIndex, D3D12_STATIC_SAMPLER_DESC *pSamplerDesc)
    {
        ZeroMemory(pSamplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
        pSamplerDesc->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        pSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        pSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        pSamplerDesc->MinLOD = 0.0f;
        pSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
        pSamplerDesc->MipLODBias = 0;
        pSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        pSamplerDesc->MaxAnisotropy = 1;
        pSamplerDesc->ShaderRegister = samplerIndex;
        pSamplerDesc->RegisterSpace = 0;
        pSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
}

