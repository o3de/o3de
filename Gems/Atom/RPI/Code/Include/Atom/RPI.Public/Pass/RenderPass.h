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
#pragma once

#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraph;
        class FrameGraphCompileContext;
        class FrameGraphExecuteContext;
    }

    namespace RPI
    {
        class ImageAttachmentCopy;
        class RenderPass;
        class Query;

        //! Implements RHI::ScopeProducer by simply forwarding it's
        //! virtual function calls to it's parent RenderPass
        class PassScopeProducer
            : public RHI::ScopeProducer
        {
        public:
            AZ_CLASS_ALLOCATOR(PassScopeProducer, SystemAllocator, 0);

            PassScopeProducer(const RHI::ScopeId& scopeId, RenderPass* parent, uint16_t childIndex)
                : ScopeProducer(scopeId)
                , m_parentPass(parent)
                , m_childIndex(childIndex)
            { }

            virtual ~PassScopeProducer() = default;

        private:

            // RHI::ScopeProducer overrides
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override final;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override final;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override final;

            // Pointer to the RenderPass that owns this scope producer
            RenderPass* m_parentPass;

            // Index so that the scope producer can identify itself to the parent pass during callbacks
            uint16_t m_childIndex;
        };

        //! A RenderPass is a leaf Pass (i.e. a Pass that has no children) that 
        //! implements rendering functionality (raster, compute, copy)
        class RenderPass
            : public Pass
        {
            AZ_RPI_PASS(RenderPass);

            friend class PassScopeProducer;
            friend class ImageAttachmentPreviewPass;

            using ScopeQuery = AZStd::array<RHI::Ptr<Query>, static_cast<size_t>(ScopeQueryType::Count)>;

        public:
            AZ_RTTI(RenderPass, "{9441D114-60FD-487B-B2B7-0FBBC8A96FC2}", Pass);
            AZ_CLASS_ALLOCATOR(RenderPass, SystemAllocator, 0);
            virtual ~RenderPass();

            //! Build and return RenderAttachmentConfiguration of this pass from its render attachments
            //! This function usually need to be called after pass attachments rebuilt to reflect latest layout
            RHI::RenderAttachmentConfiguration GetRenderAttachmentConfiguration() const;

            //! Get MultisampleState of this pass from its output attachments
            RHI::MultisampleState GetMultisampleState() const;

            //! Capture pass's output and input/output attachments in following frames
            void ReadbackAttachment(AZStd::shared_ptr<AttachmentReadback> readback, const PassAttachment* attachment);

            //! Returns a pointer to the Pass ShaderResourceGroup
            Data::Instance<ShaderResourceGroup> GetShaderResourceGroup();

            // Pass overrides...
            const PipelineViewTag& GetPipelineViewTag() const override;

            //! Return the View if this pass is associated with a pipeline view via PipelineViewTag.
            //! It may return nullptr if this pass is independent with any views.
            ViewPtr GetView();

        protected:
            explicit RenderPass(const PassDescriptor& descriptor);

            // Adds a new scope producer to the list
            void AddScopeProducer();

            // Imports all the RenderPass's scope producers into the FrameGraphBuilder
            void ImportScopeProducers(RHI::FrameGraphBuilder* frameGraphBuilder);

            // Binds all attachments from the pass 
            void DeclareAttachmentsToFrameGraph(RHI::FrameGraphInterface frameGraph) const;

            // Declares explicitly set dependencies between passes (execute after and execute before)
            // Note most pass ordering is determined by attachments. This is only used for
            // dependencies between passes that don't have any attachments/connections in common.
            void DeclarePassDependenciesToFrameGraph(RHI::FrameGraphInterface frameGraph) const;

            // Binds the pass's attachments to the provide SRG (this will usually be the pass's own SRG)
            void BindPassSrg(const RHI::FrameGraphCompileContext& context, Data::Instance<ShaderResourceGroup>& shaderResourceGroup);

            // Pass behavior overrides
            void OnBuildAttachmentsFinishedInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // Helper functions for srgs used for pass
            // Collect low frequency srgs for draw or compute. These srgs include scene srg, view srg and pass srg
            void CollectSrgs();

            // Clear the srg list
            void ResetSrgs();

            // Add a srg to srg list to be bound for this pass
            void BindSrg(const RHI::ShaderResourceGroup* srg);
            
            // Set srgs for pass's execution
            void SetSrgsForDraw(RHI::CommandList* commandList);
            void SetSrgsForDispatch(RHI::CommandList* commandList);

            // Set PipelineViewTag associated for this pass
            // If the View bound to the tag exists,the view's srg will be collected to pass' srg bind list
            void SetPipelineViewTag(const PipelineViewTag& viewTag);

            // The shader resource group for this pass
            Data::Instance<ShaderResourceGroup> m_shaderResourceGroup = nullptr;

            // Desired number of scopes for this RenderPass
            uint32_t m_numberOfScopes = 1;

            // Scope producer functions - derived classes must implements these
            virtual void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer);
            virtual void CompileResources(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer) = 0;
            virtual void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const PassScopeProducer& producer) = 0;

        private:
            // Helper function that binds a single attachment to the pass shader resource group
            void BindAttachment(const RHI::FrameGraphCompileContext& context, const PassAttachmentBinding& binding, int16_t& imageIndex, int16_t& bufferIndex);

            // Helper function to get the query by the scope index and query type
            RHI::Ptr<Query> GetQuery(uint16_t scopeIdx, ScopeQueryType queryType) const;

            // Executes a lambda depending on the passed ScopeQuery types
            template <typename Func>
            void ExecuteOnTimestampQuery(uint16_t scopeIdx, Func&& func);
            template <typename Func>
            void ExecuteOnPipelineStatisticsQuery(uint16_t scopeIdx, Func&& func);

            // RPI::Pass overrides...
            TimestampResult GetTimestampResultInternal() const override;
            PipelineStatisticsResult GetPipelineStatisticsResultInternal() const override;

            // Adds a ScopeQuery entry
            void AddScopeQuery();

            // Add the ScopeQuery's QueryPool to the FrameGraph
            void AddScopeQueryToFrameGraph(uint16_t scopeIdx, RHI::FrameGraphInterface frameGraph);

            // Begin recording commands for the ScopeQueries 
            void BeginScopeQuery(uint16_t scopeIdx, const RHI::FrameGraphExecuteContext& context);

            // End recording commands for the ScopeQueries
            void EndScopeQuery(uint16_t scopeIdx, const RHI::FrameGraphExecuteContext& context);

            // Readback the results from the ScopeQueries
            void ReadbackScopeQueryResults();

            // Scope producers used to render the pass (in cases like split-screen where
            // there are multiple views, the pass will have one scope producer per view)
            AZStd::vector<AZStd::unique_ptr<PassScopeProducer>> m_scopeProducers;

            // For read back attachments
            AZStd::shared_ptr<AttachmentReadback> m_attachmentReadback;

            AZStd::weak_ptr<ImageAttachmentCopy> m_attachmentCopy;

            // Readback results from the Timestamp queries
            AZStd::vector<TimestampResult> m_timestampResults;
            // Readback results from the PipelineStatistics queries
            AZStd::vector<PipelineStatisticsResult> m_statisticsResults;

            // For each ScopeProducer an instance of the ScopeQuery is created, which consists
            // of an Timestamp and PipelineStatistic query.
            AZStd::vector<ScopeQuery> m_scopeQueries;

            // List of all ShaderResourceGroups to be bound during rendering or computing
            // Derived classed may call BindSrg function to add other srgs the list
            ShaderResourceGroupList m_shaderResourceGroupsToBind;
            
            // View tag used to associate a pipeline view for this pass.
            PipelineViewTag m_viewTag;
        };
    }   // namespace RPI
}   // namespace AZ
