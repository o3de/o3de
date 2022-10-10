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
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Action/EditorAction.h>
#include <AzToolsFramework/ActionManager/Action/EditorActionContext.h>
#include <AzToolsFramework/ActionManager/Action/EditorWidgetAction.h>

namespace AzToolsFramework
{
    //! Action Manager class definition.
    //! Handles Editor Actions and allows registration and access across tools.
    class ActionManager final
        : private ActionManagerInterface
        , private ActionManagerInternalInterface
    {
    public:
        ActionManager();
        ~ActionManager();

    private:
        // ActionManagerInterface overrides ...
        ActionManagerOperationResult RegisterActionContext(
            const AZStd::string& parentContextIdentifier,
            const AZStd::string& contextIdentifier,
            const ActionContextProperties& properties,
            QWidget* widget
        ) override;
        bool IsActionContextRegistered(const AZStd::string& contextIdentifier) const override;
        ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const ActionProperties& properties,
            AZStd::function<void()> handler
        ) override;
        ActionManagerOperationResult RegisterCheckableAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const ActionProperties& properties,
            AZStd::function<void()> handler,
            AZStd::function<bool()> checkStateCallback
        ) override;
        bool IsActionRegistered(const AZStd::string& actionIdentifier) const override;
        ActionManagerGetterResult GetActionName(const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult SetActionName(const AZStd::string& actionIdentifier, const AZStd::string& name) override;
        ActionManagerGetterResult GetActionDescription(const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult SetActionDescription(const AZStd::string& actionIdentifier, const AZStd::string& description) override;
        ActionManagerGetterResult GetActionCategory(const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult SetActionCategory(const AZStd::string& actionIdentifier, const AZStd::string& category) override;
        ActionManagerGetterResult GetActionIconPath(const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult SetActionIconPath(const AZStd::string& actionIdentifier, const AZStd::string& iconPath) override;
        ActionManagerBooleanResult IsActionEnabled(const AZStd::string& actionIdentifier) const override;
        ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult InstallEnabledStateCallback(const AZStd::string& actionIdentifier, AZStd::function<bool()> enabledStateCallback) override;
        ActionManagerOperationResult UpdateAction(const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult RegisterActionUpdater(const AZStd::string& actionUpdaterIdentifier) override;
        ActionManagerOperationResult AddActionToUpdater(const AZStd::string& actionUpdaterIdentifier, const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult TriggerActionUpdater(const AZStd::string& actionUpdaterIdentifier) override;
        ActionManagerOperationResult RegisterWidgetAction(
            const AZStd::string& widgetActionIdentifier,
            const WidgetActionProperties& properties,
            AZStd::function<QWidget*()> generator
        ) override;
        bool IsWidgetActionRegistered(const AZStd::string& widgetActionIdentifier) const override;
        ActionManagerGetterResult GetWidgetActionName(const AZStd::string& widgetActionIdentifier) override;
        ActionManagerOperationResult SetWidgetActionName(const AZStd::string& widgetActionIdentifier, const AZStd::string& name) override;
        ActionManagerGetterResult GetWidgetActionCategory(const AZStd::string& widgetActionIdentifier) override;
        ActionManagerOperationResult SetWidgetActionCategory(
            const AZStd::string& widgetActionIdentifier, const AZStd::string& category) override;
        ActionManagerOperationResult RegisterActionContextMode(
            const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier) override;
        ActionManagerOperationResult AssignModeToAction(
            const AZStd::string& modeIdentifier, const AZStd::string& actionIdentifier) override;
        ActionManagerOperationResult SetActiveActionContextMode(
            const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier) override;
        ActionManagerGetterResult GetActiveActionContextMode(const AZStd::string& actionContextIdentifier) const override;

        // ActionManagerInternalInterface overrides ...
        QAction* GetAction(const AZStd::string& actionIdentifier) override;
        const QAction* GetActionConst(const AZStd::string& actionIdentifier) const override;
        EditorAction* GetEditorAction(const AZStd::string& actionIdentifier) override;
        const EditorAction* GetEditorActionConst(const AZStd::string& actionIdentifier) const override;
        bool GetHideFromMenusWhenDisabled(const AZStd::string& actionIdentifier) const override;
        bool GetHideFromToolBarsWhenDisabled(const AZStd::string& actionIdentifier) const override;
        QWidget* GenerateWidgetFromWidgetAction(const AZStd::string& widgetActionIdentifier) override;
        void UpdateAllActionsInActionContext(const AZStd::string& actionContextIdentifier) override;

        void ClearActionContextMap();

        AZStd::unordered_map<AZStd::string, EditorActionContext*> m_actionContexts;
        AZStd::unordered_map<AZStd::string, EditorAction> m_actions;
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> m_actionUpdaters;
        AZStd::unordered_map<AZStd::string, EditorWidgetAction> m_widgetActions;
    };

} // namespace AzToolsFramework
