/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace LmbrCentral
{
    /**
     * Messages serviced by the WindVolumeComponent
     */
    class WindVolumeRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Sets the falloff. Only affects the wind speed in the volume. A value of 0 will reduce the speed from the center towards the edge of the volume.
         * A value of 1 or greater will have no effect.
         */
        virtual void SetFalloff(float falloff) = 0;

        /**
         * Gets the falloff.
         */
        virtual float GetFalloff() = 0;

        /**
         * Sets the speed of the wind. The air resistance must be non zero to affect physical objects.
         */
        virtual void SetSpeed(float speed) = 0;

        /**
         * Gets the speed of the wind.
         */
        virtual float GetSpeed() = 0;

        /** 
         * Sets the air resistance causing objects moving through the volume to slow down.
         */ 
        virtual void SetAirResistance(float airResistance) = 0;

        /** 
         * Gets the air resistance.
         */
        virtual float GetAirResistance() = 0;

        /**
         * Sets the density of the volume. 
         * Objects with lower density will experience a buoyancy force. Objects with higher density will sink.
         */
        virtual void SetAirDensity(float airDensity) = 0;

        /**
         * Gets the air density.
         */
        virtual float GetAirDensity() = 0;

        /**
         * Sets the direction the wind is blowing. If zero, then the direction is considered omnidirectional.
         */
        virtual void SetWindDirection(const AZ::Vector3& direction) = 0;

        /**
         * Gets the direction the wind is blowing.
         */
        virtual const AZ::Vector3& GetWindDirection() = 0;
    };

    using WindVolumeRequestBus = AZ::EBus<WindVolumeRequests>;
}
