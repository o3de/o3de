/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/Pass.h>
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
        class RenderPass;
        class Query;

        //! A RenderPass is a leaf Pass (i.e. a Pass that has no children) that 
        //! implements rendering functionality (raster, compute, copy)
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API RenderPass :
            public Pass,
            public RHI::ScopeProducer
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            AZ_RPI_PASS(RenderPass);

            using ScopeQuery = AZStd::array<RHI::Ptr<Query>, static_cast<size_t>(ScopeQueryType::Count)>;

        public:
            AZ_RTTI(RenderPass, "{9441D114-60FD-487B-B2B7-0FBBC8A96FC2}", Pass);
            AZ_CLASS_ALLOCATOR(RenderPass, SystemAllocator);
            virtual ~RenderPass();

            //! Returns the RenderAttachmentConfiguration of this pass from its render attachments
            //! This function usually need to be called after pass attachments rebuilt to reflect latest layout
            RHI::RenderAttachmentConfiguration GetRenderAttachmentConfiguration();

            void SetRenderAttachmentConfiguration(
                const RHI::RenderAttachmentConfiguration& configuration, const AZ::RHI::ScopeGroupId& subpassGroupId);

            //! Get MultisampleState of this pass from its output attachments
            RHI::MultisampleState GetMultisampleState() const;
            
            //! Returns a pointer to the Pass ShaderResourceGroup
            Data::Instance<ShaderResourceGroup> GetShaderResourceGroup();

            //! Return the View if this pass is associated with a pipeline view via PipelineViewTag.
            //! It may return nullptr if this pass is independent with any views.
            ViewPtr GetView() const;

            // Add a srg to srg list to be bound for this pass
            void BindSrg(const RHI::ShaderResourceGroup* srg);

        protected:
            explicit RenderPass(const PassDescriptor& descriptor);

            //! Can instances of this class be merged as subpasses?
            bool CanBecomeSubpass() const;

            //! Builds Subpass Attachment Layout data into @subpassLayoutBuilder.
            //! @returns true if successful.
            bool BuildSubpassLayout(RHI::RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder& subpassLayoutBuilder);

            void BuildRenderAttachmentConfiguration();

            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            // Note: BuildCommandList is the final implementation. And all derived passes should override BuildCommandListInternal to build their CommandList
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) final;

            virtual void BuildCommandListInternal([[maybe_unused]] const RHI::FrameGraphExecuteContext& context){};

            // Declares explicitly set dependencies between passes (execute after and execute before)
            // Note most pass ordering is determined by attachments. This is only used for
            // dependencies between passes that don't have any attachments/connections in common.
            void DeclarePassDependenciesToFrameGraph(RHI::FrameGraphInterface frameGraph) const;

            // Binds the pass's attachments to the provide SRG (this will usually be the pass's own SRG)
            void BindPassSrg(const RHI::FrameGraphCompileContext& context, Data::Instance<ShaderResourceGroup>& shaderResourceGroup);

            // Pass behavior overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;
            void ResetInternal() override;

            // Helper functions for srgs used for pass
            // Collect low frequency srgs for draw or compute. These srgs include scene srg, view srg and pass srg
            void CollectSrgs();

            // Clear the srg list
            void ResetSrgs();

            // Set srgs for pass's execution
            void SetSrgsForDraw(const RHI::FrameGraphExecuteContext& context);
            void SetSrgsForDispatch(const RHI::FrameGraphExecuteContext& context);

            // Set PipelineViewTag associated for this pass
            // If the View bound to the tag exists,the view's srg will be collected to pass' srg bind list
            void SetPipelineViewTag(const PipelineViewTag& viewTag);

            // Add the ScopeQuery's QueryPool to the FrameGraph
            void AddScopeQueryToFrameGraph(RHI::FrameGraphInterface frameGraph);

            const AZ::RHI::ScopeGroupId& GetSubpassGroupId() const;

            // The shader resource group for this pass
            Data::Instance<ShaderResourceGroup> m_shaderResourceGroup = nullptr;

            // Determines which hardware queue the pass will run on
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;

        private:
            // Helper function that binds a single attachment to the pass shader resource group
            void BindAttachment(const RHI::FrameGraphCompileContext& context, PassAttachmentBinding& binding, int16_t& imageIndex, int16_t& bufferIndex);

            // Helper function to get the query by the scope index and query type
            RHI::Ptr<Query> GetQuery(ScopeQueryType queryType);

            // Executes a lambda depending on the passed ScopeQuery types
            template <typename Func>
            void ExecuteOnTimestampQuery(Func&& func);
            template <typename Func>
            void ExecuteOnPipelineStatisticsQuery(Func&& func);

            // RPI::Pass overrides...
            TimestampResult GetTimestampResultInternal() const override;
            PipelineStatisticsResult GetPipelineStatisticsResultInternal() const override;

            // Begin recording commands for the ScopeQueries 
            void BeginScopeQuery(const RHI::FrameGraphExecuteContext& context);

            // End recording commands for the ScopeQueries
            void EndScopeQuery(const RHI::FrameGraphExecuteContext& context);

            // Readback the results from the ScopeQueries
            void ReadbackScopeQueryResults();

            // Readback results from the Timestamp queries
            TimestampResult m_timestampResult;
            // Readback results from the PipelineStatistics queries
            PipelineStatisticsResult m_statisticsResult;

            // The device index the pass ran on during the last frame, necessary to read the queries.
            int m_lastDeviceIndex = RHI::MultiDevice::InvalidDeviceIndex;

            // For each ScopeProducer an instance of the ScopeQuery is created, which consists
            // of an Timestamp and PipelineStatistic query.
            ScopeQuery m_scopeQueries;

            // List of all ShaderResourceGroups to be bound during rendering or computing
            // Derived classed may call BindSrg function to add other srgs the list
            AZStd::unordered_map<uint8_t, const RHI::ShaderResourceGroup*> m_shaderResourceGroupsToBind;

            //! The following variables are only relevant when this RenderPass will be merged by the RHI
            //! as a subpass (upon request from an instance of ParentPass).

            //! Stores the RenderAttachmentLayout that should be used when GetRenderAttachmentConfiguration() is called.
            //! If this pointer is invalid when GetRenderAttachmentConfiguration() is called then the pass will build
            //! the RanderAttachmentLayout at that moment.
            AZStd::optional<RHI::RenderAttachmentConfiguration> m_renderAttachmentConfiguration;
            AZ::RHI::ScopeGroupId m_subpassGroupId;
        };
    }   // namespace RPI
}   // namespace AZ
