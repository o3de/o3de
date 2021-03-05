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
#pragma once

#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/Name/Name.h>

#include <limits>

namespace AZ
{
    namespace RPI
    {
        enum class QueryResultCode : uint8_t
        {
            Success,     /// Query task performed successfully.
            Fail,        /// Query task execution failed.
        };

        enum ScopeQueryType : uint32_t
        {
            Timestamp = 0,
            PipelineStatistics,

            Count,
            Invalid = Count
        };

        //! The structure that is used to store the readback data from the timestamp queries
        class TimestampResult
        {
        public:
            TimestampResult() = default;
            TimestampResult(uint64_t timestampInTicks);
            TimestampResult(uint64_t timestampQueryResultLow, uint64_t timestampQueryResultHigh);
            TimestampResult(AZStd::array_view<TimestampResult>&& timestampResultArray);

            uint64_t GetTimestampInNanoseconds() const;
            uint64_t GetTimestampInTicks() const;

        private:
            uint64_t m_timestampInTicks = 0u;
        };

        //! The structure that is used to read back the results form the PipelineStatistics queries 
        //! NOTE: The number of supported entries in the PipelineStatistics depends on the flags that are
        //! defined in GpuQuerySystemDescriptor::m_statisticsQueryFlags. The number of flags must
        //! be equal to PipelineStatisticsResult's member count.
        struct PipelineStatisticsResult
        {
            AZ_TYPE_INFO(PipelineStatisticsResult, "{8C4A07F0-5B77-4614-9007-E6E1F08FAC73}");
            static void Reflect(AZ::ReflectContext* context);

            PipelineStatisticsResult() = default;
            PipelineStatisticsResult(AZStd::array_view<PipelineStatisticsResult>&& statisticsResultArray);

            PipelineStatisticsResult& operator+=(const PipelineStatisticsResult& rhs);

            uint64_t m_vertexCount = 0u;
            uint64_t m_primitiveCount = 0u;
            uint64_t m_vertexShaderInvocationCount = 0u;
            uint64_t m_rasterizedPrimitiveCount = 0u;
            uint64_t m_renderedPrimitiveCount = 0u;
            uint64_t m_pixelShaderInvocationCount = 0u;
            uint64_t m_computeShaderInvocationCount = 0u;
        };

    }; // namespace RPI
}; // Namespace AZ
