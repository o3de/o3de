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
#include "Device.h"
#include "GPUTimestamps.h"

namespace CAULDRON_DX12
{
    void GPUTimestamps::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers)
    {
        m_NumberOfBackBuffers = numberOfBackBuffers;

        D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
        queryHeapDesc.Count = MaxValuesPerFrame * numberOfBackBuffers * sizeof(UINT64);
        queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        queryHeapDesc.NodeMask = 0;
        ThrowIfFailed(pDevice->GetDevice()->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&m_pQueryHeap)));

        ThrowIfFailed(
            pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint64_t) * numberOfBackBuffers * MaxValuesPerFrame),
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_pBuffer))
        );
        SetName(m_pBuffer, "GPUTimestamps::m_pBuffer");

        //pDevice->SetStablePowerState(true);
    }

    void GPUTimestamps::OnDestroy()
    {
        m_pBuffer->Release();

        m_pQueryHeap->Release();
    }

    void GPUTimestamps::GetTimeStamp(ID3D12GraphicsCommandList *pCommandList, char *label)
    {
        uint32_t measurements = (uint32_t)m_labels[m_frame].size();
        pCommandList->EndQuery(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, m_frame*MaxValuesPerFrame + measurements);
        m_labels[m_frame].push_back(label);
    }

    void GPUTimestamps::CollectTimings(ID3D12GraphicsCommandList *pCommandList)
    {
        uint32_t measurements = (uint32_t)m_labels[m_frame].size();

        pCommandList->ResolveQueryData(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, m_frame*MaxValuesPerFrame, measurements, m_pBuffer, m_frame * MaxValuesPerFrame);
    }

    void GPUTimestamps::OnBeginFrame(UINT64 gpuTicksPerSecond, std::vector<TimeStamp> *pTimestamp)
    {
        pTimestamp->clear();

        double microsecondsPerTick = 1000000.0 / (double)gpuTicksPerSecond;

        uint32_t measurements = (uint32_t)m_labels[m_frame].size();
        uint32_t ini = MaxValuesPerFrame * m_frame;
        uint32_t fin = MaxValuesPerFrame * (m_frame + 1);

        CD3DX12_RANGE readRange( ini * sizeof(UINT64), fin * sizeof(UINT64));
        UINT64 *pTimingsInTicks = NULL;
        m_pBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pTimingsInTicks));

        for (uint32_t i = 0; i < measurements; i++)
        {
            TimeStamp ts = { m_labels[m_frame][i], float(microsecondsPerTick * (double)(pTimingsInTicks[i] - pTimingsInTicks[0])) };
            pTimestamp->push_back(ts);
        }

        m_pBuffer->Unmap(0, NULL);

        m_labels[m_frame].clear();
    }

    void GPUTimestamps::OnEndFrame()
    {
        m_frame = (m_frame + 1) % m_NumberOfBackBuffers;
    }
}
