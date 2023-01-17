/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Viewport/ViewBookmarkSystemComponent.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>

namespace AzToolsFramework
{
    void ViewBookmarkSystemComponent::Activate()
    {
        m_viewBookmarkLoader.RegisterViewBookmarkInterface();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void ViewBookmarkSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        m_viewBookmarkLoader.UnregisterViewBookmarkInterface();
    }

    void ViewBookmarkSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ViewBookmarkSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void ViewBookmarkSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ViewBookmarkSystem"));
    }

    void ViewBookmarkSystemComponent::OnActionRegistrationHook()
    {
        PaintBrushManipulator::RegisterActions();
    }

    void ViewBookmarkSystemComponent::OnMenuBindingHook()
    {
        PaintBrushManipulator::BindActionsToMenus();
    }

} // namespace AzToolsFramework
