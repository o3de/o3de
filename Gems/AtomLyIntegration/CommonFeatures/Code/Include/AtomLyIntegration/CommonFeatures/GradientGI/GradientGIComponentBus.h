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
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        // =====================================================================
        // Request Bus
        // =====================================================================

        class GradientGIComponentRequests
            : public ComponentBus
        {
        public:
            // Single-layer color setters/getters
            virtual void SetLowColor(const Color& color) = 0;
            virtual Color GetLowColor() const = 0;

            virtual void SetMidColor(const Color& color) = 0;
            virtual Color GetMidColor() const = 0;

            virtual void SetHighColor(const Color& color) = 0;
            virtual Color GetHighColor() const = 0;

            // Set all three gradient layers at once
            virtual void SetGradientColors(const Color& low, const Color& mid, const Color& high) = 0;

            virtual void SetExposure(float exposure) = 0;
            virtual float GetExposure() const = 0;

            // Script Canvas / Lua represent every number as a Number (floating point), so the
            // scripted face-resolution API uses float -- this matches a Number slot directly and
            // avoids the "expects an integer" implicit-conversion warning node. The value is
            // rounded and clamped to the valid [4..256] range by the controller. (Getter and
            // setter must share a type: BehaviorContext VirtualProperty requires it.)
            virtual void SetFaceResolution(float resolution) = 0;
            virtual float GetFaceResolution() const = 0;

            virtual void SetUpdateMode(GradientGIUpdateMode mode) = 0;
            virtual GradientGIUpdateMode GetUpdateMode() const = 0;

            // Detail texture layer (GPU/Dynamic mode)
            // The Data::Asset accessors are the C++ core (not script-reflected); scripting uses
            // the AssetId / AssetPath variants below to avoid a hard Asset<> dependency.
            virtual void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;
            virtual Data::Asset<RPI::StreamingImageAsset> GetDetailTexture() const = 0;
            virtual void SetDetailTextureAssetId(const Data::AssetId& id) = 0;
            virtual Data::AssetId GetDetailTextureAssetId() const = 0;
            virtual void SetDetailTextureAssetPath(const AZStd::string& path) = 0;
            virtual AZStd::string GetDetailTextureAssetPath() const = 0;
            virtual void SetDetailMapping(GradientGITextureMapping mapping) = 0;
            virtual GradientGITextureMapping GetDetailMapping() const = 0;
            virtual void SetDetailBlend(GradientGIBlendMode blend) = 0;
            virtual GradientGIBlendMode GetDetailBlend() const = 0;
            virtual void SetDetailStrength(float strength) = 0;
            virtual float GetDetailStrength() const = 0;

            // Specular texture layer (GPU/Dynamic mode)
            virtual void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;
            virtual Data::Asset<RPI::StreamingImageAsset> GetSpecularTexture() const = 0;
            virtual void SetSpecularTextureAssetId(const Data::AssetId& id) = 0;
            virtual Data::AssetId GetSpecularTextureAssetId() const = 0;
            virtual void SetSpecularTextureAssetPath(const AZStd::string& path) = 0;
            virtual AZStd::string GetSpecularTextureAssetPath() const = 0;
            virtual void SetSpecularMapping(GradientGITextureMapping mapping) = 0;
            virtual GradientGITextureMapping GetSpecularMapping() const = 0;
            virtual void SetSpecularBlend(GradientGIBlendMode blend) = 0;
            virtual GradientGIBlendMode GetSpecularBlend() const = 0;
            virtual void SetSpecularStrength(float strength) = 0;
            virtual float GetSpecularStrength() const = 0;
        };

        using GradientGIComponentRequestBus = EBus<GradientGIComponentRequests>;

    } // namespace Render
} // namespace AZ
