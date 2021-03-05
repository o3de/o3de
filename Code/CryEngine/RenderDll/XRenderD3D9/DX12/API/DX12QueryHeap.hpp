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
    class QueryHeap : public DeviceObject
    {
    public:
        QueryHeap(Device* device);
        virtual ~QueryHeap();

        bool Init(Device* device, const D3D12_QUERY_HEAP_DESC& desc);
        void Reset();

        ID3D12QueryHeap* GetD3D12QueryHeap() const { return /*PassAddRef*/ (m_QueryHeap); }

        UINT GetType() const { return m_Desc12.Type; }
        UINT GetCapacity() const { return m_Desc12.Count; }

    private:
        SmartPtr<ID3D12QueryHeap> m_QueryHeap;
        D3D12_QUERY_HEAP_DESC m_Desc12;
    };
}
