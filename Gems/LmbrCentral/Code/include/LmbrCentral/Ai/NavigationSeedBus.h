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
