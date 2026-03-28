/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    //! Feature processor that generates a procedural gradient cubemap and writes it
    //! to the scene SRG's IBL slots (m_diffuseEnvMap, m_specularEnvMap, m_iblExposure).
    //!
    //! This replicates Unity's Gradient GI: three colors (low/mid/high) produce a
    //! vertical gradient cubemap for generic ambient fill lighting. The cubemap is
    //! generated on the CPU at low resolution (4-256 px per face) and uploaded as a
    //! StreamingImage. It composites naturally with DiffuseProbeGrid as fallback ambient.
    class GradientGIFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::GradientGIFeatureProcessorInterface, "{8A3D1F9E-C4B2-4E6D-A7F0-3B5C8D2E1A09}", AZ::RPI::FeatureProcessor);

        //! Set the three gradient colors. Low = nadir, mid = horizon, high = zenith.
        virtual void SetGradientColors(const Color& low, const Color& mid, const Color& high) = 0;

        //! Set IBL exposure in EV stops. Final intensity = base * 2^exposure.
        virtual void SetExposure(float exposureStops) = 0;

        //! Set cubemap face resolution in pixels (default 64). Clamped to [4..256].
        virtual void SetFaceResolution(uint32_t resolution) = 0;

        //! Returns true if the gradient cubemap has been built and is active.
        virtual bool IsActive() const = 0;

        //! Reset to defaults — removes the gradient cubemap from the IBL slots.
        virtual void Reset() = 0;
    };
} // namespace AZ::Render
