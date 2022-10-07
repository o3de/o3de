/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/EditorActionContext.h>

namespace AzToolsFramework
{
    EditorActionContext::EditorActionContext(
        AZStd::string identifier, AZStd::string name, AZStd::string parentIdentifier, QWidget* widget)
        : m_identifier(AZStd::move(identifier))
        , m_name(AZStd::move(name))
        , m_parentIdentifier(AZStd::move(parentIdentifier))
        , m_widget(widget)
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

    void EditorActionContext::AddAction(AZStd::string actionIdentifier)
    {
        m_actionIdentifiers.emplace(AZStd::move(actionIdentifier));
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

    QWidget* EditorActionContext::GetWidget()
    {
        return m_widget;
    }

} // namespace AzToolsFramework
