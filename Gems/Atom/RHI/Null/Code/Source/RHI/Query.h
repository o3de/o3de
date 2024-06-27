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
    namespace Null
    {
        class Query final
            : public RHI::DeviceQuery
        {
            friend class QueryPool;
            using Base = RHI::DeviceQuery;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::ThreadPoolAllocator);
            AZ_RTTI(Query, "{2DD0558E-F422-4268-8C1F-9900818A7B5A}", Base);
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
