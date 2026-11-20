/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}Pass.h
// -----------------------------------------------------------------------------
// Custom Atom RPI fullscreen post-process pass class.
//
// ROLE IN THE TEMPLATE:
//   This is the C++ entry point Atom calls when it sees ${Name}Pass referenced
//   from a .pass template (Assets/Passes/${Name}.pass). It subclasses
//   AZ::RPI::FullscreenTrianglePass so we get the "draw a single fullscreen
//   triangle that runs a pixel shader against the current attachments" plumbing
//   for free -- we only customize what runs per-frame.
//
// REGISTRATION CHAIN:
//   ${GemName}RenderingSystemComponent::Activate()
//     -> AZ::RPI::PassSystemInterface::Get()->AddPassCreator(
//             AZ::Name("${Name}Pass"), &${Name}Pass::Create);
//   That AddPassCreator line is injected by the wizard's add_pass_creator_call
//   command; do not delete it.
//
// SIBLING FILES TOUCHED BY THIS PASS:
//   Assets/Passes/${Name}.pass             - declares the pass template name
//                                            (default ${Name}Template),
//                                            PassClass: "${Name}Pass",
//                                            ShaderAsset: ${Name}.shader
//   Assets/Shaders/PostProcessing/${Name}.shader  - JSON wrapper around the azsl
//   Assets/Shaders/PostProcessing/${Name}.azsl    - the actual pixel shader,
//                                            with all four render variants inline
// =============================================================================

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace ${GemName}
{
    //! Custom fullscreen post-process pass.
    //!
    //! Override the protected RPI::FullscreenTrianglePass hooks below to add
    //! per-frame work: writing custom SRG values, querying feature processors,
    //! attaching extra resources, etc. The default implementation just runs the
    //! shader as-is, which is enough for color grading, edge detect, vignette,
    //! and similar pure-pixel-shader effects.
    class ${Name}Pass
        : public AZ::RPI::FullscreenTrianglePass
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}Pass, "{${Random_Uuid}}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(${SanitizedCppName}Pass, AZ::SystemAllocator);

        //! Factory entry point. Atom's PassSystem invokes this through the
        //! creator function pointer that ${GemName}RenderingSystemComponent
        //! registers via AddPassCreator.
        static AZ::RPI::Ptr<${Name}Pass> Create(const AZ::RPI::PassDescriptor& descriptor);

        ~${Name}Pass() override = default;

    protected:
        explicit ${Name}Pass(const AZ::RPI::PassDescriptor& descriptor);

        // ---------------------------------------------------------------------
        // RPI::Pass overrides
        // ---------------------------------------------------------------------

        //! Called once after the pass is constructed but before the first frame.
        //! Use this to look up SRG indices from the loaded shader and cache
        //! them. The base class loads the shader in its own InitializeInternal,
        //! so call into the base FIRST then read m_shader.
        void InitializeInternal() override;

        //! Called every frame, before the GPU executes work for this pass.
        //! This is where you push per-frame uniform values into the pass SRG
        //! (m_shaderResourceGroup), e.g. update m_tint based on a CVar or a
        //! component-driven setting.
        void FrameBeginInternal(FramePrepareParams params) override;

    private:
        // ---------------------------------------------------------------------
        // Cached SRG input indices.
        //
        // These mirror the example code in the .cpp. Uncomment a member here
        // when you uncomment the matching FindShaderInputConstantIndex /
        // SetConstant lines in InitializeInternal()/FrameBeginInternal(); they
        // are commented out by default so the generated file compiles cleanly
        // even when no SRG constants have been added yet.
        //
        //   AZ::RHI::ShaderInputConstantIndex m_tintIndex;
        // ---------------------------------------------------------------------
    };

} // namespace ${GemName}
