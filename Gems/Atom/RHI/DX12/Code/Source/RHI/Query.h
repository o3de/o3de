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
    namespace DX12
    {
        class QueryPool;

        class Query final
            : public RHI::Query
        {
            friend class QueryPool;
            using Base = RHI::Query;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Query, "{87F8BCCF-A4DD-484F-917B-FBE6715F23D6}", Base);
            ~Query() = default;

            static RHI::Ptr<Query> Create();           

        private:
            Query() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Query
            RHI::ResultCode BeginInternal(RHI::CommandList& commandList, RHI::QueryControlFlags flags) override;
            RHI::ResultCode EndInternal(RHI::CommandList& commandList) override;
            RHI::ResultCode WriteTimestampInternal(RHI::CommandList& commandList) override;
            //////////////////////////////////////////////////////////////////////////

            RHI::QueryControlFlags m_currentControlFlags = RHI::QueryControlFlags::None;
            uint64_t m_resultFenceValue = 0;
        };
    }
}