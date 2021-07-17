/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    class LightConfiguration;

    /*!
    * EditorLightComponentRequestBus
    * Editor/UI messages serviced by the light component.
    */
    class EditorLightComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~EditorLightComponentRequests() {}

        //! Recreates the render light.
        virtual void RefreshLight() {}

        //! Sets the active cubemap resource.
        virtual void SetCubemap(const AZStd::string& /*cubemap*/) {}
        virtual void SetProjectorTexture(const AZStd::string& /*projectorTexture*/) {}

        //! Retrieves configured cubemap resolution for generation.
        virtual AZ::u32 GetCubemapResolution() { return 0; }
        //! Sets cubemap resolution for generation. See LightConfiguration::ResolutionSetting for options.
        virtual void SetCubemapResolution(AZ::u32 /*resolution*/) = 0;

        //! Retrieves Configuration
        virtual const LightConfiguration& GetConfiguration() const = 0;

        //! get if it's customized cubemap
        virtual bool UseCustomizedCubemap() const { return false; }
        ////////////////////////////////////////////////////////////////////
        // Modifiers - these must match the same virtual methods in LightComponentRequests

        // General Light Settings
        virtual void SetVisible(bool /*isVisible*/) {}
        virtual bool GetVisible() { return true; }

        // Turned on by default?
        virtual void SetOnInitially(bool /*onInitially*/) {}
        virtual bool GetOnInitially() { return true; }

        virtual void SetColor(const AZ::Color& /*newColor*/) {}
        virtual const AZ::Color GetColor() { return AZ::Color::CreateOne(); }

        virtual void SetDiffuseMultiplier(float /*newMultiplier*/) {}
        virtual float GetDiffuseMultiplier() { return FLT_MAX; }

        virtual void SetSpecularMultiplier(float /*newMultiplier*/) {}
        virtual float GetSpecularMultiplier() { return FLT_MAX; }

        virtual void SetAmbient(bool /*isAmbient*/) {}
        virtual bool GetAmbient() { return true; }

        virtual void SetIndoorOnly(bool /*indoorOnly*/) {}
        virtual bool GetIndoorOnly() { return false; }

        virtual void SetCastShadowSpec(AZ::u32 /*castShadowSpec*/) {}
        virtual AZ::u32 GetCastShadowSpec() { return 0; }

        virtual void SetViewDistanceMultiplier(float /*viewDistanceMultiplier*/) {}
        virtual float GetViewDistanceMultiplier() { return 0.0f; }

        virtual void SetVolumetricFog(bool /*volumetricFog*/) {}
        virtual bool GetVolumetricFog() { return false; }

        virtual void SetVolumetricFogOnly(bool /*volumetricFogOnly*/) {}
        virtual bool GetVolumetricFogOnly() { return false; }

        virtual void SetUseVisAreas(bool /*useVisAreas*/) {}
        virtual bool GetUseVisAreas() { return false; }

        virtual void SetAffectsThisAreaOnly(bool /*affectsThisAreaOnly*/) {}
        virtual bool GetAffectsThisAreaOnly() { return false; }

        // Point Light Specific Modifiers
        virtual void SetPointMaxDistance(float /*newMaxDistance*/) {}
        virtual float GetPointMaxDistance() { return FLT_MAX; }

        virtual void SetPointAttenuationBulbSize(float /*newAttenuationBulbSize*/) {}
        virtual float GetPointAttenuationBulbSize() { return FLT_MAX; }

        // Area Light Specific Modifiers
        virtual void SetAreaMaxDistance(float /*newMaxDistance*/) {}
        virtual float GetAreaMaxDistance() { return FLT_MAX; }

        virtual void SetAreaWidth(float /*newWidth*/) {}
        virtual float GetAreaWidth() { return FLT_MAX; }

        virtual void SetAreaHeight(float /*newHeight*/) {}
        virtual float GetAreaHeight() { return FLT_MAX; }

        virtual void SetAreaFOV(float /*newFOV*/) {}
        virtual float GetAreaFOV() { return FLT_MAX; }

        // Project Light Specific Modifiers
        virtual void SetProjectorMaxDistance(float /*newMaxDistance*/) {}
        virtual float GetProjectorMaxDistance() { return FLT_MAX; }

        virtual void SetProjectorAttenuationBulbSize(float /*newAttenuationBulbSize*/) {}
        virtual float GetProjectorAttenuationBulbSize() { return FLT_MAX; }

        virtual void SetProjectorFOV(float /*newFOV*/) {}
        virtual float GetProjectorFOV() { return FLT_MAX; }

        virtual void SetProjectorNearPlane(float /*newNearPlane*/) {}
        virtual float GetProjectorNearPlane() { return FLT_MAX; }

        // Environment Probe Settings
        virtual void SetProbeAreaDimensions(const AZ::Vector3& newDimensions) { (void)newDimensions; }
        virtual const AZ::Vector3 GetProbeAreaDimensions() { return AZ::Vector3::CreateOne(); }

        virtual void SetProbeSortPriority(AZ::u32 newPriority) { (void)newPriority; }
        virtual AZ::u32 GetProbeSortPriority() { return 0; }

        virtual void SetProbeBoxProjected(bool isProbeBoxProjected) { (void)isProbeBoxProjected; }
        virtual bool GetProbeBoxProjected() { return false; }

        virtual void SetProbeBoxHeight(float newHeight) { (void)newHeight; }
        virtual float GetProbeBoxHeight() { return FLT_MAX; }

        virtual void SetProbeBoxLength(float newLength) { (void)newLength; }
        virtual float GetProbeBoxLength() { return FLT_MAX; }

        virtual void SetProbeBoxWidth(float newWidth) { (void)newWidth; }
        virtual float GetProbeBoxWidth() { return FLT_MAX; }

        virtual void SetProbeAttenuationFalloff(float newAttenuationFalloff) { (void)newAttenuationFalloff; }
        virtual float GetProbeAttenuationFalloff() { return FLT_MAX; }

        virtual void SetProbeFade(float fade) { (void)fade; }
        virtual float GetProbeFade() { return 1.0f; }

        // Environment Light Specific Modifiers (probes)
        virtual void SetProbeArea(const AZ::Vector3& /*probeArea*/) {}
        virtual AZ::Vector3 GetProbeArea() { return AZ::Vector3::CreateZero(); }

        virtual void SetAttenuationFalloffMax(float /*attenFalloffMax*/) {}
        virtual float GetAttenuationFalloffMax() { return 0; }

        virtual void SetBoxHeight(float /*boxHeight*/) {}
        virtual float GetBoxHeight() { return 0.0f; }

        virtual void SetBoxWidth(float /*boxWidth*/) {}
        virtual float GetBoxWidth() { return 0.0f; }

        virtual void SetBoxLength(float /*boxLength*/) {}
        virtual float GetBoxLength() { return 0.0f; }

        virtual void SetBoxProjected(bool /*boxProjected*/) {}
        virtual bool GetBoxProjected() { return false; }
        ////////////////////////////////////////////////////////////////////

        // Shadow parameters
        virtual void SetShadowBias(float /*shadowBias*/) {}
        virtual float GetShadowBias() { return 0.0f; }

        virtual void SetShadowSlopeBias(float /*shadowSlopeBias*/) {}
        virtual float GetShadowSlopeBias() { return 0.0f; }

        virtual void SetShadowResScale(float /*shadowResScale*/) {}
        virtual float GetShadowResScale() { return 0.0f; }

        virtual void SetShadowUpdateMinRadius(float /*shadowUpdateMinRadius*/) {}
        virtual float GetShadowUpdateMinRadius() { return 0.0f; }

        virtual void SetShadowUpdateRatio(float /*shadowUpdateRatio*/) {}
        virtual float GetShadowUpdateRatio() { return 0.0f; }

        // Animation parameters
        virtual void SetAnimIndex(AZ::u32 /*animIndex*/) {}
        virtual AZ::u32 GetAnimIndex() { return 0; }

        virtual void SetAnimSpeed(float /*animSpeed*/) {}
        virtual float GetAnimSpeed() { return 0.0f; }

        virtual void SetAnimPhase(float /*animPhase*/) {}
        virtual float GetAnimPhase() { return 0.0f; }
    };

    using EditorLightComponentRequestBus = AZ::EBus<EditorLightComponentRequests>;
} // namespace LmbrCentral
