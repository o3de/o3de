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

#include <Atom/RHI/ImagePool.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace Render
    {

        /**
         *  The bake ACES output transofrm LUT pass.
         *  This pass the RRT and ODT components of the ACES pipeline into a LUT.
         */
        class BakeAcesOutputTransformLutPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(BakeAcesOutputTransformLutPass);

        public:
            AZ_RTTI(BakeAcesOutputTransformLutPass, "{383C28CD-D744-4B48-A30D-086EF66E7BFB}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(BakeAcesOutputTransformLutPass, SystemAllocator);
            virtual ~BakeAcesOutputTransformLutPass();

            /// Creates a DisplayMapperPass
            static RPI::Ptr<BakeAcesOutputTransformLutPass> Create(const RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(RHI::Format format);

            const ShaperParams& GetShaperParams() const;

        private:
            explicit BakeAcesOutputTransformLutPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            void AcquireLutImage();
            void ReleaseLutImage();

            bool m_resourcesInitialized = false;

            RHI::ShaderInputConstantIndex m_shaderInputColorMatIndex;
            RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputAcesSplineParamsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputFlagsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputOutputModeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSurroundGammaIndex;
            RHI::ShaderInputConstantIndex m_shaderInputGammaIndex;
            RHI::ShaderInputConstantIndex m_shaderInputShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputShaperScaleIndex;

            RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            DisplayMapperLut m_displayMapperLut = {};

            bool m_needToUpdateLut = false;
            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
            OutputDeviceTransformType m_outputDeviceTransformType = OutputDeviceTransformType_48Nits;
            ShaperParams m_shaperParams;
        };
    }   // namespace Render
}   // namespace AZ
