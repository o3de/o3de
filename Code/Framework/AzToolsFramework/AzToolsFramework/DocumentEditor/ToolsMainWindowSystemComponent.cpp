/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/DocumentEditor/ToolsMainWindowRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/DocumentEditor/ToolsMainWindowSystemComponent.h>

namespace AzToolsFramework
{
    void ToolsMainWindowSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ToolsMainWindowSystemComponent, AZ::Component>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ToolsMainWindowRequestBus>("ToolsMainWindowRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "tools")
                ->Event("ActivateWindow", &ToolsMainWindowRequestBus::Events::ActivateWindow)
                ->Event("SetDockWidgetVisible", &ToolsMainWindowRequestBus::Events::SetDockWidgetVisible)
                ->Event("IsDockWidgetVisible", &ToolsMainWindowRequestBus::Events::IsDockWidgetVisible)
                ->Event("GetDockWidgetNames", &ToolsMainWindowRequestBus::Events::GetDockWidgetNames)
                ->Event("QueueUpdateMenus", &ToolsMainWindowRequestBus::Events::QueueUpdateMenus)
                ->Event("SetStatusMessage", &ToolsMainWindowRequestBus::Events::SetStatusMessage)
                ->Event("SetStatusWarning", &ToolsMainWindowRequestBus::Events::SetStatusWarning)
                ->Event("SetStatusError", &ToolsMainWindowRequestBus::Events::SetStatusError)
                ->Event("ResizeViewportRenderTarget", &ToolsMainWindowRequestBus::Events::ResizeViewportRenderTarget)
                ->Event("LockViewportRenderTargetSize", &ToolsMainWindowRequestBus::Events::LockViewportRenderTargetSize)
                ->Event("UnlockViewportRenderTargetSize", &ToolsMainWindowRequestBus::Events::UnlockViewportRenderTargetSize)
                ;
        }
    }

    void ToolsMainWindowSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ToolsMainWindowSystemService"));
    }

    void ToolsMainWindowSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ToolsMainWindowSystemService"));
    }

    void ToolsMainWindowSystemComponent::Init()
    {
    }

    void ToolsMainWindowSystemComponent::Activate()
    {
    }

    void ToolsMainWindowSystemComponent::Deactivate()
    {
    }

} // namespace AzToolsFramework
