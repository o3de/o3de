/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    //! Query types supported by the RHI
    enum class QueryType : uint32_t
    {
        Occlusion = 0,     ///< Binary or Precise (if supported) occlusion type.
        Timestamp,          ///< Query used for getting the gpu timestamp at a precise moment. Not supported by all platforms.
        PipelineStatistics, ///< Query used for gathering pipeline statistics during a collection of events. Not supported by all platforms.
        Count,
        Invalid = Count
    };
    static const uint32_t QueryTypeCount = static_cast<uint32_t>(QueryType::Count);

        
    //! Flags for specifying multiple query types. Use for specifying supported queries.
    enum class QueryTypeFlags : uint32_t
    {
        None = 0,
        Occlusion = AZ_BIT(static_cast<uint32_t>(QueryType::Occlusion)),
        Timestamp = AZ_BIT(static_cast<uint32_t>(QueryType::Timestamp)),
        PipelineStatistics = AZ_BIT(static_cast<uint32_t>(QueryType::PipelineStatistics)),
        All = Occlusion | Timestamp | PipelineStatistics
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::QueryTypeFlags)

        
    //! Pipeline statistics available for the PipelineStatistics query type.
    enum class PipelineStatisticsFlags : uint64_t
    {
        None = 0,
        IAVertices = AZ_BIT(0),     ///< Number of vertices read by input assembler.
        IAPrimitives = AZ_BIT(1),   ///< Number of primitives read by the input assembler.
        VSInvocations = AZ_BIT(2),  ///< Number of times a vertex shader was invoked.
        GSInvocations = AZ_BIT(3),  ///< Number of times a geometry shader was invoked.
        GSPrimitives = AZ_BIT(4),   ///< Number of primitives output by a geometry shader.
        CInvocations = AZ_BIT(5),   ///< Number of primitives that were sent to the rasterizer.
        CPrimitives = AZ_BIT(6),    ///< Number of primitives that were rendered (the number of primitives output by the Primitive Clipping stage).
        PSInvocations = AZ_BIT(7),  ///< Number of times a pixel shader was invoked.
        HSInvocations = AZ_BIT(8),  ///< Number of times a hull shader was invoked.
        DSInvocations = AZ_BIT(9),  ///< Number of times a domain shader was invoked.
        CSInvocations = AZ_BIT(10), ///< Number of times a compute shader was invoked.
        All =   IAVertices | IAPrimitives | VSInvocations | GSInvocations | GSPrimitives | CInvocations | CPrimitives |
                PSInvocations | HSInvocations | DSInvocations | CSInvocations
    };
        
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::PipelineStatisticsFlags)

        
    //! Descriptor for the QueryPool RHI type. Contains the type and count
    //! when initializing a query pool.
    class QueryPoolDescriptor
        : public ResourcePoolDescriptor
    {
    public:
        virtual ~QueryPoolDescriptor() = default;
        AZ_RTTI(QueryPoolDescriptor, "{770C9C44-8E5D-4A23-87A4-2308CD2C5162}", ResourcePoolDescriptor);
        AZ_CLASS_ALLOCATOR(QueryPoolDescriptor, SystemAllocator)
        static void Reflect(AZ::ReflectContext* context);

        QueryPoolDescriptor() = default;

        uint32_t m_queriesCount = 0;
        QueryType m_type = QueryType::Occlusion;
        /// Mask of pipeline statistics that the pool will collect. Only valid for the QueryType::PipelineStatistics type.
        PipelineStatisticsFlags m_pipelineStatisticsMask = PipelineStatisticsFlags::None; 
    };
}
