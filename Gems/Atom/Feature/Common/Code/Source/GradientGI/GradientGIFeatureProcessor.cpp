/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientGIFeatureProcessor.h"
#include "GradientGIConstants.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>

#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>

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
        m_iblFeatureProcessor = GetParentScene()->GetFeatureProcessor<ImageBasedLightFeatureProcessorInterface>();
        AZ_Error("GradientGI", m_iblFeatureProcessor, "ImageBasedLightFeatureProcessorInterface not found on scene. GradientGI requires it.");
        m_needsRebuild = false;
        m_active = false;
    }

    void GradientGIFeatureProcessor::Deactivate()
    {
        if (m_active && m_iblFeatureProcessor)
        {
            m_iblFeatureProcessor->Reset();
        }
        m_cubemapImage = nullptr;
        m_active = false;
        m_iblFeatureProcessor = nullptr;
    }

    void GradientGIFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& /*packet*/)
    {
        AZ_PROFILE_SCOPE(RPI, "GradientGIFeatureProcessor: Simulate");

        if (!m_needsRebuild || !m_iblFeatureProcessor)
        {
            return;
        }

        // Rebuild cubemap and feed it to the IBL FP (single owner of scene SRG IBL slots)
        m_cubemapImage = BuildGradientCubemap();
        m_active = (m_cubemapImage != nullptr);
        m_needsRebuild = false;

        if (m_active && m_imageAsset.GetId().IsValid())
        {
            m_iblFeatureProcessor->SetSpecularImage(m_imageAsset);
            m_iblFeatureProcessor->SetDiffuseImage(m_imageAsset);
            m_iblFeatureProcessor->SetExposure(m_exposure);
        }
    }

    // =========================================================================
    // Public API
    // =========================================================================

    void GradientGIFeatureProcessor::SetGradientColors(const Color& low, const Color& mid, const Color& high)
    {
        m_lowColor = low;
        m_midColor = mid;
        m_highColor = high;
        m_needsRebuild = true;
    }

    void GradientGIFeatureProcessor::SetExposure(float exposureStops)
    {
        m_exposure = exposureStops;
        if (m_active && m_iblFeatureProcessor)
        {
            m_iblFeatureProcessor->SetExposure(exposureStops);
        }
    }

    void GradientGIFeatureProcessor::SetFaceResolution(uint32_t resolution)
    {
        m_faceResolution = AZStd::clamp(resolution, 4u, 256u);
        m_needsRebuild = true;
    }

    bool GradientGIFeatureProcessor::IsActive() const
    {
        return m_active;
    }

    void GradientGIFeatureProcessor::Reset()
    {
        if (m_active && m_iblFeatureProcessor)
        {
            m_iblFeatureProcessor->Reset();
        }
        m_cubemapImage = nullptr;
        m_imageAsset = {};
        m_active = false;
        m_needsRebuild = false;
    }

    // =========================================================================
    // Format Selection
    // =========================================================================

    RHI::Format GradientGIFeatureProcessor::ChooseBestFormat() const
    {
        // Prefer R16G16B16A16_FLOAT (full HDR, matches engine convention).
        // Fall back to R8G8B8A8_UNORM for universal compatibility.
        // R11G11B10_FLOAT is skipped as a default because it lacks alpha and
        // has inconsistent storage support on mobile -- the gradient data is
        // simple enough that UNORM8 with exposure compensation works well.
        //
        // A future enhancement could query RHI::FormatCapabilities::Sample
        // per format, but all devices that run O3DE support both of these
        // as shader-readable textures.
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
                // Texel center -> normalized cubemap coordinates
                float uc = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;
                float vc = (2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize)) - 1.0f;

                Vector3 dir = CubeFaceDirection(face, uc, vc);
                dir.NormalizeSafe();

                // Vertical gradient: Y axis maps [-1..1] -> [0..1], bottom to top
                float t = (dir.GetY() + 1.0f) * 0.5f;

                // Two-segment lerp: low->mid (t=0..0.5), mid->high (t=0.5..1.0)
                Color c;
                if (t < 0.5f)
                {
                    c = m_lowColor.Lerp(m_midColor, t * 2.0f);
                }
                else
                {
                    c = m_midColor.Lerp(m_highColor, (t - 0.5f) * 2.0f);
                }

                // Write pixel in the selected format
                size_t offset = (static_cast<size_t>(y) * faceSize + x) * bpp;
                GradientGI::WritePixel(outPixels.data() + offset, c.GetR(), c.GetG(), c.GetB(), 1.0f, format);
            }
        }
    }

    // =========================================================================
    // Cubemap Asset Construction
    // =========================================================================

    Data::Instance<RPI::StreamingImage> GradientGIFeatureProcessor::BuildGradientCubemap()
    {
        const RHI::Format format = ChooseBestFormat();
        const uint32_t faceSize = m_faceResolution;

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
        Data::Instance<RPI::StreamingImage> image =
            RPI::StreamingImage::FindOrCreate(m_imageAsset);

        if (!image)
        {
            AZ_Error("GradientGI", false, "Failed to create StreamingImage instance.");
            return nullptr;
        }

        return image;
    }

} // namespace AZ::Render
