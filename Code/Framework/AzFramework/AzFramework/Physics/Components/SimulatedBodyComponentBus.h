/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AzPhysics
{
    //! Requests for physics simulated body components.
    class SimulatedBodyComponentRequests
        : public AZ::ComponentBus
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        //! Enable physics for this body.
        virtual void EnablePhysics() = 0;
        //! Disable physics for this body.
        virtual void DisablePhysics() = 0;
        //! Retrieve whether physics is enabled for this body.
        virtual bool IsPhysicsEnabled() const = 0;

        //! Retrieves the AABB(aligned-axis bounding box) for this body.
        virtual AZ::Aabb GetAabb() const = 0;
        //! Get the Simulated Body Handle for this body.
        virtual AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const = 0;
        //! Retrieves current WorldBody* for this body.
        //! @note Do not hold a reference to AzPhysics::SimulatedBody* as it could be deleted or moved.
        virtual AzPhysics::SimulatedBody* GetSimulatedBody() = 0;
        //! Perform a single-object raycast against this body.
        virtual AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) = 0;
    };
    using SimulatedBodyComponentRequestsBus = AZ::EBus<SimulatedBodyComponentRequests>;
}
