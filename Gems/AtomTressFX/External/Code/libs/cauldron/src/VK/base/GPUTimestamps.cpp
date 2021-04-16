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

namespace CAULDRON_VK
{
    void GPUTimestamps::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers)
    {
        m_pDevice = pDevice;
        m_NumberOfBackBuffers = numberOfBackBuffers;
        m_queryNeedsInitialReset = true;
        m_frame = 0;

        const VkQueryPoolCreateInfo queryPoolCreateInfo =
        {
            VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,     // VkStructureType                  sType
            NULL,                                         // const void*                      pNext
            (VkQueryPoolCreateFlags)0,                    // VkQueryPoolCreateFlags           flags
            VK_QUERY_TYPE_TIMESTAMP ,                     // VkQueryType                      queryType
            MaxValuesPerFrame * numberOfBackBuffers,      // deUint32                         entryCount
            0,                                            // VkQueryPipelineStatisticFlags    pipelineStatistics
        };

        VkResult res = vkCreateQueryPool(pDevice->GetDevice(), &queryPoolCreateInfo, NULL, &m_QueryPool);
    }

    void GPUTimestamps::OnDestroy()
    {
        vkDestroyQueryPool(m_pDevice->GetDevice(), m_QueryPool, nullptr);

        for (uint32_t i = 0; i < m_NumberOfBackBuffers; i++)
            m_labels[i].clear();
    }

    void GPUTimestamps::GetTimeStamp(VkCommandBuffer cmd_buf, char *label)
    {
        uint32_t measurements = (uint32_t)m_labels[m_frame].size();
        uint32_t offset = m_frame*MaxValuesPerFrame + measurements;

        vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPool, offset);

        m_labels[m_frame].push_back(label);
    }

    void GPUTimestamps::OnBeginFrame(VkCommandBuffer cmd_buf, std::vector<TimeStamp> *pTimestamp)
    {
        if (m_queryNeedsInitialReset)
        {
            vkCmdResetQueryPool(cmd_buf, m_QueryPool, 0, MaxValuesPerFrame*MaxValuesPerFrame);
            m_queryNeedsInitialReset = false;
        }

        pTimestamp->clear();

        // timestampPeriod is the number of nanoseconds per timestamp value increment
        double microsecondsPerTick = (1e-3f * m_pDevice->GetPhysicalDeviceProperries().limits.timestampPeriod);

        uint32_t measurements = (uint32_t)m_labels[m_frame].size();
        uint32_t offset = m_frame*MaxValuesPerFrame;

        UINT64 TimingsInTicks[256] = {};

        if (measurements > 0)
        {
            vkGetQueryPoolResults(m_pDevice->GetDevice(), m_QueryPool, offset, measurements, sizeof(TimingsInTicks) * sizeof(UINT64), &TimingsInTicks, sizeof(UINT64), VK_QUERY_RESULT_64_BIT);
        }
        vkCmdResetQueryPool(cmd_buf, m_QueryPool, offset, measurements);

        for (uint32_t i = 0; i < measurements; i++)
        {
            TimeStamp ts = { m_labels[m_frame][i], float(microsecondsPerTick * (double)(TimingsInTicks[i] - TimingsInTicks[0])) };
            pTimestamp->push_back(ts);
        }

        m_labels[m_frame].clear();

        GetTimeStamp(cmd_buf, "Begin Frame");
    }

    void GPUTimestamps::OnEndFrame()
    {
        m_frame = (m_frame + 1) % m_NumberOfBackBuffers;
    }
}