/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphExecuteContext.h>

namespace AZ
{
    namespace RHI
    {
        FrameGraphExecuteContext::FrameGraphExecuteContext(const Descriptor& descriptor)
            : m_descriptor{descriptor}
        {}

        const ScopeId& FrameGraphExecuteContext::GetScopeId() const
        {
            return m_descriptor.m_scopeId;
        }

        uint32_t FrameGraphExecuteContext::GetCommandListIndex() const
        {
            return m_descriptor.m_commandListIndex;
        }

        uint32_t FrameGraphExecuteContext::GetCommandListCount() const
        {
            return m_descriptor.m_commandListCount;
        }

        CommandList* FrameGraphExecuteContext::GetCommandList() const
        {
            return m_descriptor.m_commandList;
        }

        void FrameGraphExecuteContext::SetCommandList(CommandList& commandList)
        {
            m_descriptor.m_commandList = &commandList;
        }
    }
}
