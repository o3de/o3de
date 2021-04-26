/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    class FogVolumeConfiguration;

    enum class FogVolumeType : AZ::s32
    {
        None = -1,
        Ellipsoid = 0,
        RectangularPrism = 1,
    };

    class FogVolumeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~FogVolumeComponentRequests() {}

        /// Propagates updated configuration to the fog volume component
        virtual void RefreshFog() {};

        /**
         * Gets the volume shape type.
         */
        virtual FogVolumeType GetVolumeType() { return FogVolumeType::None; }
        /**
         * Sets the volume shape type.
         */
        virtual void SetVolumeType([[maybe_unused]] FogVolumeType volumeType) {};

        /**
         * Gets color of the fog.
         */
        virtual AZ::Color GetColor() { return AZ::Color::CreateOne(); }
        /**
         * Sets color of the fog.
         */
        virtual void SetColor([[maybe_unused]] AZ::Color color) {};

        /**
         * Gets HDR Dynamic.
         */
        virtual float GetHdrDynamic() { return FLT_MAX; }
        /**
         * Set HDR Dynamic.
         */
        virtual void SetHdrDynamic([[maybe_unused]] float hdrDynamic) {};

        /**
         * Returns true if global fog color is used instead of the color property.
         */
        virtual bool GetUseGlobalFogColor() { return false; }
        /**
         * Sets whether global fog color is used instead of the color property.
         */
        virtual void SetUseGlobalFogColor([[maybe_unused]] bool useGlobalFogColor) {};

        /**
         * Gets Fog density.
         */
        virtual float GetGlobalDensity() { return FLT_MAX; }
        /**
         * Sets Fog density. Fog density controls how thick the fog appears.
         */
        virtual void SetGlobalDensity([[maybe_unused]] float globalDensity) {};

        /**
         * Gets density offset
         */
        virtual float GetDensityOffset() { return FLT_MAX; }
        /**
         * Sets density offset. Density offset additionaly controls to the density of the fog volume.
         */
        virtual void SetDensityOffset([[maybe_unused]] float densityOffset) {};

        /**
         * Gets near cutoff distance
         */
        virtual float GetNearCutoff() { return FLT_MAX; }
        /**
         * Sets near cutoff distance. Stops rendering the volume, depending on the camera distance to the object (m).
         */
        virtual void SetNearCutoff([[maybe_unused]] float nearCutoff) {};

        /**
         * Gets fall off direction horizontally.
         */
        virtual float GetFallOffDirLong() { return FLT_MAX; }
        /**
         * Sets fall off direction horizontally.
         */
        virtual void SetFallOffDirLong([[maybe_unused]] float fallOffDirLong) {};

        /**
         * Gets vertical fall off direction.
         */
        virtual float GetFallOffDirLatitude() { return FLT_MAX; }
        /**
         * Sets vertical fall off direction. Controls the latitude of the world space fall off direction of the fog. 
         * A value of 90 degrees lets the fall off direction point upwards in world space (respectively, -90° points downwards).
         */
        virtual void SetFallOffDirLatitude([[maybe_unused]] float fallOffDirLatitude) {};

        /**
         * Gets fall off shift.
         */
        virtual float GetFallOffShift() { return FLT_MAX; }
        /**
         * Sets fall off shift. Controls how much to shift the fog density along the fall off direction. 
         * Positive values will move thicker fog layers along the fall off direction into the fog volume. 
         * Negative values will move thick layers along the negative fall off direction out of the fog volume. 
         */
        virtual void SetFallOffShift([[maybe_unused]] float fallOffShift) {};

        /**
         * Gets the scale of density distribution along the fall off direction.
         */
        virtual float GetFallOffScale() { return FLT_MAX; }
        /**
         * Sets the scale of the density distribution along the fall off direction. Higher values will make the fog fall off more rapidly.
         */
        virtual void SetFallOffScale([[maybe_unused]] float fallOffScale) {};

        /**
         * Gets a factor that is used to soften the edges of the fog volume when viewed from outside.
         */
        virtual float GetSoftEdges() { return FLT_MAX; }
        /**
         * Sets a factor that is used to soften the edges of the fog volume when viewed from outside.
         */
        virtual void SetSoftEdges([[maybe_unused]] float softEdges) {};

        /**
         * Gets the start distance of fog density ramp in world units (m) Only works with Volumetric Fog.
         */
        virtual float GetRampStart() { return FLT_MAX; }
        /**
         * Sets the start distance of fog density ramp in world units (m) Only works with Volumetric Fog.
         */
        virtual void SetRampStart([[maybe_unused]] float rampStart) {};

        /**
         * Gets the end distance of fog density ramp in world units (m). Only works with Volumetric Fog.
         */
        virtual float GetRampEnd() { return FLT_MAX; }
        /**
         * Sets the end distance of fog density ramp in world units (m). Only works with Volumetric Fog.
         */
        virtual void SetRampEnd([[maybe_unused]] float rampEnd) {};

        /**
         * Gets the influence of fog density ramp. Only works with Volumetric Fog.
         */
        virtual float GetRampInfluence() { return FLT_MAX; }
        /**
         * Sets the influence of fog density ramp. Only works with Volumetric Fog.
         */
        virtual void SetRampInfluence([[maybe_unused]] float rampInfluence) {};

        /**
         * Gets the influence of the global wind upon the fog volume. Only works with Volumetric Fog.
         */
        virtual float GetWindInfluence() { return FLT_MAX; }
        /**
         * Sets the influence of the global wind upon the fog volume. Only works with Volumetric Fog.
         */
        virtual void SetWindInfluence([[maybe_unused]] float windInfluence) {};

        /**
         * Gets the scale of the noise value for the density. Only works with Volumetric Fog.
         */
        virtual float GetDensityNoiseScale() { return FLT_MAX; }
        /**
         * Sets the scale of the noise value for the density. This will define the thickness of the individual patches of fog (clouds), 
         * or if you look at it from the other direction it will define how big the spaces are in-between each cloud. Reducing the value 
         * will thin out the cloud density and increase the spacing between each cloud. 
         * Only works with Volumetric Fog.
         */
        virtual void SetDensityNoiseScale([[maybe_unused]] float densityNoiseScale) {};

        /**
         * Gets the offset of the noise value for the density. Only works with Volumetric Fog.
         */
        virtual float GetDensityNoiseOffset() { return FLT_MAX; }
        /**
         * Sets the offset of the noise value for the density. Noise breaks up the solid shape of the fog volume into patches.
         * Only works with Volumetric Fog.
         */
        virtual void SetDensityNoiseOffset([[maybe_unused]] float densityNoiseOffset) {};

        /**
         * Gets the time frequency of the noise for the density. High frequencies produce fast changing fog. Allows the individual fog patches to morph into 
         * different shapes during the course of their lifetime. Keep this value low otherwise they will morph too quickly and will look very unnatural 
         */
        virtual float GetDensityNoiseTimeFrequency() { return FLT_MAX; }
        /**
         * Sets the time frequency of the noise for the density. High frequencies produce fast changing fog. Only works with Volumetric Fog.
         */
        virtual void SetDensityNoiseTimeFrequency([[maybe_unused]] float timeFrequency) {};

        /**
         * Gets the spatial frequency of the noise for the density.
         */
        virtual AZ::Vector3 GetDensityNoiseFrequency() { return AZ::Vector3::CreateOne(); }
        /**
         * Sets the spatial frequency of the noise for the density. Allows defining how many fog patches are there. 
         * Increasing Z value will create a layered effect within the volume simulating different fog patches stacked on top of each other. 
         * Adding higher values in X & Y would mean there would be more individual fog patches within the volume. 
         * Only works with Volumetric Fog
         */
         virtual void SetDensityNoiseFrequency([[maybe_unused]] AZ::Vector3 densityNoiseFrequency) {};

        /**
         * Returns true if the FogVolume entity should respond to VisAreas and ClipVolumes.
         */
        virtual bool GetIgnoresVisAreas() { return false; };
        /**
         * Sets whether the FogVolume entity should respond to VisAreas and ClipVolumes.
         */
        virtual void SetIgnoresVisAreas([[maybe_unused]] bool ignoresVisAreas) {};

        /**
         * Returns true if the FogVolume entity effect doesn't occur in multiple VisAreas and ClipVolumes.
         */
        virtual bool GetAffectsThisAreaOnly() { return false; };
        /**
         * Sets whether the FogVolume entity effect doesn't occur in multiple VisAreas and ClipVolumes.
         */
        virtual void SetAffectsThisAreaOnly([[maybe_unused]] bool affectsThisAreaOnly) {};
    };

    using FogVolumeComponentRequestBus = AZ::EBus<FogVolumeComponentRequests>;
}
