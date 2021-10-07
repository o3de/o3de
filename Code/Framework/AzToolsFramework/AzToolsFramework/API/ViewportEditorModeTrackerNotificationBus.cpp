/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>

namespace AzToolsFramework
{
    void ViewportEditorModeNotifications::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ViewportEditorModeNotificationsBus>("ViewportEditorModeNotificationsBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Event("OnEditorModeActivated", &ViewportEditorModeNotifications::OnEditorModeActivated)
                ->Event("OnEditorModeDeactivated", &ViewportEditorModeNotifications::OnEditorModeDeactivated)
                ;
        }
    }
} // namespace AzToolsFramework
