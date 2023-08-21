/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceResource.h>
#include <Atom/RHI/Query.h>

namespace AZ::RHI
{
    class CommandList;
    class MultiDeviceQueryPool;

    //! MultiDeviceQuery resource for recording gpu data like occlusion, timestamp or pipeline statistics.
    //! Queries belong to a MultiDeviceQueryPool and their types are determined by the pool.
    class MultiDeviceQuery : public MultiDeviceResource
    {
        friend class MultiDeviceQueryPool;

    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceQuery, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceQuery, "{F72033E8-7A91-40BF-80E2-7262223362DB}", MultiDeviceResource);
        MultiDeviceQuery() = default;
        virtual ~MultiDeviceQuery() override = default;

        //! Returns the device-specific Query for the given index
        inline Ptr<Query> GetDeviceQuery(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceQuery",
                m_deviceQueries.find(deviceIndex) != m_deviceQueries.end(),
                "No DeviceQuery found for device index %d\n",
                deviceIndex);

            return m_deviceQueries.at(deviceIndex);
        }

        //! Returns the query pool that this query belongs to.
        const MultiDeviceQueryPool* GetQueryPool() const;
        MultiDeviceQueryPool* GetQueryPool();

        //! Returns the device-specific query handle
        //! @param deviceIndex Specify from which device the query handle should be retrieved
        QueryHandle GetHandle(int deviceIndex)
        {
            AZ_Assert(
                m_deviceQueries.find(deviceIndex) != m_deviceQueries.end(), "No DeviceQuery found for device index %d\n", deviceIndex);
            return m_deviceQueries.at(deviceIndex)->GetHandle();
        }

        //! Shuts down the device-specific resources by detaching them from their parent pool.
        void Shutdown() override final;

        //! Invalidates all views by setting off events on all device-specific ResourceInvalidateBusses
        void InvalidateViews() override final;

    private:
        //! A map of all device-specific Queries, indexed by the device index
        AZStd::unordered_map<int, Ptr<Query>> m_deviceQueries;
    };
} // namespace AZ::RHI
