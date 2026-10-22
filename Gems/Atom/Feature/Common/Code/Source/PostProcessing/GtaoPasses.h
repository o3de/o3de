/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoSettingsInterface.h>

namespace AZ
{
    namespace Render
    {
        // Parent pass for GTAO that contains the GTAO compute pass
        class GtaoParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(GtaoParentPass);

        public:
            AZ_RTTI(AZ::Render::GtaoParentPass, "{6EA8F19C-78E7-475A-A0B6-92948D5C4DB5}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(GtaoParentPass, SystemAllocator);
            virtual ~GtaoParentPass() = default;

            //! Creates an GtaoParentPass
            static RPI::Ptr<GtaoParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            GtaoParentPass(const RPI::PassDescriptor& descriptor);

            // Direct pointers to child passes for enable/disable
            ParentPass* m_blurParentPass = nullptr;
            FastDepthAwareBlurHorPass* m_blurHorizontalPass = nullptr;
            FastDepthAwareBlurVerPass* m_blurVerticalPass = nullptr;
            Pass* m_downsamplePass = nullptr;
            Pass* m_upsamplePass = nullptr;
        };

        // Computer shader pass that calculates GTAO from a linear depth buffer
        class GtaoComputePass final
            : public RPI::ComputePass,
              public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(GtaoComputePass);

        public:
            AZ_RTTI(AZ::Render::GtaoComputePass, "{0BA5F6F7-15D2-490A-8254-7E61F25B62F9}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(GtaoComputePass, SystemAllocator);
            virtual ~GtaoComputePass() = default;

            //! Creates an GtaoComputePass
            static RPI::Ptr<GtaoComputePass> Create(const RPI::PassDescriptor& descriptor);

            void SetQualityLevel(Ao::GtaoQualityLevel qualityLevel);

        protected:
            // Behavior functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            GtaoComputePass(const RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            Ao::GtaoQualityLevel m_qualityLevel = Ao::GtaoQualityLevel::Medium;
            const AZ::Name m_qualityLevelShaderVariantOptionName{ "o_qualityLevel" };
            bool m_needToUpdateShaderVariant = true;

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
