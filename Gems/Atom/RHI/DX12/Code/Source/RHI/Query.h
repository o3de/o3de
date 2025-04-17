/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceQuery.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class QueryPool;

        class Query final
            : public RHI::DeviceQuery
        {
            friend class QueryPool;
            using Base = RHI::DeviceQuery;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::ThreadPoolAllocator);
            AZ_RTTI(Query, "{87F8BCCF-A4DD-484F-917B-FBE6715F23D6}", Base);
            ~Query() = default;

            static RHI::Ptr<Query> Create();           

        private:
            Query() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceQuery
            RHI::ResultCode BeginInternal(RHI::CommandList& commandList, RHI::QueryControlFlags flags) override;
            RHI::ResultCode EndInternal(RHI::CommandList& commandList) override;
            RHI::ResultCode WriteTimestampInternal(RHI::CommandList& commandList) override;
            //////////////////////////////////////////////////////////////////////////

            RHI::QueryControlFlags m_currentControlFlags = RHI::QueryControlFlags::None;
            uint64_t m_resultFenceValue = 0;
        };
    }
}
