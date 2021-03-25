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
            AZ_CLASS_ALLOCATOR(DepthOfFieldCompositePass, SystemAllocator, 0);
            virtual ~DepthOfFieldCompositePass() = default;

            //! Creates a DepthOfFieldCompositePass
            static RPI::Ptr<DepthOfFieldCompositePass> Create(const RPI::PassDescriptor& descriptor);

            void SetEnabledDebugColoring(bool enabled);

        protected:
            // Pass behavior overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            DepthOfFieldCompositePass(const RPI::PassDescriptor& descriptor);
            void Init() override;
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            bool m_needToUpdateShaderVariant = true;

            bool m_enabledDebugColoring = false;

            // SRG binding indices...
            AZ::RHI::ShaderInputConstantIndex m_backBlendFactorDivision2Index;
            AZ::RHI::ShaderInputConstantIndex m_backBlendFactorDivision4Index;
            AZ::RHI::ShaderInputConstantIndex m_backBlendFactorDivision8Index;
            AZ::RHI::ShaderInputConstantIndex m_frontBlendFactorDivision2Index;
            AZ::RHI::ShaderInputConstantIndex m_frontBlendFactorDivision4Index;
            AZ::RHI::ShaderInputConstantIndex m_frontBlendFactorDivision8Index;

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
