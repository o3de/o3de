/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/Math/Color.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ::Render
{
    //! Generates single-mip gradient cubemaps for ambient IBL. Specular output is not roughness-prefiltered.
    //! CPU image and exposure updates are published after simulation and consumed by IBL in the next simulation.
    //! Call configuration and ownership methods on the main thread.
    class GradientGIFeatureProcessorInterface : public RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(AZ::Render::GradientGIFeatureProcessorInterface, "{8A3D1F9E-C4B2-4E6D-A7F0-3B5C8D2E1A09}", AZ::RPI::FeatureProcessor);

        enum class UpdateMode : uint8_t
        {
            //! CPU-generated StreamingImage, rebuilt when gradient inputs change.
            Static = 0,

            //! GPU-generated AttachmentImage, updated when inputs change.
            //! Falls back to Static when writable cubemap allocation is unavailable.
            Dynamic = 1,
        };

        //! Set linear HDR colors at the nadir (-Y), horizon, and zenith (+Y).
        virtual void SetGradientColors(const Color& low, const Color& mid, const Color& high) = 0;

        //! Set exposure in EV stops. IBL sampling applies the multiplier 2^exposure.
        virtual void SetExposure(float exposureStops) = 0;

        //! Set face size in pixels, clamped to [4, 256]. CPU output snaps to a power of two and debounces changes.
        virtual void SetFaceResolution(uint32_t resolution) = 0;

        //! Select a generation backend. Dynamic falls back to Static if the writable-cubemap probe fails.
        virtual void SetUpdateMode(UpdateMode mode) = 0;

        //! Set a ready detail asset, or an invalid asset to disable the layer. GPU mode only.
        virtual void SetDetailTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;

        //! Set mapping/blend codes matching the component enums and a blend strength in [0, 1].
        virtual void SetDetailParams(uint8_t mapping, uint8_t blend, float strength) = 0;

        //! Set a ready specular asset, or an invalid asset to use the diffuse base. GPU mode only.
        virtual void SetSpecularTexture(const Data::Asset<RPI::StreamingImageAsset>& texture) = 0;

        //! Set mapping/blend codes matching the component enums and a blend strength in [0, 1].
        virtual void SetSpecularParams(uint8_t mapping, uint8_t blend, float strength) = 0;

        //! Return the active backend, including any CPU fallback.
        virtual UpdateMode GetUpdateMode() const = 0;

        //! Register an owner. The newest enabled owner drives the scene until removed.
        virtual void Enable(EntityId owner) = 0;

        //! Remove an owner and promote the newest survivor. Stop output when no owners remain.
        virtual void Disable(EntityId owner) = 0;

        using OwnerChangedEvent = AZ::Event<EntityId>;

        //! Notifications run synchronously on the main thread so a promoted owner can stage its configuration.
        virtual void ConnectToOwnerChangedEvent(OwnerChangedEvent::Handler& handler) = 0;

        //! Current owner, or an invalid ID when no owners are registered.
        virtual EntityId GetCurrentOwner() const = 0;
    };

} // namespace AZ::Render
