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

#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>

#include <Atom/RHI/RHIUtils.h>

#include <AzCore/std/algorithm.h>

namespace AZ
{
    namespace RPI
    {
        // --- TimestampResult ---
        TimestampResult::TimestampResult(uint64_t beginTick, uint64_t endTick, RHI::HardwareQueueClass hardwareQueueClass)
        {
            AZ_Assert(endTick >= beginTick, "TimestampResult: bad inputs");
            m_begin = beginTick;
            m_duration = endTick - beginTick;
            m_hardwareQueueClass = hardwareQueueClass;
        }

        uint64_t TimestampResult::GetDurationInNanoseconds() const
        {
            const RHI::Ptr<RHI::Device> device = RHI::GetRHIDevice();
            const AZStd::chrono::microseconds timeInMicroseconds = device->GpuTimestampToMicroseconds(m_duration, m_hardwareQueueClass);
            const auto timeInNanoseconds = AZStd::chrono::nanoseconds(timeInMicroseconds);

            return static_cast<uint64_t>(timeInNanoseconds.count());
        }

        uint64_t TimestampResult::GetDurationInTicks() const
        {
            return m_duration;
        }

        uint64_t TimestampResult::GetTimestampBeginInTicks() const
        {
            return m_begin;
        }

        void TimestampResult::Add(const TimestampResult& extent)
        {
            uint64_t end1 = m_begin + m_duration;
            uint64_t end2 = extent.m_begin + extent.m_duration;
            m_begin = m_begin < extent.m_begin ? m_begin : extent.m_begin;
            m_duration = (end1 > end2 ? end1 : end2) - m_begin;
        }

        // --- PipelineStatisticsResult ---

        PipelineStatisticsResult::PipelineStatisticsResult(AZStd::array_view<PipelineStatisticsResult>&& statisticsResultArray)
        {
            for (const PipelineStatisticsResult& result : statisticsResultArray)
            {
                *this += result;
            }
        }

        void PipelineStatisticsResult::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PipelineStatisticsResult>()
                    ->Version(1)
                    ->Field("vertexCount", &PipelineStatisticsResult::m_vertexCount)
                    ->Field("primitiveCount", &PipelineStatisticsResult::m_primitiveCount)
                    ->Field("vertexShaderInvocationCount", &PipelineStatisticsResult::m_vertexShaderInvocationCount)
                    ->Field("rasterizedPrimitiveCount", &PipelineStatisticsResult::m_rasterizedPrimitiveCount)
                    ->Field("renderedPrimitiveCount", &PipelineStatisticsResult::m_renderedPrimitiveCount)
                    ->Field("pixelShaderInvocationCount", &PipelineStatisticsResult::m_pixelShaderInvocationCount)
                    ->Field("computeShaderInvocationCount", &PipelineStatisticsResult::m_computeShaderInvocationCount)
                    ;
            }
        }

        PipelineStatisticsResult& PipelineStatisticsResult::operator+=(const PipelineStatisticsResult& rhs)
        {
            m_vertexCount += rhs.m_vertexCount;
            m_primitiveCount += rhs.m_primitiveCount;
            m_vertexShaderInvocationCount += rhs.m_vertexShaderInvocationCount;
            m_rasterizedPrimitiveCount += rhs.m_rasterizedPrimitiveCount;
            m_renderedPrimitiveCount += rhs.m_renderedPrimitiveCount;
            m_pixelShaderInvocationCount += rhs.m_pixelShaderInvocationCount;
            m_computeShaderInvocationCount += rhs.m_computeShaderInvocationCount;

            return *this;
        }


    };  // Namespace RPI
};  // Namespace AZ
