/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/EditorActionContext.h>
#include <AzToolsFramework/ActionManager/Action/EditorAction.h>

namespace AzToolsFramework
{
    EditorActionContext::EditorActionContext(
        AZStd::string identifier, AZStd::string name)
        : m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
    {
    }

    bool EditorActionContext::HasMode(const AZStd::string& modeIdentifier) const
    {
        return m_modeIdentifiers.contains(modeIdentifier);
    }

    void EditorActionContext::AddMode(AZStd::string modeIdentifier)
    {
        m_modeIdentifiers.emplace(AZStd::move(modeIdentifier));
    }

    AZStd::string EditorActionContext::GetActiveMode() const
    {
        return m_activeModeIdentifier;
    }

    bool EditorActionContext::SetActiveMode(AZStd::string modeIdentifier)
    {
        bool modeChanged = (modeIdentifier != m_activeModeIdentifier);
        m_activeModeIdentifier = AZStd::move(modeIdentifier);

        return modeChanged;
    }

    void EditorActionContext::AddAction(EditorAction* editorAction)
    {
        m_actionIdentifiers.emplace(AZStd::move(editorAction->GetActionIdentifier()));
        m_actions.append(editorAction->GetAction());
    }

    void EditorActionContext::IterateActionIdentifiers(const AZStd::function<bool(const AZStd::string&)>& callback) const
    {
        for (const AZStd::string& actionIdentifier : m_actionIdentifiers)
        {
            if(!callback(actionIdentifier))
            {
                break;
            }
        }
    }

    const QList<QAction*>& EditorActionContext::GetActions()
    {
        return m_actions;
    }

} // namespace AzToolsFramework
