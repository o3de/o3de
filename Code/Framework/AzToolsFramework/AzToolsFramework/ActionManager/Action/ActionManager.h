/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/EditorAction.h>
#include <AzToolsFramework/ActionManager/Action/EditorActionContext.h>

namespace AzToolsFramework
{
    class ActionManager
        : private ActionManagerInterface
    {
    public:
        ActionManager();
        virtual ~ActionManager();

    private:
        // ActionManagerInterface overrides ...
        void RegisterActionContext(
            QWidget* widget,
            AZStd::string_view identifier,
            AZStd::string_view name,
            AZStd::string_view parentIdentifier) override;
        void RegisterAction(
            AZStd::string_view contextIdentifier,
            AZStd::string_view identifier,
            AZStd::string_view name,
            AZStd::string_view description,
            AZStd::string_view category,
            AZStd::string_view iconPath,
            AZStd::function<void()> handler) override;
        QAction* GetAction(AZStd::string_view actionIdentifier) override;

        void ClearActionContextMap();

        AZStd::unordered_map<AZStd::string, EditorActionContext*> m_actionContexts;
        AZStd::unordered_map<AZStd::string, EditorAction> m_actions;
    };

} // namespace AzToolsFramework
