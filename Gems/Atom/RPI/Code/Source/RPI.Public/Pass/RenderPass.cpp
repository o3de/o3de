/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RHI/RHIUtils.h>

#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>

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
            m_flags.m_canBecomeASubpass = true;
            // Read view tag from pass data
            const RenderPassData* passData = PassUtils::GetPassData<RenderPassData>(descriptor);
            if (passData)
            {
                if (!passData->m_pipelineViewTag.IsEmpty())
                {
                    SetPipelineViewTag(passData->m_pipelineViewTag);
                }
                if (passData->m_bindViewSrg)
                {
                    m_flags.m_bindViewSrg = true;
                }
                m_flags.m_canBecomeASubpass = passData->m_canBecomeASubpass;
            }
        }

        RenderPass::~RenderPass()
        {
        }

        bool RenderPass::CanBecomeSubpass() const
        {
            return m_flags.m_canBecomeASubpass;
        }

        RHI::RenderAttachmentConfiguration RenderPass::GetRenderAttachmentConfiguration()
        {
            AZ_Assert(
                m_renderAttachmentConfiguration.has_value(), "Null RenderAttachmentConfiguration for pass [%s]", GetPathName().GetCStr());
            return m_renderAttachmentConfiguration.value();
        }

        void RenderPass::SetRenderAttachmentConfiguration(
            const RHI::RenderAttachmentConfiguration& configuration, const AZ::RHI::ScopeGroupId& subpassGroupId)
        {
            m_renderAttachmentConfiguration = configuration;
            m_subpassGroupId = subpassGroupId;
        }

        bool RenderPass::BuildSubpassLayout(RHI::RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder& subpassLayoutBuilder)
        {
            // Replace all subpass inputs as shader inputs if we are the first subpass in the group.
            // This could happen if we have a subpass group that could be merged with other group(s), but it didn't happen
            // due to some pass breaking the subpass chaining.
            if (m_flags.m_hasSubpassInput && subpassLayoutBuilder.GetSubpassIndex() == 0)
            {
                ReplaceSubpassInputs(RHI::SubpassInputSupportType::None);
            }

            for (size_t slotIndex = 0; slotIndex < m_attachmentBindings.size(); ++slotIndex)
            {
                const PassAttachmentBinding& binding = m_attachmentBindings[slotIndex];

                if (!binding.GetAttachment())
                {
                    continue;
                }

                // Handle the depth-stencil attachment. There should be only one.
                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                {
                    subpassLayoutBuilder.DepthStencilAttachment(
                        binding.GetAttachment()->m_descriptor.m_image.m_format,
                        binding.GetAttachment()->GetAttachmentId(),
                        binding.m_unifiedScopeDesc.m_loadStoreAction,
                        binding.GetAttachmentAccess(),
                        binding.m_scopeAttachmentStage);
                    continue;
                }

                // Handle shading rate attachment. There should be only one.
                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::ShadingRate)
                {
                    subpassLayoutBuilder.ShadingRateAttachment(
                        binding.GetAttachment()->m_descriptor.m_image.m_format, binding.GetAttachment()->GetAttachmentId());
                    continue;
                }

                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::SubpassInput)
                {
                    AZ_Assert(subpassLayoutBuilder.GetSubpassIndex() > 0, "The first subpass can't have attachments used as SubpassInput");
                    AZ_Assert(
                        binding.m_unifiedScopeDesc.GetType() == RHI::AttachmentType::Image,
                        "Only image attachments are allowed as SubpassInput.");
                    const auto aspectFlags = binding.m_unifiedScopeDesc.GetAsImage().m_imageViewDescriptor.m_aspectFlags;
                    subpassLayoutBuilder.SubpassInputAttachment(
                        binding.GetAttachment()->GetAttachmentId(),
                        aspectFlags,
                        binding.m_unifiedScopeDesc.m_loadStoreAction);
                    continue;
                }

                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget)
                {
                    RHI::Format format = binding.GetAttachment()->m_descriptor.m_image.m_format;
                    subpassLayoutBuilder.RenderTargetAttachment(
                        format,
                        binding.GetAttachment()->GetAttachmentId(),
                        binding.m_unifiedScopeDesc.m_loadStoreAction,
                        false /*resolve*/);
                    continue;
                }

                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::Resolve)
                {
                    // A Resolve attachment must be declared immediately after the RenderTarget it is supposed to resolve.
                    AZ_Assert(slotIndex > 0, "A Resolve attachment can not be in the first slot binding.");
                    const auto& renderTargetBinding = m_attachmentBindings[slotIndex - 1];
                    AZ_Assert(renderTargetBinding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget,
                        "A Resolve attachment must be declared immediately after a RenderTarget attachment.");
                    subpassLayoutBuilder.ResolveAttachment(renderTargetBinding.GetAttachment()->GetAttachmentId(), binding.GetAttachment()->GetAttachmentId());
                    continue;
                }
            }

            return true;
        }

        void RenderPass::BuildRenderAttachmentConfiguration()
        {
            if (m_renderAttachmentConfiguration)
            {
                // Already has a render attachment configuration. Nothing to do.
                return;
            }

            RHI::RenderAttachmentLayoutBuilder builder;
            auto* layoutBuilder = builder.AddSubpass();
            BuildSubpassLayout(*layoutBuilder);
            if (!layoutBuilder->HasAttachments())
            {
                return;
            }

            RHI::RenderAttachmentLayout subpassLayout;
            [[maybe_unused]] RHI::ResultCode result = builder.End(subpassLayout);
            AZ_Assert(
                result == RHI::ResultCode::Success, "RenderPass [%s] failed to create render attachment configuration", GetPathName().GetCStr());
            m_renderAttachmentConfiguration = RHI::RenderAttachmentConfiguration{ AZStd::move(subpassLayout), 0 };
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
                if (!binding.GetAttachment())
                {
                    continue;
                }

                if (binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget
                    || binding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                {
                    if (!wasSet)
                    {
                        // save multi-sample state found in the first output color attachment
                        outputMultiSampleState = binding.GetAttachment()->m_descriptor.m_image.m_multisampleState;
                        wasSet = true;
                    }
                    else if (PassValidation::IsEnabled())
                    {
                        // return false directly if the current output color attachment has different multi-sample state then previous ones
                        if (outputMultiSampleState != binding.GetAttachment()->m_descriptor.m_image.m_multisampleState)
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
                    PassAttachment* attachment = binding.GetAttachment().get();

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
                        AZ_Error( "Pass System", AZ::RHI::IsNullRHI(), "[Pass %s] Could not bind shader buffer index '%s' because it has no attachment.", GetName().GetCStr(), shaderName.GetCStr());
                        binding.m_shaderInputIndex = PassAttachmentBinding::ShaderInputNoBind;
                    }
                }
            }
            BuildRenderAttachmentConfiguration();
        }

        void RenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (IsTimestampQueryEnabled())
            {
                m_timestampResult = AZ::RPI::TimestampResult();
            }

            // the pass may potentially migrate between devices dynamically at runtime so the deviceIndex is updated every frame.
            if (GetScopeId().IsEmpty() || (ScopeProducer::GetDeviceIndex() != Pass::GetDeviceIndex()))
            {
                InitScope(RHI::ScopeId(GetPathName()), m_hardwareQueueClass, Pass::GetDeviceIndex());
            }

            params.m_frameGraphBuilder->ImportScopeProducer(*this);

            // Read back the ScopeQueries submitted from previous frames
            ReadbackScopeQueryResults();

            CollectSrgs();

            PassSystemInterface::Get()->IncrementFrameRenderPassCount();
        }


        void RenderPass::FrameEndInternal()
        {
            ResetSrgs();
        }

        void RenderPass::ResetInternal()
        {
            m_renderAttachmentConfiguration.reset();
            m_subpassGroupId = RHI::ScopeGroupId();
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

            m_lastDeviceIndex = context.GetDeviceIndex();
        }

        void RenderPass::DeclarePassDependenciesToFrameGraph(RHI::FrameGraphInterface frameGraph) const
        {
            for (Pass* pass : m_executeAfterPasses)
            {
                RenderPass* renderPass = azrtti_cast<RenderPass*>(pass);
                if (renderPass)
                {
                    frameGraph.ExecuteAfter(renderPass->GetScopeId());
                }
            }
            for (Pass* pass : m_executeBeforePasses)
            {
                RenderPass* renderPass = azrtti_cast<RenderPass*>(pass);
                if (renderPass)
                {
                    frameGraph.ExecuteBefore(renderPass->GetScopeId());
                }
            }
            frameGraph.SetGroupId(GetSubpassGroupId());
        }

        void RenderPass::BindAttachment(const RHI::FrameGraphCompileContext& context, PassAttachmentBinding& binding, int16_t& imageIndex, int16_t& bufferIndex)
        {
            PassAttachment* attachment = binding.GetAttachment().get();
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
                    const RHI::ImageView* imageView =
                        context.GetImageView(attachment->GetAttachmentId(), binding.m_unifiedScopeDesc.GetImageViewDescriptor(), binding.m_scopeAttachmentUsage);

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
                        binding.m_scopeAttachmentUsage != RHI::ScopeAttachmentUsage::DepthStencil &&
                        binding.m_scopeAttachmentUsage != RHI::ScopeAttachmentUsage::Resolve)
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
                    const RHI::BufferView* bufferView = context.GetBufferView(attachment->GetAttachmentId(), binding.m_scopeAttachmentUsage);
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
            if (m_pipeline)
            {
                return m_pipeline->GetFirstView(GetPipelineViewTag());
            }
            return nullptr;
        }

        void RenderPass::CollectSrgs()
        {
            // Scene srg
            const RHI::ShaderResourceGroup* sceneSrg = m_pipeline->GetScene()->GetRHIShaderResourceGroup();
            BindSrg(sceneSrg);

            // View srg
            if (m_flags.m_bindViewSrg)
            {
                ViewPtr view = GetView();
                if (view)
                {
                    BindSrg(view->GetRHIShaderResourceGroup());
                }
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
                m_shaderResourceGroupsToBind[aznumeric_caster(srg->GetBindingSlot())] = srg;
            }
        }

        void RenderPass::SetSrgsForDraw(const RHI::FrameGraphExecuteContext& context)
        {
            for (auto itr : m_shaderResourceGroupsToBind)
            {
                context.GetCommandList()->SetShaderResourceGroupForDraw(*(itr.second->GetDeviceShaderResourceGroup(context.GetDeviceIndex())));
            }
        }

        void RenderPass::SetSrgsForDispatch(const RHI::FrameGraphExecuteContext& context)
        {
            for (auto itr : m_shaderResourceGroupsToBind)
            {
                context.GetCommandList()->SetShaderResourceGroupForDispatch(
                    *(itr.second->GetDeviceShaderResourceGroup(context.GetDeviceIndex())));
            }
        }

        void RenderPass::SetPipelineViewTag(const PipelineViewTag& viewTag)
        {
            if (m_viewTag != viewTag)
            {
                m_viewTag = viewTag;
                if (m_pipeline)
                {
                    m_pipeline->MarkPipelinePassChanges(PipelinePassChanges::PipelineViewTagChanged);
                }
            }
            m_flags.m_bindViewSrg = !viewTag.IsEmpty();
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

        const AZ::RHI::ScopeGroupId& RenderPass::GetSubpassGroupId() const
        {
            return m_subpassGroupId;
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

            // This scope query implementation should be replaced by
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
                query->GetLatestResult(&timestampResult, sizeof(uint64_t) * TimestampResultQueryCount, m_lastDeviceIndex);
                m_timestampResult = TimestampResult(timestampResult[0], timestampResult[1], RHI::HardwareQueueClass::Graphics);
            });

            ExecuteOnPipelineStatisticsQuery([this](RHI::Ptr<Query> query)
            {
                query->GetLatestResult(&m_statisticsResult, sizeof(PipelineStatisticsResult), m_lastDeviceIndex);
            });
        }
    }   // namespace RPI
}   // namespace AZ
