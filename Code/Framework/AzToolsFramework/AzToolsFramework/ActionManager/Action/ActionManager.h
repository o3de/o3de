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
            AZStd::string_view identifier,
            AZStd::string_view name,
            AZStd::string_view parentIdentifier) override;
        ActionManagerOperationResult RegisterAction(
            AZStd::string_view contextIdentifier,
            AZStd::string_view identifier,
            AZStd::string_view name,
            AZStd::string_view description,
            AZStd::string_view category,
            AZStd::string_view iconPath,
            AZStd::function<void()> handler) override;
        QAction* GetAction(AZStd::string_view actionIdentifier) override;
        const QAction* GetActionConst(AZStd::string_view actionIdentifier) const override;

        void ClearActionContextMap();

        AZStd::unordered_map<AZStd::string, EditorActionContext*> m_actionContexts;
        AZStd::unordered_map<AZStd::string, EditorAction> m_actions;
    };

} // namespace AzToolsFramework
