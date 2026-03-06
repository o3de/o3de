/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Vector3;
    class Transform;
}

namespace PhysX
{
    //! Interface to communicate with a sensor in a PhysX reduced co-ordinate articulation.
    class ArticulationCacheRequests : public AZ::ComponentBus
    {
    public:
        // TODO: Add additional setters and getters

        //! Get the force reported by the sensor.
        //! The sensor index is per-link, and differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetForce(AZ::u32 sensorIndex) const = 0;

        //! Get the torque reported by the sensor.
        //! The sensor index is per-link, and differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetTorque(AZ::u32 sensorIndex) const = 0;
    };

    using ArticulationCacheRequestBus = AZ::EBus<ArticulationCacheRequests>;
} // namespace PhysX
