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
#include "DX12QueryHeap.hpp"

namespace DX12
{
    QueryHeap::QueryHeap(Device* device)
        : DeviceObject(device)
    {
    }

    QueryHeap::~QueryHeap()
    {
    }

    bool QueryHeap::Init(Device* device, const D3D12_QUERY_HEAP_DESC& desc)
    {
        if (!IsInitialized())
        {
            SetDevice(device);

            ID3D12QueryHeap* heap;
            GetDevice()->GetD3D12Device()->CreateQueryHeap(&desc, IID_GRAPHICS_PPV_ARGS(&heap));

            m_QueryHeap = heap;
            heap->Release();

            m_Desc12 = desc;

            IsInitialized(true);
        }

        Reset();
        return true;
    }

    void QueryHeap::Reset()
    {
    }
}
