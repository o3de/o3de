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

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>

#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>

#include <Atom/RPI.Public/GpuQuery/Query.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {
        // --- PassScopeProducer ---

        void PassScopeProducer::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            m_parentPass->SetupFrameGraphDependencies(frameGraph, *this);

            m_parentPass->AddScopeQueryToFrameGraph(m_childIndex, frameGraph);
        }

        void PassScopeProducer::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            m_parentPass->CompileResources(context, *this);
        }

        void PassScopeProducer::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            m_parentPass->BeginScopeQuery(m_childIndex, context);

            m_parentPass->BuildCommandList(context, *this);

            m_parentPass->EndScopeQuery(m_childIndex, context);
        }

        // --- RenderPass ---

        RenderPass::RenderPass(const PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            // Read view tag from pass data
            const RenderPassData* passData = PassUtils::GetPassData<RenderPassData>(descriptor);
            if (passData && !passData->m_pipelineViewTag.IsEmpty())
            {
                SetPipelineViewTag(passData->m_pipelineViewTag);
            }
        }

        RenderPass::~RenderPass()
        {
            m_timestampResults.clear();
            m_statisticsResults.clear();
            m_scopeQueries.clear();
            m_scopeProducers.clear();
        }

        const PipelineViewTag& RenderPass::GetPipelineViewTag() const
        {
            return m_viewTag;
        }

        RHI::RenderAttachmentConfiguration RenderPass::GetRenderAttachmentConfiguration() const
        {
            RHI::RenderAttachmentLayoutBuilder builder;
            auto* pass = builder.AddSubpass();

            for (size_t slotIndex = 0; slotIndex < m_attachmentBindings.size(); ++slotIndex)
            {
                const PassAttachmentBinding& binding = m_attachmentBindings[slotIndex];

                // Handle the depth-stencil attachment. There should be only one.
                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                {
                    pass->DepthStencilAttachment(binding.m_attachment->m_descriptor.m_image.m_format);
                    continue;
                }

                // Skip bindings that aren't outputs or inputOutputs
                if (binding.m_slotType != PassSlotType::Output && binding.m_slotType != PassSlotType::InputOutput)
                {
                    continue;
                }

                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget)
                {
                    RHI::Format format = binding.m_attachment->m_descriptor.m_image.m_format;
                    pass->RenderTargetAttachment(format);
                }
            }

            RHI::RenderAttachmentLayout layout;
            RHI::ResultCode result = builder.End(layout);
            AZ_Assert(result == RHI::ResultCode::Success, "RenderPass [%s] failed to create render attachment layout", GetPathName().GetCStr());
            return RHI::RenderAttachmentConfiguration{ layout, 0 };
        }

        RHI::MultisampleState RenderPass::GetMultisampleState() const
        {
            RHI::MultisampleState outputMultiSampleState;
            bool wasSet = false;
            for (size_t slotIndex = 0; slotIndex < m_attachmentBindings.size(); ++slotIndex)
            {
                const PassAttachmentBinding& binding = m_attachmentBindings[slotIndex];
                if (binding.m_slotType != PassSlotType::Output && binding.m_slotType != PassSlotType::InputOutput)
                {
                    continue;
                }

                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget
                    || binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                {
                    if (!wasSet)
                    {
                        // save multi-sample state found in the first output color attachment
                        outputMultiSampleState = binding.m_attachment->m_descriptor.m_image.m_multisampleState;
                        wasSet = true;
                    }
                    else if (PassValidation::IsEnabled())
                    {
                        // return false directly if the current output color attachment has different multi-sample state then previous ones
                        if (outputMultiSampleState != binding.m_attachment->m_descriptor.m_image.m_multisampleState)
                        {
                            AZ_Error("RPI", false, "Pass %s has different multi-sample states within its color attachments", GetPathName().GetCStr());
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }

            return outputMultiSampleState;
        }

        void RenderPass::AddScopeProducer()
        {
            // Scope's index is the position it will be in when we add it to the array
            size_t index = m_scopeProducers.size();

            // The scope's ID = PathNameOfPass.index
            AZStd::string producerName = AZStd::string::format("%s.%zu", GetPathName().GetCStr(), index);

            // Create new scope producer
            PassScopeProducer* producer = aznew PassScopeProducer(RHI::ScopeId(producerName), this, uint16_t(index));

            // And add it to the list
            m_scopeProducers.emplace_back(producer);

            // Add a ScopeQuery instance for the ScopeProducer
            AddScopeQuery();
        }

        void RenderPass::AddScopeQuery()
        {
            // Cache the Timestamp and PipelineStatistics' results for each ScopeProducer
            m_timestampResults.emplace_back();
            m_statisticsResults.emplace_back();

            // Add a ScopeQuery instance for each scope
            const RHI::Ptr<Query> timestampQuery = GpuQuerySystemInterface::Get()->CreateQuery(RHI::QueryType::Timestamp, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            const RHI::Ptr<Query> statisticsQuery = GpuQuerySystemInterface::Get()->CreateQuery(RHI::QueryType::PipelineStatistics, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
            m_scopeQueries.emplace_back(ScopeQuery{ { timestampQuery, statisticsQuery } });
        }

        void RenderPass::ImportScopeProducers(RHI::FrameGraphBuilder* frameGraphBuilder)
        {
            // Update scope producer count
            if (m_scopeProducers.size() != m_numberOfScopes)
            {
                // Clear the Timestamp and PipelineStatistics' result caches
                m_timestampResults.clear();
                m_statisticsResults.clear();
                // Clear the ScopeQueries
                m_scopeQueries.clear();

                // Clear old scope producers and build new ones to match the desired count
                m_scopeProducers.clear();
                for (uint32_t i = 0; i < m_numberOfScopes; ++i)
                {
                    AddScopeProducer();
                }
            }

            // Import scope producers in list
            for (const auto& producer : m_scopeProducers)
            {
                frameGraphBuilder->ImportScopeProducer(*producer.get());
            }
        }

        void RenderPass::OnBuildAttachmentsFinishedInternal()
        {
            if (m_shaderResourceGroup != nullptr)
            {
                Name autoBind = Name("AutoBind");
                Name noBind = Name("NoBind");

                for (PassAttachmentBinding& binding : m_attachmentBindings)
                {
                    const Name& shaderName = binding.m_shaderInputName;
                    PassAttachment* attachment = binding.m_attachment.get();

                    if (shaderName == autoBind)
                    {
                        binding.m_shaderInputIndex = PassAttachmentBinding::ShaderInputAutoBind;
                    }
                    else if (shaderName == noBind)
                    {
                        binding.m_shaderInputIndex = PassAttachmentBinding::ShaderInputNoBind;
                    }
                    else if (attachment)
                    {
                        if (attachment->GetAttachmentType() == RHI::AttachmentType::Image)
                        {
                            RHI::ShaderInputImageIndex idx = m_shaderResourceGroup->FindShaderInputImageIndex(shaderName);
                            AZ_Error("Pass System", idx.IsValid(), "[Pass %s] Could not retrieve Shader Image Index for SRG variable'%s'", GetName().GetCStr(), shaderName.GetCStr());
                            binding.m_shaderInputIndex = idx.IsValid() ? static_cast<int16_t>(idx.GetIndex()) : PassAttachmentBinding::ShaderInputNoBind;
                        }
                        else if (attachment->GetAttachmentType() == RHI::AttachmentType::Buffer)
                        {
                            RHI::ShaderInputBufferIndex idx = m_shaderResourceGroup->FindShaderInputBufferIndex(shaderName);
                            AZ_Error("Pass System", idx.IsValid(), "[Pass %s] Could not retrieve Shader Buffer Index for SRG variable '%s'", GetName().GetCStr(), shaderName.GetCStr());
                            binding.m_shaderInputIndex = idx.IsValid() ? static_cast<int16_t>(idx.GetIndex()) : PassAttachmentBinding::ShaderInputNoBind;
                        }
                    }
                }
            }

            // Need to recreate the dest attachment because the source attachment might be changed
            if (!m_attachmentCopy.expired())
            {
                m_attachmentCopy.lock()->InvalidateDestImage();
            }
        }

        void RenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            ImportScopeProducers(params.m_frameGraphBuilder);

            // Read the attachment for one frame. The reference can be released afterwards
            if (m_attachmentReadback)
            {
                m_attachmentReadback->FrameBegin(params);
                m_attachmentReadback = nullptr;
            }

            // Read back the ScopeQueries submitted from previous frames
            ReadbackScopeQueryResults();
             
            if (!m_attachmentCopy.expired())
            {
                m_attachmentCopy.lock()->FrameBegin(params);
            }
            CollectSrgs();
        }


        void RenderPass::FrameEndInternal()
        {
            ResetSrgs();
        }

        void RenderPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, [[maybe_unused]] const PassScopeProducer& producer)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);
        }

        void RenderPass::DeclareAttachmentsToFrameGraph(RHI::FrameGraphInterface frameGraph) const
        {
            for (const PassAttachmentBinding& attachmentBinding : m_attachmentBindings)
            {
                if (attachmentBinding.m_attachment != nullptr &&
                    frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentBinding.m_attachment->GetAttachmentId()))
                {
                    switch (attachmentBinding.m_unifiedScopeDesc.GetType())
                    {
                    case RHI::AttachmentType::Image:
                    {
                        frameGraph.UseAttachment(attachmentBinding.m_unifiedScopeDesc.GetAsImage(), attachmentBinding.GetAttachmentAccess(), attachmentBinding.m_scopeAttachmentUsage);
                        break;
                    }
                    case RHI::AttachmentType::Buffer:
                    {
                        frameGraph.UseAttachment(attachmentBinding.m_unifiedScopeDesc.GetAsBuffer(), attachmentBinding.GetAttachmentAccess(), attachmentBinding.m_scopeAttachmentUsage);
                        break;
                    }
                    default:
                        AZ_Assert(false, "Error, trying to bind an attachment that is neither an image nor a buffer!");
                        break;
                    }
                }
            }
        }

        void RenderPass::DeclarePassDependenciesToFrameGraph(RHI::FrameGraphInterface frameGraph) const
        {
            for (Pass* pass : m_executeAfterPasses)
            {
                RenderPass* renderPass = azrtti_cast<RenderPass*>(pass);
                if (renderPass)
                {
                    for (const auto& scopeProducer : renderPass->m_scopeProducers)
                    {
                        frameGraph.ExecuteAfter(scopeProducer->GetScopeId());
                    }
                }
            }
            for (Pass* pass : m_executeBeforePasses)
            {
                RenderPass* renderPass = azrtti_cast<RenderPass*>(pass);
                if (renderPass)
                {
                    for (const auto& scopeProducer : renderPass->m_scopeProducers)
                    {
                        frameGraph.ExecuteBefore(scopeProducer->GetScopeId());
                    }
                }
            }
        }

        void RenderPass::BindAttachment(const RHI::FrameGraphCompileContext& context, const PassAttachmentBinding& binding, int16_t& imageIndex, int16_t& bufferIndex)
        {
            if (binding.m_shaderInputIndex == PassAttachmentBinding::ShaderInputNoBind ||
                binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget ||
                binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
            {
                return;
            }

            PassAttachment* attachment = binding.m_attachment.get();
            if (attachment)
            {
                int16_t inputIndex = binding.m_shaderInputIndex;
                uint16_t arrayIndex = binding.m_shaderInputArrayIndex;
                if (attachment->GetAttachmentType() == RHI::AttachmentType::Image)
                {
                    if (inputIndex == PassAttachmentBinding::ShaderInputAutoBind)
                    {
                        inputIndex = imageIndex;
                    }
                    const RHI::ImageView* imageView = context.GetImageView(attachment->GetAttachmentId(), binding.m_attachmentUsageIndex);
                    m_shaderResourceGroup->SetImageView(RHI::ShaderInputImageIndex(inputIndex), imageView, arrayIndex);
                    ++imageIndex;
                }
                else if (attachment->GetAttachmentType() == RHI::AttachmentType::Buffer)
                {
                    if (inputIndex == PassAttachmentBinding::ShaderInputAutoBind)
                    {
                        inputIndex = bufferIndex;
                    }
                    const RHI::BufferView* bufferView = context.GetBufferView(attachment->GetAttachmentId(), binding.m_attachmentUsageIndex);
                    m_shaderResourceGroup->SetBufferView(RHI::ShaderInputBufferIndex(inputIndex), bufferView, arrayIndex);
                    ++bufferIndex;
                }
            }
        }

        void RenderPass::BindPassSrg(const RHI::FrameGraphCompileContext& context, [[maybe_unused]] Data::Instance<ShaderResourceGroup>& shaderResourceGroup)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "Passing a null shader resource group to RenderPass::BindPassSrg");

            int16_t imageIndex = 0;
            int16_t bufferIndex = 0;

            // Bind the input attachments to the SRG
            for (uint32_t idx = 0; idx < GetInputCount(); ++idx)
            {
                PassAttachmentBinding& binding = GetInputBinding(idx);

                AZ_Assert(binding.m_scopeAttachmentUsage != RHI::ScopeAttachmentUsage::RenderTarget,
                    "Attachment bindings that are inputs cannot have their type set to 'RenderTarget'. Binding in question is %s on pass %s.",
                    binding.m_name.GetCStr(),
                    GetPathName().GetCStr());

                BindAttachment(context, binding, imageIndex, bufferIndex);
            }

            // Bind the input/output attachments to the SRG
            for (uint32_t idx = 0; idx < GetInputOutputCount(); ++idx)
            {
                PassAttachmentBinding& binding = GetInputOutputBinding(idx);
                BindAttachment(context, binding, imageIndex, bufferIndex);
            }

            // Bind the output attachments to the SRG
            for (uint32_t idx = 0; idx < GetOutputCount(); ++idx)
            {
                PassAttachmentBinding& binding = GetOutputBinding(idx);
                BindAttachment(context, binding, imageIndex, bufferIndex);
            }
        }

        ViewPtr RenderPass::GetView()
        {
            if (m_flags.m_hasPipelineViewTag && m_pipeline)
            {
                const AZStd::vector<ViewPtr>& views = m_pipeline->GetViews(m_viewTag);
                if (views.size() > 0)
                {
                    return views[0];
                }
            }
            return nullptr;
        }

        void RenderPass::CollectSrgs()
        {
            // Scene srg
            const RHI::ShaderResourceGroup* sceneSrg = m_pipeline->GetScene()->GetRHIShaderResourceGroup();
            BindSrg(sceneSrg);

            // View srg
            ViewPtr view = GetView();
            if (view)
            {
                BindSrg(view->GetRHIShaderResourceGroup());
            }

            // Pass srg
            if (m_shaderResourceGroup)
            {
                BindSrg(m_shaderResourceGroup->GetRHIShaderResourceGroup());
            }
        }

        void RenderPass::ResetSrgs()
        {
            m_shaderResourceGroupsToBind.clear();
        }

        void RenderPass::BindSrg(const RHI::ShaderResourceGroup* srg)
        {
            if (srg)
            {
                m_shaderResourceGroupsToBind.push_back(srg);
            }
        }

        void RenderPass::SetSrgsForDraw(RHI::CommandList* commandList)
        {
            for (const RHI::ShaderResourceGroup* shaderResourceGroup : m_shaderResourceGroupsToBind)
            {
                commandList->SetShaderResourceGroupForDraw(*shaderResourceGroup);
            }
        }

        void RenderPass::SetSrgsForDispatch(RHI::CommandList* commandList)
        {
            for (const RHI::ShaderResourceGroup* shaderResourceGroup : m_shaderResourceGroupsToBind)
            {
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup);
            }
        }

        void RenderPass::SetPipelineViewTag(const PipelineViewTag& viewTag)
        {
            m_viewTag = viewTag;
            m_flags.m_hasPipelineViewTag = !viewTag.IsEmpty();
        }

        void RenderPass::ReadbackAttachment(AZStd::shared_ptr<AttachmentReadback> readback, const PassAttachment* attachment)
        {
            m_attachmentReadback = readback;

            uint32_t bindingIndex = 0;
            for (auto& binding : m_attachmentBindings)
            {
                if (attachment == binding.m_attachment)
                {
                    RHI::AttachmentType type = binding.m_attachment->GetAttachmentType();
                    if (type == RHI::AttachmentType::Buffer || type == RHI::AttachmentType::Image)
                    {
                        RHI::AttachmentId attachmentId = binding.m_attachment->GetAttachmentId();

                        // Append slot index and pass name so the read back's name won't be same as the attachment used in other passes.
                        AZStd::string readbackName = AZStd::string::format("%s_%d_%s", attachmentId.GetCStr(),
                            bindingIndex, GetName().GetCStr());
                        m_attachmentReadback->ReadPassAttachment(binding.m_attachment.get(), AZ::Name(readbackName));
                        return;
                    }
                }
                bindingIndex++;
            }
        }

        TimestampResult RenderPass::GetTimestampResultInternal() const
        {
            // Get the Timestamp result of the current pass by iterating through all scopes, and summing all the results
            return TimestampResult(m_timestampResults);
        }

        PipelineStatisticsResult RenderPass::GetPipelineStatisticsResultInternal() const
        {
            // Get the PipelineStatistics of the current pass by iterating through all scopes, and summing all the results
            return PipelineStatisticsResult(m_statisticsResults);
        }

        Data::Instance<RPI::ShaderResourceGroup> RenderPass::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        RHI::Ptr<Query> RenderPass::GetQuery(uint16_t scopeIdx, ScopeQueryType queryType) const
        {
            return m_scopeQueries[scopeIdx][static_cast<uint32_t>(queryType)];
        }

        template<typename Func>
        inline void RenderPass::ExecuteOnTimestampQuery(uint16_t scopeIdx, Func&& func)
        {
            if (IsTimestampQueryEnabled())
            {
                auto query = GetQuery(scopeIdx, ScopeQueryType::Timestamp);
                if (query)
                {
                    func(query);
                }
            }
        }

        template<typename Func>
        inline void RenderPass::ExecuteOnPipelineStatisticsQuery(uint16_t scopeIdx, Func&& func)
        {
            if (IsPipelineStatisticsQueryEnabled())
            {
                auto query = GetQuery(scopeIdx, ScopeQueryType::PipelineStatistics);
                if (query)
                {
                    func(query);
                }
            }
        }

        void RenderPass::AddScopeQueryToFrameGraph(uint16_t scopeIdx, RHI::FrameGraphInterface frameGraph)
        {
            const auto addToFrameGraph = [&frameGraph](RHI::Ptr<Query> query)
            {
                query->AddToFrameGraph(frameGraph);
            };

            ExecuteOnTimestampQuery(scopeIdx, addToFrameGraph);
            ExecuteOnPipelineStatisticsQuery(scopeIdx, addToFrameGraph);
        }

        void RenderPass::BeginScopeQuery(uint16_t scopeIdx, const RHI::FrameGraphExecuteContext& context)
        {
            const auto beginQuery = [&context](RHI::Ptr<Query> query)
            {
                query->BeginQuery(context);
            };

            ExecuteOnTimestampQuery(scopeIdx, beginQuery);
            ExecuteOnPipelineStatisticsQuery(scopeIdx, beginQuery);
        }

        void RenderPass::EndScopeQuery(uint16_t scopeIdx, const RHI::FrameGraphExecuteContext& context)
        {
            const auto endQuery = [&context](RHI::Ptr<Query> query)
            {
                query->EndQuery(context);
            };

            ExecuteOnTimestampQuery(scopeIdx, endQuery);
            ExecuteOnPipelineStatisticsQuery(scopeIdx, endQuery);
        }

        void RenderPass::ReadbackScopeQueryResults()
        {
            for (uint32_t scopeQueryIdx = 0u; scopeQueryIdx < m_scopeQueries.size(); scopeQueryIdx++)
            {
                ExecuteOnTimestampQuery(scopeQueryIdx, [this, scopeQueryIdx](RHI::Ptr<Query> query)
                {
                    const uint32_t TimestampResultQueryCount = 2u;
                    uint64_t timestampResult[TimestampResultQueryCount];
                    query->GetLatestResult(&timestampResult, sizeof(uint64_t) * TimestampResultQueryCount);
                    m_timestampResults[scopeQueryIdx] = TimestampResult(timestampResult[0], timestampResult[1]);
                });

                ExecuteOnPipelineStatisticsQuery(scopeQueryIdx, [this, scopeQueryIdx](RHI::Ptr<Query> query)
                {
                    query->GetLatestResult(&m_statisticsResults[scopeQueryIdx], sizeof(PipelineStatisticsResult));
                });
            }
        }
    }   // namespace RPI
}   // namespace AZ
