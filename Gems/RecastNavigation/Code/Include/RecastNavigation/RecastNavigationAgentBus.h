/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>

namespace RecastNavigation
{
    class RecastNavigationAgentRequests
        : public AZ::ComponentBus
    {
    public:

        virtual AZStd::vector<AZ::Vector3> PathToEntity( AZ::EntityId targetEntity ) = 0;
        virtual AZStd::vector<AZ::Vector3> PathToPosition( const AZ::Vector3& targetWorldPosition ) = 0;
        virtual void CancelPath() = 0;
    };

    using RecastNavigationAgentRequestBus = AZ::EBus<RecastNavigationAgentRequests>;

} // namespace RecastNavigation
