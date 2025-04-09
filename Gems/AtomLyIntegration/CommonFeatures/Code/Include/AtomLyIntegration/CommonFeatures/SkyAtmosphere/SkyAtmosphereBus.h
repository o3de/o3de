/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/SkyAtmosphere/SkyAtmosphereComponentConfig.h>

namespace AZ::Render
{
    //! The EBus for requests to modify sky atmosphere components.
    class SkyAtmosphereRequests
        : public ComponentBus
    {
    public:
        AZ_RTTI(AZ::Render::SkyAtmosphereRequests, "{7F8704EE-196D-4371-B185-F209FE58F2D1}");

        /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        virtual ~SkyAtmosphereRequests() {}

        //! Enable/disable atmosphere without having to deactivate the entity
        //! @param enabled Set whether the atmosphere is enabled or not 
        virtual void SetEnabled(bool enabled) = 0;
        //! @return true if the current atmosphere is enabled 
        virtual bool GetEnabled() = 0;

        //! Set the planet origin mode
        //! @param mode Set the origin mode to use 
        virtual void SetPlanetOriginMode(SkyAtmosphereComponentConfig::AtmosphereOrigin mode) = 0;
        //! @return the planet origin mode
        virtual SkyAtmosphereComponentConfig::AtmosphereOrigin GetPlanetOriginMode() = 0;

        //! Set the atmosphere height in kilometers 
        //! @param atmosphereHeightKm height in km 
        virtual void SetAtmosphereHeight(float atmosphereHeightKm) = 0;
        //! @return the atmosphere height in kilometers 
        virtual float GetAtmosphereHeight() = 0;

        //! Set the planet radius in kilometers 
        //! @param planetRadiusKm radius in km 
        virtual void SetPlanetRadius(float planetRadiusKm) = 0;
        //! @return the planet radius in kilometers 
        virtual float GetPlanetRadius() = 0;

        //! Set the ground albedo 
        //! @param groundAlbedo The ground albedo color 
        virtual void SetGroundAlbedo(const AZ::Vector3& groundAlbedo) = 0;
        //! @return the ground albedo 
        virtual AZ::Vector3 GetGroundAlbedo() = 0;

        //! Set the luminance factor of the atmosphere 
        //! @param luminanceFactor The luminance factor 
        virtual void SetLuminanceFactor(const AZ::Vector3& luminanceFactor) = 0;
        //! @return the luminance factor of the atmosphere 
        virtual AZ::Vector3 GetLuminanceFactor() = 0;

        //! Set the mie absorption amount 
        //! @param mieAbsorption The mie absorption amount 
        virtual void SetMieAbsorption(const AZ::Vector3& mieAbsorption) = 0;
        //! @return the mie absorption amount 
        virtual AZ::Vector3 GetMieAbsorption() = 0;

        //! Set the mie distribution amount 
        //! @param mieExpDistribution The mie distribution amount 
        virtual void SetMieExpDistribution(float mieExpDistribution) = 0;
        //! @return the mie distribution amount 
        virtual float GetMieExpDistribution() = 0;

        //! Set the mie scattering color 
        //! @param mieScattering The mie scattering color 
        virtual void SetMieScattering(const AZ::Vector3& mieScattering) = 0;
        //! @return the mie scattering color 
        virtual AZ::Vector3 GetMieScattering() = 0;

        //! Set the rayleigh exp distribution amount 
        //! @param rayleighExpDistribution The rayleight exp distribution 
        virtual void SetRayleighExpDistribution(float rayleighExpDistribution) = 0;
        //! @return the rayleigh exp distribution amount 
        virtual float GetRayleighExpDistribution() = 0;

        //! Set the rayleigh scattering color 
        //! @param rayleighScattering The rayleigh scattering color 
        virtual void SetRayleighScattering(const AZ::Vector3& rayleighScattering) = 0;
        //! @return the rayleigh scattering color 
        virtual AZ::Vector3 GetRayleighScattering() = 0;

        //! Set the maximum number of samples to use for each ray cast from the
        //! camera into the atmosphere 
        //! @param maxSamples The maximum number of samples 
        virtual void SetMaxSamples(uint8_t maxSamples) = 0;
        //! @return the maximum number of samples to use for each ray cast 
        virtual uint8_t GetMaxSamples() = 0;

        //! Set the minimum number of samples to use for each ray cast from the
        //! camera into the atmosphere 
        //! @param minSamples The minimum number of samples 
        virtual void SetMinSamples(uint8_t minSamples) = 0;
        //! @return the minimum number of samples to use for each ray cast
        virtual uint8_t GetMinSamples() = 0;

        //! Enable or disable drawing on the sun 
        //! @param enabled Enable drawing the sun or not 
        virtual void SetSunEnabled(bool enabled) = 0;
        //! @return whether drawing the sun is enabled 
        virtual bool GetSunEnabled() = 0;

        //! Set the sun entity to use for the orientation of the sun
        //! @param entityId The Entity Id of the entity to use
        virtual void SetSunEntityId(AZ::EntityId entityId) = 0;
        //! @return the sun entity to use for the orientation of the sun
        virtual AZ::EntityId GetSunEntityId() = 0;

        //! Set the sun color
        //! @param sunColor The sun color
        virtual void SetSunColor(const AZ::Color sunColor) = 0;
        //! @return the sun color
        virtual AZ::Color GetSunColor() = 0;

        //! Set the sun luminance factor
        //! @param factor The luminance color
        virtual void SetSunLuminanceFactor(float factor) = 0;
        //! @return the sun luminance factor
        virtual float GetSunLuminanceFactor() = 0;

        //! Set the sun limb (outer edge) color
        //! @param sunLimbColor The sun limb color
        virtual void SetSunLimbColor(const AZ::Color sunLimbColor) = 0;
        //! @return the sun limb (outer edge) color
        virtual AZ::Color GetSunLimbColor() = 0;

        //! Set the sun falloff factor, controls how the sun disk fades at the edges
        //! @param factor The falloff factor for the edge of the sun 
        virtual void SetSunFalloffFactor(float factor) = 0;
        //! @return the sun falloff factor
        virtual float GetSunFalloffFactor() = 0;

        //! Set the sun radius factor, controls how large the sun disk is
        //! @param factor The radius factor for the edge of the sun 
        virtual void SetSunRadiusFactor(float factor) = 0;
        //! @return the sun radius factor
        virtual float GetSunRadiusFactor() = 0;

        //! Set the sun direction, has not effect if the sun entity has been set
        //! @param sunDirection The direction for the sun to point at 
        virtual void SetSunDirection(const AZ::Vector3& sunDirection) = 0;
        //! @return the sun direction
        virtual AZ::Vector3 GetSunDirection() = 0;

        //! Enable using a look up table for all sky pixels that are not obstructed
        //! @param enabled Whether to enable or disable fast sky 
        virtual void SetFastSkyEnabled(bool enabled) = 0;
        //! @return whether fast sky is enabled 
        virtual bool GetFastSkyEnabled() = 0;

        //! Enable using a look up table for all sky pixels that are obstructed
        //! @param enabled Whether to enable or disable fast aerial perspective 
        virtual void SetFastAerialPerspectiveEnabled(bool enabled) = 0;
        //! @return whether fast aerial perspective is enabled
        virtual bool GetFastAerialPerspectiveEnabled() = 0;

        //! Enable or disable drawing aerial perspective on top of the scene.
        //! This can be used to speed up indoor scenes
        //! @param enabled Whether to enable or aerial perspective 
        virtual void SetAerialPerspectiveEnabled(bool enabled) = 0;
        //! @return whether aerial perspective is enabled or not
        virtual bool GetAerialPerspectiveEnabled() = 0;

        //! Set the near clip distance at which aerial perspective starts drawing
        //! This value is useful when you are inside a building looking out and don't want to enable
        //! shadows, instead you can set the near clip so aerial perspective isn't applied until
        //! pixels are outside whatever building the camera is in 
        //! @param nearClip The distance in meters at which to start applying aerial perspective 
        virtual void SetNearClip(float nearClip) = 0;
        //! @return the near clip distance at which aerial perspective starts drawing
        virtual float GetNearClip() = 0;

        //! Set the distance over which to fade in aerial perspective starting from the near clip distance 
        //! If this value is set to 0, there may be a distinct visible line 
        //! @param nearFadeDistance The distance in meters at which to fade into aerial perspective 
        virtual void SetNearFadeDistance(float nearFadeDistance) = 0;
        //! @return the distance over which to fade in aerial perspective starting from the near clip distance 
        virtual float GetNearFadeDistance() = 0;

        //! Set the factor to apply to the depth of all pixels that obstruct the sky,
        //! causing objects to appear further away in the atmosphere 
        //! @param aerialDepthFactor The factor to multiply to all obstructed pixels depth 
        virtual void SetAerialDepthFactor(float aerialDepthFactor) = 0;
        //! @return the factor applied to the depth of all pixels that obstruct the sky,
        virtual float GetAerialDepthFactor() = 0;

        //! Set whether shadows affect the atmosphere or not 
        //! Shadows have no effect when fast aerial perspective and fast sky are enabled
        //! @param enabled True to allow shadows to affect the atmosphere 
        virtual void SetShadowsEnabled(bool enabled) = 0;
        //! @return whether shadows affect the atmosphere 
        virtual bool GetShadowsEnabled() = 0;
    };

    typedef AZ::EBus<SkyAtmosphereRequests> SkyAtmosphereRequestBus;
}
