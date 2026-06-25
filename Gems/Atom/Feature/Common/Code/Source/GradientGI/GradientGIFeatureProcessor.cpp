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

        m_needsRebuild = false;
        m_active       = false;

        // Static mode: trigger an immediate CPU rebuild.
        if (m_updateMode == UpdateMode::Static)
        {
            m_needsRebuild = true;
        }

        // Cache the scene SRG IBL slot indices for Dynamic mode's direct writes.
        if (m_updateMode == UpdateMode::Dynamic && !m_sceneSrgIndicesCached)
        {
            CacheSceneSrgIndices();
        }
    }

    void GradientGIFeatureProcessor::SafeRemoveDynamicPass()
    {
        if (m_gradientPassPtr)
        {
            // Pipeline rebuilds can orphan dynamically injected passes, setting
            // their parent pointer to null. Only queue for removal if the pass
            // still has a parent; otherwise just release our reference.
            if (m_gradientPassPtr->GetParent())
            {
                m_gradientPassPtr->QueueForRemoval();
            }
            m_gradientPassPtr = nullptr;
            m_gradientPass    = nullptr;
        }
    }

    void GradientGIFeatureProcessor::EnsureDynamicPassExists()
    {
        // Check for orphaned pass (parent null after pipeline rebuild)
        if (m_gradientPassPtr && !m_gradientPassPtr->GetParent())
        {
            m_gradientPassPtr = nullptr;
            m_gradientPass    = nullptr;
        }

        if (!m_gradientPassPtr)
        {
            auto defaultPipeline = GetParentScene()->GetDefaultRenderPipeline();
            if (defaultPipeline)
            {
                CreateAndInjectPass(defaultPipeline.get());
            }
        }
    }

    void GradientGIFeatureProcessor::Deactivate()
    {
        // Remove the GPU pass from the pipeline, then drop our reference.
        SafeRemoveDynamicPass();

        // Release the IBL slots for Static mode -- but only if we still own them, so we
        // never wipe a foreign IBL we were yielding to.
        if (m_updateMode == UpdateMode::Static && OwnsIblSlots())
        {
            m_iblFeatureProcessor->Reset();
        }

        m_cubemapImage  = nullptr;
        m_imageAsset    = {};
        m_active        = false;
        m_needsRebuild  = false;
        m_iblFeatureProcessor = nullptr;
        m_sceneSrg      = nullptr;
        m_sceneSrgIndicesCached = false;
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

        if (m_updateMode != UpdateMode::Static || !m_iblFeatureProcessor)
        {
            return;
        }

        // Capture slot ownership BEFORE a rebuild swaps our cubemap instance out.
        // We "own" the slots when the IBL FP is currently holding the exact image we
        // last published. A foreign owner is any other non-default image (e.g. a Global
        // Skylight on another entity that activated after us).
        const RPI::Image* slotImage = m_iblFeatureProcessor->GetSpecularImage().get();
        const bool weOwnedSlots    = (m_cubemapImage && slotImage == m_cubemapImage.get());
        const bool foreignOwnsSlots = m_iblFeatureProcessor->IsImageSet() && !weOwnedSlots;

        // --- Rebuild the CPU cubemap when parameters changed (max once per frame) ----
        // Each rebuild allocates a fresh StreamingImage (the type is immutable, so it cannot
        // be updated in place). One rebuild per frame is safe and matches a tick-driven script
        // changing colors at runtime. The editor color picker, however, can fire several
        // changes within a single frame; m_rebuiltThisFrame (cleared each frame in Render())
        // caps us to one rebuild per frame regardless. The latest values are buffered in the
        // member colors, so a coalesced rebuild always uses the final values.
        bool justRebuilt = false;
        if (m_needsRebuild && !m_rebuiltThisFrame)
        {
            // Release the previous cubemap before building its replacement so we never hold
            // two full cubemaps in the streaming pool at once.
            m_cubemapImage = nullptr;
            m_imageAsset   = {};

            m_cubemapImage     = BuildGradientCubemap();
            m_active           = (m_cubemapImage != nullptr);
            m_needsRebuild     = false;
            m_rebuiltThisFrame = true;
            justRebuilt        = m_active;
        }

        if (!m_active || !m_imageAsset.GetId().IsValid())
        {
            return;
        }

        // --- Cooperative IBL ownership -----------------------------------------
        // The IBL FP is the sole writer of the scene SRG IBL slots; we delegate our cubemap
        // to it. While a foreign IBL owns the slots we yield and leave them untouched. When
        // that system deactivates it Resets the IBL FP back to its default (IsImageSet()
        // becomes false) and we reclaim on the next frame. An explicit local change
        // (justRebuilt) re-asserts our cubemap.
        if (foreignOwnsSlots && !justRebuilt)
        {
            return;
        }

        // (Re)publish the image only when we just rebuilt or the slots are free to claim;
        // otherwise we already own them and just keep exposure in sync (cheap, and makes a
        // floor exposure read as ~black immediately).
        if (justRebuilt || !m_iblFeatureProcessor->IsImageSet())
        {
            m_iblFeatureProcessor->SetSpecularImage(m_imageAsset);
            m_iblFeatureProcessor->SetDiffuseImage(m_imageAsset);
        }
        m_iblFeatureProcessor->SetExposure(m_exposure);
    }

    // =========================================================================
    // AddRenderPasses -- Dynamic mode: inject GPU compute pass into the pipeline
    // =========================================================================

    void GradientGIFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        if (m_updateMode != UpdateMode::Dynamic)
        {
            return;
        }

        // Detect orphaned pass (detached from pipeline tree by a rebuild).
        if (m_gradientPassPtr && !m_gradientPassPtr->GetParent())
        {
            m_gradientPassPtr = nullptr;
            m_gradientPass    = nullptr;
        }

        // Only create one pass (for the first pipeline that registers with this FP).
        if (m_gradientPassPtr)
        {
            return;
        }

        CreateAndInjectPass(renderPipeline);
    }

    void GradientGIFeatureProcessor::CreateAndInjectPass(RPI::RenderPipeline* renderPipeline)
    {
        RPI::PassDescriptor passDescriptor(AZ::Name("GradientGICubemapPass"));
        m_gradientPassPtr = aznew GradientGICubemapPass(passDescriptor);
        m_gradientPass    = m_gradientPassPtr.get();

        // Push current gradient state into the pass before it starts running.
        m_gradientPass->SetGradientColors(m_lowColor, m_midColor, m_highColor, m_exposure, m_faceResolution);
        m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);

        // Inject before DepthPrePass so it runs early in the frame.
        // The cubemap is ready by the time IBL sampling occurs later in the pipeline.
        renderPipeline->AddPassBefore(m_gradientPassPtr, AZ::Name("DepthPrePass"));

        m_active = true;
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

        if (m_updateMode != UpdateMode::Dynamic || !m_gradientPass || !m_sceneSrg)
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
            return;
        }

        // Write the gradient cubemaps to their respective IBL slots. This runs on the render
        // thread after all Simulate() jobs have completed, so there is no race with the IBL FP.
        // The cubemap SRV (default view set at image creation) is used for sampling. In I.1 the
        // two images hold identical gradient data; later phases composite distinct detail and
        // specular layers so the diffuse and specular slots diverge.
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
            // Same mode requested. In Dynamic mode, verify the pass is still healthy.
            if (mode == UpdateMode::Dynamic)
            {
                EnsureDynamicPassExists();
            }
            return;
        }

        m_updateMode = mode;

        // =================================================================
        // Switch TO Static
        // =================================================================
        if (mode == UpdateMode::Static)
        {
            // Tear down dynamic pass safely.
            SafeRemoveDynamicPass();

            // Trigger CPU rebuild so the IBL FP picks up our cubemap again
            m_needsRebuild = true;
        }
        // =================================================================
        // Switch TO Dynamic
        // =================================================================
        else if (mode == UpdateMode::Dynamic)
        {
            // Cache scene SRG indices if not already done
            if (!m_sceneSrgIndicesCached)
            {
                CacheSceneSrgIndices();
            }

            // Disengage the IBL FP from OUR static image so it stops writing it in Simulate().
            // Guarded so we never wipe a foreign IBL we were yielding to; Dynamic mode then
            // drives the scene SRG slots directly from Render().
            if (OwnsIblSlots())
            {
                m_iblFeatureProcessor->Reset();
            }

            // Create and inject the GPU compute pass at runtime
            if (!m_gradientPassPtr)
            {
                auto defaultPipeline = GetParentScene()->GetDefaultRenderPipeline();
                if (defaultPipeline)
                {
                    CreateAndInjectPass(defaultPipeline.get());
                }
                else
                {
                    AZ_Error("GradientGI", false,
                        "No default render pipeline found! Cannot inject compute pass.");
                }
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

    bool GradientGIFeatureProcessor::IsActive() const
    {
        return m_active;
    }

    void GradientGIFeatureProcessor::Reset()
    {
        // NOTE: We intentionally do NOT destroy the dynamic pass here.
        // Reset() is called on every Controller Deactivate/Activate cycle, including
        // property edits in the editor. Destroying and recreating the GPU compute pass
        // on every slider drag tick is expensive and causes visible stuttering.
        // The pass is only torn down when actually switching modes (SetUpdateMode)
        // or when the FP deactivates. Orphaned passes (from pipeline rebuilds) are
        // detected and handled in SetUpdateMode() and AddRenderPasses().

        if (m_updateMode == UpdateMode::Static && OwnsIblSlots())
        {
            m_iblFeatureProcessor->Reset();
        }

        m_cubemapImage = nullptr;
        m_imageAsset   = {};
        m_active       = false;
        m_needsRebuild = false;
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
