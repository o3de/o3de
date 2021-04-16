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

namespace CAULDRON_VK
{
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

        void GetTimeStamp(VkCommandBuffer cmd_buf, char *label);

        void OnBeginFrame(VkCommandBuffer cmd_buf, std::vector<TimeStamp> *pTimestamp);
        void OnEndFrame();

    private:
        Device* m_pDevice;

        const uint32_t MaxValuesPerFrame = 128;
        bool m_queryNeedsInitialReset = true;
        VkQueryPool m_QueryPool;

        uint32_t m_frame = 0;
        uint32_t m_NumberOfBackBuffers = 0;

        std::vector<std::string> m_labels[5];
    };
}