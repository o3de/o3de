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

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/Feature/ACES/Aces.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        static const char* const ToneMapperShaderVariantOptionName{ "o_tonemapperType" };
        static const char* const TransferFunctionShaderVariantOptionName{ "o_transferFunctionType" };

        /**
         * This pass is used to apply tonemapping and output transforms other than ACES.
         */
        class OutputTransformPass
            : public DisplayMapperFullScreenPass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(OutputTransformPass);

        public:
            AZ_RTTI(OutputTransformPass, "{1703EB2E-2415-41AE-9C10-06151F795A4A}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(OutputTransformPass, SystemAllocator, 0);
            virtual ~OutputTransformPass();

            /// Creates a OutputTransformPass
            static RPI::Ptr<OutputTransformPass> Create(const RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(RHI::Format format);

            void SetToneMapperType(const ToneMapperType& toneMapperType);
            void SetTransferFunctionType(const TransferFunctionType& transferFunctionType);

        protected:
            explicit OutputTransformPass(const RPI::PassDescriptor& descriptor);
            void Init() override;

        private:

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            const AZ::Name m_toneMapperShaderVariantOptionName;
            const AZ::Name m_transferFunctionShaderVariantOptionName;

            ToneMapperType m_toneMapperType = ToneMapperType::None;
            TransferFunctionType m_transferFunctionType = TransferFunctionType::None;

            bool m_needToUpdateShaderVariant = false;
            RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
        };
    }   // namespace Render
}   // namespace AZ
