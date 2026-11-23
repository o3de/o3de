/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AtomLyIntegration/CommonFeatures/GradientGlobalIllumination/GradientGlobalIlluminationComponentConstants.h>

namespace AZ::Render
{
    //! Per-entity configuration. Only the most recently activated component drives scene output.
    //! Texture binding changes are deferred to tick.
    class GradientGlobalIlluminationComponentRequests
        : public ComponentBus
    {
    public:
        virtual void SetLowColor(const Color& color) = 0;
        virtual Color GetLowColor() const = 0;

        virtual void SetMidColor(const Color& color) = 0;
        virtual Color GetMidColor() const = 0;

        virtual void SetHighColor(const Color& color) = 0;
        virtual Color GetHighColor() const = 0;

        virtual void SetGradientColors(const Color& low, const Color& mid, const Color& high) = 0;

        virtual void SetExposure(float exposure) = 0;
        virtual float GetExposure() const = 0;

        //! Set a floating-point Script Canvas Number, rounded and clamped to [4, 256]. CPU output snaps to a power of two.
        virtual void SetFaceResolution(float resolution) = 0;

        //! Return the configured size. CPU image regeneration can lag while edits settle.
        virtual float GetFaceResolution() const = 0;

        virtual void SetUpdateMode(GradientGlobalIlluminationUpdateMode mode) = 0;

        //! Return the configured mode. The active backend may have fallen back to CPU.
        virtual GradientGlobalIlluminationUpdateMode GetUpdateMode() const = 0;

        //! Texture layers affect GPU output only. Script Canvas exposes the AssetId variants.
        virtual void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;
        virtual Data::Asset<RPI::StreamingImageAsset> GetDetailTexture() const = 0;
        virtual void SetDetailTextureAssetId(const Data::AssetId& id) = 0;
        virtual Data::AssetId GetDetailTextureAssetId() const = 0;
        virtual void SetDetailTextureAssetPath(const AZStd::string& path) = 0;
        virtual AZStd::string GetDetailTextureAssetPath() const = 0;
        virtual void SetDetailMapping(GradientGlobalIlluminationTextureMapping mapping) = 0;
        virtual GradientGlobalIlluminationTextureMapping GetDetailMapping() const = 0;
        virtual void SetDetailBlend(GradientGlobalIlluminationBlendMode blend) = 0;
        virtual GradientGlobalIlluminationBlendMode GetDetailBlend() const = 0;
        virtual void SetDetailStrength(float strength) = 0;
        virtual float GetDetailStrength() const = 0;

        virtual void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;
        virtual Data::Asset<RPI::StreamingImageAsset> GetSpecularTexture() const = 0;
        virtual void SetSpecularTextureAssetId(const Data::AssetId& id) = 0;
        virtual Data::AssetId GetSpecularTextureAssetId() const = 0;
        virtual void SetSpecularTextureAssetPath(const AZStd::string& path) = 0;
        virtual AZStd::string GetSpecularTextureAssetPath() const = 0;
        virtual void SetSpecularMapping(GradientGlobalIlluminationTextureMapping mapping) = 0;
        virtual GradientGlobalIlluminationTextureMapping GetSpecularMapping() const = 0;
        virtual void SetSpecularBlend(GradientGlobalIlluminationBlendMode blend) = 0;
        virtual GradientGlobalIlluminationBlendMode GetSpecularBlend() const = 0;
        virtual void SetSpecularStrength(float strength) = 0;
        virtual float GetSpecularStrength() const = 0;
    };

    using GradientGlobalIlluminationComponentRequestBus = EBus<GradientGlobalIlluminationComponentRequests>;
} // namespace AZ::Render
