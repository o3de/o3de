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

#include <Atom/RHI/Scope.h>

namespace AZ
{
    namespace Null
    {
        class Scope final
            : public RHI::Scope
        {
            using Base = RHI::Scope;
        public:
            AZ_RTTI(Scope, "{2F46C8DD-08CB-427C-8EE7-7D00F140A22F}", Base);
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator, 0);

            static RHI::Ptr<Scope> Create();

        private: 
            Scope() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Scope
            void DeactivateInternal() override {}
            void CompileInternal([[maybe_unused]] RHI::Device& device) override {}
            void AddQueryPoolUse([[maybe_unused]] RHI::Ptr<RHI::QueryPool> queryPool, [[maybe_unused]] const RHI::Interval& interval, [[maybe_unused]] RHI::ScopeAttachmentAccess access) override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
