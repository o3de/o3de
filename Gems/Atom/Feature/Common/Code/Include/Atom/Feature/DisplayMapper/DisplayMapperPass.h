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
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/AcesOutputTransformLutPass.h>
#include <Atom/Feature/DisplayMapper/AcesOutputTransformPass.h>
#include <Atom/Feature/DisplayMapper/ApplyShaperLookupTablePass.h>
#include <Atom/Feature/DisplayMapper/BakeAcesOutputTransformLutPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <Atom/Feature/DisplayMapper/OutputTransformPass.h>

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
            , public AzFramework::WindowNotificationBus::Handler
        {
            AZ_RPI_PASS(DisplayMapperPass);

        public:
            AZ_RTTI(DisplayMapperPass, "{B022D9D6-BDFA-4435-B27C-466DC4C91D18}", RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DisplayMapperPass, SystemAllocator, 0);
            virtual ~DisplayMapperPass();

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

            // WindowNotificationBus::Handler overrides ...
            void OnWindowResized(uint32_t width, uint32_t height) override;

        private:
            void ConfigureDisplayParameters();

            void BuildGradingLutTemplate();
            void CreateGradingAndAcesPasses();

            void GetDisplayMapperConfiguration();
            void ClearChildren();

            DisplayMapperConfigurationDescriptor m_displayMapperConfigurationDescriptor;
            bool m_needToRebuildChildren = false;

            const RPI::PassAttachmentBinding* m_swapChainAttachmentBinding = nullptr;

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            AZStd::shared_ptr<RPI::PassTemplate> m_acesOutputTransformTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_acesOutputTransformLutTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_bakeAcesOutputTransformLutTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_passthroughTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_gammaCorrectionTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_ldrGradingLookupTableTemplate;
            AZStd::shared_ptr<RPI::PassTemplate> m_outputTransformTemplate;
            RPI::Ptr<AcesOutputTransformPass> m_acesOutputTransformPass = nullptr;
            RPI::Ptr<BakeAcesOutputTransformLutPass> m_bakeAcesOutputTransformLutPass = nullptr;
            RPI::Ptr<AcesOutputTransformLutPass> m_acesOutputTransformLutPass = nullptr;
            RPI::Ptr<DisplayMapperFullScreenPass> m_displayMapperPassthroughPass = nullptr;
            RPI::Ptr<DisplayMapperFullScreenPass> m_displayMapperOnlyGammaCorrectionPass = nullptr;
            RPI::Ptr<ApplyShaperLookupTablePass> m_ldrGradingLookupTablePass = nullptr;
            RPI::Ptr<OutputTransformPass> m_outputTransformPass = nullptr;

            const Name m_applyShaperLookupTableOutputName = Name{ "ApplyShaperLookupTableOutput" };
            const Name m_applyShaperLookupTablePassName = Name{ "ApplyShaperLookupTable" };
            const Name m_ldrGradingLookupTablePassName = Name{ "LdrGradingLookupTable" };
            const Name m_acesOutputTransformPassName = Name{ "AcesOutputTransform" };
            const Name m_bakeAcesOutputTransformLutPassName = Name{ "BakeAcesOutputTransformLut" };
            const Name m_acesOutputTransformLutPassName = Name{ "AcesLutPass" };
            const Name m_displayMapperPassthroughPassName = Name{ "DisplayMapperPassthrough" };
            const Name m_displayMapperOnlyGammaCorrectionPassName = Name{ "DisplayMapperOnlyGammaCorrection" };
            const Name m_outputTransformPassName = Name{ "OutputTransform" };

            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
        };
    }   // namespace Render
}   // namespace AZ
