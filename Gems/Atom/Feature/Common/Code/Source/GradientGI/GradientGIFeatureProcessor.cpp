/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientGIFeatureProcessor.h"
#include "GradientGICubemapPass.h"
#include "GradientGIConstants.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Name/Name.h>

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace AZ::Render
{
    // =========================================================================
    // Reflect
    // =========================================================================

    void GradientGIFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientGIFeatureProcessor, RPI::FeatureProcessor>()
                ->Version(0);
        }
    }

    // =========================================================================
    // Feature Processor Lifecycle
    // =========================================================================

    void GradientGIFeatureProcessor::Activate()
    {
        m_iblFeatureProcessor =
            GetParentScene()->GetFeatureProcessor<ImageBasedLightFeatureProcessorInterface>();
        AZ_Error("GradientGI", m_iblFeatureProcessor,
            "ImageBasedLightFeatureProcessorInterface not found on scene. GradientGI requires it.");

        // If Dynamic mode was requested, check platform support; fall back to Static if needed.
        const UpdateMode resolvedMode =
            GradientGI::ResolveUpdateMode(m_updateMode, GradientGICubemapPass::IsGpuComputeSupported());
        AZ_Warning("GradientGI", resolvedMode == m_updateMode,
            "GPU compute UAV cubemaps are not supported on this platform. "
            "GradientGI falling back to Static mode.");
        m_updateMode = resolvedMode;

        // The feature processor starts inert. It produces nothing until a component takes
        // ownership via Enable(), so it can never light the scene with no owner alive.
        m_enabled      = false;
        m_needsRebuild = false;

        // Cache the scene SRG IBL slot indices for Dynamic mode's direct writes.
        if (m_updateMode == UpdateMode::Dynamic && !m_sceneSrgIndicesCached)
        {
            CacheSceneSrgIndices();
        }

        // Listen for pipeline pass rebuilds so we can re-establish our compute pass
        // (e.g. when maximized play mode tears down and recreates the render pipeline).
        EnableSceneNotification();
    }

    void GradientGIFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        // Scene teardown -- release immediately rather than waiting out a grace period that will
        // never tick, since OnBeginPrepareRender stops arriving once the notification bus is off.
        ReleaseOutput();
        ReleaseGeneratedImages();

        m_owners.Clear();
        m_enabled               = false;
        m_teardownCountdown     = 0;
        m_iblFeatureProcessor   = nullptr;
        m_sceneSrg              = nullptr;
        m_sceneSrgIndicesCached = false;
    }

    // =========================================================================
    // Ownership -- one component drives the feature at a time
    // =========================================================================

    void GradientGIFeatureProcessor::Enable(EntityId owner)
    {
        // Register this owner and let it drive. Earlier owners stay registered so that when this
        // one leaves the feature is handed back to them instead of switching off.
        m_owners.Add(owner);
        m_enabled = true;

        // We are wanted again -- cancel any pending teardown and keep what we already generated.
        m_teardownCountdown = 0;

        // Give the scene a sane ambient floor before we start driving it, so any frame we are not
        // writing (a pipeline swap, a mode switch) reads as the engine default rather than black.
        EnsureIblFloor();

        if (m_updateMode == UpdateMode::Static)
        {
            // Only force a rebuild when there is nothing to publish. The controller pushes the whole
            // configuration before enabling us, so a rebuild it has already requested (colours,
            // resolution) must never be cleared here -- otherwise a newly activated component would
            // inherit the previous one's gradient. Retaining the cubemap across a disable makes the
            // common re-enable (property edit, game mode transition) allocation-free: Simulate sees
            // the IBL slots no longer hold our image and simply re-publishes it.
            if (!m_cubemapImage)
            {
                m_needsRebuild = true;
            }
        }
        else
        {
            EnsurePass();
        }

        if (m_gradientPass)
        {
            m_gradientPass->SetEnabled(true);
        }
    }

    void GradientGIFeatureProcessor::Disable(EntityId owner)
    {
        m_owners.Remove(owner);

        // Re-arm the duplicate report. Clearing this only from the per-frame check would miss a
        // level reload, where every owner deregisters and re-registers within a single frame -- the
        // check never samples a count below two, so the latch would suppress the warning for a
        // level that genuinely does contain duplicates.
        m_multiOwnerFrames   = 0;
        m_reportedOwnerCount = 0;

        // Stop only once nobody wants us any more. While another component is still registered --
        // a duplicate deleted with the original still alive, or the interleaved teardown of a
        // spawned prefab copy -- we keep running and it simply takes over. See
        // GradientGI::OwnerRegistry.
        if (!m_owners.Empty())
        {
            return;
        }

        m_enabled = false;
        ReleaseOutput();
    }

    void GradientGIFeatureProcessor::ReleaseOutput()
    {
        // Static: hand the IBL slots back, but only if they still hold our cubemap, so we never
        // wipe another provider's IBL. The cubemap itself is kept -- see the grace period below.
        if (OwnsIblSlots())
        {
            m_iblFeatureProcessor->Reset();
        }
        m_needsRebuild = false;

        // Dynamic: silence the compute dispatch. Render() stops rebinding the scene SRG, so the
        // IBL feature processor's own values take effect again on the next frame.
        if (m_gradientPass)
        {
            m_gradientPass->SetEnabled(false);
        }

        // Keep everything we generated for a short grace period. A re-enable within it (a property
        // edit, a game mode transition) cancels the teardown and costs nothing; outliving it
        // releases the lot.
        if (m_gradientPass || m_cubemapImage)
        {
            m_teardownCountdown = TeardownGraceFrames;
        }
    }

    void GradientGIFeatureProcessor::TickDeferredTeardown()
    {
        if (m_teardownCountdown == 0)
        {
            return;
        }

        if (--m_teardownCountdown == 0)
        {
            ReleaseGeneratedImages();
        }
    }

    // =========================================================================
    // Duplicate reporting
    // =========================================================================

    void GradientGIFeatureProcessor::TickMultipleOwnerReport()
    {
        const size_t ownerCount = m_owners.Count();

        if (ownerCount < 2)
        {
            // Back to a single owner -- re-arm so a later duplicate is reported again.
            m_multiOwnerFrames   = 0;
            m_reportedOwnerCount = 0;
            return;
        }

        // Only report once the overlap has persisted. Handing over between an editor-only component
        // and its spawned counterpart can register both for a frame; a real duplicate never clears.
        ++m_multiOwnerFrames;
        if (m_multiOwnerFrames >= MultiOwnerReportFrames && ownerCount != m_reportedOwnerCount)
        {
            m_reportedOwnerCount = ownerCount;
            ReportMultipleOwners();
        }
    }

    void GradientGIFeatureProcessor::ReportMultipleOwners() const
    {
        // Name the entities, so the redundant one can be found without hunting the level. Only the
        // newest has any visible effect, which is exactly why a duplicate goes unnoticed.
        AZStd::string entityList;
        for (const EntityId& owner : m_owners.Entities())
        {
            AZStd::string name;
            AZ::ComponentApplicationBus::BroadcastResult(
                name, &AZ::ComponentApplicationRequests::GetEntityName, owner);

            if (!entityList.empty())
            {
                entityList += ", ";
            }
            entityList += name.empty() ? AZStd::string("<unnamed>") : name;
            entityList += AZStd::string::format(" [%llu]", static_cast<AZ::u64>(owner));
        }

        AZ_Warning("GradientGI", false,
            "%zu Gradient GI components are lighting this scene at the same time: %s. "
            "Only the most recently activated one has any effect -- the others are redundant. "
            "If this is not deliberate, one of them is probably a duplicated entity or prefab.",
            m_owners.Count(), entityList.c_str());
    }

    void GradientGIFeatureProcessor::ReleaseGeneratedImages()
    {
        DestroyPass();
        m_diffuseImage  = nullptr;
        m_specularImage = nullptr;
        m_cubemapImage  = nullptr;
        m_imageAsset    = {};
        m_needsRebuild  = false;
    }

    // =========================================================================
    // CacheSceneSrgIndices -- cache IBL slot indices from the scene SRG
    // =========================================================================

    void GradientGIFeatureProcessor::CacheSceneSrgIndices()
    {
        m_sceneSrg = GetParentScene()->GetShaderResourceGroup();
        if (m_sceneSrg)
        {
            m_specularEnvMapIndex = m_sceneSrg->FindShaderInputImageIndex(AZ::Name("m_specularEnvMap"));
            m_diffuseEnvMapIndex  = m_sceneSrg->FindShaderInputImageIndex(AZ::Name("m_diffuseEnvMap"));
            m_iblExposureIndex    = m_sceneSrg->FindShaderInputConstantIndex(AZ::Name("m_iblExposure"));
            m_sceneSrgIndicesCached = true;
        }
        else
        {
            AZ_Error("GradientGI", false, "Scene SRG is NULL! Dynamic mode cannot write IBL slots.");
        }
    }

    // =========================================================================
    // Simulate -- Static mode cubemap rebuild (runs on a job thread)
    // =========================================================================

    void GradientGIFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& /*packet*/)
    {
        AZ_PROFILE_SCOPE(RPI, "GradientGIFeatureProcessor: Simulate");

        if (!m_enabled || m_updateMode != UpdateMode::Static || !m_iblFeatureProcessor)
        {
            return;
        }

        // --- Rebuild the CPU cubemap when parameters changed (max once per frame) ----
        // Each rebuild allocates a fresh StreamingImage (the type is immutable, so it cannot
        // be updated in place). One rebuild per frame is safe and matches a tick-driven script
        // changing colors at runtime. The editor color picker, however, can fire several
        // changes within a single frame; m_rebuiltThisFrame (cleared each frame in Render())
        // caps us to one rebuild per frame regardless. The latest values are buffered in the
        // member colors, so a coalesced rebuild always uses the final values.
        if (m_needsRebuild && !m_rebuiltThisFrame)
        {
            // Release the previous cubemap before building its replacement so we never hold
            // two full cubemaps in the streaming pool at once.
            m_cubemapImage = nullptr;
            m_imageAsset   = {};

            m_cubemapImage     = BuildGradientCubemap();
            m_needsRebuild     = false;
            m_rebuiltThisFrame = true;
        }

        if (!m_cubemapImage || !m_imageAsset.GetId().IsValid())
        {
            return;
        }

        // --- IBL delegation ------------------------------------------------------
        // The IBL feature processor is the sole writer of the scene SRG IBL slots, so we hand
        // it our cubemap rather than writing the slots ourselves. Re-assert whenever the slots
        // drift away from our image -- a fresh rebuild, a pipeline rebuild, or another IBL
        // provider resetting them. One pointer compare per frame makes the delegation
        // self-healing, instead of depending on lifecycle events firing in a particular order.
        if (!OwnsIblSlots())
        {
            m_iblFeatureProcessor->SetSpecularImage(m_imageAsset);
            m_iblFeatureProcessor->SetDiffuseImage(m_imageAsset);
        }
        m_iblFeatureProcessor->SetExposure(m_exposure);
    }

    // =========================================================================
    // Dynamic Mode -- GPU compute pass lifecycle
    // =========================================================================

    void GradientGIFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        if (!m_enabled)
        {
            return;
        }

        EnsurePass(renderPipeline);
    }

    bool GradientGIFeatureProcessor::NeedsPassRehost() const
    {
        if (!m_gradientPassPtr)
        {
            return true;
        }

        // Detached from its pass tree by a rebuild.
        if (!m_gradientPassPtr->GetParent())
        {
            return true;
        }

        // Host pipeline no longer present in the scene. A removed pipeline can leave our pass with
        // a stale but non-null parent, in which case the pass never dispatches again and the
        // cubemap we keep binding to the scene SRG is never written.
        return GetParentScene()->GetRenderPipeline(m_passPipelineId) == nullptr;
    }

    void GradientGIFeatureProcessor::EnsurePass(RPI::RenderPipeline* renderPipeline)
    {
        if (m_updateMode != UpdateMode::Dynamic)
        {
            return;
        }

        if (!NeedsPassRehost())
        {
            return;
        }

        // Choose the host BEFORE tearing the old pass down, since DestroyPass clears the recorded
        // host id: an explicitly supplied pipeline first, then the pipeline that hosted us if it is
        // still alive, then the scene's default.
        RPI::RenderPipelinePtr keepAlive;
        if (!renderPipeline)
        {
            keepAlive = GetParentScene()->GetRenderPipeline(m_passPipelineId);
            if (!keepAlive)
            {
                keepAlive = GetParentScene()->GetDefaultRenderPipeline();
            }
            renderPipeline = keepAlive.get();
        }

        if (!renderPipeline)
        {
            // No pipeline yet -- normal when a component activates before the scene's pipeline is
            // registered. AddRenderPasses injects the pass once one appears.
            return;
        }

        // Do not re-attempt a host we already know we cannot anchor into; the per-frame health
        // check would otherwise allocate and discard a pass every frame.
        if (renderPipeline->GetId() == m_failedHostPipelineId)
        {
            return;
        }

        // Drop the stale pass before hosting a fresh one.
        DestroyPass();

        RPI::PassDescriptor passDescriptor(AZ::Name("GradientGICubemapPass"));
        m_gradientPassPtr = aznew GradientGICubemapPass(passDescriptor);
        m_gradientPass    = m_gradientPassPtr.get();

        // Hand over the output cubemaps from the previous instance of the pass. When they match the
        // current face size the replacement adopts them, so a pass-tree rebuild costs no allocation
        // and the scene keeps sampling valid contents instead of an unwritten (black) cubemap.
        m_gradientPass->AdoptOutputImages(m_diffuseImage, m_specularImage);

        // Push current gradient state into the pass before it starts running.
        m_gradientPass->SetGradientColors(m_lowColor, m_midColor, m_highColor, m_exposure, m_faceResolution);
        m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        m_gradientPass->SetEnabled(m_enabled);

        // Inject before DepthPrePass so it runs early in the frame.
        // The cubemap is ready by the time IBL sampling occurs later in the pipeline.
        const bool injected = renderPipeline->AddPassBefore(m_gradientPassPtr, AZ::Name("DepthPrePass"));

        if (!injected)
        {
            // Auxiliary pipelines (the BRDF LUT pipeline, for one) have no DepthPrePass to anchor
            // against. Never keep a pass we could not inject: Render() would bind the cubemap of a
            // pass that never dispatches and the scene would go black, instead of falling through
            // to the IBL default.
            m_failedHostPipelineId = renderPipeline->GetId();
            DestroyPass();
            return;
        }

        m_passPipelineId = renderPipeline->GetId();
    }

    void GradientGIFeatureProcessor::DestroyPass()
    {
        if (!m_gradientPassPtr)
        {
            return;
        }

        // A pipeline rebuild can orphan the pass (null parent). Only queue removal while it is
        // still in the tree; otherwise just release our reference.
        if (m_gradientPassPtr->GetParent())
        {
            m_gradientPassPtr->QueueForRemoval();
        }
        m_gradientPassPtr = nullptr;
        m_gradientPass    = nullptr;
        m_passPipelineId  = RPI::RenderPipelineId();
    }

    void GradientGIFeatureProcessor::EnsureIblFloor()
    {
        // Only when nothing has ever populated the IBL slots. A provider that has set an image
        // (a Global Skylight, or our own Static cubemap) is left strictly alone.
        if (m_iblFeatureProcessor && !m_iblFeatureProcessor->GetSpecularImage())
        {
            m_iblFeatureProcessor->Reset();
        }
    }

    // =========================================================================
    // OnRenderPipelineChanged -- re-establish state after a pipeline pass rebuild
    // =========================================================================

    void GradientGIFeatureProcessor::OnRenderPipelineChanged(
        RPI::RenderPipeline* renderPipeline,
        RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        using ChangeType = RPI::SceneNotification::RenderPipelineChangeType;

        // Nothing to re-establish while no component owns us. Doing so would resurrect the gradient
        // with nothing alive to drive it -- which is exactly how the lighting used to survive into
        // game mode and fight the spawned copy of itself.
        if (!m_enabled)
        {
            return;
        }

        // The set of pipelines changed, so a host we previously failed to inject into is worth
        // trying again.
        m_failedHostPipelineId = RPI::RenderPipelineId();

        // Our host pipeline is going away and takes the pass with it. Release the reference now
        // rather than waiting to notice: the pass keeps a stale non-null parent through the swap.
        if (changeType == ChangeType::Removed && renderPipeline &&
            renderPipeline->GetId() == m_passPipelineId)
        {
            m_gradientPassPtr = nullptr;
            m_gradientPass    = nullptr;
            m_passPipelineId  = RPI::RenderPipelineId();
        }

        if (m_updateMode == UpdateMode::Dynamic)
        {
            // The scene SRG can be rebuilt across a pipeline change, so re-cache the slot indices.
            m_sceneSrgIndicesCached = false;
            CacheSceneSrgIndices();

            // Re-host promptly when a pipeline is added, so the gradient is present on the very
            // first frame it renders. Every other case is left to the per-frame health check in
            // OnBeginPrepareRender -- a PassChanged notification arrives *before* the rebuild
            // detaches our pass, so repairing from this notification alone always misses it.
            if (changeType == ChangeType::Added)
            {
                EnsurePass(renderPipeline);
            }
        }

        // Static needs nothing here: Simulate() re-asserts the cubemap whenever the IBL slots
        // drift away from it, so a pipeline swap repairs itself on the next frame.
    }

    // =========================================================================
    // OnBeginPrepareRender -- per-frame pass health check (main thread)
    // =========================================================================

    void GradientGIFeatureProcessor::OnBeginPrepareRender()
    {
        // While no component wants us, run down the teardown grace period instead of re-hosting.
        if (!m_enabled)
        {
            TickDeferredTeardown();
            return;
        }

        TickMultipleOwnerReport();

        if (m_updateMode != UpdateMode::Dynamic || !NeedsPassRehost())
        {
            return;
        }

        EnsurePass();
    }

    // =========================================================================
    // Render -- Dynamic mode: write AttachmentImage to scene SRG IBL slots
    // =========================================================================

    void GradientGIFeatureProcessor::Render(const FeatureProcessor::RenderPacket& /*packet*/)
    {
        AZ_PROFILE_SCOPE(RPI, "GradientGIFeatureProcessor: Render");

        // Frame boundary: Render() is called every frame (Simulate() may not be), so this is
        // the reliable place to re-arm the once-per-frame CPU rebuild cap.
        m_rebuiltThisFrame = false;

        // Commit a deferred resolution change once the slider has settled (both modes).
        TickResolutionSettle();

        if (!m_enabled || m_updateMode != UpdateMode::Dynamic || !m_gradientPass || !m_sceneSrg)
        {
            return;
        }

        WriteSceneSrgFromPass();
    }

    void GradientGIFeatureProcessor::WriteSceneSrgFromPass()
    {
        auto diffuseImage  = m_gradientPass->GetDiffuseImage();
        auto specularImage = m_gradientPass->GetSpecularImage();
        if (!diffuseImage || !specularImage)
        {
            // The pass has not produced its output images yet -- BuildInternal has not run, because
            // the pass was only just injected and has not been built into the tree.
            return;
        }

        // Write the gradient cubemaps to their respective IBL slots. This runs on the render
        // thread after all Simulate() jobs have completed, so there is no race with the IBL FP.
        // The cubemap SRV (default view set at image creation) is used for sampling. In I.1 the
        // two images hold identical gradient data; later phases composite distinct detail and
        // specular layers so the diffuse and specular slots diverge.

        // Cache the pass's outputs so they outlive it. A pass-tree rebuild discards the pass and we
        // host a replacement; EnsurePass hands these back to it. The size is re-checked on adoption,
        // so a resolution change simply allocates a new pair rather than reusing a mismatched one.
        m_diffuseImage  = diffuseImage;
        m_specularImage = specularImage;

        m_sceneSrg->SetImageView(m_specularEnvMapIndex, specularImage->GetImageView());
        m_sceneSrg->SetImageView(m_diffuseEnvMapIndex,  diffuseImage->GetImageView());
        m_sceneSrg->SetConstant(m_iblExposureIndex, m_exposure);
    }

    // =========================================================================
    // Public API
    // =========================================================================

    void GradientGIFeatureProcessor::SetGradientColors(
        const Color& low, const Color& mid, const Color& high)
    {
        // Idempotent. The controller re-pushes the entire configuration on every Activate, and the
        // editor cycles Deactivate/Activate on every property edit and every game mode transition.
        // Treating an identical push as a change would rebuild the CPU cubemap (a fresh
        // StreamingImage and GPU upload) each time, which reads as a flicker on the way back into
        // the editor.
        if (low == m_lowColor && mid == m_midColor && high == m_highColor)
        {
            return;
        }

        m_lowColor  = low;
        m_midColor  = mid;
        m_highColor = high;

        if (m_updateMode == UpdateMode::Static)
        {
            m_needsRebuild = true;
        }
        else if (m_gradientPass)
        {
            m_gradientPass->SetGradientColors(low, mid, high, m_exposure, m_faceResolution);
        }
    }

    void GradientGIFeatureProcessor::SetExposure(float exposureStops)
    {
        m_exposure = exposureStops;

        // Static: exposure is pushed to the IBL FP from Simulate(), and only while we own
        // the slots -- so a foreign IBL's exposure is never disturbed, and the floor value
        // (~2^-20) reads as effectively black without conflicting with other lighting.
        // Dynamic: update the live compute pass.
        if (m_updateMode == UpdateMode::Dynamic && m_gradientPass)
        {
            // Push full color state (exposure is embedded in the shader SRG).
            m_gradientPass->SetGradientColors(m_lowColor, m_midColor, m_highColor, m_exposure, m_faceResolution);
        }
    }

    void GradientGIFeatureProcessor::SetFaceResolution(uint32_t resolution)
    {
        const uint32_t clamped = GradientGI::ClampFaceResolution(resolution);

        // GPU/Dynamic mode reallocates a lightweight AttachmentImage (NOT the DX12 tiled streaming
        // pool), so a resolution change is cheap and safe to apply live every tick. Applying it
        // immediately also avoids the settle delay, which otherwise reads as a flicker while the
        // resolution slider is scrubbed. Only CPU/Static mode -- which churns the tiled streaming
        // pool and crashes the RHI on a size change -- needs the debounce below.
        if (m_updateMode == UpdateMode::Dynamic)
        {
            m_resolutionThrottle.Commit(clamped);
            ApplyFaceResolution(clamped);
            return;
        }

        // The very first push (component activation, no scrub) applies immediately so the initial
        // build uses the configured resolution with no transient flash at the default size.
        if (!m_faceResolutionApplied)
        {
            m_resolutionThrottle.Commit(clamped);
            ApplyFaceResolution(clamped);
            return;
        }

        // CPU/Static while editing: defer to the scrub throttle. The build keeps using the previous
        // stable size until the slider settles -- safe, exactly like a colour scrub.
        m_resolutionThrottle.Request(clamped);
    }

    void GradientGIFeatureProcessor::ApplyFaceResolution(uint32_t value)
    {
        // Idempotent for the same reason as SetGradientColors -- an unchanged resolution must not
        // cost a rebuild. The first push still applies, so activation always establishes a size.
        if (m_faceResolutionApplied && value == m_faceResolution)
        {
            return;
        }

        m_faceResolution        = value;
        m_faceResolutionApplied = true;

        if (m_updateMode == UpdateMode::Static)
        {
            m_needsRebuild = true;
        }
        else if (m_gradientPass)
        {
            // Dynamic mode: push the new resolution to the live pass, which reallocates its
            // output cubemap (GradientGICubemapPass::SetGradientColors queues a pass rebuild
            // when the face size changes).
            m_gradientPass->SetGradientColors(m_lowColor, m_midColor, m_highColor, m_exposure, m_faceResolution);
        }
    }

    void GradientGIFeatureProcessor::TickResolutionSettle()
    {
        // Commit the one and only reallocation at the final size once the slider has settled.
        if (const AZStd::optional<uint32_t> settled = m_resolutionThrottle.AdvanceFrame())
        {
            ApplyFaceResolution(*settled);
        }
    }

    void GradientGIFeatureProcessor::SetUpdateMode(UpdateMode requestedMode)
    {
        // GPU/Dynamic requires compute UAV cubemap support; fall back to Static if unavailable.
        const UpdateMode mode =
            GradientGI::ResolveUpdateMode(requestedMode, GradientGICubemapPass::IsGpuComputeSupported());
        AZ_Warning("GradientGI", mode == requestedMode,
            "GPU compute UAV cubemaps not supported. Staying in Static mode.");

        if (m_updateMode == mode)
        {
            return;
        }

        m_updateMode = mode;

        // =================================================================
        // Switch TO Static
        // =================================================================
        if (mode == UpdateMode::Static)
        {
            // Release the GPU path entirely -- the cached output cubemaps are of no use to the CPU
            // path and would otherwise sit in the attachment pool until the scene is torn down.
            DestroyPass();
            m_diffuseImage  = nullptr;
            m_specularImage = nullptr;

            // Rebuild the CPU cubemap so the IBL FP picks it up again -- but only if a component
            // is actually driving us; an idle feature processor stays idle.
            m_needsRebuild = m_enabled;
        }
        // =================================================================
        // Switch TO Dynamic
        // =================================================================
        else
        {
            if (!m_sceneSrgIndicesCached)
            {
                CacheSceneSrgIndices();
            }

            // Disengage the IBL FP from our Static image so it stops writing it in Simulate();
            // Dynamic drives the scene SRG slots directly from Render(). Guarded so we never
            // wipe another provider's IBL.
            if (OwnsIblSlots())
            {
                m_iblFeatureProcessor->Reset();
            }
            m_cubemapImage = nullptr;
            m_imageAsset   = {};

            if (m_enabled)
            {
                EnsurePass();
            }
        }
    }

    void GradientGIFeatureProcessor::SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
    {
        // Resolve the asset to a resident runtime instance once. Color edits never touch this;
        // they only update SRG constants, so the texture stays loaded (the cheap-rebuild goal).
        m_detailTexture = texture.GetId().IsValid()
            ? RPI::StreamingImage::FindOrCreate(texture)
            : Data::Instance<RPI::Image>{};

        if (m_gradientPass)
        {
            m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        }
    }

    void GradientGIFeatureProcessor::SetDetailParams(uint8_t mapping, uint8_t blend, float strength)
    {
        m_detailMapping  = mapping;
        m_detailBlend    = blend;
        m_detailStrength = strength;

        if (m_gradientPass)
        {
            m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        }
    }

    void GradientGIFeatureProcessor::SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
    {
        m_specularTexture = texture.GetId().IsValid()
            ? RPI::StreamingImage::FindOrCreate(texture)
            : Data::Instance<RPI::Image>{};

        if (m_gradientPass)
        {
            m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        }
    }

    void GradientGIFeatureProcessor::SetSpecularParams(uint8_t mapping, uint8_t blend, float strength)
    {
        m_specularMapping  = mapping;
        m_specularBlend    = blend;
        m_specularStrength = strength;

        if (m_gradientPass)
        {
            m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        }
    }

    GradientGIFeatureProcessorInterface::UpdateMode GradientGIFeatureProcessor::GetUpdateMode() const
    {
        return m_updateMode;
    }

    bool GradientGIFeatureProcessor::OwnsIblSlots() const
    {
        return m_iblFeatureProcessor && m_cubemapImage &&
               m_iblFeatureProcessor->GetSpecularImage().get() == m_cubemapImage.get();
    }

    // =========================================================================
    // Format Selection
    // =========================================================================

    RHI::Format GradientGIFeatureProcessor::ChooseBestFormat() const
    {
        return RHI::Format::R16G16B16A16_FLOAT;
    }

    // =========================================================================
    // Cubemap Direction Mapping
    // =========================================================================

    Vector3 GradientGIFeatureProcessor::CubeFaceDirection(uint32_t face, float uc, float vc)
    {
        // Standard cubemap convention: vc increases downward in texture space,
        // so side faces use -vc for Y (up) to keep the gradient oriented correctly.
        switch (face)
        {
        case 0: return Vector3( 1.0f,  -vc,  -uc);   // +X
        case 1: return Vector3(-1.0f,  -vc,   uc);   // -X
        case 2: return Vector3(   uc,  1.0f,   vc);   // +Y (zenith)
        case 3: return Vector3(   uc, -1.0f,  -vc);   // -Y (nadir)
        case 4: return Vector3(   uc,  -vc,  1.0f);   // +Z
        case 5: return Vector3(  -uc,  -vc, -1.0f);   // -Z
        default: return Vector3(0.0f, 1.0f, 0.0f);
        }
    }

    // =========================================================================
    // Face Pixel Generation
    // =========================================================================

    void GradientGIFeatureProcessor::GenerateFacePixels(
        uint32_t face,
        uint32_t faceSize,
        RHI::Format format,
        AZStd::vector<uint8_t>& outPixels) const
    {
        const size_t bpp = GradientGI::GetBytesPerPixel(format);
        outPixels.resize(faceSize * faceSize * bpp);

        for (uint32_t y = 0; y < faceSize; ++y)
        {
            for (uint32_t x = 0; x < faceSize; ++x)
            {
                float uc = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;
                float vc = (2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;

                Vector3 dir = CubeFaceDirection(face, uc, vc);
                dir.NormalizeSafe();

                float t = (dir.GetY() + 1.0f) * 0.5f;

                Color c;
                if (t < 0.5f)
                {
                    c = m_lowColor.Lerp(m_midColor, t * 2.0f);
                }
                else
                {
                    c = m_midColor.Lerp(m_highColor, (t - 0.5f) * 2.0f);
                }

                size_t offset = (static_cast<size_t>(y) * faceSize + x) * bpp;
                GradientGI::WritePixel(outPixels.data() + offset, c.GetR(), c.GetG(), c.GetB(), 1.0f, format);
            }
        }
    }

    // =========================================================================
    // Cubemap Asset Construction (Static mode)
    // =========================================================================

    Data::Instance<RPI::StreamingImage> GradientGIFeatureProcessor::BuildGradientCubemap()
    {
        const RHI::Format format   = ChooseBestFormat();
        // Snap to a power of two: O3DE's runtime StreamingImage upload renders certain non-power-of-
        // two face sizes black (observed dead band ~91..127 at R16G16B16A16). Imperceptible for a
        // smooth gradient. See GradientGI::SnapCpuFaceResolutionToPow2.
        const uint32_t    faceSize = GradientGI::SnapCpuFaceResolutionToPow2(m_faceResolution);

        // Step 1: Generate pixel data for each of the 6 faces
        AZStd::array<AZStd::vector<uint8_t>, 6> facePixels;
        for (uint32_t face = 0; face < 6; ++face)
        {
            GenerateFacePixels(face, faceSize, format, facePixels[face]);
        }

        // Step 2: Compute subresource layout for one face
        RHI::Size faceDimensions(faceSize, faceSize, 1);
        RHI::DeviceImageSubresourceLayout faceLayout = RHI::GetImageSubresourceLayout(faceDimensions, format);
        const size_t bytesPerFace = faceLayout.m_bytesPerImage;

        // Step 3: Build ImageMipChainAsset (1 mip level, 6 array slices)
        Data::Asset<RPI::ImageMipChainAsset> mipChainAsset;
        {
            RPI::ImageMipChainAssetCreator mipCreator;
            mipCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), /*mipLevels=*/1, /*arraySize=*/6);

            mipCreator.BeginMip(faceLayout);
            for (uint32_t face = 0; face < 6; ++face)
            {
                AZ_Assert(facePixels[face].size() >= bytesPerFace,
                    "GradientGI: Face pixel data too small (expected %zu, got %zu)",
                    bytesPerFace, facePixels[face].size());

                mipCreator.AddSubImage(facePixels[face].data(), bytesPerFace);
            }
            mipCreator.EndMip();

            if (!mipCreator.End(mipChainAsset))
            {
                AZ_Error("GradientGI", false, "Failed to create ImageMipChainAsset.");
                return nullptr;
            }
        }

        // Step 4: Build StreamingImageAsset
        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead,
            faceSize,
            format);

        auto* imageSystem = RPI::ImageSystemInterface::Get();
        if (!imageSystem)
        {
            AZ_Error("GradientGI", false, "ImageSystemInterface not available.");
            return nullptr;
        }

        {
            RPI::StreamingImageAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            assetCreator.SetImageDescriptor(imageDesc);
            assetCreator.SetImageViewDescriptor(RHI::ImageViewDescriptor::CreateCubemap());
            assetCreator.SetFlags(RPI::StreamingImageFlags::NotStreamable);
            assetCreator.SetPoolAssetId(imageSystem->GetSystemStreamingPool()->GetAssetId());
            assetCreator.AddMipChainAsset(*mipChainAsset.Get());

            if (!assetCreator.End(m_imageAsset))
            {
                AZ_Error("GradientGI", false, "Failed to create StreamingImageAsset.");
                return nullptr;
            }
        }

        // Step 5: Create runtime GPU image instance
        Data::Instance<RPI::StreamingImage> image = RPI::StreamingImage::FindOrCreate(m_imageAsset);
        if (!image)
        {
            AZ_Error("GradientGI", false, "Failed to create StreamingImage instance.");
            return nullptr;
        }

        return image;
    }

} // namespace AZ::Render
