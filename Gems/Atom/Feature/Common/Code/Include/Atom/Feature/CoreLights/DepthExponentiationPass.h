/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <CoreLights/Shadow.h>

namespace AZ
{
    namespace Render
    {
        //! DepthExponentiationPass output exponential of depth for ESM filering.
        class DepthExponentiationPass final
            : public RPI::ComputePass
        {
            using Base = RPI::ComputePass;
            AZ_RPI_PASS(DepthExponentiationPass);

        public:
            AZ_RTTI(DepthExponentiationPass, "9B91DE5C-0842-4CF8-AA30-B024277E0FAB", Base);
            AZ_CLASS_ALLOCATOR(DepthExponentiationPass, SystemAllocator);

            ~DepthExponentiationPass() = default;
            static RPI::Ptr<DepthExponentiationPass> Create(const RPI::PassDescriptor& descriptor);

            //! This sets the kind of shadowmaps.
            void SetShadowmapType(Shadow::ShadowmapType type);

            //! This returns the shadowmap type of this pass.
            Shadow::ShadowmapType GetShadowmapType() const;

        private:
            struct ShaderVariantInfo
            {
                const bool m_isFullyBaked = false;
                const RHI::PipelineState* m_pipelineState = nullptr;
            };

            DepthExponentiationPass() = delete;
            explicit DepthExponentiationPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;


            void InitializeShaderOption();
            void SetShaderVariantKeyFallbackValue();

            const Name m_optionName;
            const AZStd::vector<Name> m_optionValues;

            Shadow::ShadowmapType m_shadowmapType = Shadow::ShadowmapType::Directional;
            AZStd::vector<ShaderVariantInfo> m_shaderVariant;
            RPI::ShaderVariantKey m_currentShaderVarationKeyFallbackValue;

            bool m_shaderOptionInitialized = false;
            bool m_shaderVariantNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
