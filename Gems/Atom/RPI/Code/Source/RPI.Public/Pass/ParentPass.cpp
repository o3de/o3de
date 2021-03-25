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
            CreateChildPasses();
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

            QueueForBuildAttachments();

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
            auto it = AZStd::remove(m_children.begin(), m_children.end(), pass);
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

        Ptr<Pass> ParentPass::FindPassByNameRecursive(const Name& passName) const
        {
            for (const Ptr<Pass>& child : m_children)
            {
                if (child->GetName() == passName)
                {
                    return child.get();
                }

                ParentPass* asParent = child->AsParent();
                if (asParent)
                {
                    auto pass = asParent->FindPassByNameRecursive(passName);
                    if (pass)
                    {
                        return pass;
                    }
                }
            }

            return nullptr;
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
            // Flag prevents the function from executing multiple times a frame. Can happen
            // as pass system has a list of passes for which it needs to call this function.
            if (m_flags.m_alreadyCreated)
            {
                return;
            }
            m_flags.m_alreadyCreated = true;
            RemoveChildren();
            CreatePassesFromTemplate();
            CreateChildPassesInternal();

            for (Ptr<Pass>& child : m_children)
            {
                ParentPass* asParent = child->AsParent();
                if (asParent != nullptr)
                {
                    asParent->CreateChildPasses();
                }
            }
        }

        void ParentPass::ResetInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->Reset();
            }
        }

        void ParentPass::BuildAttachmentsInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->BuildAttachments();
            }
        }

        void ParentPass::OnBuildAttachmentsFinishedInternal()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->OnBuildAttachmentsFinished();
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

        TimestampResult ParentPass::GetTimestampResultInternal() const
        {
            AZStd::vector<TimestampResult> timestampResultArray;
            timestampResultArray.reserve(m_children.size());

            // Calculate the Timestamp result by summing all of its child's TimestampResults
            for (const Ptr<Pass>& childPass : m_children)
            {
                timestampResultArray.emplace_back(childPass->GetTimestampResult());
            }
            return TimestampResult(timestampResultArray);
        }

        PipelineStatisticsResult ParentPass::GetPipelineStatisticsResultInternal() const
        {
            AZStd::vector<PipelineStatisticsResult> pipelineStatisticsResultArray;
            pipelineStatisticsResultArray.reserve(m_children.size());

            // Calculate the PipelineStatistics result by summing all of its child's PipelineStatistics
            for (const Ptr<Pass>& childPass : m_children)
            {
                pipelineStatisticsResultArray.emplace_back(childPass->GetPipelineStatisticsResult());
            }
            return PipelineStatisticsResult(pipelineStatisticsResultArray);
        }

    }   // namespace RPI
}   // namespace AZ
