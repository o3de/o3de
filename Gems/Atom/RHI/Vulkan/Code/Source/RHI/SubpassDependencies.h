/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>

namespace AZ::Vulkan
{
    //! This class owns an array of VkSubpassDependency, which is all
    //! the data required in Vulkan to define Subpass Dependencies. 
    class SubpassDependencies final
    {
        friend class SubpassDependenciesManager;
        friend class RenderPass;
        friend class RenderPassBuilder;
    public:
        AZ_RTTI(SubpassDependencies, "{E45B8D93-1854-4D16-966F-2388DCC6BB22}");

        SubpassDependencies() = default;
        virtual ~SubpassDependencies() = default;

    private:
        // Copies the content of @m_subpassDependencies into @dstSubpassDependencies.
        void CopySubpassDependencies(AZStd::vector<VkSubpassDependency>& dstSubpassDependencies) const;

        //! This is the main blob of data that VkRenderPasses require to know what are the
        //! dependencies between subpasses.
        AZStd::vector<VkSubpassDependency> m_subpassDependencies;

        //! We store here how many subpasses are connected by @m_subpassDependencies.
        //! Do not assume that m_subpassCount == m_subpassDependencies.size().
        //! This variable is ONLY used for validation purposes by RenderPassBuilder..
        uint32_t m_subpassCount = 0;
    };

    //! This is a static singleton. Its main purpose is to keep a table of the SubpassDependencies
    //! used by all scopes that function as Vulkan Subpasses.
    class SubpassDependenciesManager final
    {
    public:
        static SubpassDependenciesManager& GetInstance();

        static AZStd::shared_ptr<SubpassDependencies> BuildSubpassDependencies(const RHI::RenderAttachmentLayout& layout);

        //! The following two functions are thread safe.
        void SetSubpassDependencies(const AZStd::vector<RHI::ScopeId>& scopeIds, AZStd::shared_ptr<SubpassDependencies> subpassDependencies);
        const SubpassDependencies* GetSubpassDependencies(const RHI::ScopeId& scopeId) const;
        
    private:
        SubpassDependenciesManager() = default;

        static AZStd::unique_ptr<SubpassDependenciesManager> s_instance;

        //! Protects @m_subpassDependenciesTable.
        mutable AZStd::shared_mutex m_mutex;
        //! The reason the value is a shared_ptr is because different ScopeIds can share the same SubpassDependencies
        AZStd::unordered_map<RHI::ScopeId, AZStd::shared_ptr<SubpassDependencies>> m_subpassDependenciesTable;
    };
} // namespace AZ::Vulkan
