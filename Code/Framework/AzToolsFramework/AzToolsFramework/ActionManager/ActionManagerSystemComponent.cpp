/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>

namespace AzToolsFramework
{
    ActionManagerSystemComponent::~ActionManagerSystemComponent()
    {
        if (m_defaultParentObject)
        {
            delete m_defaultParentObject;
            m_defaultParentObject = nullptr;
        }
    }

    void ActionManagerSystemComponent::Init()
    {
        if (IsNewActionManagerEnabled())
        {
            m_defaultParentObject = new QWidget();

            m_actionManager = AZStd::make_unique<ActionManager>();
            m_hotKeyManager = AZStd::make_unique<HotKeyManager>();
            m_menuManager = AZStd::make_unique<MenuManager>(m_defaultParentObject);
            m_toolBarManager = AZStd::make_unique<ToolBarManager>(m_defaultParentObject);
        }
    }

    void ActionManagerSystemComponent::Activate()
    {
        if (IsNewActionManagerEnabled())
        {
            AzToolsFramework::EditorEventsBus::Handler::BusConnect();
        }
    }

    void ActionManagerSystemComponent::Deactivate()
    {
        if (IsNewActionManagerEnabled())
        {
            AzToolsFramework::EditorEventsBus::Handler::BusDisconnect();
        }
    }

    void ActionManagerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ActionManagerSystemComponent, AZ::Component>()->Version(1);
        }

        MenuManager::Reflect(context);
        ToolBarManager::Reflect(context);
    }

    void ActionManagerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ActionManager"));
    }

    void ActionManagerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ActionManagerSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void ActionManagerSystemComponent::NotifyMainWindowInitialized(QMainWindow* /* mainWindow */)
    {
        // Broadcast synchronization hooks.
        // Order is important since latter elements may have depencencies on earlier ones.
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnActionContextRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnActionContextModeRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnActionUpdaterRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnMenuBarRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnMenuRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnToolBarAreaRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnToolBarRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnActionRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnWidgetActionRegistrationHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnMenuBindingHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnToolBarBindingHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnPostActionManagerRegistrationHook);
    }

} // namespace AzToolsFramework
