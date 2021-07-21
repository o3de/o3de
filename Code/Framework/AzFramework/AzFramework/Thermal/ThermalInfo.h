/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// Description : Ebus for querying thermal information of the device.

#pragma once
#include <AzCore/EBus/EBus.h>

// The different types of temperature sensor that could be available on the device.
enum class ThermalSensorType : int
{
    CPU = 0,
    GPU,
    Battery,
    Count
};

// Handles requests for thermal information
class ThermalInfoHandler : public AZ::EBusTraits
{
public:
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

    virtual ~ThermalInfoHandler() = default;

    /**
    * Returns the current temperature of a specific sensor in Celcius degrees.
    * If the type of sensor is not available on the device it returns 0.
    *
    * \param sensor The sensor type.
    */
    virtual float GetSensorTemp(ThermalSensorType sensor) = 0;

    /**
    * Returns the temperature that is considered as overheating for a specific sensor.
    * The value returned is in Celcius degrees.
    *
    * \param sensor The sensor type.
    */
    virtual float GetSensorOverheatingTemp(ThermalSensorType sensor) = 0;
};

using ThermalInfoRequestsBus = AZ::EBus<ThermalInfoHandler>;


