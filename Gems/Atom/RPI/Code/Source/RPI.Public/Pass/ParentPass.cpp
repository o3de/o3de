/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/std/containers/vector_set.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/PassRequest.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<ParentPass> ParentPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<ParentPass> pass = aznew ParentPass(descriptor);
            return pass;
        }

        Ptr<ParentPass> ParentPass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew ParentPass(desc);
            return pass;
        }

        ParentPass::ParentPass(const PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            m_flags.m_createChildren = true;
        }

        ParentPass::~ParentPass()
        {
            // Explicitly remove children so we call their OnOrphan function
            RemoveChildren();
        }

        // --- Child Pass Addition ---

        void ParentPass::AddChild(const Ptr<Pass>& child)
        {
            AZ_Assert(child->m_parent == nullptr, "Can't add Pass that already has a parent. Remove the Pass from it's parent before adding it to another Pass.");

            m_children.push_back(child);
            child->m_parent = this;
            child->OnHierarchyChange();

            QueueForBuildAndInitialization();

            // Notify pipeline
            if (m_pipeline)
            {
                m_pipeline->SetPassModified();

                // Set child's pipeline if the parent has a owning pipeline
                child->SetRenderPipeline(m_pipeline);
            }
        }

        void ParentPass::OnHierarchyChange()
        {
            Pass::OnHierarchyChange();

            // Notify children about hierarchy change
            for (const Ptr<Pass>& child : m_children)
            {
                child->OnHierarchyChange();
            }
        }

        // --- Child Pass Removal ---

        // The parameter type cannot be a reference as "const Ptr<Pass>&".
        // Otherwise, the content of "pass" will be overwritten by AZStd::remove()
        // when "pass" is a reference of a member of m_children.
        void ParentPass::RemoveChild(Ptr<Pass> pass)
        {
            AZ_Assert(pass->m_parent == this, "Trying to remove a pass of which we are not the parent.");

            // Find child and move it to the end of the list
            [[maybe_unused]] auto it = AZStd::remove(m_children.begin(), m_children.end(), pass);
            AZ_Assert((it + 1) == m_children.end(), "Pass::RemoveChild found more than one Ptr<Pass> in m_children, which is not allowed.");

            // Delete the child that is now at the end of the list
            m_children.pop_back();

            // Signal child that it was orphaned
            pass->OnOrphan();

            // Notify pipeline
            if (m_pipeline)
            {
                m_pipeline->SetPassModified();
            }
        }

        void ParentPass::RemoveChildren()
        {
            for (auto child : m_children)
            {
                child->OnOrphan();
            }
            m_children.clear();

            // Notify pipeline
            if (m_pipeline)
            {
                m_pipeline->SetPassModified();
            }
        }

        void ParentPass::OnOrphan()
        {
            Pass::OnOrphan();

            for (const Ptr<Pass>& child : m_children)
            {
                child->OnHierarchyChange();
            }
        }

        // --- Finders ---

        Pass::ChildPassIndex ParentPass::FindChildPassIndex(const Name& passName) const
        {
            for (uint32_t index = 0; index < m_children.size(); ++index)
            {
                if (passName == m_children[index]->GetName())
                {
                    return ChildPassIndex(index);
                }
            }
            return ChildPassIndex::Null;
        }

        Ptr<Pass> ParentPass::FindChildPass(const Name& passName) const
        {
            ChildPassIndex index = FindChildPassIndex(passName);
            return index.IsValid() ? m_children[index.GetIndex()] : Ptr<Pass>(nullptr);
        }

        const Pass* ParentPass::FindPass(RHI::DrawListTag drawListTag) const
        {
            if (HasDrawListTag() && GetDrawListTag() == drawListTag)
            {
                return this;
            }

            for (const Ptr<Pass>& child : m_children)
            {
                ParentPass* asParent = child->AsParent();
                if (asParent)
                {
                    const Pass* pass = asParent->FindPass(drawListTag);
                    if (pass)
                    {
                        return pass;
                    }
                }
                else if (child->HasDrawListTag() && child->GetDrawListTag() == drawListTag)
                {
                    return child.get();
                }
            }

            return nullptr;
        }

        // --- Timestamp functions ---

        void ParentPass::SetTimestampQueryEnabled(bool enable)
        {
            Pass::SetTimestampQueryEnabled(enable);
            for (auto& child : m_children)
            {
                child->SetTimestampQueryEnabled(enable);
            }
        }

        void ParentPass::SetPipelineStatisticsQueryEnabled(bool enable)
        {
            Pass::SetPipelineStatisticsQueryEnabled(enable);
            for (auto& child : m_children)
            {
                child->SetPipelineStatisticsQueryEnabled(enable);
            }
        }

        // --- PassTemplate related functions ---

        void ParentPass::CreatePassesFromTemplate()
        {
            PassSystemInterface* passSystem = PassSystemInterface::Get();

            if (m_template == nullptr)
            {
                return;
            }

            for (const PassRequest& request : m_template->m_passRequests)
            {
                Ptr<Pass> pass = passSystem->CreatePassFromRequest(&request);
                if (pass)
                {
                    AddChild(pass);
                }
            }
        }

        // --- Pass behavior functions ---

        void ParentPass::CreateChildPasses()
        {
            // The already created flag prevents this function from executing multiple times a frame
            if (!m_flags.m_createChildren || m_flags.m_alreadyCreatedChildren)
            {
                return;
            }
            m_flags.m_alreadyCreatedChildren = true;

            RemoveChildren();
            CreatePassesFromTemplate();
            CreateChildPassesInternal();

            m_flags.m_createChildren = false;
        }

        void ParentPass::ResetInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->Reset();
            }
        }

        void ParentPass::BuildInternal()
        {
            CreateChildPasses();

            for (const Ptr<Pass>& child : m_children)
            {
                child->Build();
            }
        }

        void ParentPass::OnInitializationFinishedInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->OnInitializationFinished();
            }
        }

        void ParentPass::InitializeInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->Initialize();
            }
        }

        void ParentPass::Validate(PassValidationResults& validationResults)
        {
            if (PassValidation::IsEnabled())
            {
                Pass::Validate(validationResults);

                for (const Ptr<Pass>& child : m_children)
                {
                    child->Validate(validationResults);
                }
            }
        }

        void ParentPass::FrameBeginInternal(FramePrepareParams params)
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->FrameBegin(params);
            }
        }

        void ParentPass::FrameEndInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->FrameEnd();
            }
        }

        // --- Misc ---

        void ParentPass::SetRenderPipeline(RenderPipeline* pipeline)
        {
            // Call base implementation
            Pass::SetRenderPipeline(pipeline);

            // Set render pipeline on children
            for (const auto& child : m_children)
            {
                child->SetRenderPipeline(pipeline);
            }
        }

        void ParentPass::GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, PassesByDrawList& outPassesByDrawList, const PipelineViewTag& viewTag) const
        {
            // Call base implementation
            Pass::GetViewDrawListInfo(outDrawListMask, outPassesByDrawList, viewTag);

            // Get mask from children
            for (const auto& child : m_children)
            {
                child->GetViewDrawListInfo(outDrawListMask, outPassesByDrawList, viewTag);
            }
        }

        void ParentPass::GetPipelineViewTags(SortedPipelineViewTags& outTags) const
        {
            // Call base implementation
            Pass::GetPipelineViewTags(outTags);

            // Get pipeline view tags from children
            for (const auto& child : m_children)
            {
                child->GetPipelineViewTags(outTags);
            }
        }

        Ptr<PassAttachment> ParentPass::GetOwnedAttachment(const Name& attachmentName) const
        {
            for (Ptr<PassAttachment> attachment : m_ownedAttachments)
            {
                if (attachment->m_name == attachmentName)
                {
                    return attachment;
                }
            }
            return nullptr;
        }

        // --- Debug functions ---

        AZStd::array_view<Ptr<Pass>> ParentPass::GetChildren() const
        {
            return m_children;
        }

        void ParentPass::DebugPrint() const
        {
            if (PassValidation::IsEnabled())
            {
                Pass::DebugPrint();

                // Print children
                for (const Ptr<Pass>& child : m_children)
                {
                    child->DebugPrint();
                }
            }
        }

        PipelineStatisticsResult ParentPass::GetPipelineStatisticsResultInternal() const
        {
            AZStd::vector<PipelineStatisticsResult> pipelineStatisticsResultArray;
            pipelineStatisticsResultArray.reserve(m_children.size());

            // Calculate the PipelineStatistics result by summing all of its child's PipelineStatistics
            for (const Ptr<Pass>& childPass : m_children)
            {
                pipelineStatisticsResultArray.emplace_back(childPass->GetLatestPipelineStatisticsResult());
            }
            return PipelineStatisticsResult(pipelineStatisticsResultArray);
        }

    }   // namespace RPI
}   // namespace AZ
