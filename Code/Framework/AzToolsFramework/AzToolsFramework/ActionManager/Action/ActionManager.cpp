/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>

namespace AzToolsFramework
{
    ActionManager::ActionManager()
    {
        AZ::Interface<ActionManagerInterface>::Register(this);
    }

    ActionManager::~ActionManager()
    {
        AZ::Interface<ActionManagerInterface>::Unregister(this);

        ClearActionContextMap();
    }

    ActionManagerOperationResult ActionManager::RegisterActionContext(
        QWidget* widget,
        const AZStd::string& identifier,
        const AZStd::string& name,
        const AZStd::string& parentIdentifier)
    {
        if (!widget)
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action context \"%s\" to a null widget.", identifier.c_str())
            );
        }

        m_actionContexts.insert(
            {
                identifier,
                new EditorActionContext(identifier, name, parentIdentifier, widget)
            }
        );

        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::RegisterAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& identifier,
        const AZStd::string& name,
        const AZStd::string& description,
        const AZStd::string& category,
        [[maybe_unused]] const AZStd::string& iconPath,
        AZStd::function<void()> handler)
    {
        if (!m_actionContexts.contains(contextIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%s\" - context \"%s\" has not been registered.",
                identifier.c_str(), 
                contextIdentifier.c_str()
            ));
        }

        if (m_actions.contains(identifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%.s\" twice.", identifier.c_str()));
        }

        m_actions.insert(
            {
                identifier,
                EditorAction(m_actionContexts[contextIdentifier]->GetWidget(), identifier, name, description, category, handler)
            }
        );
        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::TriggerAction(const AZStd::string& actionIdentifier)
    {
        if (!m_actions.contains(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not trigger action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        m_actions[actionIdentifier].GetAction()->trigger();
        return AZ::Success();
    }

    QAction* ActionManager::GetAction(const AZStd::string& actionIdentifier)
    {
        if (!m_actions.contains(actionIdentifier))
        {
            return nullptr;
        }

        return m_actions[actionIdentifier].GetAction();
    }

    const QAction* ActionManager::GetActionConst(const AZStd::string& actionIdentifier)
    {
        if (!m_actions.contains(actionIdentifier))
        {
            return nullptr;
        }

        return m_actions[actionIdentifier].GetAction();
    }

    void ActionManager::ClearActionContextMap()
    {
        for (auto elem : m_actionContexts)
        {
            delete elem.second;
        }
    }

} // namespace AzToolsFramework
