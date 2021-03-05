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

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>

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
            AZ_CLASS_ALLOCATOR(AcesOutputTransformLutPass, SystemAllocator, 0);
            virtual ~AcesOutputTransformLutPass();

            /// Creates a AcesOutputTransformLutPass
            static RPI::Ptr<AcesOutputTransformLutPass> Create(const RPI::PassDescriptor& descriptor);

            void SetShaperParams(const ShaperParams& shaperParams);
        private:
            explicit AcesOutputTransformLutPass(const RPI::PassDescriptor& descriptor);
            void Init() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;

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
