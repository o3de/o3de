/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        //! Pass used to enabled/disabled debug colors by shader option.
        class DepthOfFieldCompositePass final
            : public RPI::FullscreenTrianglePass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(DepthOfFieldCompositePass);

        public:
            AZ_RTTI(AZ::Render::DepthOfFieldCompositePass, "{7595A972-7ED5-46FE-BBE0-3262846E2964}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldCompositePass, SystemAllocator);
            virtual ~DepthOfFieldCompositePass() = default;

            //! Creates a DepthOfFieldCompositePass
            static RPI::Ptr<DepthOfFieldCompositePass> Create(const RPI::PassDescriptor& descriptor);

            void SetEnabledDebugColoring(bool enabled);

        protected:
            // Pass behavior overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            DepthOfFieldCompositePass(const RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            bool m_needToUpdateShaderVariant = true;

            bool m_enabledDebugColoring = false;

            // SRG binding indices...
            RHI::ShaderInputNameIndex m_backBlendFactorDivision2Index = "m_backBlendFactorDivision2";
            RHI::ShaderInputNameIndex m_backBlendFactorDivision4Index = "m_backBlendFactorDivision4";
            RHI::ShaderInputNameIndex m_backBlendFactorDivision8Index = "m_backBlendFactorDivision8";
            RHI::ShaderInputNameIndex m_frontBlendFactorDivision2Index = "m_frontBlendFactorDivision2";
            RHI::ShaderInputNameIndex m_frontBlendFactorDivision4Index = "m_frontBlendFactorDivision4";
            RHI::ShaderInputNameIndex m_frontBlendFactorDivision8Index = "m_frontBlendFactorDivision8";

            // scale / offset to convert DofFactor to blend ratio for back buffer.
            AZStd::array<float, 2> m_backBlendFactorDivision2 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_backBlendFactorDivision4 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_backBlendFactorDivision8 = { { 0.0f, 0.0f } };
            // scale / offset to convert DofFactor to blend ratio for front buffer.
            AZStd::array<float, 2> m_frontBlendFactorDivision2 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_frontBlendFactorDivision4 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_frontBlendFactorDivision8 = { { 0.0f, 0.0f } };

            const AZ::Name m_optionName;
            const AZStd::vector<AZ::Name> m_optionValues;
        };
    }   // namespace Render
}   // namespace AZ
