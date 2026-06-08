/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${Name}FeatureProcessor.h"

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/SerializeContext.h>

AZ_CVAR(
    bool,
    r_${SanitizedCppName}Enabled,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Enable/disable the ${Name} fullscreen post-process pass at runtime.");

namespace ${GemName}
{
    // -------------------------------------------------------------------------
    // Insertion anchor.
    // The pass is inserted AFTER this parent pass in every pipeline that has
    // it. "PostProcessPass" is the standard Atom anchor for screen-space
    // effects that should run after tonemapping but before UI compositing.
    // Change to "Forward" or another stable parent if your pipeline differs.
    // -------------------------------------------------------------------------
    static constexpr const char* k_anchorPassName = "PostProcessPass";

    // The PassTemplate name written into the .pass asset's "Name" field.
    // Must match Assets/Passes/${Name}.pass -> ClassData.PassTemplate.Name.
    static constexpr const char* k_passTemplateName = "${Name}Template";

    // The runtime name the inserted pass is given inside the pipeline.
    static constexpr const char* k_passInstanceName = "${Name}";

    void ${SanitizedCppName}FeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${SanitizedCppName}FeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(0);
        }
    }

    void ${SanitizedCppName}FeatureProcessor::Activate()
    {
        EnableSceneNotification();
    }

    void ${SanitizedCppName}FeatureProcessor::Deactivate()
    {
        DisableSceneNotification();
        m_pass = nullptr;
        m_renderPipeline = nullptr;
    }

    void ${SanitizedCppName}FeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        // Only inject into the default view pipeline. Skip MRT capture pipelines
        // and other auxiliary pipelines so the effect doesn't poison thumbnails.
        if (renderPipeline->GetViewType() != AZ::RPI::ViewType::Default)
        {
            return;
        }

        // Already injected into another pipeline -- one instance is enough.
        if (m_renderPipeline && m_renderPipeline != renderPipeline)
        {
            return;
        }

        // Already present in this pipeline -- recache the handle and exit.
        const AZ::Name templateName(k_passTemplateName);
        auto filter = AZ::RPI::PassFilter::CreateWithTemplateName(templateName, renderPipeline);
        if (auto existing = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(filter); existing)
        {
            m_pass = existing;
            m_renderPipeline = renderPipeline;
            return;
        }

        // Sanity-check the anchor pass exists in this pipeline.
        const AZ::Name anchorName(k_anchorPassName);
        if (renderPipeline->FindFirstPass(anchorName) == nullptr)
        {
            AZ_Warning(
                "${SanitizedCppName}FeatureProcessor", false,
                "Anchor pass '%s' not found in pipeline '%s'; skipping ${Name} injection.",
                anchorName.GetCStr(), renderPipeline->GetId().GetCStr());
            return;
        }

        // Build the PassRequest that will splice ${Name} into the graph.
        AZ::RPI::PassRequest request;
        request.m_passName = AZ::Name(k_passInstanceName);
        request.m_templateName = templateName;
        request.m_passEnabled = r_${SanitizedCppName}Enabled;

        // Wire the pass's InputColor slot to the anchor's Output. The anchor's
        // attachment name varies by anchor pass; "Output" is correct for
        // PostProcessPass. If you change k_anchorPassName, update this too.
        //
        // OutputColor is intentionally NOT connected here: the .pass template
        // owns its output (OutputColorAttachment, sized/formatted from
        // InputColor), so the pass is a complete, schedulable render pass on its
        // own. See Assets/Passes/${Name}.pass.
        //
        // VISIBILITY NOTE (sample model): because source and destination are
        // distinct buffers, the effect is computed into OutputColorAttachment
        // but the rest of the pipeline still reads the anchor's original Output.
        // To make the result reach the screen you must route a downstream pass
        // to read this pass's OutputColor (a pipeline-azasset edit), or switch
        // the .pass to the in-place model (DeferredFog.pass shape) -- see the
        // ${Name}.pass README. Uncomment the PROBE block in ${Name}.azsl to
        // verify the pass runs end-to-end once that routing is in place.
        request.AddInputConnection(AZ::RPI::PassConnection{
            AZ::Name("InputColor"),
            AZ::RPI::PassAttachmentRef{ anchorName, AZ::Name("Output") }
        });

        if (auto pass = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(&request); pass != nullptr)
        {
            renderPipeline->AddPassAfter(pass, anchorName);
            m_pass = pass.get();
            m_renderPipeline = renderPipeline;
        }
        else
        {
            AZ_Warning(
                "${SanitizedCppName}FeatureProcessor", false,
                "CreatePassFromRequest failed for template '%s'. Verify the .pass asset built and ${Name}Pass is registered with PassSystemInterface.",
                templateName.GetCStr());
        }
    }

    void ${SanitizedCppName}FeatureProcessor::OnRenderEnd()
    {
        SetPassEnabled(r_${SanitizedCppName}Enabled);
    }

    void ${SanitizedCppName}FeatureProcessor::SetPassEnabled(bool enabled)
    {
        if (m_pass)
        {
            m_pass->SetEnabled(enabled);
        }
    }

} // namespace ${GemName}
