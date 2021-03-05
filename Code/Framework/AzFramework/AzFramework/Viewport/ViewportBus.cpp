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
