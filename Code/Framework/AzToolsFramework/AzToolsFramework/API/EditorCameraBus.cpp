/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                ->Event("GetCurrentViewEntityId", &EditorCameraRequestBus::Events::GetCurrentViewEntityId)
                ->Event("GetActiveCameraPosition", &EditorCameraRequestBus::Events::GetActiveCameraPosition)
                ;
        }
    }
}
