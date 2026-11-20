/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}Pass.cpp
// -----------------------------------------------------------------------------
// Implementation of the custom fullscreen post-process pass.
//
// =============================================================================
// END-TO-END "TURN THE SCREEN RED" CHECKLIST
// -----------------------------------------------------------------------------
// The wizard generates the C++ scaffolding and the asset stubs, but it
// deliberately does NOT modify your project's render pipeline -- that step is
// project-specific and risky to automate. Until you do step 4, the pass is
// registered but unused; nothing renders. Walk these in order:
//
//   [1] Confirm the assets built.
//       Look in your project's Cache/ for:
//           Assets/Shaders/PostProcessing/${Name}.azshader   (compiled shader)
//           Assets/Passes/${Name}.pass                       (pass template)
//       If either is missing, open AssetProcessor and search by filename for
//       red errors. Common causes: an azsli include path mistake, or the
//       .shader EntryPoints not matching the function names in the .azsl.
//
//   [2] Confirm the pass class is registered.
//       Set a breakpoint in ${GemName}RenderingSystemComponent::Activate on
//       the AddPassCreator call. It must be hit during editor / runtime
//       startup. If it isn't, the module never picks up the system component
//       (verify GS_PlayModule.cpp has CreateDescriptor() and the typeid in
//       GetRequiredSystemComponents).
//
//   [3] Uncomment the PROBE in ${Name}.azsl.
//       Open Assets/Shaders/PostProcessing/${Name}.azsl, find the active
//       variant block (the one whose name matches your render_variant choice
//       at generation time), and uncomment the PROBE line:
//           // OUT.m_color = float4(1.0, 0.0, 0.0, 1.0);
//       Save. AssetProcessor will rebuild the shader. WAIT for it to finish
//       (status bar should hit 0 jobs) before launching.
//
//   [4] Add a PassRequest to your project's render pipeline.   <-- COMMON MISS
//       This is the step that actually causes Atom to instantiate the pass and
//       run the shader. The pass file alone does nothing.
//       a. Find your project's pipeline asset, typically:
//            <Project>/Passes/MainPipeline.pass
//          or referenced from an .azasset under <Project>/Passes/.
//       b. Inside its "PassRequests" array, paste the snippet from the
//          _README_ block of <gem>/Registry/${GemName}PostProcessRegistry.setreg.
//          Substitute "PreviousPassInPipeline" / "NextPassInPipeline" with
//          actual pass names from your pipeline -- a safe choice is to splice
//          BETWEEN the LDR output of tonemapping and the UI overlay (e.g.
//          after "DisplayMapperPass" and before "UIPass" / "ImGuiPass").
//       c. Save the pipeline asset. AssetProcessor reprocesses it. Restart
//          the editor / project; render pipelines are not hot-reloaded.
//
//   [5] Verify in editor.
//       Open a level, switch to Game view (or just run the project). You
//       should see the entire viewport turn red. UI overlays will still draw
//       on top -- that's expected, you spliced in front of the UI pass.
//
// IF STEP 5 STILL DOESN'T SHOW RED:
//   * Double-check the slot names in your pasted PassRequest match the slot
//     names of the surrounding passes in your pipeline. A mismatch is a
//     silent failure -- Atom logs to the console but does not pop a dialog.
//   * Open the Pass Tree window in the editor (Tools -> Other -> Pass Tree)
//     and confirm "${Name}" appears under the active pipeline. If it isn't
//     there, the pipeline didn't pick up your PassRequest.
//   * Check Console / log for "Failed to load shader" or "PassClass not
//     found" -- those point straight at the broken link.
// =============================================================================
//
// FRAME LIFECYCLE (what runs when):
//   1. PassSystem instantiates the pass via Create() once, the first time the
//      .pass template is requested by a render pipeline.
//   2. InitializeInternal() runs once after construction. The base class loads
//      the shader referenced by Assets/Passes/${Name}.pass at this point; if
//      that fails, every later frame is a no-op.
//   3. FrameBeginInternal() runs every frame, before the GPU executes work for
//      this pass. Push per-frame uniforms into the pass SRG here.
//   4. The base class draws a single fullscreen triangle that runs the pixel
//      shader entry point declared in ${Name}.shader.
// =============================================================================

#include "${Name}Pass.h"

namespace ${GemName}
{
    AZ::RPI::Ptr<${SanitizedCppName}Pass> ${SanitizedCppName}Pass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        // aznew is required by RPI; never use the global new operator for passes.
        return aznew ${SanitizedCppName}Pass(descriptor);
    }

    ${SanitizedCppName}Pass::${SanitizedCppName}Pass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
    {
    }

    void ${SanitizedCppName}Pass::InitializeInternal()
    {
        // Base class binds the shader and creates the pass SRG. Always call
        // through first; m_shader and m_shaderResourceGroup are not valid until
        // the base has run.
        AZ::RPI::FullscreenTrianglePass::InitializeInternal();

        // Cache SRG indices here. Example (uncomment together with the matching
        // m_tintIndex member in ${Name}Pass.h AND a 'float4 m_tint' entry in the
        // azsl PassSrg):
        //
        //   if (m_shaderResourceGroup)
        //   {
        //       m_tintIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(
        //           AZ::Name("m_tint"));
        //   }
    }

    void ${SanitizedCppName}Pass::FrameBeginInternal(FramePrepareParams params)
    {
        // Push per-frame SRG values BEFORE delegating to the base class so the
        // base sees the updated values when it compiles resources for the
        // frame. Example (uncomment together with m_tintIndex in ${Name}Pass.h
        // and the corresponding constant in the azsl PassSrg):
        //
        //   if (m_shaderResourceGroup && m_tintIndex.IsValid())
        //   {
        //       m_shaderResourceGroup->SetConstant(m_tintIndex, AZ::Vector4(1, 0, 0, 1));
        //   }

        AZ::RPI::FullscreenTrianglePass::FrameBeginInternal(params);
    }

} // namespace ${GemName}
