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
#ifndef __DX12VIEW__
#define __DX12VIEW__

#include "DX12Base.hpp"

namespace DX12
{
    class SwapChain;

    enum EViewType
    {
        EVT_Unknown = 0,
        EVT_VertexBufferView,
        EVT_IndexBufferView,
        EVT_ConstantBufferView,
        EVT_ShaderResourceView,
        EVT_UnorderedAccessView,
        EVT_DepthStencilView,
        EVT_RenderTargetView
    };

    class ResourceView
        : public AzRHI::ReferenceCounted
    {
    public:
        ResourceView();
        virtual ~ResourceView();

        ResourceView(ResourceView&& r);
        ResourceView& operator=(ResourceView&& r);

        bool Init(Resource& resource, EViewType type, UINT64 size = 0);

        //template<class T = ID3D12Resource>
        ID3D12Resource* GetD3D12Resource() const;

        ILINE Resource& GetDX12Resource() const
        {
            return *m_pResource;
        }

        ILINE void SetType(EViewType evt)
        {
            m_Type = evt;
        }
        ILINE EViewType GetType() const
        {
            return m_Type;
        }

        ILINE void HasDesc(bool has)
        {
            m_HasDesc = has;
        }
        ILINE bool HasDesc() const
        {
            return m_HasDesc;
        }

        ILINE void SetSize(UINT64 size)
        {
            m_Size = size;
        }
        ILINE UINT64 GetSize() const
        {
            return m_Size;
        }

        ILINE D3D12_DEPTH_STENCIL_VIEW_DESC& GetDSVDesc()
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
            return m_unDSVDesc;
        }
        ILINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDSVDesc() const
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
            return m_unDSVDesc;
        }

        ILINE D3D12_RENDER_TARGET_VIEW_DESC& GetRTVDesc()
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
            return m_unRTVDesc;
        }
        ILINE const D3D12_RENDER_TARGET_VIEW_DESC& GetRTVDesc() const
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
            return m_unRTVDesc;
        }

        ILINE D3D12_VERTEX_BUFFER_VIEW& GetVBVDesc()
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unVBVDesc;
        }
        ILINE const D3D12_VERTEX_BUFFER_VIEW& GetVBVDesc() const
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unVBVDesc;
        }

        ILINE D3D12_INDEX_BUFFER_VIEW& GetIBVDesc()
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unIBVDesc;
        }
        ILINE const D3D12_INDEX_BUFFER_VIEW& GetIBVDesc() const
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unIBVDesc;
        }

        ILINE D3D12_CONSTANT_BUFFER_VIEW_DESC& GetCBVDesc()
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unCBVDesc;
        }
        ILINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetCBVDesc() const
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unCBVDesc;
        }

        ILINE D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc()
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unSRVDesc;
        }
        ILINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc() const
        {
            //      DX12_ASSERT(m_pResource && true);
            return m_unSRVDesc;
        }

        ILINE D3D12_UNORDERED_ACCESS_VIEW_DESC& GetUAVDesc()
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
            return m_unUAVDesc;
        }
        ILINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetUAVDesc() const
        {
            //      DX12_ASSERT(m_pResource && (m_pResource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
            return m_unUAVDesc;
        }

        ILINE void SetDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& descriptorHandle)
        {
            m_DescriptorHandle = descriptorHandle;
        }
        ILINE D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const
        {
            return m_DescriptorHandle;
        }

    private:
        Resource* m_pResource;
        EViewType m_Type;
        D3D12_CPU_DESCRIPTOR_HANDLE m_DescriptorHandle;

        union
        {
            D3D12_VERTEX_BUFFER_VIEW m_unVBVDesc;
            D3D12_INDEX_BUFFER_VIEW m_unIBVDesc;
            D3D12_CONSTANT_BUFFER_VIEW_DESC m_unCBVDesc;
            D3D12_SHADER_RESOURCE_VIEW_DESC m_unSRVDesc;
            D3D12_UNORDERED_ACCESS_VIEW_DESC m_unUAVDesc;
            D3D12_DEPTH_STENCIL_VIEW_DESC m_unDSVDesc;
            D3D12_RENDER_TARGET_VIEW_DESC m_unRTVDesc;
        };

        // Some views can be created without descriptor (DSV)
        bool m_HasDesc;

        // View size in bytes
        UINT64 m_Size;
    };
}

#endif // __DX12VIEW__
