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
#include "EditorCameraBus.h"

namespace Camera
{
    void EditorCameraRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorCameraRequestBus>("EditorCameraRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Event("SetViewFromEntityPerspective", &EditorCameraRequestBus::Events::SetViewFromEntityPerspective)
                ->Event("SetViewAndMovementLockFromEntityPerspective", &EditorCameraRequestBus::Events::SetViewAndMovementLockFromEntityPerspective)
                ->Event("GetCurrentViewEntityId", &EditorCameraRequestBus::Events::GetCurrentViewEntityId)
                ->Event("GetActiveCameraPosition", &EditorCameraRequestBus::Events::GetActiveCameraPosition)
                ;
        }
    }
}
