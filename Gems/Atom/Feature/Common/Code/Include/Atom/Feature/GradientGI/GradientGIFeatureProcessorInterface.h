/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Color.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ::Render
{
    //! Feature processor that generates a procedural gradient cubemap and writes it
    //! to the scene SRG's IBL slots (m_diffuseEnvMap, m_specularEnvMap, m_iblExposure).
    //!
    //! This creates a GI: where three colors (low/mid/high) produce a
    //! vertical gradient cubemap for generic ambient fill lighting.
    //!
    //! Two update modes are supported:
    //!   Static  -- CPU-generated StreamingImage (mobile-safe, one rebuild per change)
    //!   Dynamic -- GPU compute AttachmentImage  (desktop-only, runs every frame)
    class GradientGIFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::GradientGIFeatureProcessorInterface, "{8A3D1F9E-C4B2-4E6D-A7F0-3B5C8D2E1A09}", AZ::RPI::FeatureProcessor);

        // =====================================================================
        // Update Mode
        // =====================================================================

        enum class UpdateMode : uint8_t
        {
            //! CPU-side StreamingImage rebuild -- one GPU upload per change.
            //! Works on all platforms including mobile.
            Static = 0,

            //! GPU compute pass writing an AttachmentImage every frame.
            //! Requires compute shader UAV cubemap support (DX12 / Vulkan desktop).
            //! Falls back to Static automatically if the platform does not support it.
            Dynamic = 1,
        };

        // =====================================================================
        // Interface
        // =====================================================================

        //! Set the three gradient colors. Low = nadir (-Y), mid = horizon, high = zenith (+Y).
        virtual void SetGradientColors(const Color& low, const Color& mid, const Color& high) = 0;

        //! Set IBL exposure in EV stops. Final intensity = base * 2^exposure.
        virtual void SetExposure(float exposureStops) = 0;

        //! Set cubemap face resolution in pixels (default 64). Clamped to [4..256].
        //! Resolution changes only take effect on next component activation for Dynamic mode.
        virtual void SetFaceResolution(uint32_t resolution) = 0;

        //! Set the update mode (Static CPU or Dynamic GPU).
        //! If the platform does not support GPU compute UAV cubemaps, Dynamic silently
        //! falls back to Static.
        virtual void SetUpdateMode(UpdateMode mode) = 0;

        //! Set the detail texture layer (GPU/Dynamic mode only). Pass an invalid asset to clear.
        virtual void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;

        //! Set detail layer parameters. mapping/blend are codes matching the component enums
        //! (GradientGITextureMapping / GradientGIBlendMode); strength is the 0..1 layer weight.
        virtual void SetDetailParams(uint8_t mapping, uint8_t blend, float strength) = 0;

        //! Set the specular texture layer (GPU/Dynamic mode only). Composites into the specular
        //! IBL slot only. Pass an invalid asset to clear.
        virtual void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;

        //! Set specular layer parameters (codes match the component enums; strength is 0..1).
        virtual void SetSpecularParams(uint8_t mapping, uint8_t blend, float strength) = 0;

        //! Returns the currently active update mode (may differ from requested if
        //! the platform forced a fallback).
        virtual UpdateMode GetUpdateMode() const = 0;

        //! Start driving the scene's ambient light on behalf of an owning entity.
        //!
        //! The feature processor is a scene-wide singleton shared by every GradientGI component,
        //! so it tracks which entity currently owns it: the most recently enabled owner wins.
        //! Until an owner enables it the feature processor produces no output at all, even across
        //! render pipeline rebuilds.
        virtual void Enable(EntityId owner) = 0;

        //! Stop driving the scene's ambient light and release the IBL slots.
        //!
        //! Ignored unless `owner` is the current owner, so a component that is shutting down can
        //! never switch off one that has already taken over. See GradientGI::ShouldAcceptDisable.
        virtual void Disable(EntityId owner) = 0;
    };

} // namespace AZ::Render
