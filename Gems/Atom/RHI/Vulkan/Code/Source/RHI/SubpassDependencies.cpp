/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/SubpassDependencies.h>

namespace AZ::Vulkan
{
    //! RHI::SubpassDependencies override
    bool SubpassDependencies::IsValid() const
    {
        return (m_subpassCount > 1) && (!m_subpassDependencies.empty());
    }

    void SubpassDependencies::ApplySubpassDependencies(RenderPass::Descriptor& dstRenderPassDescriptor) const
    {
        AZ_Assert(m_subpassCount > 1, "The Subpass Dependency data seems invalid because the subpass count is less than 2.");

        AZ_Assert(!m_subpassDependencies.empty(), "The Subpass Dependency data should not be empty.");

        AZ_Assert(m_subpassCount == dstRenderPassDescriptor.m_subpassCount,
            "Subpass count mismatch. Was expecting %u, but the destination RenderPass::Descriptor has %u subpasses.",
            m_subpassCount, dstRenderPassDescriptor.m_subpassCount);

        dstRenderPassDescriptor.m_subpassDependencies.resize_no_construct(m_subpassDependencies.size());
        AZStd::copy(AZStd::begin(m_subpassDependencies), AZStd::end(m_subpassDependencies), dstRenderPassDescriptor.m_subpassDependencies.begin());
    }

} // namespace AZ::Vulkan
