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
     * LightComponentRequestBus
     * Messages serviced by the light component.
     */
    class LightComponentRequests
        : public AZ::ComponentBus
    {
    public:

        enum class State
        {
            Off = 0,
            On,
        };

        virtual ~LightComponentRequests() {}

        //! Control light state.
        virtual void SetLightState([[maybe_unused]] State state) {}

        //! Turns light on. Returns true if the light was successfully turned on
        virtual bool TurnOnLight() { return false; }

        //! Turns light off. Returns true if the light was successfully turned off
        virtual bool TurnOffLight() { return false; }

        //! Toggles light state.
        virtual void ToggleLight() {}

        ////////////////////////////////////////////////////////////////////
        // Modifiers - these must match the same virutal methods in LightComponentEditorRequests

        // General Settings Modifiers
        virtual void SetVisible(bool isVisible) { (void)isVisible; }
        virtual bool GetVisible() { return true; }

        virtual void SetColor(const AZ::Color& newColor) { (void)newColor; };
        virtual const AZ::Color GetColor() { return AZ::Color::CreateOne(); }

        virtual void SetDiffuseMultiplier(float newMultiplier) { (void)newMultiplier; };
        virtual float GetDiffuseMultiplier() { return FLT_MAX; }

        virtual void SetSpecularMultiplier(float newMultiplier) { (void)newMultiplier; };
        virtual float GetSpecularMultiplier() { return FLT_MAX; }

        virtual void SetAmbient(bool isAmbient) { (void)isAmbient; }
        virtual bool GetAmbient() { return true; }

        // Point Light Specific Modifiers
        virtual void SetPointMaxDistance(float newMaxDistance) { (void)newMaxDistance; };
        virtual float GetPointMaxDistance() { return FLT_MAX; }

        virtual void SetPointAttenuationBulbSize(float newAttenuationBulbSize) { (void)newAttenuationBulbSize; };
        virtual float GetPointAttenuationBulbSize() { return FLT_MAX; }

        // Area Light Specific Modifiers
        virtual void SetAreaMaxDistance(float newMaxDistance) { (void)newMaxDistance; };
        virtual float GetAreaMaxDistance() { return FLT_MAX; }

        virtual void SetAreaWidth(float newWidth) { (void)newWidth; };
        virtual float GetAreaWidth() { return FLT_MAX; }

        virtual void SetAreaHeight(float newHeight) { (void)newHeight; };
        virtual float GetAreaHeight() { return FLT_MAX; }

        virtual void SetAreaFOV(float newFOV) { (void)newFOV; };
        virtual float GetAreaFOV() { return FLT_MAX; }

        // Project Light Specific Modifiers
        virtual void SetProjectorMaxDistance(float newMaxDistance) { (void)newMaxDistance; };
        virtual float GetProjectorMaxDistance() { return FLT_MAX; }

        virtual void SetProjectorAttenuationBulbSize(float newAttenuationBulbSize) { (void)newAttenuationBulbSize; };
        virtual float GetProjectorAttenuationBulbSize() { return FLT_MAX; }

        virtual void SetProjectorFOV(float newFOV) { (void)newFOV; };
        virtual float GetProjectorFOV() { return FLT_MAX; }

        virtual void SetProjectorNearPlane(float newNearPlane) { (void)newNearPlane; };
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
        ////////////////////////////////////////////////////////////////////  
    };

    using LightComponentRequestBus = AZ::EBus<LightComponentRequests>;

    /*!
     * LightComponentNotificationBus
     * Events dispatched by the light component.
     */
    class LightComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        virtual ~LightComponentNotifications() {}

        // Sent when the light is turned on.
        virtual void LightTurnedOn() {}

        // Sent when the light is turned off.
        virtual void LightTurnedOff() {}
    };

    using LightComponentNotificationBus = AZ::EBus < LightComponentNotifications >;

    /*!
    * LightSettingsNotifications
    * Events dispatched by the light component or light component editor when settings have changed.
    */
    class LightSettingsNotifications
        : public AZ::ComponentBus
    {
    public:

        virtual ~LightSettingsNotifications() {}

        virtual void AnimationSettingsChanged() = 0;
    };

    using LightSettingsNotificationsBus = AZ::EBus < LightSettingsNotifications >;
    
} // namespace LmbrCentral
