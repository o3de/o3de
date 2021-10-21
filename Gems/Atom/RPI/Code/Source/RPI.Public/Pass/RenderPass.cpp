/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>

#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>
#include <Atom/RHI.Reflect/Size.h>

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

                if (!binding.m_attachment)
                {
                    continue;
                }

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
            [[maybe_unused]] RHI::ResultCode result = builder.End(layout);
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
                if (!binding.m_attachment)
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


        void RenderPass::InitializeInternal()
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
                    else
                    {
                        AZ_Error( "Pass System", AZ::RHI::IsNullRenderer(), "[Pass %s] Could not bind shader buffer index '%s' because it has no attachment.", GetName().GetCStr(), shaderName.GetCStr());
                        binding.m_shaderInputIndex = PassAttachmentBinding::ShaderInputNoBind;
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
            if (GetScopeId().IsEmpty())
            {
                SetScopeId(RHI::ScopeId(GetPathName()));
            }

            params.m_frameGraphBuilder->ImportScopeProducer(*this);

            // Read back the ScopeQueries submitted from previous frames
            ReadbackScopeQueryResults();
             
            if (!m_attachmentCopy.expired())
            {
                m_attachmentCopy.lock()->FrameBegin(params);
            }
            CollectSrgs();

            PassSystemInterface::Get()->IncrementFrameRenderPassCount();
        }


        void RenderPass::FrameEndInternal()
        {
            ResetSrgs();
        }

        void RenderPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);
            AddScopeQueryToFrameGraph(frameGraph);
        }

        void RenderPass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            BeginScopeQuery(context);
            BuildCommandListInternal(context);
            EndScopeQuery(context);
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
                    frameGraph.ExecuteAfter(GetScopeId());
                }
            }
            for (Pass* pass : m_executeBeforePasses)
            {
                RenderPass* renderPass = azrtti_cast<RenderPass*>(pass);
                if (renderPass)
                {
                    frameGraph.ExecuteBefore(GetScopeId());
                }
            }
        }

        void RenderPass::BindAttachment(const RHI::FrameGraphCompileContext& context, PassAttachmentBinding& binding, int16_t& imageIndex, int16_t& bufferIndex)
        {
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

                    if (binding.m_shaderImageDimensionsNameIndex.HasName())
                    {
                        RHI::Size size = attachment->m_descriptor.m_image.m_size;

                        AZ::Vector4 imageDimensions;
                        imageDimensions.SetX(float(size.m_width));
                        imageDimensions.SetY(float(size.m_height));
                        imageDimensions.SetZ(1.0f / float(size.m_width));
                        imageDimensions.SetW(1.0f / float(size.m_height));

                        [[maybe_unused]]
                        bool success = m_shaderResourceGroup->SetConstant(binding.m_shaderImageDimensionsNameIndex, imageDimensions);
                        AZ_Assert(success, "Pass [%s] Could not find float4 constant [%s] in Shader Resource Group [%s]",
                            GetPathName().GetCStr(),
                            binding.m_shaderImageDimensionsNameIndex.GetNameForDebug().GetCStr(),
                            m_shaderResourceGroup->GetDatabaseName());
                    }

                    if (binding.m_shaderInputIndex != PassAttachmentBinding::ShaderInputNoBind &&
                        binding.m_scopeAttachmentUsage != RHI::ScopeAttachmentUsage::RenderTarget &&
                        binding.m_scopeAttachmentUsage != RHI::ScopeAttachmentUsage::DepthStencil)
                    {
                        m_shaderResourceGroup->SetImageView(RHI::ShaderInputImageIndex(inputIndex), imageView, arrayIndex);
                        ++imageIndex;
                    }
                }
                else if (attachment->GetAttachmentType() == RHI::AttachmentType::Buffer)
                {
                    if (binding.m_shaderInputIndex == PassAttachmentBinding::ShaderInputNoBind)
                    {
                        return;
                    }

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

        ViewPtr RenderPass::GetView() const
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

        TimestampResult RenderPass::GetTimestampResultInternal() const
        {
            return m_timestampResult;
        }

        PipelineStatisticsResult RenderPass::GetPipelineStatisticsResultInternal() const
        {
            return m_statisticsResult;
        }

        Data::Instance<RPI::ShaderResourceGroup> RenderPass::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        RHI::Ptr<Query> RenderPass::GetQuery(ScopeQueryType queryType)
        {
            uint32_t typeIndex = static_cast<uint32_t>(queryType);
            if (!m_scopeQueries[typeIndex])
            {
                RHI::Ptr<Query> query;
                switch (queryType)
                {
                case ScopeQueryType::Timestamp:
                    query = GpuQuerySystemInterface::Get()->CreateQuery(
                        RHI::QueryType::Timestamp, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
                    break;
                case ScopeQueryType::PipelineStatistics:
                    query = GpuQuerySystemInterface::Get()->CreateQuery(
                        RHI::QueryType::PipelineStatistics, RHI::QueryPoolScopeAttachmentType::Global, RHI::ScopeAttachmentAccess::Write);
                    break;
                }

                m_scopeQueries[typeIndex] = query;
            }

            return m_scopeQueries[typeIndex];
        }

        template<typename Func>
        inline void RenderPass::ExecuteOnTimestampQuery(Func&& func)
        {
            if (IsTimestampQueryEnabled())
            {
                auto query = GetQuery(ScopeQueryType::Timestamp);
                if (query)
                {
                    func(query);
                }
            }
        }

        template<typename Func>
        inline void RenderPass::ExecuteOnPipelineStatisticsQuery(Func&& func)
        {
            if (IsPipelineStatisticsQueryEnabled())
            {
                auto query = GetQuery(ScopeQueryType::PipelineStatistics);
                if (query)
                {
                    func(query);
                }
            }
        }

        void RenderPass::AddScopeQueryToFrameGraph(RHI::FrameGraphInterface frameGraph)
        {
            const auto addToFrameGraph = [&frameGraph](RHI::Ptr<Query> query)
            {
                query->AddToFrameGraph(frameGraph);
            };

            ExecuteOnTimestampQuery(addToFrameGraph);
            ExecuteOnPipelineStatisticsQuery(addToFrameGraph);
        }

        void RenderPass::BeginScopeQuery(const RHI::FrameGraphExecuteContext& context)
        {
            const auto beginQuery = [&context, this](RHI::Ptr<Query> query)
            {
                if (query->BeginQuery(context) == QueryResultCode::Fail)
                {
                    AZ_UNUSED(this); // Prevent unused warning in release builds
                    AZ_WarningOnce("RenderPass", false, "BeginScopeQuery failed. Make sure AddScopeQueryToFrameGraph was called in SetupFrameGraphDependencies"
                        " for this pass: %s", this->RTTI_GetTypeName());
                }
            };

            if (context.GetCommandListIndex() == 0)
            {
                ExecuteOnTimestampQuery(beginQuery);
                ExecuteOnPipelineStatisticsQuery(beginQuery);
            }
        }

        void RenderPass::EndScopeQuery(const RHI::FrameGraphExecuteContext& context)
        {
            const auto endQuery = [&context](RHI::Ptr<Query> query)
            {
                query->EndQuery(context);
            };

            // This scopy query implmentation should be replaced by
            // [ATOM-5407] [RHI][Core] - Add GPU timestamp and pipeline statistic support for scopes
            
            // For timestamp query, it's okay to execute across different command lists
            if (context.GetCommandListIndex() == context.GetCommandListCount() - 1)
            {
                ExecuteOnTimestampQuery(endQuery);
            }
            // For all the other types of queries except timestamp, the query start and end has to be in the same command list
            // Here only tracks the PipelineStatistics for the first command list due to that we don't know how many queries are
            // needed when AddScopeQueryToFrameGraph is called.
            // This implementation leads to an issue that we may not get accurate pipeline statistic data
            // for passes which were executed with more than one command list
            if (context.GetCommandListIndex() == 0)
            {
                ExecuteOnPipelineStatisticsQuery(endQuery);
            }
        }

        void RenderPass::ReadbackScopeQueryResults()
        {
            ExecuteOnTimestampQuery([this](RHI::Ptr<Query> query)
            {
                const uint32_t TimestampResultQueryCount = 2u;
                uint64_t timestampResult[TimestampResultQueryCount] = {0};
                query->GetLatestResult(&timestampResult, sizeof(uint64_t) * TimestampResultQueryCount);
                m_timestampResult = TimestampResult(timestampResult[0], timestampResult[1], RHI::HardwareQueueClass::Graphics);
            });

            ExecuteOnPipelineStatisticsQuery([this](RHI::Ptr<Query> query)
            {
                query->GetLatestResult(&m_statisticsResult, sizeof(PipelineStatisticsResult));
            });
        }
    }   // namespace RPI
}   // namespace AZ
