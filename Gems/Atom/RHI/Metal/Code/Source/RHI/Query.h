/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Query.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Metal
    {
        class QueryPool;

        class Query final
            : public RHI::Query
        {
            friend class QueryPool;
            using Base = RHI::Query;
        public:
            AZ_CLASS_ALLOCATOR(Query, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Query, "{07E43C0C-A2BD-4DD1-B0F2-F4C62BE023E6}", Base);
            ~Query() = default;

            static RHI::Ptr<Query> Create();           
            Device& GetDevice() const;
            const bool IsCommandBufferCompleted() const;

        private:
            Query() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Query
            RHI::ResultCode BeginInternal(RHI::CommandList& commandList, RHI::QueryControlFlags flags) override;
            RHI::ResultCode EndInternal(RHI::CommandList& commandList) override;
            RHI::ResultCode WriteTimestampInternal(RHI::CommandList& commandList) override;
            //////////////////////////////////////////////////////////////////////////

            RHI::QueryControlFlags m_currentControlFlags = RHI::QueryControlFlags::None;
            bool m_commandBufferCompleted = false;
        };
    }
}
