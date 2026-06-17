/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/GradientGI/GradientGIFeatureProcessorInterface.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
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
        bool IsActive() const override;
        void Reset() override;

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

        //! Commit m_pendingFaceResolution into m_faceResolution and trigger the rebuild/realloc.
        void ApplyFaceResolution();

        //! Per-frame: advance the settle counter and commit a resolution change once it stabilises.
        void TickResolutionSettle();

        // =====================================================================
        // Dynamic Mode -- GPU Compute Pass Path
        // =====================================================================

        //! Safely remove the dynamic pass, handling the case where pipeline rebuilds
        //! have orphaned it (parent pointer null). Used by Deactivate and SetUpdateMode.
        void SafeRemoveDynamicPass();

        //! Verify the dynamic pass is still healthy; recreate if orphaned or missing.
        //! Called from SetUpdateMode "same mode" path and AddRenderPasses.
        void EnsureDynamicPassExists();

        //! Create the GPU compute pass and add it to the given pipeline.
        void CreateAndInjectPass(RPI::RenderPipeline* renderPipeline);

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
        // Changing the resolution allocates a brand-new, differently sized tiled StreamingImage
        // from the DX12 streaming pool (a reserved resource). Rebuilding a different size on every
        // frame of a slider scrub churns the pool's tile allocations and crashes inside the DX12
        // RHI (GetResourceTiling, Device.cpp). Colour is safe to rebuild live because every rebuild
        // is the SAME size. So m_faceResolution is held steady during a scrub and only advanced to
        // m_pendingFaceResolution once the slider settles -- only the final size is ever allocated.
        // See SetFaceResolution / TickResolutionSettle / ApplyFaceResolution.
        uint32_t m_pendingFaceResolution   = 64;     // latest requested size (may differ mid-scrub)
        uint8_t  m_resolutionSettleFrames  = 0;      // frames the request has held steady
        bool     m_resolutionChangePending = false;  // a settled commit is owed
        bool     m_faceResolutionApplied   = false;  // first push (initial config) applies immediately

        static constexpr uint8_t ResolutionSettleFrameThreshold = 3; // ~50ms @60fps after release

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
        bool       m_active     = false;

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
