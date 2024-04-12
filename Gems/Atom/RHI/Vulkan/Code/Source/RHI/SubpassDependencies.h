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
    //! This is the Vulkan concrete implementation of RHI::SubpassDependencies.
    //! Basically this class owns an array of VkSubpassDependency, which is all
    //! the data required in Vulkan to define Subpass Dependencies. 
    class SubpassDependencies final : public RHI::SubpassDependencies
    {
        friend class RenderPass; // Builds the native vulkan data that is encapsulated by this class.
        friend class RenderPassBuilder; // Consumes the data by calling ApplySubpassDependencies()
    public:
        AZ_RTTI(SubpassDependencies, "{E45B8D93-1854-4D16-966F-2388DCC6BB22}", RHI::SubpassDependencies);

        SubpassDependencies() = default;
        virtual ~SubpassDependencies() = default;

        //! RHI::SubpassDependencies override
        bool IsValid() const override;

    private:
        //! Called by AZ::Vulkan::RenderPassBuilder to apply Subpass Dependency data when creating
        //! a VkRenderPass.
        //! @param dstRenderPassDescriptor The recipient/destination structure that needs the
        //!        subpass dependency array.
        void ApplySubpassDependencies(RenderPass::Descriptor& dstRenderPassDescriptor) const;

        //! This is the main blob of data that VkRenderPasses require to know what are the
        //! dependencies between subpasses. This array is created by AZ::Vulkan::RenderPass class.
        AZStd::vector<VkSubpassDependency> m_subpassDependencies;

        //! We store here how many subpasses are connected by @m_subpassDependencies.
        //! Do not assume that m_subpassCount == m_subpassDependencies.size().
        //! This variable is ONLY used for validation purposes when ApplySubpassDependencies() is called
        //! because it helps detect errors when the number of subpasses in dstRenderPassDescriptor does not match.
        uint32_t m_subpassCount = 0;
    };
} // namespace AZ::Vulkan
