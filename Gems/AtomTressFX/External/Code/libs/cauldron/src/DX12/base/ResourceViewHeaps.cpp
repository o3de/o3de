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
#include "ResourceViewHeaps.h"
#include "../../common/Misc/Misc.h"

namespace CAULDRON_DX12
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void StaticResourceViewHeap::OnCreate(Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount, bool forceCPUVisible)
    {
        m_descriptorCount = descriptorCount;
        m_index = 0;
        
        m_descriptorElementSize = pDevice->GetDevice()->GetDescriptorHandleIncrementSize(heapType);

        D3D12_DESCRIPTOR_HEAP_DESC descHeap;
        descHeap.NumDescriptors = descriptorCount;
        descHeap.Type = heapType;
        descHeap.Flags = ((heapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) || (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)) ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (forceCPUVisible)
        {
            descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        }
        descHeap.NodeMask = 0;
        ThrowIfFailed(
            pDevice->GetDevice()->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(&m_pHeap))
        );
        SetName(m_pHeap, "StaticHeapDX12");
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void StaticResourceViewHeap::OnDestroy()
    {
        m_pHeap->Release();
    }
}

