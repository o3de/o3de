/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <QApplication>

namespace AzToolsFramework
{
    namespace Internal
    {
        struct ActionManagerRegistrationNotificationBusHandler final
            : public ActionManagerRegistrationNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
            AZ_EBUS_BEHAVIOR_BINDER(
                ActionManagerRegistrationNotificationBusHandler,
                "{A9397A5D-0B04-4552-814F-0B4F385A3890}",
                AZ::SystemAllocator,
                OnActionContextRegistrationHook,
                OnActionContextModeRegistrationHook,
                OnActionUpdaterRegistrationHook,
                OnMenuBarRegistrationHook,
                OnMenuRegistrationHook,
                OnToolBarAreaRegistrationHook,
                OnToolBarRegistrationHook,
                OnActionRegistrationHook,
                OnWidgetActionRegistrationHook,
                OnActionContextModeBindingHook,
                OnMenuBindingHook,
                OnToolBarBindingHook,
                OnPostActionManagerRegistrationHook
            );

            void OnActionContextRegistrationHook() override
            {
                Call(FN_OnActionContextRegistrationHook);
            }

            void OnActionContextModeRegistrationHook() override
            {
                Call(FN_OnActionContextModeRegistrationHook);
            }

            void OnActionUpdaterRegistrationHook() override
            {
                Call(FN_OnActionUpdaterRegistrationHook);
            }

            void OnMenuBarRegistrationHook() override
            {
                Call(FN_OnMenuBarRegistrationHook);
            }

            void OnMenuRegistrationHook() override
            {
                Call(FN_OnMenuRegistrationHook);
            }

            void OnToolBarAreaRegistrationHook() override
            {
                Call(FN_OnToolBarAreaRegistrationHook);
            }

            void OnToolBarRegistrationHook() override
            {
                Call(FN_OnToolBarRegistrationHook);
            }

            void OnActionRegistrationHook() override
            {
                Call(FN_OnActionRegistrationHook);
            }

            void OnWidgetActionRegistrationHook() override
            {
                Call(FN_OnWidgetActionRegistrationHook);
            }

            void OnActionContextModeBindingHook() override
            {
                Call(FN_OnActionContextModeBindingHook);
            }

            void OnMenuBindingHook() override
            {
                Call(FN_OnMenuBindingHook);
            }

            void OnToolBarBindingHook() override
            {
                Call(FN_OnToolBarBindingHook);
            }

            void OnPostActionManagerRegistrationHook() override
            {
                Call(FN_OnPostActionManagerRegistrationHook);
            }
        };
    } // namespace Internal

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
        // The ActionManagerSystemComponent could be created in a tooling context
        // where a QGuiApplication does not exist such as the SerializeContextTools
        auto qAppInstance = QApplication::instance();
        if (qAppInstance != nullptr)
        {
            m_defaultParentObject = new QWidget();

            m_actionManager = AZStd::make_unique<ActionManager>();
            m_hotKeyManager = AZStd::make_unique<HotKeyManager>();
            m_menuManager = AZStd::make_unique<MenuManager>(m_defaultParentObject);
            m_toolBarManager = AZStd::make_unique<ToolBarManager>(m_defaultParentObject);

            m_hotKeyWidgetRegistrationHelper = AZStd::make_unique<HotKeyWidgetRegistrationHelper>();
        }
    }

    void ActionManagerSystemComponent::Activate()
    {
    }

    void ActionManagerSystemComponent::Deactivate()
    {
    }
    void ActionManagerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        MenuManager::Reflect(context);
        ToolBarManager::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ActionManagerSystemComponent, AZ::Component>()->Version(1);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ActionManagerRegistrationNotificationBus>("ActionManagerRegistrationNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Handler<Internal::ActionManagerRegistrationNotificationBusHandler>()
                ->Event("OnActionContextRegistrationHook", &ActionManagerRegistrationNotifications::OnActionContextRegistrationHook)
                ->Event("OnActionContextModeRegistrationHook", &ActionManagerRegistrationNotifications::OnActionContextModeRegistrationHook)
                ->Event("OnActionUpdaterRegistrationHook", &ActionManagerRegistrationNotifications::OnActionUpdaterRegistrationHook)
                ->Event("OnMenuBarRegistrationHook", &ActionManagerRegistrationNotifications::OnMenuBarRegistrationHook)
                ->Event("OnMenuRegistrationHook", &ActionManagerRegistrationNotifications::OnMenuRegistrationHook)
                ->Event("OnToolBarAreaRegistrationHook", &ActionManagerRegistrationNotifications::OnToolBarAreaRegistrationHook)
                ->Event("OnToolBarRegistrationHook", &ActionManagerRegistrationNotifications::OnToolBarRegistrationHook)
                ->Event("OnActionRegistrationHook", &ActionManagerRegistrationNotifications::OnActionRegistrationHook)
                ->Event("OnWidgetActionRegistrationHook", &ActionManagerRegistrationNotifications::OnWidgetActionRegistrationHook)
                ->Event("OnActionContextModeBindingHook", &ActionManagerRegistrationNotifications::OnActionContextModeBindingHook)
                ->Event("OnMenuBindingHook", &ActionManagerRegistrationNotifications::OnMenuBindingHook)
                ->Event("OnToolBarBindingHook", &ActionManagerRegistrationNotifications::OnToolBarBindingHook)
                ->Event("OnPostActionManagerRegistrationHook", &ActionManagerRegistrationNotifications::OnPostActionManagerRegistrationHook)
                ;
        }
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

    void ActionManagerSystemComponent::TriggerRegistrationNotifications()
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
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnActionContextModeBindingHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnMenuBindingHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnToolBarBindingHook);
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Broadcast(
            &AzToolsFramework::ActionManagerRegistrationNotifications::OnPostActionManagerRegistrationHook);
    }

} // namespace AzToolsFramework
