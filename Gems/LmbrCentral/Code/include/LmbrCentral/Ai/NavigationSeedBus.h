/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace LmbrCentral
{
    /**
     * Messages serviced by the NavigationSeedComponent.
     * The NavigationSeedComponent traverses all the Navmesh nodes around it
     * and mark the ones it was able to reach
     */
    class NavigationSeedRequests
        : public AZ::EBusTraits
    {
    public:
        virtual void RecalculateReachabilityAroundSelf() {}
        virtual ~NavigationSeedRequests() = default;
    };

    using NavigationSeedRequestsBus = AZ::EBus<NavigationSeedRequests>;
} // namespace LmbrCentral
