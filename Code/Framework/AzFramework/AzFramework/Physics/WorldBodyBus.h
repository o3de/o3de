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
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace Physics
{
    //! Requests for generic physical world bodies
    class WorldBodyRequests
        : public AZ::ComponentBus
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        //! Enable physics for this body
        virtual void EnablePhysics() = 0;
        //! Disable physics for this body
        virtual void DisablePhysics() = 0;
        //! Retrieve whether physics is enabled for this body
        virtual bool IsPhysicsEnabled() const = 0;

        //! Retrieves the AABB(aligned-axis bounding box) for this body
        virtual AZ::Aabb GetAabb() const = 0;
        //! Retrieves current WorldBody* for this body. Note: Do not hold a reference to AzPhysics::SimulatedBody* as could be deleted 
        virtual AzPhysics::SimulatedBody* GetWorldBody() = 0;

        //! Perform a single-object raycast against this body
        virtual AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) = 0;
    };
    using WorldBodyRequestBus = AZ::EBus<WorldBodyRequests>;

    //! Notifications for generic physical world bodies
    class WorldBodyNotifications
        : public AZ::ComponentBus
    {
    public:
        //! Notification for physics enabled
        virtual void OnPhysicsEnabled() = 0;
        //! Notification for physics disabled
        virtual void OnPhysicsDisabled() = 0;
    };
    using WorldBodyNotificationBus = AZ::EBus<WorldBodyNotifications>;
}
