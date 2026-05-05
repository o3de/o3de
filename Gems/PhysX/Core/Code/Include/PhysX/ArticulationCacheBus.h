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
    //! Interface to communicate with the articulation cache in a PhysX reduced co-ordinate articulation.
    class ArticulationCacheRequests : public AZ::ComponentBus
    {
    public:
        // TODO: Add additional getters

        //! Get the force for a link recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetForce(AZ::u32 linkIndex) const = 0;

        //! Get the torque reported recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetTorque(AZ::u32 linkIndex) const = 0;
    };

    using ArticulationCacheRequestBus = AZ::EBus<ArticulationCacheRequests>;
} // namespace PhysX
