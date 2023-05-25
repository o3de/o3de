/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyWidgetRegistrationInterface.h>

class QWidget;

namespace AzToolsFramework
{
    class HotKeyManagerInterface;
    class HotKeyWidgetRegistrationInterface;

    class HotKeyWidgetRegistrationHelper final
        : private ActionManagerRegistrationNotificationBus::Handler
        , public HotKeyWidgetRegistrationInterface
    {
    public:
        HotKeyWidgetRegistrationHelper();
        virtual ~HotKeyWidgetRegistrationHelper();

        // HotKeyWidgetRegistrationInterface overrides ...
        void AssignWidgetToActionContext(const AZStd::string& actionContextIdentifier, QWidget* widget) override;

    private:
        // ActionManagerRegistrationNotificationBus overrides ...
        void OnPostActionManagerRegistrationHook() override;

        bool m_isRegistrationCompleted = false;

        struct HotKeyActionContextPair;
        AZStd::vector<HotKeyActionContextPair> m_widgetContextQueue;

        HotKeyManagerInterface* m_hotKeyManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
