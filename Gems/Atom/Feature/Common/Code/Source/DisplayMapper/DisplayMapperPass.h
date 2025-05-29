/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <DisplayMapper/AcesOutputTransformLutPass.h>
#include <DisplayMapper/AcesOutputTransformPass.h>
#include <DisplayMapper/ApplyShaperLookupTablePass.h>
#include <DisplayMapper/BakeAcesOutputTransformLutPass.h>
#include <DisplayMapper/DisplayMapperFullScreenPass.h>
#include <DisplayMapper/OutputTransformPass.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace Render
    {
        static const char* const DisplayMapperPassTemplateName = "DisplayMapperTemplate";

        /**
         *  The display mapper pass.
         *  This pass implements grading and output transform passes.
         *  If an ACES output transform is configured, the respective passes implement the
         *  ACES Reference Rendering Transform(RRT) and Output Device Transform(ODT).
         *
         *  The passes created will be in the form of
         *      <HDR Grading LUT>,  <Output Transform>, <LDR Grading LUT>
         *
         *  <HDR Grading LUT> if enabled, is an instance of ApplyShaperLookupTablePass
         *  <Output Transform> depends on the DisplayMapper function:
         *      Aces                - AcesOutputTransformPass
         *      AcesLUT             - BakeAcesOutputTransformLutPass, AcesOutputTransformLutPass
         *      Passthrough         - DisplayMapperFullscreenPass
         *      Gamma Correction    - DisplayMapperFullscreenPass
         *  <LDR Grading LUT> if enabled, is an instance of ApplyShaperLookupTablePass
         *  [GFX TODO][ATOM-4189] Optimize the passthrough function in the DisplayMapper
         */
        class DisplayMapperPass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(DisplayMapperPass);

        public:
            AZ_RTTI(DisplayMapperPass, "{B022D9D6-BDFA-4435-B27C-466DC4C91D18}", RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DisplayMapperPass, SystemAllocator);
            virtual ~DisplayMapperPass() = default;

            //! Creates a DisplayMapperPass
            static RPI::Ptr<DisplayMapperPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            DisplayMapperPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides
            void BuildInternal() final;
            void InitializeInternal() final;
            void FrameBeginInternal(FramePrepareParams params) final;
            void FrameEndInternal() final;
            void CreateChildPassesInternal() final;

        private:
            void ConfigureDisplayParameters();

            void BuildGradingLutTemplate();
            void CreateGradingAndAcesPasses();

            void UpdateDisplayMapperConfiguration();
            void ClearChildren();

            // Return true if it needs a separate ldr color grading pass
            bool UsesLdrGradingLutPass() const;
            // Return true if it needs an output transform pass (for doing certain tonemapping and gamma correction)
            bool UsesOutputTransformPass() const;

            DisplayMapperConfigurationDescriptor m_displayMapperConfigurationDescriptor;
            DisplayMapperConfigurationDescriptor m_defaultDisplayMapperConfigurationDescriptor;
            bool m_mergeLdrGradingLut = false;
            RPI::AssetReference m_outputTransformOverrideShader;
            bool m_needToRebuildChildren = false;

            const RPI::PassAttachmentBinding* m_pipelineOutput = nullptr;

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            AZStd::shared_ptr<RPI::PassTemplate> m_acesOutputTransformTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_acesOutputTransformLutTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_bakeAcesOutputTransformLutTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_passthroughTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_sRGBTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_ldrGradingLookupTableTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_outputTransformTemplate;
            RPI::Ptr<AcesOutputTransformPass> m_acesOutputTransformPass = nullptr;
            RPI::Ptr<BakeAcesOutputTransformLutPass> m_bakeAcesOutputTransformLutPass = nullptr;
            RPI::Ptr<AcesOutputTransformLutPass> m_acesOutputTransformLutPass = nullptr;
            RPI::Ptr<DisplayMapperFullScreenPass> m_displayMapperPassthroughPass = nullptr;
            RPI::Ptr<DisplayMapperFullScreenPass> m_displayMapperSRGBPass = nullptr;
            RPI::Ptr<ApplyShaperLookupTablePass> m_ldrGradingLookupTablePass = nullptr;
            RPI::Ptr<OutputTransformPass> m_outputTransformPass = nullptr;

            const Name m_applyShaperLookupTableOutputName = Name{ "ApplyShaperLookupTableOutput" };
            const Name m_applyShaperLookupTablePassName = Name{ "ApplyShaperLookupTable" };
            const Name m_ldrGradingLookupTablePassName = Name{ "LdrGradingLookupTable" };
            const Name m_acesOutputTransformPassName = Name{ "AcesOutputTransform" };
            const Name m_bakeAcesOutputTransformLutPassName = Name{ "BakeAcesOutputTransformLut" };
            const Name m_acesOutputTransformLutPassName = Name{ "AcesLutPass" };
            const Name m_displayMapperPassthroughPassName = Name{ "DisplayMapperPassthrough" };
            const Name m_displayMapperSRGBPassName = Name{ "DisplayMapperSRGB" };
            const Name m_outputTransformPassName = Name{ "OutputTransform" };

            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
        };
    }   // namespace Render
}   // namespace AZ
