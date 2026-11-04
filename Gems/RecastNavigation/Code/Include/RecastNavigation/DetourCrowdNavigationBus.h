/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <RecastNavigation/DetourCrowdAgentParams.h>

namespace RecastNavigation
{
    //! Interface for crowd navigation API.
    class DetourCrowdNavigationRequests : public AZ::ComponentBus
    {
    public:
        //! Adds an agent entity with the crowd component.
        virtual AZ::Outcome<void, AZStd::string> AddAgent(
            AZ::EntityId agentEntityId, const AZ::Vector3& worldPosition, const DetourCrowdAgentParams& agentParams) = 0;

        //! Removes a previously added agent entity.
        virtual AZ::Outcome<void, AZStd::string> RemoveAgent(AZ::EntityId agentEntityId) = 0;

        //! Sets the movement target for an agent in the crowd.
        virtual AZ::Outcome<void, AZStd::string> SetAgentMoveTarget(AZ::EntityId agentEntityId, const AZ::Vector3& targetPosition) = 0;

        //! Clears the movement target for an agent in the crowd.
        virtual AZ::Outcome<void, AZStd::string> ResetAgentMoveTarget(AZ::EntityId agentEntityId) = 0;
    };

    //! Request EBus for a crowd navigation component.
    using DetourCrowdNavigationRequestBus = AZ::EBus<DetourCrowdNavigationRequests>;
} // namespace RecastNavigation
