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
            AZStd::function<void()> handler,
            AZStd::function<bool()> updateCallback = nullptr
        );

        // Returns the pointer to the action.
        QAction* GetAction();

        // Calls the callback to update the action's checked state, if any.
        void Update();

        // Returns whether the action is checkable.
        bool IsCheckable();

    private:
        QAction* m_action = nullptr;

        AZStd::string m_identifier;
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_category;

        AZStd::function<bool()> m_updateCallback = nullptr;

        AZStd::string m_parentIdentifier;
    };

} // namespace AzToolsFramework
