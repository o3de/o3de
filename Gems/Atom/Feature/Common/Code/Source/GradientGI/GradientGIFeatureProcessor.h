/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/GradientGI/GradientGIFeatureProcessorInterface.h>
#include <Atom/Feature/GradientGI/GradientGILogic.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RHI/ShaderResourceGroup.h>

namespace AZ::Render
{
    class GradientGICubemapPass;

    class GradientGIFeatureProcessor final
        : public GradientGIFeatureProcessorInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientGIFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI(AZ::Render::GradientGIFeatureProcessor, "{F2E7A3B1-D5C8-4F9A-B0E6-7C1D3A8F5E02}", AZ::Render::GradientGIFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        GradientGIFeatureProcessor() = default;
        ~GradientGIFeatureProcessor() override = default;

        // =====================================================================
        // FeatureProcessor Overrides
        // =====================================================================

        void Activate() override;
        void Deactivate() override;
        void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
        void Render(const FeatureProcessor::RenderPacket& packet) override;
        void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;

        //! Re-establish pipeline-dependent state when a pipeline's passes are rebuilt -- e.g.
        //! entering/exiting maximized play mode. Without this our injected compute pass (Dynamic) is
        //! left orphaned and the scene IBL slots stale, so the gradient lighting stops applying.
        void OnRenderPipelineChanged(
            RPI::RenderPipeline* renderPipeline,
            RPI::SceneNotification::RenderPipelineChangeType changeType) override;

        //! Per-frame, main-thread health check for the injected compute pass.
        //!
        //! Pipeline notifications alone are not enough: a PassChanged notification can arrive
        //! *before* the rebuild actually detaches our injected pass, so a check driven purely by
        //! events sees a healthy pass and then the rebuild orphans it with no further event to
        //! react to. An orphaned pass never dispatches again, while Render() happily keeps binding
        //! its stale cubemap -- which is what left the scene black after maximized play mode.
        void OnBeginPrepareRender() override;

        // =====================================================================
        // GradientGIFeatureProcessorInterface
        // =====================================================================

        void SetGradientColors(const Color& low, const Color& mid, const Color& high) override;
        void SetExposure(float exposureStops) override;
        void SetFaceResolution(uint32_t resolution) override;
        void SetUpdateMode(UpdateMode mode) override;
        void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) override;
        void SetDetailParams(uint8_t mapping, uint8_t blend, float strength) override;
        void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) override;
        void SetSpecularParams(uint8_t mapping, uint8_t blend, float strength) override;
        UpdateMode GetUpdateMode() const override;
        void Enable(EntityId owner) override;
        void Disable(EntityId owner) override;

    private:
        GradientGIFeatureProcessor(const GradientGIFeatureProcessor&) = delete;

        // =====================================================================
        // Static Mode -- CPU StreamingImage Path
        // =====================================================================

        RHI::Format ChooseBestFormat() const;

        void GenerateFacePixels(
            uint32_t face,
            uint32_t faceSize,
            RHI::Format format,
            AZStd::vector<uint8_t>& outPixels) const;

        Data::Instance<RPI::StreamingImage> BuildGradientCubemap();

        static Vector3 CubeFaceDirection(uint32_t face, float uc, float vc);

        // =====================================================================
        // Resolution Scrub Debounce (both modes)
        // =====================================================================

        //! Apply a face resolution into m_faceResolution and trigger the rebuild/realloc.
        void ApplyFaceResolution(uint32_t value);

        //! Per-frame: advance the scrub throttle and apply a resolution change once it settles.
        void TickResolutionSettle();

        // =====================================================================
        // Ownership
        // =====================================================================

        //! Drop every output this feature processor is driving: hand the IBL slots back (Static)
        //! and silence the compute pass (Dynamic). Shared by Disable and Deactivate.
        void ReleaseOutput();

        // =====================================================================
        // Dynamic Mode -- GPU Compute Pass Path
        // =====================================================================

        //! Make sure the GPU compute pass exists and is injected into a live pipeline, creating it
        //! if it is missing, orphaned, or hosted by a pipeline that has gone away. No-op when the
        //! pass is healthy or in Static mode. Pass a pipeline to target it explicitly; otherwise
        //! the scene's default pipeline is used.
        void EnsurePass(RPI::RenderPipeline* renderPipeline = nullptr);

        //! Remove the GPU compute pass from the pipeline and drop our reference.
        void DestroyPass();

        //! Count down the teardown grace period while disabled, releasing the compute pass and the
        //! generated images once it expires. See TeardownGraceFrames.
        void TickDeferredTeardown();

        //! Drop every generated image (both modes) and the compute pass.
        void ReleaseGeneratedImages();

        //! Warn, once, that several GradientGI components are driving the scene at the same time,
        //! naming the entities so the duplicate can be found. See m_multiOwnerFrames.
        void ReportMultipleOwners() const;

        //! Per-frame check behind that warning.
        void TickMultipleOwnerReport();

        //! True when the pass is missing, was detached from its pass tree, or its host pipeline is
        //! no longer the one we should be rendering through.
        bool NeedsPassRehost() const;

        //! Guarantee the IBL feature processor holds real images. Stock O3DE leaves its specular
        //! and diffuse instances null until some component sets or resets them, and it binds them
        //! to the scene SRG every frame regardless -- so a scene whose only ambient light is
        //! GradientGI reads pure black, not the engine default, any frame we do not write. Calling
        //! Reset() once installs the engine defaults as a floor. Guarded so it never disturbs an
        //! IBL another provider has set.
        void EnsureIblFloor();

        //! Write the compute pass's AttachmentImage to the scene SRG IBL slots.
        //! Called from Render() (render thread, after all Simulate jobs complete).
        void WriteSceneSrgFromPass();

        //! True when the IBL FP currently holds the exact cubemap we last published --
        //! i.e. we own the scene IBL slots and may safely release them. Used so teardown
        //! never wipes a foreign IBL (e.g. a Global Skylight) that we were yielding to.
        bool OwnsIblSlots() const;

        // =====================================================================
        // State -- Shared
        // =====================================================================

        // IBL FP delegate (Static mode only -- single owner of scene SRG IBL slots)
        ImageBasedLightFeatureProcessorInterface* m_iblFeatureProcessor = nullptr;

        // Gradient parameters
        Color    m_lowColor       = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color    m_midColor       = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color    m_highColor      = Color(0.85f, 0.95f, 1.0f,  1.0f);
        float    m_exposure       = 0.0f;
        uint32_t m_faceResolution = 64;   // size builds actually use; held steady during a scrub

        // --- Resolution scrub debounce ---------------------------------------------------
        // A CPU/Static resolution change rebuilds a different-sized tiled StreamingImage from the
        // DX12 streaming pool. Rebuilding a different size on every frame of a slider scrub churns
        // the pool's tile allocations and crashes inside the DX12 RHI (GetResourceTiling,
        // Device.cpp). Colour is safe to rebuild live because every rebuild is the SAME size. The
        // throttle holds m_faceResolution steady during a scrub and yields the final size only once
        // the slider settles. See GradientGI::ResolutionScrubThrottle and SetFaceResolution.
        GradientGI::ResolutionScrubThrottle m_resolutionThrottle;
        bool                                m_faceResolutionApplied = false; // first push applies immediately

        // Detail texture layer (GPU/Dynamic mode). Resolved instance is resident; mapping/blend
        // codes match the component enums. Bound to the pass once, recombined cheaply on color edits.
        Data::Instance<RPI::Image> m_detailTexture;
        uint8_t                    m_detailMapping  = 1;   // Stretched
        uint8_t                    m_detailBlend    = 3;   // Overlay
        float                      m_detailStrength = 1.0f;

        // Specular texture layer (GPU/Dynamic mode). Composites into the specular output only.
        Data::Instance<RPI::Image> m_specularTexture;
        uint8_t                    m_specularMapping  = 1; // Stretched
        uint8_t                    m_specularBlend    = 3; // Overlay
        float                      m_specularStrength = 1.0f;

        UpdateMode m_updateMode = UpdateMode::Static;

        // Ownership. The feature processor is a scene-wide singleton shared by every GradientGI
        // component, so it tracks every component that wants it running -- not just the latest --
        // and only stops once the last one has gone. m_enabled is the single "should I be producing
        // output at all" gate. Every output path -- Simulate, Render, AddRenderPasses,
        // OnRenderPipelineChanged -- checks it, so a pipeline rebuild can never resurrect the
        // gradient while no component is alive to own it.
        GradientGI::OwnerRegistry m_owners;
        bool                      m_enabled = false;

        // Duplicate reporting. Several components driving the scene at once is legal but almost
        // always a mistake -- a duplicated entity or prefab that the author has not noticed, since
        // the extra one is invisible (the newest simply wins).
        //
        // Rather than trying to detect whether a scene or play-mode transition is in progress --
        // which this gem cannot see from here -- the check is simply debounced. A transition's
        // overlap is momentary, whereas a genuine duplicate is permanent, so sustained overlap is
        // the signal. Reported once per distinct count, so going from two to three warns again but
        // a steady two does not repeat.
        static constexpr uint32_t MultiOwnerReportFrames = 30;
        uint32_t                  m_multiOwnerFrames     = 0;
        size_t                    m_reportedOwnerCount   = 0;

        // =====================================================================
        // State -- Static Mode
        // =====================================================================

        Data::Instance<RPI::StreamingImage>     m_cubemapImage;
        Data::Asset<RPI::StreamingImageAsset>   m_imageAsset;
        bool                                    m_needsRebuild = false;

        // Rebuild throttle: StreamingImage is immutable, so every parameter change allocates a
        // new one. A single rebuild per frame is safe (matches a tick-driven runtime script).
        // The editor color picker, however, can fire several changes within one frame; this
        // flag (cleared each frame in Render()) caps us to one rebuild per frame. The latest
        // parameters are always buffered in the member colors, so a coalesced rebuild uses the
        // final values.
        bool                                    m_rebuiltThisFrame = false;

        // =====================================================================
        // State -- Dynamic Mode
        // =====================================================================

        // The GPU compute pass (owned via intrusive_ptr by the render pipeline;
        // m_gradientPassPtr keeps an additional reference so we can call into it).
        using PassPtr = AZStd::intrusive_ptr<GradientGICubemapPass>;
        PassPtr                m_gradientPassPtr;
        GradientGICubemapPass* m_gradientPass = nullptr;

        // Which pipeline currently hosts the pass. Maximized play mode does not merely rebuild
        // passes -- it removes the editor's pipeline and adds a new one. The orphaned pass keeps a
        // non-null parent through that swap, so "does it still have a parent" is not enough to tell
        // that its host is gone; we compare pipeline identity instead and re-inject when it changes.
        RPI::RenderPipelineId m_passPipelineId;

        // Host we failed to inject into (an auxiliary pipeline such as the BRDF LUT pipeline has no
        // DepthPrePass to anchor against). Stops the per-frame health check from allocating and
        // discarding a pass every frame; cleared whenever the scene's pipelines change.
        RPI::RenderPipelineId m_failedHostPipelineId;

        // Output cubemaps produced by the compute pass, cached here so they survive the pass.
        // A pipeline pass-tree rebuild discards the injected pass and we host a replacement; handing
        // these to it means no reallocation and no black frame while the first dispatch lands.
        Data::Instance<RPI::AttachmentImage> m_diffuseImage;
        Data::Instance<RPI::AttachmentImage> m_specularImage;

        // Deferred teardown of everything this feature processor generates.
        //
        // Disabling cannot release immediately: the editor cycles a component through
        // Deactivate/Activate on every property edit and on every game mode transition, so an
        // immediate teardown would rebuild a GPU pass -- or a CPU cubemap -- on every slider tick.
        // Retaining forever is not acceptable either: this feature processor is created once per
        // scene and never deactivates across level loads, so a deleted component (or a level with
        // no GradientGI) would keep the pass and its cubemaps alive for the rest of the session.
        // A disable therefore starts a short grace period -- a re-enable inside it cancels the
        // teardown and costs nothing, and outliving it releases everything.
        static constexpr uint8_t TeardownGraceFrames = 5;
        uint8_t                  m_teardownCountdown = 0;

        // Direct scene SRG access (borrowed; scene owns the SRG lifetime).
        Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg;
        RHI::ShaderInputImageIndex               m_specularEnvMapIndex;
        RHI::ShaderInputImageIndex               m_diffuseEnvMapIndex;
        RHI::ShaderInputConstantIndex            m_iblExposureIndex;
        bool                                     m_sceneSrgIndicesCached = false;

        //! Cache scene SRG indices (extracted for reuse during runtime mode switches).
        void CacheSceneSrgIndices();

    };

} // namespace AZ::Render
