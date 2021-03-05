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
#pragma once
#ifndef __DX12ROOTSIGNATURE__
#define __DX12ROOTSIGNATURE__

#include "DX12Shader.hpp"
#include <XRenderD3D9/DeviceManager/Enums.h>

namespace DX12
{
    // Map of all <stage,slot> pairs to descriptor-tables offsets (the CBV+SRV+UAV/SMP index is the offset)
    struct ResourceLayoutBinding
    {
        D3D12_DESCRIPTOR_RANGE_TYPE ViewType;
        EShaderStage ShaderStage;
        uint8_t ShaderSlot;
#ifdef GFX_DEBUG
        uint8_t DescriptorOffset;
#endif // !NDEBUG
    };

    struct ConstantBufferLayoutBinding
    {
        EShaderStage ShaderStage;
        uint8_t ShaderSlot;
        uint8_t RootParameterIndex;
    };

    struct PipelineLayout
    {
        std::vector<ConstantBufferLayoutBinding> m_ConstantViews;
        std::vector<ResourceLayoutBinding> m_TableResources;
        std::vector<ResourceLayoutBinding> m_Samplers;

        AZ::u32 m_ShaderStageAccess;

        // Memory holding all descriptor-ranges which will be added to the root-signature (basically a heap)
        CD3DX12_DESCRIPTOR_RANGE m_DescRanges[256];
        AZ::u32 m_DescRangeCursor;

        // Consecutive list of all root-parameter descriptions of all types (points to m_DescRanges)
        CD3DX12_ROOT_PARAMETER m_RootParameters[256];
        AZ::u32 m_NumRootParameters;

        // Static samplers, directly embedded in the shaders once PSO is created with correspondig root signature
        D3D12_STATIC_SAMPLER_DESC m_StaticSamplers[16];
        AZ::u32 m_NumStaticSamplers;

        // Consecutive list of all descriptor-tables which are in the root-signature (without holes)
        struct DescriptorTableInfo
        {
            D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
            AZ::u32 m_Offset;
        };

        DescriptorTableInfo m_DescriptorTables[256];
        AZ::u32 m_NumDescriptorTables;

        // Total number of resources bound to all shader stages (CBV+SRV+UAV)
        size_t m_NumDescriptors;

        // Total number of samplers bound to all shader stages
        size_t m_NumDynamicSamplers;

        PipelineLayout();

        void Build(Shader* vs, Shader* hs, Shader* ds, Shader* gs, Shader* ps);
        void Build(Shader* cs);

        void Clear();
        void DebugPrint() const;

    private:
        void AppendDescriptorTables(Shader* pShader, EShaderStage eStage);
        void AppendRootConstantBuffers(Shader* pShader, EShaderStage eStage);
        void MakeDescriptorTable(AZ::u32 currentRangeCursor, AZ::u32& lastRangeCursor, AZ::u32 currentOffset, AZ::u32& lastOffset, D3D12_DESCRIPTOR_HEAP_TYPE eHeap, D3D12_SHADER_VISIBILITY eVisibility);
    };

    class RootSignature
        : public DeviceObject
    {
    public:
        struct GraphicsInitParams
        {
            Shader* m_VertexShader;
            Shader* m_HullShader;
            Shader* m_DomainShader;
            Shader* m_GeometryShader;
            Shader* m_PixelShader;
        };

        struct ComputeInitParams
        {
            Shader* m_ComputeShader;
        };

        RootSignature(Device* device);
        ~RootSignature();

        bool Init(const GraphicsInitParams& params);
        bool Init(const ComputeInitParams& params);
        bool Init(PipelineLayout& resourceMappings, CommandMode commandType);

        inline THash GetHash() const
        {
            return m_Hash;
        }

        inline const PipelineLayout& GetPipelineLayout() const
        {
            return m_PipelineLayout;
        }

        inline ID3D12RootSignature* GetD3D12RootSignature() const
        {
            return /*PassAddRef*/ (m_pRootSignature);
        }

    private:
        bool Init(CommandMode commandType);

        PipelineLayout m_PipelineLayout;
        SmartPtr<ID3D12RootSignature> m_pRootSignature;
        AZ::u32 m_nodeMask;
        THash m_Hash;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class RootSignatureCache
    {
    public:
        RootSignatureCache();
        ~RootSignatureCache();

        bool Init(Device* device);

        RootSignature* AcquireRootSignature(const RootSignature::ComputeInitParams& params);
        RootSignature* AcquireRootSignature(const RootSignature::GraphicsInitParams& params);

    private:
        Device* m_pDevice;

        AZStd::unordered_map<THash, SmartPtr<RootSignature>> m_RootSignatureMap;
    };
}

#endif // __DX12ROOTSIGNATURE__
