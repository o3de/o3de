/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <ACES/Aces.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <DisplayMapper/ApplyShaperLookupTablePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        static const char* const ToneMapperShaderVariantOptionName{ "o_tonemapperType" };
        static const char* const TransferFunctionShaderVariantOptionName{ "o_transferFunctionType" };
        static const char* const LdrGradingLutShaderVariantOptionName{ "o_colorGradingEnabled" };

        /**
         * This pass is used to apply tonemapping and output transforms other than ACES.
         */
        class OutputTransformPass
            : public ApplyShaperLookupTablePass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(OutputTransformPass);

        public:
            AZ_RTTI(OutputTransformPass, "{1703EB2E-2415-41AE-9C10-06151F795A4A}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(OutputTransformPass, SystemAllocator);
            virtual ~OutputTransformPass();

            /// Creates a OutputTransformPass
            static RPI::Ptr<OutputTransformPass> Create(const RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(RHI::Format format);

            void SetToneMapperType(const ToneMapperType& toneMapperType);
            void SetTransferFunctionType(const TransferFunctionType& transferFunctionType);

        protected:
            explicit OutputTransformPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides
            void InitializeInternal() override;

        private:
            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Pass behavior overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            const AZ::Name m_toneMapperShaderVariantOptionName;
            const AZ::Name m_transferFunctionShaderVariantOptionName;
            const AZ::Name m_ldrGradingLutShaderVariantOptionName;

            ToneMapperType m_toneMapperType = ToneMapperType::None;
            TransferFunctionType m_transferFunctionType = TransferFunctionType::None;

            bool m_needToUpdateShaderVariant = false;
            RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;
            RHI::ShaderInputNameIndex m_exposureControlBufferInputIndex = "m_exposureControl";

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
        };
    }   // namespace Render
}   // namespace AZ
