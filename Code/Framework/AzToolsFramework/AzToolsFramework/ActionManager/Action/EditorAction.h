/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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

        //! Returns the pointer to the action.
        QAction* GetAction();

        //! Calls the callback to update the action's checked state, if any.
        void Update();

        //! Returns whether the action is checkable.
        bool IsCheckable();

    private:
        void UpdateIconFromPath();

        QAction* m_action = nullptr;
        QIcon m_icon;

        AZStd::string m_identifier;
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_category;
        AZStd::string m_iconPath;

        AZStd::function<bool()> m_checkStateCallback = nullptr;

        AZStd::string m_parentIdentifier;
    };

} // namespace AzToolsFramework
