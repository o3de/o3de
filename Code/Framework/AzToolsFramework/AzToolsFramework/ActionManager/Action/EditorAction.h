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

#include <QAction>
#include <QIcon>

namespace AzToolsFramework
{
    class EditorAction
    {
    public:
        EditorAction() = default;
        EditorAction(
            QWidget* parentWidget,
            AZStd::string identifier,
            AZStd::string name,
            AZStd::string description,
            AZStd::string category,
            AZStd::string iconPath,
            bool hideFromMenusWhenDisabled,
            bool hideFromToolBarsWhenDisabled,
            AZStd::function<void()> handler,
            AZStd::function<bool()> checkStateCallback = nullptr
        );

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
        bool GetHideFromMenusWhenDisabled() const;
        bool GetHideFromToolBarsWhenDisabled() const;

        //! Returns the pointer to the action.
        QAction* GetAction();
        const QAction* GetAction() const;

        //! Sets the enabled state callback for the action.
        void SetEnabledStateCallback(AZStd::function<bool()> enabledStateCallback);

        //! Returns true if the EditorAction has an enabled state callback set, false otherwise.
        bool HasEnabledStateCallback() const;

        //! Returns true if the EditorAction is enabled, false otherwise.
        bool IsEnabled() const;

        //! Returns whether the action is checkable.
        bool IsCheckable();

        //! Calls the callback to update the action's checked and enabled state, if any.
        void Update();

    private:
        void UpdateIconFromPath();
        void UpdateTooltipText();

        QAction* m_action = nullptr;
        QIcon m_icon;

        AZStd::string m_identifier;
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_category;
        AZStd::string m_iconPath;

        AZStd::function<bool()> m_checkStateCallback = nullptr;
        AZStd::function<bool()> m_enabledStateCallback = nullptr;

        bool m_hideFromMenusWhenDisabled;
        bool m_hideFromToolBarsWhenDisabled;
    };

} // namespace AzToolsFramework
