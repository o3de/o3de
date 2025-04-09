/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Resource.h>
#include <Atom/RHI/DeviceQuery.h>

namespace AZ::RHI
{
    class CommandList;
    class QueryPool;

    //! Query resource for recording gpu data like occlusion, timestamp or pipeline statistics.
    //! Queries belong to a QueryPool and their types are determined by the pool.
    class Query : public Resource
    {
        friend class QueryPool;

    public:
        AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator, 0);
        AZ_RTTI(Query, "{F72033E8-7A91-40BF-80E2-7262223362DB}", Resource);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Query);
        Query() = default;
        virtual ~Query() override = default;

        //! Returns the query pool that this query belongs to.
        const QueryPool* GetQueryPool() const;
        QueryPool* GetQueryPool();

        //! Returns the device-specific query handle
        //! @param deviceIndex Specify from which device the query handle should be retrieved
        QueryHandle GetHandle(int deviceIndex)
        {
            return GetDeviceQuery(deviceIndex)->GetHandle();
        }

        //! Shuts down the device-specific resources by detaching them from their parent pool.
        void Shutdown() override final;
    };
} // namespace AZ::RHI
