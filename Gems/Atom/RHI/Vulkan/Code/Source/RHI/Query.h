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
