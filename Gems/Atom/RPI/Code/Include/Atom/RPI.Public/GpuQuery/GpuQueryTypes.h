/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RPI.Public/Configuration.h>

#include <AzCore/std/containers/span.h>
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
        class ATOM_RPI_PUBLIC_API TimestampResult
        {
        public:
            TimestampResult() = default;
            TimestampResult(uint64_t beginTick, uint64_t endTick, RHI::HardwareQueueClass hardwareQueueClass);

            uint64_t GetDurationInNanoseconds() const;
            uint64_t GetDurationInTicks() const;
            uint64_t GetTimestampBeginInTicks() const;

            void Add(const TimestampResult& extent);

        private:
            // the timestamp of begin and duration in ticks.
            uint64_t m_begin = 0;
            uint64_t m_duration = 0;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
        };

        //! The structure that is used to read back the results form the PipelineStatistics queries 
        //! NOTE: The number of supported entries in the PipelineStatistics depends on the flags that are
        //! defined in GpuQuerySystemDescriptor::m_statisticsQueryFlags. The number of flags must
        //! be equal to PipelineStatisticsResult's member count.
        struct ATOM_RPI_PUBLIC_API PipelineStatisticsResult
        {
            AZ_TYPE_INFO(PipelineStatisticsResult, "{8C4A07F0-5B77-4614-9007-E6E1F08FAC73}");
            static void Reflect(AZ::ReflectContext* context);

            PipelineStatisticsResult() = default;
            PipelineStatisticsResult(AZStd::span<const PipelineStatisticsResult>&& statisticsResultArray);

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
