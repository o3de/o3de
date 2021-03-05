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
#include "DX12DescriptorHeap.hpp"

namespace DX12
{
    DescriptorHeap::DescriptorHeap(Device* device)
        : DeviceObject(device)
    {
    }

    DescriptorHeap::~DescriptorHeap()
    {
    }

    bool DescriptorHeap::Init(const D3D12_DESCRIPTOR_HEAP_DESC& desc)
    {
        if (!IsInitialized())
        {
            ID3D12DescriptorHeap* heap;
            GetDevice()->GetD3D12Device()->CreateDescriptorHeap(&desc, IID_GRAPHICS_PPV_ARGS(&heap));

            m_pDescriptorHeap = heap;
            heap->Release();

            m_Desc12 = m_pDescriptorHeap->GetDesc();
            m_HeapStartCPU = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            m_HeapStartGPU = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            m_DescSize = GetDevice()->GetD3D12Device()->GetDescriptorHandleIncrementSize(m_Desc12.Type);

            IsInitialized(true);
        }

        Reset();
        return true;
    }

    DescriptorBlock::DescriptorBlock(const SDescriptorBlock& block)
    {
        m_pDescriptorHeap = reinterpret_cast<DescriptorHeap*>(block.pBuffer);
        m_BlockStart = block.offset;
        m_Capacity = block.size;
        m_Cursor = 0;
    }
}
