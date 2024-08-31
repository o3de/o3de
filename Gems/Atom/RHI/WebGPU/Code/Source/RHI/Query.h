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
    namespace WebGPU
    {
        class Query final
            : public RHI::DeviceQuery
        {
            friend class QueryPool;
            using Base = RHI::DeviceQuery;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::ThreadPoolAllocator);
            AZ_RTTI(Query, "{108A77D5-87C8-46A7-8B0A-73DEB818D086}", Base);
            ~Query() = default;

            static RHI::Ptr<Query> Create();           
            
        private:
            Query() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceQuery
            RHI::ResultCode BeginInternal([[maybe_unused]] RHI::CommandList& commandList, [[maybe_unused]] RHI::QueryControlFlags flags) override { return RHI::ResultCode::Success;}
            RHI::ResultCode EndInternal([[maybe_unused]] RHI::CommandList& commandList) override { return RHI::ResultCode::Success;}
            RHI::ResultCode WriteTimestampInternal([[maybe_unused]] RHI::CommandList& commandList) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
