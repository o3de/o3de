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
        uint32_t m_faceResolution = 64;

        UpdateMode m_updateMode = UpdateMode::Static;
        bool       m_active     = false;

        // =====================================================================
        // State -- Static Mode
        // =====================================================================

        Data::Instance<RPI::StreamingImage>     m_cubemapImage;
        Data::Asset<RPI::StreamingImageAsset>   m_imageAsset;
        bool                                    m_needsRebuild = false;

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

        // =====================================================================
        // Diagnostic Flags (log-once guards to avoid per-frame spam)
        // =====================================================================

        bool m_diagnosticLogSimulateSkip = true;
        bool m_diagnosticLogRenderSkip   = true;
        bool m_diagnosticLogWriteSrg     = true;
    };

} // namespace AZ::Render
