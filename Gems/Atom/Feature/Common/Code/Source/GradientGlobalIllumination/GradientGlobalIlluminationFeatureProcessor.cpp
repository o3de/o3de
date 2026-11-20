/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientGlobalIlluminationFeatureProcessor.h"
#include "GradientGlobalIlluminationCubemapPass.h"
#include "GradientGlobalIlluminationImageUtils.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>

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
    void GradientGlobalIlluminationFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GradientGlobalIlluminationFeatureProcessor, RPI::FeatureProcessor>()->Version(0);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::Activate()
    {
        m_iblFeatureProcessor = GetParentScene()->GetFeatureProcessor<ImageBasedLightFeatureProcessor>();
        AZ_Error(
            "GradientGlobalIllumination",
            m_iblFeatureProcessor,
            "ImageBasedLightFeatureProcessorInterface not found on scene. GradientGlobalIllumination requires it.");

        const UpdateMode resolvedMode = GradientGlobalIllumination::ResolveUpdateMode(
            m_updateMode,
            m_updateMode != UpdateMode::Dynamic || GradientGlobalIlluminationCubemapPass::IsGpuComputeSupported());
        AZ_Warning(
            "GradientGlobalIllumination",
            resolvedMode == m_updateMode,
            "GPU compute UAV cubemaps are not supported on this platform. "
            "GradientGlobalIllumination falling back to Static mode.");
        m_updateMode = resolvedMode;

        m_enabled = false;
        m_needsRebuild = false;

        EnableSceneNotification();
    }

    void GradientGlobalIlluminationFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        ReleaseOutput();
        ReleaseGeneratedImages();

        m_owners.Clear();
        m_enabled = false;
        m_teardownCountdown = 0;
        m_iblFeatureProcessor = nullptr;
    }

    void GradientGlobalIlluminationFeatureProcessor::Enable(EntityId owner)
    {
        m_owners.Add(owner);

        m_teardownCountdown = 0;

        // Notify before enabling output so the new owner can stage its configuration.
        m_ownerChangedEvent.Signal(owner);
        m_enabled = true;

        if (m_updateMode == UpdateMode::Static)
        {
            // Preserve any rebuild requested by the owner while reusing an unchanged cached image.
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

    void GradientGlobalIlluminationFeatureProcessor::Disable(EntityId owner)
    {
        const EntityId previousOwner = m_owners.Current();
        m_owners.Remove(owner);

        // A level can reload all owners between frames, so reset the warning here too.
        m_multiOwnerFrames = 0;
        m_reportedOwnerCount = 0;

        if (!m_owners.Empty())
        {
            if (m_owners.Current() != previousOwner)
            {
                m_ownerChangedEvent.Signal(m_owners.Current());
            }
            return;
        }

        m_enabled = false;
        ReleaseOutput();
    }

    void GradientGlobalIlluminationFeatureProcessor::ConnectToOwnerChangedEvent(OwnerChangedEvent::Handler& handler)
    {
        handler.Connect(m_ownerChangedEvent);
    }

    EntityId GradientGlobalIlluminationFeatureProcessor::GetCurrentOwner() const
    {
        return m_owners.Current();
    }

    void GradientGlobalIlluminationFeatureProcessor::ReleaseOutput()
    {
        if (m_iblFeatureProcessor)
        {
            m_iblFeatureProcessor->SetGradientOverride({}, {}, 0);
        }
        m_detailTexture = nullptr;
        m_specularTexture = nullptr;

        if (m_gradientPass)
        {
            m_gradientPass->SetEnabled(false);
        }

        // Reuse resources if an editor reactivation occurs before the grace period expires.
        if (m_gradientPass || m_cubemapImage)
        {
            m_teardownCountdown = TeardownGraceFrames;
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::TickDeferredTeardown()
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

    void GradientGlobalIlluminationFeatureProcessor::TickMultipleOwnerReport()
    {
        const size_t ownerCount = m_owners.Count();

        if (ownerCount < 2)
        {
            m_multiOwnerFrames = 0;
            m_reportedOwnerCount = 0;
            return;
        }

        // Tolerate brief editor/play overlap before reporting duplicate components.
        ++m_multiOwnerFrames;
        if (m_multiOwnerFrames >= MultiOwnerReportFrames && ownerCount != m_reportedOwnerCount)
        {
            m_reportedOwnerCount = ownerCount;
            ReportMultipleOwners();
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::ReportMultipleOwners() const
    {
        AZStd::string entityList;
        for (const EntityId& owner : m_owners.Entities())
        {
            AZStd::string name;
            AZ::ComponentApplicationBus::BroadcastResult(name, &AZ::ComponentApplicationRequests::GetEntityName, owner);

            if (!entityList.empty())
            {
                entityList += ", ";
            }
            if (name.empty())
            {
                name = "<unnamed>";
            }
            entityList += name;
            entityList += AZStd::string::format(" [%llu]", static_cast<AZ::u64>(owner));
        }

        AZ_Warning(
            "GradientGlobalIllumination",
            false,
            "%zu Gradient Global Illumination components are lighting this scene at the same time: %s. "
            "Only the most recently activated one has any effect -- the others are redundant. "
            "If this is not deliberate, one of them is probably a duplicated entity or prefab.",
            m_owners.Count(),
            entityList.c_str());
    }

    void GradientGlobalIlluminationFeatureProcessor::ReleaseGeneratedImages()
    {
        DestroyPass();
        m_diffuseImage = nullptr;
        m_specularImage = nullptr;
        m_cubemapImage = nullptr;
        m_imageAsset = {};
        m_needsRebuild = false;
    }

    void GradientGlobalIlluminationFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket&)
    {
        AZ_PROFILE_SCOPE(RPI, "GradientGlobalIlluminationFeatureProcessor: Simulate");

        if (!m_enabled || (m_updateMode == UpdateMode::Dynamic && m_gradientPass) || !m_iblFeatureProcessor)
        {
            return;
        }

        // StreamingImage is immutable. Coalesce changes to at most one replacement per rendered frame.
        if ((m_needsRebuild || !m_cubemapImage) && !m_rebuiltThisFrame)
        {
            m_cubemapImage = nullptr;
            m_imageAsset = {};

            m_cubemapImage = BuildGradientCubemap();
            m_needsRebuild = false;
            m_rebuiltThisFrame = true;
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
    {
        if (!m_enabled)
        {
            return;
        }

        EnsurePass(renderPipeline);
    }

    bool GradientGlobalIlluminationFeatureProcessor::NeedsPassRehost() const
    {
        if (!m_gradientPass)
        {
            return true;
        }

        if (!m_gradientPass->GetParent())
        {
            return true;
        }

        // Pipeline removal can leave a stale, non-null parent on the pass.
        return GetParentScene()->GetRenderPipeline(m_passPipelineId) == nullptr;
    }

    void GradientGlobalIlluminationFeatureProcessor::EnsurePass(RPI::RenderPipeline* renderPipeline)
    {
        if (m_updateMode != UpdateMode::Dynamic)
        {
            return;
        }

        if (!NeedsPassRehost())
        {
            return;
        }

        // Choose a host before DestroyPass clears its recorded ID.
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
            // AddRenderPasses will retry when a pipeline becomes available.
            return;
        }

        // Retry failed hosts only after a pipeline change.
        if (renderPipeline->GetId() == m_failedHostPipelineId)
        {
            return;
        }

        DestroyPass();

        RPI::PassDescriptor passDescriptor(AZ_NAME_LITERAL("GradientGlobalIlluminationCubemapPass"));
        m_gradientPass = aznew GradientGlobalIlluminationCubemapPass(passDescriptor);

        // Keep initialized output available while replacing the pass.
        m_gradientPass->AdoptOutputImages(m_diffuseImage, m_specularImage);

        m_gradientPass->SetGradientColors(m_lowColor, m_midColor, m_highColor, m_faceResolution);
        m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        m_gradientPass->SetEnabled(m_enabled);

        // Dispatch before passes that sample IBL.
        AZ::Name anchor = AZ_NAME_LITERAL("DepthPrePass");
        if (!renderPipeline->FindFirstPass(anchor))
        {
            anchor = AZ_NAME_LITERAL("ForwardPass");
        }
        const bool injected = renderPipeline->FindFirstPass(anchor) && renderPipeline->AddPassBefore(m_gradientPass, anchor);

        if (!injected)
        {
            // Keep the CPU fallback active until a pipeline can host the pass.
            m_failedHostPipelineId = renderPipeline->GetId();
            DestroyPass();
            return;
        }

        m_passPipelineId = renderPipeline->GetId();
    }

    void GradientGlobalIlluminationFeatureProcessor::DestroyPass()
    {
        if (!m_gradientPass)
        {
            return;
        }

        // An orphaned pass is no longer in a tree that can process its removal.
        if (m_gradientPass->GetParent())
        {
            m_gradientPass->QueueForRemoval();
        }
        m_gradientPass = nullptr;
        m_passPipelineId = RPI::RenderPipelineId();
    }

    void GradientGlobalIlluminationFeatureProcessor::OnRenderPipelineChanged(
        RPI::RenderPipeline* renderPipeline,
        RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        using ChangeType = RPI::SceneNotification::RenderPipelineChangeType;

        if (!m_enabled)
        {
            return;
        }

        // A pipeline change can make a previously unsupported host usable.
        m_failedHostPipelineId = RPI::RenderPipelineId();

        // Drop the pass when its host is removed, even if its parent pointer remains non-null.
        if (changeType == ChangeType::Removed && renderPipeline && renderPipeline->GetId() == m_passPipelineId)
        {
            m_gradientPass = nullptr;
            m_passPipelineId = RPI::RenderPipelineId();
        }

        if (m_updateMode == UpdateMode::Dynamic)
        {
            // PassChanged can precede detachment. OnBeginPrepareRender handles the eventual orphan.
            if (changeType == ChangeType::Added)
            {
                EnsurePass(renderPipeline);
            }
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::OnBeginPrepareRender()
    {
        if (!m_enabled)
        {
            TickDeferredTeardown();
            return;
        }

        TickMultipleOwnerReport();

        // A host can accept the pass before shader or output allocation fails in BuildInternal.
        if (m_gradientPass && m_gradientPass->HasBuildFailed())
        {
            SetUpdateMode(UpdateMode::Static);
        }

        if (m_updateMode == UpdateMode::Dynamic && NeedsPassRehost())
        {
            EnsurePass();
        }

        if (!m_iblFeatureProcessor)
        {
            return;
        }

        if (m_gradientPass && m_gradientPass->HasWrittenOutputs())
        {
            m_diffuseImage = m_gradientPass->GetDiffuseImage();
            m_specularImage = m_gradientPass->GetSpecularImage();
            m_iblFeatureProcessor->SetGradientOverride(m_diffuseImage, m_specularImage, m_exposure);
            m_cubemapImage = nullptr;
            m_imageAsset = {};
        }
        else if (m_cubemapImage)
        {
            m_iblFeatureProcessor->SetGradientOverride(m_cubemapImage, m_cubemapImage, m_exposure);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::Render(const FeatureProcessor::RenderPacket&)
    {
        AZ_PROFILE_SCOPE(RPI, "GradientGlobalIlluminationFeatureProcessor: Render");

        // Render runs even when simulation is skipped, keeping the rebuild cap frame-based.
        m_rebuiltThisFrame = false;

        TickResolutionSettle();
    }

    void GradientGlobalIlluminationFeatureProcessor::SetGradientColors(const Color& low, const Color& mid, const Color& high)
    {
        // Editor reactivation reapplies configuration, so unchanged colors must not trigger an upload.
        if (low == m_lowColor && mid == m_midColor && high == m_highColor)
        {
            return;
        }

        m_lowColor = low;
        m_midColor = mid;
        m_highColor = high;

        if (!m_gradientPass)
        {
            m_needsRebuild = true;
        }
        else
        {
            m_gradientPass->SetGradientColors(low, mid, high, m_faceResolution);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::SetExposure(float exposureStops)
    {
        m_exposure = exposureStops;

        // Apply exposure at IBL sampling, without rebuilding the cubemaps.
    }

    void GradientGlobalIlluminationFeatureProcessor::SetFaceResolution(uint32_t resolution)
    {
        const uint32_t clamped = GradientGlobalIllumination::ClampFaceResolution(resolution);

        // GPU output resizes attachment images directly. Only CPU StreamingImage allocations are debounced.
        if (m_updateMode == UpdateMode::Dynamic)
        {
            m_resolutionDebounce.Commit(clamped);
            ApplyFaceResolution(clamped);
            return;
        }

        // Apply the initial size immediately so the first image uses the configured resolution.
        if (!m_faceResolutionApplied)
        {
            m_resolutionDebounce.Commit(clamped);
            ApplyFaceResolution(clamped);
            return;
        }

        m_resolutionDebounce.Request(clamped);
    }

    void GradientGlobalIlluminationFeatureProcessor::ApplyFaceResolution(uint32_t value)
    {
        if (m_faceResolutionApplied && value == m_faceResolution)
        {
            return;
        }

        m_faceResolution = value;
        m_faceResolutionApplied = true;

        if (!m_gradientPass)
        {
            m_needsRebuild = true;
        }
        else
        {
            m_gradientPass->SetGradientColors(m_lowColor, m_midColor, m_highColor, m_faceResolution);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::TickResolutionSettle()
    {
        if (const AZStd::optional<uint32_t> settled = m_resolutionDebounce.AdvanceFrame())
        {
            ApplyFaceResolution(*settled);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::SetUpdateMode(UpdateMode requestedMode)
    {
        if (m_updateMode == requestedMode)
        {
            return;
        }

        const UpdateMode mode = GradientGlobalIllumination::ResolveUpdateMode(
            requestedMode,
            requestedMode != UpdateMode::Dynamic || GradientGlobalIlluminationCubemapPass::IsGpuComputeSupported());
        AZ_Warning("GradientGlobalIllumination", mode == requestedMode, "Requested gradient update mode is unavailable. Using Static mode.");

        if (m_updateMode == mode)
        {
            return;
        }

        m_updateMode = mode;

        // Consume a pending CPU size before switching backends.
        if (m_resolutionDebounce.HasPendingCommit())
        {
            const uint32_t resolution = m_resolutionDebounce.Pending();
            m_resolutionDebounce.Commit(resolution);
            ApplyFaceResolution(resolution);
        }

        if (mode == UpdateMode::Static)
        {
            DestroyPass();
            m_diffuseImage = nullptr;
            m_specularImage = nullptr;

            m_needsRebuild = m_enabled;
        }
        else if (m_enabled)
        {
            EnsurePass();
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
    {
        // Retain the runtime texture so color edits do not reload it.
        m_detailTexture = nullptr;
        if (texture.GetId().IsValid())
        {
            m_detailTexture = RPI::StreamingImage::FindOrCreate(texture);
        }

        if (m_gradientPass)
        {
            m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::SetDetailParams(uint8_t mapping, uint8_t blend, float strength)
    {
        m_detailMapping = mapping;
        m_detailBlend = blend;
        m_detailStrength = strength;

        if (m_gradientPass)
        {
            m_gradientPass->SetDetailLayer(m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture)
    {
        m_specularTexture = nullptr;
        if (texture.GetId().IsValid())
        {
            m_specularTexture = RPI::StreamingImage::FindOrCreate(texture);
        }

        if (m_gradientPass)
        {
            m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::SetSpecularParams(uint8_t mapping, uint8_t blend, float strength)
    {
        m_specularMapping = mapping;
        m_specularBlend = blend;
        m_specularStrength = strength;

        if (m_gradientPass)
        {
            m_gradientPass->SetSpecularLayer(m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        }
    }

    GradientGlobalIlluminationFeatureProcessorInterface::UpdateMode GradientGlobalIlluminationFeatureProcessor::GetUpdateMode() const
    {
        return m_updateMode;
    }

    Vector3 GradientGlobalIlluminationFeatureProcessor::CubeFaceDirection(uint32_t face, float uc, float vc)
    {
        // Match the shader: texture V points downward, so side faces negate it for Y.
        switch (face)
        {
        case 0:
            return Vector3(1.0f, -vc, -uc); // +X
        case 1:
            return Vector3(-1.0f, -vc, uc); // -X
        case 2:
            return Vector3(uc, 1.0f, vc); // +Y (zenith)
        case 3:
            return Vector3(uc, -1.0f, -vc); // -Y (nadir)
        case 4:
            return Vector3(uc, -vc, 1.0f); // +Z
        case 5:
            return Vector3(-uc, -vc, -1.0f); // -Z
        default:
            return Vector3(0.0f, 1.0f, 0.0f);
        }
    }

    void GradientGlobalIlluminationFeatureProcessor::GenerateFacePixels(uint32_t face, uint32_t faceSize, AZStd::vector<uint8_t>& outPixels) const
    {
        const size_t bpp = RHI::GetFormatSize(RHI::Format::R16G16B16A16_FLOAT);
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
                GradientGlobalIllumination::WriteRgba16FloatPixel(outPixels.data() + offset, c.GetR(), c.GetG(), c.GetB(), 1.0f);
            }
        }
    }

    Data::Instance<RPI::StreamingImage> GradientGlobalIlluminationFeatureProcessor::BuildGradientCubemap()
    {
        constexpr RHI::Format format = RHI::Format::R16G16B16A16_FLOAT;
        const uint32_t faceSize = GradientGlobalIllumination::SnapCpuFaceResolutionToPow2(m_faceResolution);

        RHI::Size faceDimensions(faceSize, faceSize, 1);
        RHI::DeviceImageSubresourceLayout faceLayout = RHI::GetImageSubresourceLayout(faceDimensions, format);
        const size_t bytesPerFace = faceLayout.m_bytesPerImage;

        Data::Asset<RPI::ImageMipChainAsset> mipChainAsset;
        {
            RPI::ImageMipChainAssetCreator mipCreator;
            mipCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), 1, 6);

            AZStd::vector<uint8_t> facePixels;
            mipCreator.BeginMip(faceLayout);
            for (uint32_t face = 0; face < 6; ++face)
            {
                GenerateFacePixels(face, faceSize, facePixels);
                AZ_Assert(
                    facePixels.size() >= bytesPerFace,
                    "GradientGlobalIllumination: Face pixel data too small (expected %zu, got %zu)",
                    bytesPerFace,
                    facePixels.size());

                mipCreator.AddSubImage(facePixels.data(), bytesPerFace);
            }
            mipCreator.EndMip();

            if (!mipCreator.End(mipChainAsset))
            {
                AZ_Error("GradientGlobalIllumination", false, "Failed to create ImageMipChainAsset.");
                return nullptr;
            }
        }

        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(RHI::ImageBindFlags::ShaderRead, faceSize, format);

        auto* imageSystem = RPI::ImageSystemInterface::Get();
        if (!imageSystem)
        {
            AZ_Error("GradientGlobalIllumination", false, "ImageSystemInterface not available.");
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
                AZ_Error("GradientGlobalIllumination", false, "Failed to create StreamingImageAsset.");
                return nullptr;
            }
        }

        Data::Instance<RPI::StreamingImage> image = RPI::StreamingImage::FindOrCreate(m_imageAsset);
        if (!image)
        {
            AZ_Error("GradientGlobalIllumination", false, "Failed to create StreamingImage instance.");
            return nullptr;
        }

        return image;
    }
} // namespace AZ::Render
