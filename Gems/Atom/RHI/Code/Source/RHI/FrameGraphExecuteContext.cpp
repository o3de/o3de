/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphExecuteContext.h>

namespace AZ::RHI
{
    FrameGraphExecuteContext::FrameGraphExecuteContext(const Descriptor& descriptor)
        : m_descriptor{descriptor}
    {}

    void FrameGraphExecuteContext::SetCommandList(CommandList& commandList)
    {
        m_descriptor.m_commandList = &commandList;
        m_descriptor.m_commandList->SetSubmitRange(m_descriptor.m_submitRange);
    }
}
