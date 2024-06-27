/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI/DeviceResource.h>

namespace AZ::RHI
{
    class CommandList;
    class DeviceQueryPool;
    using QueryHandle = RHI::Handle<uint32_t>;

    //! Control how queries record information.
    enum class QueryControlFlags : uint32_t
    {
        None = 0,
        //! Enable counting of fragments that pass the occlusion test.
        //! Not supported by all platforms. Only applicable to Occlusion query type.
        PreciseOcclusion = AZ_BIT(0) 
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::QueryControlFlags)

    //! DeviceQuery resource for recording gpu data like occlusion, timestamp or pipeline statistics.
    //! Queries belong to a DeviceQueryPool and their types are determined by the pool.
    class DeviceQuery
        : public DeviceResource
    {
        friend class DeviceQueryPool;
    public:
        AZ_RTTI(DeviceQuery, "{5E4AAD1B-E1A5-45FA-B965-9E212CE58B57}", DeviceResource);
        virtual ~DeviceQuery() override = default;

        ///////////////////////////////////////////////////////////////////
        // RHI::DeviceResource overrides
        void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override;
        ///////////////////////////////////////////////////////////////////

        /// Returns the query pool that this query belongs to.
        const DeviceQueryPool* GetQueryPool() const;
        DeviceQueryPool* GetQueryPool();

        /// Returns the handle of the query.
        QueryHandle GetHandle() const;

        //! Begin recording of a query. Timestamp queries don't support a "Begin" operation.
        //! @param commandList Command list for begin recording into the query.
        //! @param flags Control how the query will begin recording the information.
        ResultCode Begin(CommandList& commandList, QueryControlFlags flags = QueryControlFlags::None);

        //! End recording of a query. Queries must be began before they can be ended.
        //! Timestamp queries don't support an "End" operation.
        //! @param commandList Command list for end recording into the query. 
        //!                    Must be the same command list used for beginning the query.
        ResultCode End(CommandList& commandList);

        //! Writes the GPU timestamp into a query. Only timestamp queries support this function.
        //! @param commandList Command list used for end recording into the query.
        ResultCode WriteTimestamp(CommandList& commandList);

    protected:
        DeviceQuery() = default;

        ////////////////////////////////////////////////////////////////////////
        // Interfaces that the platform implementation overrides.
        virtual ResultCode BeginInternal(CommandList& commandList, QueryControlFlags flags) = 0;
        virtual ResultCode EndInternal(CommandList& commandList) = 0;
        virtual ResultCode WriteTimestampInternal(CommandList& commandList) = 0;
        ////////////////////////////////////////////////////////////////////////

    private:
        QueryHandle m_handle; ///< Handle of the DeviceQuery. Assigned when initializing the DeviceQuery.
        CommandList* m_currentCommandList = nullptr;
    };
}
