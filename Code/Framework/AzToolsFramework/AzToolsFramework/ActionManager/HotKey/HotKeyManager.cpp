/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Action/EditorAction.h>

#include <QAction>
#include <QKeySequence>
#include <QWidget>

namespace AzToolsFramework
{
    HotKeyManager::HotKeyManager()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "HotKeyManager - Could not retrieve instance of ActionManagerInterface");

        m_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(m_actionManagerInternalInterface, "HotKeyManager - Could not retrieve instance of ActionManagerInternalInterface");

        AZ::Interface<HotKeyManagerInterface>::Register(this);
    }

    HotKeyManager::~HotKeyManager()
    {
        AZ::Interface<HotKeyManagerInterface>::Unregister(this);
    }

    HotKeyManagerOperationResult HotKeyManager::AssignWidgetToActionContext(
        const AZStd::string& contextIdentifier, QWidget* widget)
    {
        auto widgetWatcher = m_actionManagerInternalInterface->GetActionContextWidgetWatcher(contextIdentifier);
        if (widgetWatcher == nullptr)
        {
            return AZ::Failure(AZStd::string::format(
                "HotKey Manager - Could not assign widget to action context \"%s\" - action context widget watcher could not be found.",
                contextIdentifier.c_str()));
        }

        // Set the context identifier as a property on the widget so that the watcher can be queried easily later
        widget->setProperty(ActionManager::ActionContextWidgetIdentifier.data(), contextIdentifier.c_str());

        widget->installEventFilter(widgetWatcher);
        return AZ::Success();
    }

    HotKeyManagerOperationResult HotKeyManager::RemoveWidgetFromActionContext(
        const AZStd::string& contextIdentifier, QWidget* widget)
    {
        auto widgetWatcher = m_actionManagerInternalInterface->GetActionContextWidgetWatcher(contextIdentifier);
        if (widgetWatcher == nullptr)
        {
            return AZ::Failure(AZStd::string::format(
                "HotKey Manager - Could not remove widget from action context \"%s\" - action context widget watcher could not be found.",
                contextIdentifier.c_str()));
        }

        widget->setProperty(ActionManager::ActionContextWidgetIdentifier.data(), QVariant());
        widget->removeEventFilter(widgetWatcher);
        return AZ::Success();
    }

    HotKeyManagerOperationResult HotKeyManager::SetActionHotKey(const AZStd::string& actionIdentifier, const AZStd::string& hotKey)
    {
        if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format(
                    "HotKey Manager - Could not set hotkey \"%s\" to action \"%s\" - action could not be found.",
                    hotKey.c_str(),
                    actionIdentifier.c_str()
                )
            );
        }

        // Verify string maps to a valid key combination.
        if (QKeySequence(hotKey.c_str()) == Qt::Key_unknown)
        {
            return AZ::Failure(
                AZStd::string::format(
                    "HotKey Manager - Could not set hotkey \"%s\" to action \"%s\" - hotkey not valid.",
                    hotKey.c_str(),
                    actionIdentifier.c_str()
                )
            );
        }

        m_hotKeyMapping.m_actionToHotKeyMap[actionIdentifier] = hotKey;
        m_actionManagerInternalInterface->GetEditorAction(actionIdentifier)->SetHotKey(hotKey);

        return AZ::Success();
    }

    void HotKeyManager::Reset()
    {
        m_hotKeyMapping.m_actionToHotKeyMap.clear();
    }

} // namespace AzToolsFramework
