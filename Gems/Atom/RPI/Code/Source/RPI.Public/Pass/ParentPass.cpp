/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/std/containers/vector_set.h>

#include <Atom/RPI.Public/Pass/SlowClearPass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/SlowClearPassData.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
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
            constexpr bool callingFromDestructor = true;
            RemoveChildren(callingFromDestructor);
        }

        // --- Child Pass Addition ---

        void ParentPass::AddChild(const Ptr<Pass>& child, [[maybe_unused]] bool skipStateCheckWhenRunningTests)
        {
            // Todo: investigate if there's a way for this to not trigger on edge cases such as testing, then turn back into an Error instead of Warning.
            if (!(GetPassState() == PassState::Building || IsRootPass() || skipStateCheckWhenRunningTests))
            {
                AZ_Warning("PassSystem", false, "Do not add child passes outside of build phase");
            }

            if (child->m_parent != nullptr)
            {
                AZ_Assert(false, "Can't add Pass that already has a parent. Remove the Pass from it's parent before adding it to another Pass.");
                return;
            }

            child->m_parentChildIndex = static_cast<uint32_t>(m_children.size());
            m_children.push_back(child);
            OnChildAdded(child);
        }

        bool ParentPass::InsertChild(const Ptr<Pass>& child, ChildPassIndex position)
        {
            if (!position.IsValid())
            {
                AZ_Assert(false, "Can't insert a child pass with invalid position");
                return false;
            }
            return InsertChild(child, position.GetIndex());
        }
        
        bool ParentPass::InsertChild(const Ptr<Pass>& child, uint32_t index)
        {
            if (child->m_parent != nullptr)
            {
                AZ_Assert(false, "Can't add Pass that already has a parent. Remove the Pass from it's parent before adding it to another Pass.");
                return false;
            }

            if (index > m_children.size())
            {
                AZ_Assert(false, "Can't insert a child pass with invalid position");
                return false;
            }

            auto insertPos = m_children.cbegin() + index;
            m_children.insert(insertPos, child);
            OnChildAdded(child);

            for (; index < m_children.size(); ++index)
            {
                m_children[index]->m_parentChildIndex = index;
            }

            return true;
        }

        void ParentPass::OnChildAdded(const Ptr<Pass>& child)
        {
            child->m_parent = this;
            child->OnHierarchyChange();

            QueueForBuildAndInitialization();

            // Notify pipeline
            if (m_pipeline)
            {
                m_pipeline->MarkPipelinePassChanges(PipelinePassChanges::PassesAdded);

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
            AZ_Error("PassSystem", GetPassState() == PassState::Resetting || GetPassState() == PassState::Building || IsRootPass() ||
                (GetPassTree() && GetPassTree()->GetPassTreeState() == PassTreeState::RemovingPasses),
                "Do not remove child passes outside of the removal, reset, or build phases.");

            // Find child and move it to the end of the list
            [[maybe_unused]] auto it = AZStd::remove(m_children.begin(), m_children.end(), pass);
            AZ_Assert((it + 1) == m_children.end(), "Pass::RemoveChild found more than one Ptr<Pass> in m_children, which is not allowed.");

            // Delete the child that is now at the end of the list
            m_children.pop_back();

            // Signal child that it was orphaned
            pass->OnOrphan();

            // Update child indices
            for (u32 index = 0; index < m_children.size(); ++index)
            {
                m_children[index]->m_parentChildIndex = index;
            }

            // Notify pipeline
            if (m_pipeline)
            {
                m_pipeline->MarkPipelinePassChanges(PipelinePassChanges::PassesRemoved);
            }
        }

        void ParentPass::RemoveChildren([[maybe_unused]] bool calledFromDestructor)
        {
            AZ_Error("PassSystem", GetPassState() == PassState::Resetting || GetPassState() == PassState::Building || calledFromDestructor ||
                (GetPassTree() && GetPassTree()->GetPassTreeState() == PassTreeState::RemovingPasses),
                "Do not remove child passes outside of the removal, reset, or build phases.");

            for (auto child : m_children)
            {
                child->OnOrphan();
            }
            m_children.clear();

            // Notify pipeline
            if (m_pipeline)
            {
                m_pipeline->MarkPipelinePassChanges(PipelinePassChanges::PassesRemoved);
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

        // --- Child creation ---

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

        void ParentPass::CreateClearPassFromBinding(PassAttachmentBinding& binding, PassRequest& clearRequest)
        {
            if (binding.m_unifiedScopeDesc.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear ||
                binding.m_unifiedScopeDesc.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear)
            {
                // Set the name of the child clear pass as well as the binding it's connected to
                clearRequest.m_passName = ConcatPassName(Name("Clear"), binding.m_name);
                clearRequest.m_connections[0].m_attachmentRef.m_attachment = binding.m_name;

                // Set the pass clear value to the clear value of the attachment binding
                SlowClearPassData* clearData = static_cast<SlowClearPassData*>(clearRequest.m_passData.get());
                clearData->m_clearValue = binding.m_unifiedScopeDesc.m_loadStoreAction.m_clearValue;

                // Create and add the pass
                Ptr<Pass> clearPass = PassSystemInterface::Get()->CreatePassFromRequest(&clearRequest);
                if (clearPass)
                {
                    AddChild(clearPass);
                }
            }

        }

        void ParentPass::CreateClearPassesFromBindings()
        {
            PassRequest clearRequest;
            clearRequest.m_templateName = Name("SlowClearPassTemplate");
            clearRequest.m_passData = AZStd::make_shared<SlowClearPassData>();
            clearRequest.m_connections.emplace_back();
            clearRequest.m_connections[0].m_localSlot = Name("ClearInputOutput");
            clearRequest.m_connections[0].m_attachmentRef.m_pass = Name("Parent");

            for (uint32_t idx = 0; idx < GetInputCount(); ++idx)
            {
                CreateClearPassFromBinding(GetInputBinding(idx), clearRequest);
            }

            for (uint32_t idx = 0; idx < GetInputOutputCount(); ++idx)
            {
                CreateClearPassFromBinding(GetInputOutputBinding(idx), clearRequest);
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
            CreateClearPassesFromBindings();
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
            if (m_pipeline == pipeline)
            {
                return;
            }

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

        AZStd::span<const Ptr<Pass>> ParentPass::GetChildren() const
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
