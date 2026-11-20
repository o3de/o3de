/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}Pass.h    (variant: ScreenSpaceConstant)
// -----------------------------------------------------------------------------
// Custom AZ::RPI::FullscreenTrianglePass that runs every frame, on every
// pipeline that ${Name}FeatureProcessor injects it into. Constants/SRG values
// are global -- not per-area, not per-camera -- which is the right shape for
// effects like a global tint, a debug overlay, a screen-space scanline, etc.
//
// REGISTRATION CHAIN:
//   1. ${GemName}RenderingSystemComponent::Activate
//        AddPassCreator(AZ::Name("${Name}Pass"), &${Name}Pass::Create)
//        FeatureProcessorFactory::Get()->RegisterFeatureProcessor<${Name}FeatureProcessor>()
//   2. Per-pipeline (Atom calls automatically):
//        ${Name}FeatureProcessor::AddRenderPasses(pipeline)
//          -> CreatePassFromRequest using template "${Name}Template"
//          -> RenderPipeline::AddPassAfter("PostProcessPass", pass)
// =============================================================================

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace ${GemName}
{
    class ${Name}Pass
        : public AZ::RPI::FullscreenTrianglePass
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}Pass, "{${Random_Uuid}}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Pass, AZ::SystemAllocator);

        static AZ::RPI::Ptr<${SanitizedCppName}Pass> Create(const AZ::RPI::PassDescriptor& descriptor);

        ~${SanitizedCppName}Pass() override = default;

    protected:
        explicit ${SanitizedCppName}Pass(const AZ::RPI::PassDescriptor& descriptor);

        void InitializeInternal() override;
        void FrameBeginInternal(FramePrepareParams params) override;

    private:
        // ---------------------------------------------------------------------
        // Cached SRG input indices.
        //
        // Pair each member here with: a 'float' / 'float3' field in PassSrg
        // (in ${Name}.azsl), a FindShaderInputConstantIndex cache call in
        // InitializeInternal, and a SetConstant push in FrameBeginInternal.
        //
        //   AZ::RHI::ShaderInputConstantIndex m_tintIndex;
        // ---------------------------------------------------------------------
    };

} // namespace ${GemName}
