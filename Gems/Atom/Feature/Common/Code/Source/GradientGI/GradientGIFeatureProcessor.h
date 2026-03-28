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
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ::Render
{
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

        // =====================================================================
        // GradientGIFeatureProcessorInterface
        // =====================================================================
        void SetGradientColors(const Color& low, const Color& mid, const Color& high) override;
        void SetExposure(float exposureStops) override;
        void SetFaceResolution(uint32_t resolution) override;
        bool IsActive() const override;
        void Reset() override;

    private:
        GradientGIFeatureProcessor(const GradientGIFeatureProcessor&) = delete;

        // =====================================================================
        // Cubemap Generation
        // =====================================================================

        //! Choose the best supported format for this device.
        RHI::Format ChooseBestFormat() const;

        //! Generate CPU pixel data for one cubemap face.
        void GenerateFacePixels(
            uint32_t face,
            uint32_t faceSize,
            RHI::Format format,
            AZStd::vector<uint8_t>& outPixels) const;

        //! Build a StreamingImage cubemap from the current gradient colors.
        Data::Instance<RPI::StreamingImage> BuildGradientCubemap();

        //! Direction vector from cubemap face index and texel coordinates.
        static Vector3 CubeFaceDirection(uint32_t face, float uc, float vc);

        // =====================================================================
        // State
        // =====================================================================

        // Delegate to the IBL FP (single owner of the scene SRG IBL slots)
        ImageBasedLightFeatureProcessorInterface* m_iblFeatureProcessor = nullptr;

        // Gradient parameters
        Color m_lowColor = Color(0.05f, 0.06f, 0.08f, 1.0f);
        Color m_midColor = Color(0.20f, 0.30f, 0.55f, 1.0f);
        Color m_highColor = Color(0.85f, 0.95f, 1.0f, 1.0f);
        float m_exposure = 0.0f;
        uint32_t m_faceResolution = 64;

        // Runtime image
        Data::Instance<RPI::StreamingImage> m_cubemapImage;
        Data::Asset<RPI::StreamingImageAsset> m_imageAsset;
        bool m_needsRebuild = false;
        bool m_active = false;
    };
} // namespace AZ::Render
