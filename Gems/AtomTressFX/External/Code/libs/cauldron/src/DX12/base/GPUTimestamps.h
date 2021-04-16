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
#pragma once

#include <vector>
#include <string>

namespace CAULDRON_DX12
{
    // In DX12 timestamps are written by the GPU into a system memory resource. 
    // Its similar to a 'dynamic' buffer but this time it is the GPU who is writing 
    // and CPU is who is reading. Hence we need a sort of ring buffer to make sure 
    // we are reading from a chunk of the buffer that is not written to by the GPU
    // 

    // This class helps insert queries in the command buffer and readback the results.
    // The tricky part in fact is reading back the results without stalling the GPU. 
    // For that it splits the readback heap in <numberOfBackBuffers> pieces and it reads 
    // from the last used chuck.

    struct TimeStamp
    {
        std::string m_label;
        float       m_microseconds;
    };

    class GPUTimestamps
    {
    public:
        void OnCreate(Device *pDevice, uint32_t numberOfBackBuffers);
        void OnDestroy();

        void GetTimeStamp(ID3D12GraphicsCommandList *pCommandList, char *label);
        void CollectTimings(ID3D12GraphicsCommandList *pCommandList);

        void OnBeginFrame(UINT64 gpuTicksPerSecond, std::vector<TimeStamp> *pTimestamp);
        void OnEndFrame();

    private:
        const uint32_t MaxValuesPerFrame = 128;

        ID3D12Resource    *m_pBuffer = NULL;
        ID3D12QueryHeap   *m_pQueryHeap = NULL;

        uint32_t m_frame = 0;
        uint32_t m_NumberOfBackBuffers = 0;

        std::vector<std::string> m_labels[5];
    };
}
