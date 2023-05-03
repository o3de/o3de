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
    class ArticulationSensorRequests : public AZ::ComponentBus
    {
    public:
        //! Get the transform of the sensor relative to the body frame of the link it is attached to.
        //! The sensor index is per-link, and differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Transform GetSensorTransform(AZ::u32 sensorIndex) const = 0;

        //! Set the transform of the sensor relative to the body frame of the link it is attached to.
        //! The sensor index is per-link, and differs from the per-actuation indices used internally by PhysX.
        virtual void SetSensorTransform(AZ::u32 sensorIndex, const AZ::Transform& sensorTransform) = 0;

        //! Get the force reported by the sensor.
        //! The sensor index is per-link, and differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetForce(AZ::u32 sensorIndex) const = 0;

        //! Get the torque reported by the sensor.
        //! The sensor index is per-link, and differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetTorque(AZ::u32 sensorIndex) const = 0;
    };

    using ArticulationSensorRequestBus = AZ::EBus<ArticulationSensorRequests>;
} // namespace PhysX
