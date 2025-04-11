/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FrameGraphExecuteGroupBase.h>

namespace AZ
{
    namespace DX12
    {
        class FrameGraphExecuteGroup final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroup, AZ::PoolAllocator);

            FrameGraphExecuteGroup() = default;

            void Init(
                Device& device,
                const Scope& scope,
                uint32_t commandListCount,
                RHI::JobPolicy globalJobPolicy);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameSchedulerExecuteGroup
            void BeginContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            //////////////////////////////////////////////////////////////////////////

            const Scope* m_scope = nullptr;
        };
    }
}
