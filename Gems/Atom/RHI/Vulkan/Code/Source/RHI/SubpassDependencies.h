/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI.Reflect/SubpassDependencies.h>

#include <RHI/RenderPass.h>

namespace AZ::Vulkan
{
    //! GALIB: Add comment
    //! Remark: This is not a Resource!
    class SubpassDependencies final : public RHI::SubpassDependencies
    {
        friend class RenderPass; // Builds the native vulkan data that is encapsulated by this class.
        friend class RenderPassBuilder; // Consumes the data by calling ApplySubpassDependencies()
    public:
        AZ_RTTI(SubpassDependencies, "{E45B8D93-1854-4D16-966F-2388DCC6BB22}", RHI::SubpassDependencies);

        SubpassDependencies() = default;
        virtual ~SubpassDependencies() = default;

    private:
        void ApplySubpassDependencies(RenderPass::Descriptor& dstRenderPassDescriptor) const;

        AZStd::vector<VkSubpassDependency> m_subpassDependencies; // GALIB Add comment
        uint32_t m_subpassCount = 0; // GALIB Add comment
    };
} // namespace AZ::Vulkan
