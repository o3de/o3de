/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/RTTI/BehaviorContext.h>

#include "ViewportBus.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AzFramework
{
    void ViewportRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ViewportRequestBus>("ViewportRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Rendering/Viewport")
                ->Event("GetCameraViewMatrix", &ViewportRequestBus::Events::GetCameraViewMatrix)
                ->Event("SetCameraViewMatrix", &ViewportRequestBus::Events::SetCameraViewMatrix)
                ->Event("GetCameraProjectionMatrix", &ViewportRequestBus::Events::GetCameraProjectionMatrix)
                ->Event("SetCameraProjectionMatrix", &ViewportRequestBus::Events::SetCameraProjectionMatrix)
                ->Event("GetCameraTransform", &ViewportRequestBus::Events::GetCameraTransform)
                ->Event("SetCameraTransform", &ViewportRequestBus::Events::SetCameraTransform)
            ;
        }
    }
}
