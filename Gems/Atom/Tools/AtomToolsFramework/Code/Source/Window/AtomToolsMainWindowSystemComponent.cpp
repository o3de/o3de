/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Window/AtomToolsMainWindowSystemComponent.h>

namespace AtomToolsFramework
{
    void AtomToolsMainWindowSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomToolsMainWindowSystemComponent, AZ::Component>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomToolsMainWindowRequestBus>("AtomToolsMainWindowRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("ActivateWindow", &AtomToolsMainWindowRequestBus::Events::ActivateWindow)
                ->Event("SetDockWidgetVisible", &AtomToolsMainWindowRequestBus::Events::SetDockWidgetVisible)
                ->Event("IsDockWidgetVisible", &AtomToolsMainWindowRequestBus::Events::IsDockWidgetVisible)
                ->Event("GetDockWidgetNames", &AtomToolsMainWindowRequestBus::Events::GetDockWidgetNames)
                ->Event("QueueUpdateMenus", &AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus)
                ->Event("SetStatusMessage", &AtomToolsMainWindowRequestBus::Events::SetStatusMessage)
                ->Event("SetStatusWarning", &AtomToolsMainWindowRequestBus::Events::SetStatusWarning)
                ->Event("SetStatusError", &AtomToolsMainWindowRequestBus::Events::SetStatusError)
                ->Event("ResizeViewportRenderTarget", &AtomToolsMainWindowRequestBus::Events::ResizeViewportRenderTarget)
                ->Event("LockViewportRenderTargetSize", &AtomToolsMainWindowRequestBus::Events::LockViewportRenderTargetSize)
                ->Event("UnlockViewportRenderTargetSize", &AtomToolsMainWindowRequestBus::Events::UnlockViewportRenderTargetSize)
                ;
        }
    }

    void AtomToolsMainWindowSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomToolsMainWindowSystemService"));
    }

    void AtomToolsMainWindowSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomToolsMainWindowSystemService"));
    }

    void AtomToolsMainWindowSystemComponent::Init()
    {
    }

    void AtomToolsMainWindowSystemComponent::Activate()
    {
    }

    void AtomToolsMainWindowSystemComponent::Deactivate()
    {
    }

} // namespace AtomToolsFramework
