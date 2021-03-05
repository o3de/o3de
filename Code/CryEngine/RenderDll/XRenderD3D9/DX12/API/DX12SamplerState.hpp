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

#include "DX12Base.hpp"

namespace DX12
{
    class SamplerState : public AzRHI::ReferenceCounted
    {
    public:
        SamplerState();
        virtual ~SamplerState();

        D3D12_SAMPLER_DESC& GetSamplerDesc()
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
            return m_unSamplerDesc;
        }
        const D3D12_SAMPLER_DESC& GetSamplerDesc() const
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
            return m_unSamplerDesc;
        }

        void SetDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& descriptorHandle) { m_DescriptorHandle = descriptorHandle; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return m_DescriptorHandle; }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_DescriptorHandle;

        union
        {
            D3D12_SAMPLER_DESC m_unSamplerDesc;
        };
    };
}
