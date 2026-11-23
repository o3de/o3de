/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/GradientGlobalIllumination/GradientGlobalIlluminationFeatureProcessorInterface.h>
#include <Atom/Feature/GradientGlobalIllumination/GradientGlobalIlluminationLogic.h>
#include <ImageBasedLights/ImageBasedLightFeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RHI/ShaderResourceGroup.h>

namespace AZ::Render
{
    class GradientGlobalIlluminationCubemapPass;

    class GradientGlobalIlluminationFeatureProcessor final
        : public GradientGlobalIlluminationFeatureProcessorInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientGlobalIlluminationFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI(
            AZ::Render::GradientGlobalIlluminationFeatureProcessor,
            "{F2E7A3B1-D5C8-4F9A-B0E6-7C1D3A8F5E02}",
            AZ::Render::GradientGlobalIlluminationFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        GradientGlobalIlluminationFeatureProcessor() = default;
        ~GradientGlobalIlluminationFeatureProcessor() override = default;

        void Activate() override;
        void Deactivate() override;
        void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
        void Render(const FeatureProcessor::RenderPacket& packet) override;
        void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;

        void OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType)
            override;

        //! Publish CPU IBL state and repair GPU pass hosting after simulation jobs complete.
        void OnBeginPrepareRender() override;

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
        void ConnectToOwnerChangedEvent(OwnerChangedEvent::Handler& handler) override;
        EntityId GetCurrentOwner() const override;

    private:
        using PassPtr = AZStd::intrusive_ptr<GradientGlobalIlluminationCubemapPass>;

        GradientGlobalIlluminationFeatureProcessor(const GradientGlobalIlluminationFeatureProcessor&) = delete;

        void GenerateFacePixels(uint32_t face, uint32_t faceSize, AZStd::vector<uint8_t>& outPixels) const;

        Data::Instance<RPI::StreamingImage> BuildGradientCubemap();

        static Vector3 CubeFaceDirection(uint32_t face, float uc, float vc);

        void ApplyFaceResolution(uint32_t value);

        void TickResolutionSettle();

        //! Stop publishing output while retaining resources for the teardown grace period.
        void ReleaseOutput();

        //! Host the GPU pass in the supplied pipeline, its surviving host, or the scene default.
        void EnsurePass(RPI::RenderPipeline* renderPipeline = nullptr);

        void DestroyPass();

        //! Release cached output after the disabled grace period expires.
        void TickDeferredTeardown();

        void ReleaseGeneratedImages();

        void ReportMultipleOwners() const;

        void TickMultipleOwnerReport();

        bool NeedsPassRehost() const;



    private:
        ImageBasedLightFeatureProcessor* m_iblFeatureProcessor = nullptr;

        Color m_lowColor = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color m_midColor = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color m_highColor = Color(0.85f, 0.95f, 1.0f, 1.0f);
        float m_exposure = 0.0f;
        uint32_t m_faceResolution = 64; // size builds actually use; held steady during a scrub

        // Keep the committed CPU size while resolution edits settle.
        GradientGlobalIllumination::ResolutionDebounce m_resolutionDebounce;
        bool m_faceResolutionApplied = false; // first push applies immediately

        // Mapping and blend codes match the component enums.
        Data::Instance<RPI::Image> m_detailTexture;
        uint8_t m_detailMapping = 1; // Stretched
        uint8_t m_detailBlend = 3; // Overlay
        float m_detailStrength = 1.0f;

        Data::Instance<RPI::Image> m_specularTexture;
        uint8_t m_specularMapping = 1; // Stretched
        uint8_t m_specularBlend = 3; // Overlay
        float m_specularStrength = 1.0f;

        UpdateMode m_updateMode = UpdateMode::Static;

        GradientGlobalIllumination::OwnerRegistry m_owners;
        OwnerChangedEvent m_ownerChangedEvent;
        bool m_enabled = false;

        // Delay warnings to tolerate transient editor/play overlap. Warn again if the owner count changes.
        static constexpr uint32_t MultiOwnerReportFrames = 30;
        uint32_t m_multiOwnerFrames = 0;
        size_t m_reportedOwnerCount = 0;

        Data::Instance<RPI::StreamingImage> m_cubemapImage;
        Data::Asset<RPI::StreamingImageAsset> m_imageAsset;
        bool m_needsRebuild = false;

        // Coalesce repeated simulation updates until Render starts the next frame.
        bool m_rebuiltThisFrame = false;

        PassPtr m_gradientPass;

        // A removed pipeline can leave a non-null pass parent, so track the host ID as well.
        RPI::RenderPipelineId m_passPipelineId;

        // Avoid retrying an unsupported host until the scene pipelines change.
        RPI::RenderPipelineId m_failedHostPipelineId;

        // Retain initialized outputs across pass-tree rebuilds.
        Data::Instance<RPI::AttachmentImage> m_diffuseImage;
        Data::Instance<RPI::AttachmentImage> m_specularImage;

        // Retain output briefly across editor Deactivate/Activate cycles, then release it if unused.
        static constexpr uint8_t TeardownGraceFrames = 5;
        uint8_t m_teardownCountdown = 0;
    };
} // namespace AZ::Render
