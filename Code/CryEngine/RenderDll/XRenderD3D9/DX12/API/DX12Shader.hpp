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
#ifndef __DX12SHADER__
#define __DX12SHADER__

#include "DX12Base.hpp"

namespace DX12
{
    enum EShaderStage : int8
    {
        // Graphics pipeline
        ESS_Vertex,
        ESS_Hull,
        ESS_Domain,
        ESS_Geometry,
        ESS_Pixel,

        // Compute pipeline
        ESS_Compute,

        // Complete pipeline
        ESS_All = ESS_Compute,

        ESS_LastWithoutCompute = ESS_Compute,
        ESS_Num = ESS_Compute + 1,
        ESS_None = -1,
        ESS_First = ESS_Vertex
    };

    struct ReflectedBindingRange
    {
        AZ::u8 m_ShaderRegister;
        AZ::u8 m_Count;

        // Whether the bind point is actually used in the shader. Ideally, this is compiled out, but in order to support
        // lower tier hardware we have to define a root parameter and bind a null descriptor for it.
        AZ::u8 m_bUsed : 1;

        // Shared bindings are marked as visible across all shader stages. Practically, this means the binding
        // is not merged and will be de-duplicated when the final layout is built. Candidates for sharing are constant
        // buffers that are accessed by every stage and have a well-defined layout across all stages.
        AZ::u8 m_bShared : 1;

        // We currently merge any range where the layout contents aren't reflected by the shader. Practically,
        // this means that any constant buffer that's reflected gets placed into a root constant buffer parameter.
        AZ::u8 m_bMergeable : 1;

#ifdef GFX_DEBUG
        AZ::u8 m_Types[56];
        AZ::u8 m_Dimensions[56];
#endif

        ReflectedBindingRange()
            : m_ShaderRegister(0)
            , m_Count(0)
            , m_bUsed(true)
            , m_bShared(false)
            , m_bMergeable(true)
        {}

        ReflectedBindingRange(
            AZ::u8 shaderRegister,
            AZ::u8 count,
            [[maybe_unused]] AZ::u8 type,
            [[maybe_unused]] AZ::u8 dimension,
            bool bUsed,
            bool bShared,
            bool bMergeable)
            : m_ShaderRegister(shaderRegister)
            , m_Count(count)
            , m_bUsed(bUsed)
            , m_bShared(bShared)
            , m_bMergeable(bMergeable)
        {
#ifdef GFX_DEBUG
            for (AZ::u32 i = 0; i < count; ++i)
            {
                m_Types[i] = type;
                m_Dimensions[i] = dimension;
            }
#endif
        }
    };

    struct ReflectedBindingRangeList
    {
        std::vector<ReflectedBindingRange> m_Ranges;
        AZ::u32 m_DescriptorCount;

        ReflectedBindingRangeList()
            : m_DescriptorCount(0)
        {}
    };

    ////////////////////////////////////////////////////////////////////////////

    struct ReflectedBindings
    {
        ReflectedBindingRangeList m_ConstantBuffers;    // CBV
        ReflectedBindingRangeList m_InputResources;     // SRV
        ReflectedBindingRangeList m_OutputResources;    // UAV
        ReflectedBindingRangeList m_Samplers;           // SMP
    };

    ////////////////////////////////////////////////////////////////////////////

    class Shader : public DeviceObject
    {
    public:
        // Create new shader using DX11 reflection interface
        static Shader* CreateFromD3D11(Device* device, const D3D12_SHADER_BYTECODE& byteCode);

        Shader(Device* device);

        const AZ::Crc32 GetHash() const
        {
            return m_hash;
        }

        const ReflectedBindings& GetReflectedBindings() const
        {
            return m_Bindings;
        }

        const AZ::u32 GetReflectedBindingHash() const
        {
            return m_BindingHash;
        }

    protected:
        virtual ~Shader();

    private:

        D3D12_SHADER_BYTECODE m_Bytecode;
        ReflectedBindings m_Bindings;
        AZ::u32 m_BindingHash = 0;
        AZ::Crc32 m_hash;
    };
}

#endif // __DX12SHADER__
