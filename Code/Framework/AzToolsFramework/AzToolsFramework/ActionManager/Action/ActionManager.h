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
    //! This class is used to mute KeyPress events that are triggered after a shortcut has fired.
    class ApplicationWatcher : public QObject
    {
    public:
        void SetShortcutTriggeredFlag();

        bool eventFilter(QObject* watched, QEvent* event) override;

        bool m_shortcutWasTriggered = false;
    };

    //! This class is used to install an event filter on each widget assigned to an action context
    //! to properly handle ambiguous shorcuts.
    class ActionContextWidgetWatcher : public QObject
    {
    public:
        explicit ActionContextWidgetWatcher(ApplicationWatcher* applicationWatcher, EditorActionContext* editorActionContext);

        bool eventFilter(QObject* watched, QEvent* event) override;

        bool TriggerActiveActionsForWidget(const QWidget* watchedWidget, const QKeyEvent* keyEvent);

    private:
        static bool TriggerActiveActionsWithShortcut(
            const QList<QAction*>& contextActions, const QList<QAction*>& widgetActions, const QKeySequence& shortcutKeySequence);
        static bool TriggerActiveActionsWithShortcut(
            const QList<QAction*>& contextActions, const QList<QAction*>& widgetActions, const QKeyEvent* shortcutKeyEvent);

        ApplicationWatcher* m_applicationWatcher = nullptr;
        EditorActionContext* m_editorActionContext = nullptr;
    };

    //! Action Manager class definition.
    //! Handles Editor Actions and allows registration and access across tools.
    class ActionManager final
        : private ActionManagerInterface
        , private ActionManagerInternalInterface
    {
    public:
        ActionManager();
        ~ActionManager();

        static constexpr AZStd::string_view ActionContextWidgetIdentifier = "ActionContextWidgetIdentifier";

    private:
        // ActionManagerInterface overrides ...
        ActionManagerOperationResult RegisterActionContext(
            const AZStd::string& contextIdentifier,
            const ActionContextProperties& properties
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
        int GenerateActionAlphabeticalSortKey(const AZStd::string& actionIdentifier) override;
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
        ActionManagerBooleanResult IsActionActiveInCurrentMode(const AZStd::string& actionIdentifier) const override;
        ActionManagerOperationResult SetActiveActionContextMode(
            const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier) override;
        ActionManagerGetterResult GetActiveActionContextMode(const AZStd::string& actionContextIdentifier) const override;

        // ActionManagerInternalInterface overrides ...
        QAction* GetAction(const AZStd::string& actionIdentifier) override;
        const QAction* GetActionConst(const AZStd::string& actionIdentifier) const override;
        EditorAction* GetEditorAction(const AZStd::string& actionIdentifier) override;
        const EditorAction* GetEditorActionConst(const AZStd::string& actionIdentifier) const override;
        ActionContextWidgetWatcher* GetActionContextWidgetWatcher(const AZStd::string& actionContextIdentifier) override;
        ActionVisibility GetActionMenuVisibility(const AZStd::string& actionIdentifier) const override;
        ActionVisibility GetActionToolBarVisibility(const AZStd::string& actionIdentifier) const override;
        QWidget* GenerateWidgetFromWidgetAction(const AZStd::string& widgetActionIdentifier) override;
        void UpdateAllActionsInActionContext(const AZStd::string& actionContextIdentifier) override;
        void Reset();

        ApplicationWatcher m_applicationWatcher;

        AZStd::unordered_map<AZStd::string, EditorActionContext*> m_actionContexts;
        AZStd::unordered_map<AZStd::string, ActionContextWidgetWatcher*> m_actionContextWidgetWatchers;
        AZStd::unordered_map<AZStd::string, EditorAction*> m_actions;
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> m_actionUpdaters;
        AZStd::unordered_map<AZStd::string, EditorWidgetAction*> m_widgetActions;
    };

} // namespace AzToolsFramework
