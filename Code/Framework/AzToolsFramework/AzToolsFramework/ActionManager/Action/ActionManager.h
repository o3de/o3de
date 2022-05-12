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
    //! Action Manager class definition.
    //! Handles Editor Actions and allows registration and access across tools.
    class ActionManager final
        : private ActionManagerInterface
    {
    public:
        ActionManager();
        ~ActionManager();

    private:
        // ActionManagerInterface overrides ...
        ActionManagerOperationResult RegisterActionContext(
            QWidget* widget,
            const AZStd::string& identifier,
            const AZStd::string& name, const AZStd::string& parentIdentifier) override;
        ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& identifier,
            const AZStd::string& name,
            const AZStd::string& description,
            const AZStd::string& category,
            const AZStd::string& iconPath,
            AZStd::function<void()> handler) override;
        ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) override;
        QAction* GetAction(const AZStd::string& actionIdentifier) override;
        const QAction* GetActionConst(const AZStd::string& actionIdentifier) override;

        void ClearActionContextMap();

        AZStd::unordered_map<AZStd::string, EditorActionContext*> m_actionContexts;
        AZStd::unordered_map<AZStd::string, EditorAction> m_actions;
    };

} // namespace AzToolsFramework
