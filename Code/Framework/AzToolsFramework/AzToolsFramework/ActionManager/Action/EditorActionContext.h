/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_base.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>

#include <QAction>
#include <QList>
#include <QObject>

namespace AzToolsFramework
{
    constexpr const char* DefaultModeIdentifier = "default";

    class EditorAction;

    //! Editor Action Context class definition.
    //! Identifies a collection of Actions and their accessibility in the context of the whole O3DE Editor.
    class EditorActionContext
    {
    public:
        EditorActionContext() = default;
        EditorActionContext(AZStd::string identifier, AZStd::string name);

        bool HasMode(const AZStd::string& modeIdentifier) const;
        void AddMode(AZStd::string modeIdentifier);

        AZStd::string GetActiveMode() const;
        bool SetActiveMode(AZStd::string modeIdentifier);

        void AddAction(EditorAction* editorAction);
        void IterateActionIdentifiers(const AZStd::function<bool(const AZStd::string&)>& callback) const;

        const QList<QAction*>& GetActions();

    private:
        AZStd::string m_identifier;
        AZStd::string m_name;

        AZStd::string m_activeModeIdentifier = DefaultModeIdentifier;
        AZStd::unordered_set<AZStd::string> m_modeIdentifiers = { DefaultModeIdentifier };

        AZStd::unordered_set<AZStd::string> m_actionIdentifiers;

        QList<QAction*> m_actions;
    };

} // namespace AzToolsFramework
