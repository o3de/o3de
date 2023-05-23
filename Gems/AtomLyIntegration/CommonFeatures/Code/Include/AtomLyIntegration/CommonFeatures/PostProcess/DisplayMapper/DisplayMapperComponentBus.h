/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <ACES/Aces.h>

namespace AZ
{
    namespace Render
    {
        struct AcesParameterOverrides;

        //! DisplayMapperComponentRequests provides an interface to request operations on a DisplayMapperComponent
        class DisplayMapperComponentRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::DisplayMapperComponentRequests, "{9E2E8AF5-1176-44B4-A461-E09867753349}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            //! Load preconfigured preset for specific ODT mode
            virtual void LoadPreset(OutputDeviceTransformType preset) = 0;
            //! Set display mapper type
            virtual void SetDisplayMapperOperationType(DisplayMapperOperationType displayMapperOperationType) = 0;
            //! Get display mapper type
            virtual DisplayMapperOperationType GetDisplayMapperOperationType() const = 0;
            //! Set ACES parameter overrides for ACES mapping, display mapper must be set to Aces to see the difference
            virtual void SetAcesParameterOverrides(const AcesParameterOverrides& parameterOverrides) = 0;
            //! Get ACES parameter overrides
            virtual const AcesParameterOverrides& GetAcesParameterOverrides() const = 0;

            // Enable or disable ACES parameter overrides
            virtual void SetOverrideAcesParameters(bool value) = 0;
            // Check if ACES parameters are overriding default preset values
            virtual bool GetOverrideAcesParameters() const = 0;

            // Set gamma adjustment to compensate for dim surround
            virtual void SetAlterSurround(bool value) = 0;
            // Get gamma adjustment to compensate for dim surround
            virtual bool GetAlterSurround() const = 0;
                        
            // Set desaturation to compensate for luminance difference
            virtual void SetApplyDesaturation(bool value) = 0;
            // Get desaturation to compensate for luminance difference
            virtual bool GetApplyDesaturation() const = 0;
            
            // Set color appearance transform (CAT) from ACES white point to assumed observer adapted white point
            virtual void SetApplyCATD60toD65(bool value) = 0;
            // Get color appearance transform (CAT) from ACES white point to assumed observer adapted white point
            virtual bool GetApplyCATD60toD65() const = 0;

            // Set reference black luminance value
            virtual void SetCinemaLimitsBlack(float value) = 0;
            // Get reference black luminance value
            virtual float GetCinemaLimitsBlack() const = 0;

            // Set reference white luminance value
            virtual void SetCinemaLimitsWhite(float value) = 0;
            // Get reference white luminance value
            virtual float GetCinemaLimitsWhite() const = 0;

            // Set min luminance value
            virtual void SetMinPoint(float value) = 0;
            // Get min luminance value
            virtual float GetMinPoint() const = 0;

            // Set mid luminance value
            virtual void SetMidPoint(float value) = 0;
            // Get mid luminance value
            virtual float GetMidPoint() const = 0;

            // Set max luminance value
            virtual void SetMaxPoint(float value) = 0;
            // Get max luminance value
            virtual float GetMaxPoint() const = 0;

            // Set gamma adjustment value
            virtual void SetSurroundGamma(float value) = 0;
            // Get gamma adjustment value
            virtual float GetSurroundGamma() const = 0;

            // Set optional gamma value that is applied as basic gamma curve OETF
            virtual void SetGamma(float value) = 0;
            // Get optional gamma value that is applied as basic gamma curve OETF
            virtual float GetGamma() const = 0;
        };
        using DisplayMapperComponentRequestBus = EBus<DisplayMapperComponentRequests>;

        //! DisplayMapperComponent can send out notifications on the DisplayMapperComponentNotifications
        class DisplayMapperComponentNotifications : public ComponentBus
        {
        public:
            //! Notifies that display mapper type changed
            virtual void OnDisplayMapperOperationTypeUpdated([[maybe_unused]] const DisplayMapperOperationType& displayMapperOperationType)
            {
            }

            //! Notifies that ACES parameter overrides changed
            virtual void OnAcesParameterOverridesUpdated([[maybe_unused]] const AcesParameterOverrides& acesParameterOverrides)
            {
            }
        };
        using DisplayMapperComponentNotificationBus = EBus<DisplayMapperComponentNotifications>;

    } // namespace Render
} // namespace AZ
