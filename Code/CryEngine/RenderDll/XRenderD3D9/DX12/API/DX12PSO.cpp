/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#include "RenderDll_precompiled.h"
#include "DX12PSO.hpp"
#include "DX12Device.hpp"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DX12PSO_CPP_SECTION_1 1
#define DX12PSO_CPP_SECTION_2 2
#define DX12PSO_CPP_SECTION_3 3
#define DX12PSO_CPP_SECTION_4 4
#define DX12PSO_CPP_SECTION_5 5
#define DX12PSO_CPP_SECTION_6 6
#endif

namespace DX12
{
    bool PipelineState::Init(const RootSignature* rootSignature, ID3D12PipelineState* pipelineState)
    {
        m_RootSignature = rootSignature;
        m_PipelineState = pipelineState;

        return true;
    }

    bool GraphicsPipelineState::Init(const InitParams& params)
    {
        AZ_TRACE_METHOD();
        m_Desc = params.desc;
        m_Desc.pRootSignature = params.rootSignature->GetD3D12RootSignature();

        ID3D12PipelineState* pipelineState12 = nullptr;
        HRESULT result = GetDevice()->GetD3D12Device()->CreateGraphicsPipelineState(&m_Desc, IID_GRAPHICS_PPV_ARGS(&pipelineState12));

        if (result != S_OK)
        {
            DX12_ERROR("Could not create graphics pipeline state!");
            return false;
        }

        PipelineState::Init(params.rootSignature, pipelineState12);
        pipelineState12->Release();

        return true;
    }

    bool GraphicsPipelineState::Init(const InitParams& params, ID3D12PipelineState* pipelineState)
    {
        AZ_TRACE_METHOD();
        m_Desc = params.desc;
        m_Desc.pRootSignature = params.rootSignature->GetD3D12RootSignature();

        return PipelineState::Init(params.rootSignature, pipelineState);
    }

    bool ComputePipelineState::Init(const InitParams& params)
    {
        AZ_TRACE_METHOD();
        m_Desc = params.desc;
        m_Desc.pRootSignature = params.rootSignature->GetD3D12RootSignature();

        ID3D12PipelineState* pipelineState12 = nullptr;
        HRESULT result = GetDevice()->GetD3D12Device()->CreateComputePipelineState(&m_Desc, IID_GRAPHICS_PPV_ARGS(&pipelineState12));

        if (FAILED(result))
        {
            DX12_ERROR("Could not create graphics pipeline state!");
            return false;
        }

        PipelineState::Init(params.rootSignature, pipelineState12);
        pipelineState12->Release();

        return true;
    }

    bool ComputePipelineState::Init(const InitParams& params, ID3D12PipelineState* pipelineState)
    {
        AZ_TRACE_METHOD();
        m_Desc = params.desc;
        m_Desc.pRootSignature = params.rootSignature->GetD3D12RootSignature();

        return PipelineState::Init(params.rootSignature, pipelineState);
    }

    PipelineStateCache::PipelineStateCache()
        : m_Device(nullptr)
    {
    }

    PipelineStateCache::~PipelineStateCache()
    {
    }

    bool PipelineStateCache::Init(Device* device)
    {
        m_Device = device;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12PSO_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(DX12PSO_cpp)
#endif

        return true;
    }

    GraphicsPipelineState* PipelineStateCache::AcquirePipelineState(const GraphicsPipelineState::InitParams& params)
    {
        // LSB cleared marks graphics pipeline states, in case graphics and compute hashes collide
        THash hash = (~1) & params.hash;

        auto iter = m_Cache.find(hash);

        if (iter != m_Cache.end())
        {
            return static_cast<GraphicsPipelineState*>(iter->second.get());
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12PSO_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(DX12PSO_cpp)
#endif
        DX12::GraphicsPipelineState* result = new DX12::GraphicsPipelineState(m_Device);

        if (!result->Init(params))
        {
            DX12_ERROR("Could not create PSO!");
            return nullptr;
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12PSO_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(DX12PSO_cpp)
#endif

        m_Cache[hash] = result;

        return result;
    }

    ComputePipelineState* PipelineStateCache::AcquirePipelineState(const ComputePipelineState::InitParams& params)
    {
        // LSB filled marks compute pipeline states, in case graphics and compute hashes collide
        THash hash = (1) | params.hash;

        auto iter = m_Cache.find(hash);
        if (iter != m_Cache.end())
        {
            return static_cast<ComputePipelineState*>(iter->second.get());
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12PSO_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(DX12PSO_cpp)
#endif

        ComputePipelineState* result = new ComputePipelineState(m_Device);
        if (!result->Init(params))
        {
            DX12_ERROR("Could not create PSO!");
            return nullptr;
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12PSO_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(DX12PSO_cpp)
#endif

        m_Cache[hash] = result;
        return result;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION DX12PSO_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(DX12PSO_cpp)
#endif
}
