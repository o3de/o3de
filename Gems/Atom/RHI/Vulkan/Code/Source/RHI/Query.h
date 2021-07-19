/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Query.h>

namespace AZ
{
    namespace Vulkan
    {
        class CommandList;

        class Query final
            : public RHI::Query
        {
            using Base = RHI::Query;

        public:
            AZ_RTTI(Query, "{E27876FA-D96D-407A-926A-C480F4EDCBD0}", Base);
            AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator, 0);
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
        };
    }
}
