/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/smart_ptr/make_shared.h>

#include "SubpassDependencies.h"

namespace AZ::Vulkan
{
    void SubpassDependencies::CopySubpassDependencies(AZStd::vector<VkSubpassDependency>& dstSubpassDependencies) const
    {    
        AZ_Assert(!m_subpassDependencies.empty(), "The Subpass Dependency data should not be empty.");
        dstSubpassDependencies.resize_no_construct(m_subpassDependencies.size());
        AZStd::copy(AZStd::begin(m_subpassDependencies), AZStd::end(m_subpassDependencies), dstSubpassDependencies.begin());
    }


    AZStd::unique_ptr<SubpassDependenciesManager> SubpassDependenciesManager::s_instance;

    /*static*/ SubpassDependenciesManager& SubpassDependenciesManager::GetInstance()
    {
        if (!s_instance)
        {
            s_instance = AZStd::unique_ptr<SubpassDependenciesManager>(aznew SubpassDependenciesManager());
        }
        return *s_instance.get();
    }

    //! Appends VkSubpassDependency data to @subpassDependencies according to the current @subpassIndex.
    //! @param subpassDependencies The output vector that will be enlarged with subpass dependencies for the current @subpassIndex.
    //! @param subpassIndex The current subpass index.
    //! @param subpassLayouts Contains all the render attachment layout data required required by each subpass.
    //! @param subpassCount Defines how many subpasses are actually valid in @subpassLayouts.
    //! @remarks This function should be the ONLY place across all the Vulkan RHI where subpass dependency bitflags
    //!     are defined. This avoids redundancy, and typical VkRenderPass compatibility issues.
    static void AddSubpassDependencies(
        AZStd::vector<VkSubpassDependency>& subpassDependencies,
        const uint32_t subpassIndex,
        const AZStd::array<RHI::SubpassRenderAttachmentLayout, RHI::Limits::Pipeline::SubpassCountMax>& subpassLayouts,
        const uint32_t subpassCount)
    {
        if (subpassCount < 2)
        {
            return; // This is the most common scenario.
        }

        // Typical External dependencies for subpass 0
        if (subpassIndex == 0)
        {
            if (subpassLayouts[subpassIndex].m_depthStencilDescriptor.IsValid())
            {
                subpassDependencies.emplace_back();
                VkSubpassDependency& subpassDependency = subpassDependencies.back();
                subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                subpassDependency.dstSubpass = 0;
                subpassDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                subpassDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                subpassDependency.srcAccessMask = 0;
                subpassDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                subpassDependency.dependencyFlags = 0;
            }
            if (subpassLayouts[subpassIndex].m_rendertargetCount > 0)
            {
                subpassDependencies.emplace_back();
                VkSubpassDependency& subpassDependency = subpassDependencies.back();
                subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                subpassDependency.dstSubpass = 0;
                subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpassDependency.srcAccessMask = 0;
                subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                subpassDependency.dependencyFlags = 0;
            }
        }

        // If the next subpass is valid, then we need to set the dependencey between the current and the next subpass.
        uint32_t nextSubpassIndex = subpassIndex + 1;
        if (nextSubpassIndex < subpassCount)
        {
            subpassDependencies.emplace_back();
            VkSubpassDependency& subpassDependency = subpassDependencies.back();
            subpassDependency.srcSubpass = subpassIndex;
            subpassDependency.dstSubpass = nextSubpassIndex;
            subpassDependency.dependencyFlags =
                VK_DEPENDENCY_BY_REGION_BIT; // The only flag that makes sense in between subpasses for Tiled GPUs.

            if (subpassLayouts[subpassIndex].m_rendertargetCount > 0)
            {
                subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            else
            {
                // Most likely We are talking about a subpass that only has a vertex shader, and it's used for early depth fragment testing.
                // Also there are other pipeline stages before these, so this is a good conservative decision.
                subpassDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                subpassDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }

            if (subpassLayouts[nextSubpassIndex].m_rendertargetCount > 0)
            {
                subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                if (subpassLayouts[nextSubpassIndex].m_subpassInputCount > 0)
                {
                    subpassDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                }
                else
                {
                    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                }
            }
            else
            {
                // Most likely We are talking about a subpass that only has a vertex shader, and it's used for early depth fragment
                // testing. Also there are other pipeline stages before these, so this is a good conservative decision.
                subpassDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                subpassDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
        }
        else
        {
            // Typical External dependency for last subpass
            subpassDependencies.emplace_back();
            VkSubpassDependency& subpassDependency = subpassDependencies.back();
            subpassDependency.srcSubpass = subpassIndex;
            subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            subpassDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        }
    }

    /*static*/ AZStd::shared_ptr<SubpassDependencies> SubpassDependenciesManager::BuildSubpassDependencies(const RHI::RenderAttachmentLayout& layout)
    {
        AZ_Assert(layout.m_subpassCount > 1, "At least two subpasses are needed to build Subpass Dependencies.");
        auto subpassDependenciesPtr = AZStd::make_shared<SubpassDependencies>();
        for (uint32_t subpassIndex = 0; subpassIndex < layout.m_subpassCount; ++subpassIndex)
        {
            AddSubpassDependencies(
                subpassDependenciesPtr->m_subpassDependencies, subpassIndex, layout.m_subpassLayouts, layout.m_subpassCount);
        }
        subpassDependenciesPtr->m_subpassCount = layout.m_subpassCount;
        return subpassDependenciesPtr;
    }

    void SubpassDependenciesManager::SetSubpassDependencies(
        const AZStd::vector<RHI::ScopeId>& scopeIds, AZStd::shared_ptr<SubpassDependencies> subpassDependencies)
    {
        AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
        for (const auto& scopeId : scopeIds)
        {
            m_subpassDependenciesTable[scopeId] = subpassDependencies;
        }
    }

    const SubpassDependencies* SubpassDependenciesManager::GetSubpassDependencies(const RHI::ScopeId& scopeId) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
        const auto itor = m_subpassDependenciesTable.find(scopeId);
        if (itor != m_subpassDependenciesTable.end())
        {
            return itor->second.get();
        }
        return nullptr;
    }

} // namespace AZ::Vulkan
