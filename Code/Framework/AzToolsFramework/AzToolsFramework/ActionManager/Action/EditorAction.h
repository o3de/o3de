/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#endif

#include <QIcon>

class QAction;

namespace AzToolsFramework
{
    //! Editor Action class definitions.
    //! Wraps a QAction and provides additional metadata.
    class EditorAction
        : public QObject
    {
        Q_OBJECT

    public:
        EditorAction() = default;
        EditorAction(
            AZStd::string contextIdentifier,
            AZStd::string identifier,
            AZStd::string name,
            AZStd::string description,
            AZStd::string category,
            AZStd::string iconPath,
            ActionVisibility menuVisibility,
            ActionVisibility toolBarVisibility,
            AZStd::function<void()> handler,
            AZStd::function<bool()> checkStateCallback = nullptr
        );
        virtual ~EditorAction();

        static void Initialize();

        const AZStd::string& GetActionIdentifier() const;
        const AZStd::string& GetActionContextIdentifier() const;

        const AZStd::string& GetName() const;
        void SetName(AZStd::string name);
        const AZStd::string& GetDescription() const;
        void SetDescription(AZStd::string description);
        const AZStd::string& GetCategory() const;
        void SetCategory(AZStd::string category);
        const AZStd::string& GetIconPath() const;
        void SetIconPath(AZStd::string iconPath);
        AZStd::string GetHotKey() const;
        void SetHotKey(const AZStd::string& hotKey);
        ActionVisibility GetMenuVisibility() const;
        ActionVisibility GenerateToolBarVisibility() const;

        //! Returns the pointer to the action.
        QAction* GetAction();
        const QAction* GetAction() const;

        //! Adds an enabled state callback to the action.
        void AddEnabledStateCallback(AZStd::function<bool()> enabledStateCallback);

        //! Adds a mode to the list of modes this action is enabled in.
        void AssignToMode(AZStd::string modeIdentifier);

        //! Returns true if the EditorAction has one or more enabled state callbacks set, false otherwise.
        bool HasEnabledStateCallbacks() const;

        //! Returns true if the EditorAction is enabled, false otherwise.
        bool IsEnabled() const;

        //! Returns whether the action is checkable.
        bool IsCheckable();

        //! Updates the action's checked and enabled state via the appropriate callbacks, if any.
        void Update();

        //! Returns whether the Action is active.
        bool IsActiveInCurrentMode() const;

        //! Trigger the Action's behavior.
        void Trigger();

    private:
        void UpdateIconFromPath();
        void UpdateTooltipText();


        QAction* m_action = nullptr;
        QIcon m_icon;

        AZStd::string m_contextIdentifier;
        AZStd::string m_identifier;
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_category;
        AZStd::string m_iconPath;

        AZStd::function<void()> m_triggerBehavior = nullptr;
        AZStd::function<bool()> m_checkStateCallback = nullptr;
        AZStd::vector<AZStd::function<bool()>> m_enabledStateCallbacks;

        ActionVisibility m_menuVisibility;
        ActionVisibility m_toolBarVisibility;

        // If the modes vector is empty, the action will be enabled in all modes.
        AZStd::unordered_set<AZStd::string> m_modes;

        inline static ActionManagerInterface* s_actionManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
