/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>

namespace AzToolsFramework
{
    class ComponentModeActionHandler
        : private ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        ~ComponentModeActionHandler();

        void Initialize();

        //void InitializeComponentMode(const AZStd::string& modeIdentifier);
        //void AddActionToComponentMode();

    private:
        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionUpdaterRegistrationHook() override;
        void OnActionRegistrationHook() override;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
        HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
