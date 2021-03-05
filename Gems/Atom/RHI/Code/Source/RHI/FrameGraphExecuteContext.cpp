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