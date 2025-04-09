/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/HotKey/HotKeyWidgetRegistrationHelper.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyWidgetRegistrationInterface.h>

namespace AzToolsFramework
{
    struct HotKeyWidgetRegistrationHelper::HotKeyActionContextPair
    {
        AZStd::string actionContextIdentifier;
        QWidget* widget;
    };

    HotKeyWidgetRegistrationHelper::HotKeyWidgetRegistrationHelper()
    {
        m_hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(
            m_hotKeyManagerInterface, "HotKeyWidgetRegistrationHelper - could not get HotKeyManagerInterface on HotKeyWidgetRegistrationHelper construction.");

        ActionManagerRegistrationNotificationBus::Handler::BusConnect();
        AZ::Interface<HotKeyWidgetRegistrationInterface>::Register(this);
    }

    HotKeyWidgetRegistrationHelper::~HotKeyWidgetRegistrationHelper()
    {
        AZ::Interface<HotKeyWidgetRegistrationInterface>::Unregister(this);
        ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void HotKeyWidgetRegistrationHelper::AssignWidgetToActionContext(
        const AZStd::string& actionContextIdentifier, QWidget* widget)
    {
        if (m_isRegistrationCompleted)
        {
            // If the registration hooks were already run, just proceed with the call directly.
            m_hotKeyManagerInterface->AssignWidgetToActionContext(actionContextIdentifier, widget);
        }
        else
        {
            // If we're before the registration has been completed, store the data and call the API later.
            m_widgetContextQueue.push_back({ actionContextIdentifier, widget });
        }
    }

    void HotKeyWidgetRegistrationHelper::OnPostActionManagerRegistrationHook()
    {
        for (auto request : m_widgetContextQueue)
        {
            m_hotKeyManagerInterface->AssignWidgetToActionContext(request.actionContextIdentifier, request.widget);
        }

        m_widgetContextQueue.clear();
        m_isRegistrationCompleted = true;
    }

} // namespace AzToolsFramework
