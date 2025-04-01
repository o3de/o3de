/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

            static RHI::Ptr<Scope> Create();

        private: 
            Scope() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Scope
            void DeactivateInternal() override {}
            void CompileInternal() override {}
            void AddQueryPoolUse([[maybe_unused]] RHI::Ptr<RHI::QueryPool> queryPool, [[maybe_unused]] const RHI::Interval& interval, [[maybe_unused]] RHI::ScopeAttachmentAccess access) override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
