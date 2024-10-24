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

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <DisplayMapper/DisplayMapperFullScreenPass.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The ACES output transform LUT pass.
         *  This pass implements the RRT and ODT components of the ACES pipeline from a baked LUT.
         */
        class AcesOutputTransformLutPass final
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformLutPass);

        public:
            AZ_RTTI(AcesOutputTransformLutPass, "{914EE97F-20DA-4916-AE66-DC4141E1A06E}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(AcesOutputTransformLutPass, SystemAllocator);
            virtual ~AcesOutputTransformLutPass();

            /// Creates a AcesOutputTransformLutPass
            static RPI::Ptr<AcesOutputTransformLutPass> Create(const RPI::PassDescriptor& descriptor);

            void SetShaperParams(const ShaperParams& shaperParams);
        private:
            explicit AcesOutputTransformLutPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            void AcquireLutImage();
            void ReleaseLutImage();

            RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;
            RHI::ShaderInputImageIndex m_shaderInputColorImageIndex;
            RHI::ShaderInputConstantIndex m_shaderInputShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputShaperScaleIndex;

            DisplayMapperLut m_displayMapperLut = {};
            ShaperParams m_shaperParams;
        };
    }   // namespace Render
}   // namespace AZ
