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

#include <Atom/RHI/Query.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Null
    {
        class Query final
            : public RHI::Query
        {
            friend class QueryPool;
            using Base = RHI::Query;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Query, "{2DD0558E-F422-4268-8C1F-9900818A7B5A}", Base);
            ~Query() = default;

            static RHI::Ptr<Query> Create();           
            
        private:
            Query() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Query
            RHI::ResultCode BeginInternal([[maybe_unused]] RHI::CommandList& commandList, [[maybe_unused]] RHI::QueryControlFlags flags) override { return RHI::ResultCode::Success;}
            RHI::ResultCode EndInternal([[maybe_unused]] RHI::CommandList& commandList) override { return RHI::ResultCode::Success;}
            RHI::ResultCode WriteTimestampInternal([[maybe_unused]] RHI::CommandList& commandList) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
