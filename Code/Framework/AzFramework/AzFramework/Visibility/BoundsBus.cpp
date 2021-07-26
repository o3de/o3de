/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BoundsBus.h"

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework
{
    void BoundsRequests::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<BoundsRequestBus>("BoundsRequestBus")
                ->Event("GetWorldBounds", &BoundsRequestBus::Events::GetWorldBounds)
                ->Event("GetLocalBounds", &BoundsRequestBus::Events::GetLocalBounds);
        }
    }
} // namespace AzFramework
