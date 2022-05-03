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
        AZStd::string_view identifier,
        AZStd::string_view name,
        AZStd::string_view parentIdentifier)
    {
        if (!widget)
        {
            return AZ::Failure(
                AZStd::string::format("Action Manager - Could not register action context \"%s\" to a null widget.", identifier.data())
            );
        }

        m_actionContexts.insert(
            {
                identifier.data(),
                new EditorActionContext(identifier, name, parentIdentifier, widget)
            }
        );

        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::RegisterAction(
        AZStd::string_view contextIdentifier,
        AZStd::string_view identifier,
        AZStd::string_view name,
        AZStd::string_view description,
        AZStd::string_view category,
        [[maybe_unused]] AZStd::string_view iconPath,
        AZStd::function<void()> handler)
    {
        if (!m_actionContexts.contains(contextIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%s\" - context \"%s\" has not been registered.",
                identifier.data(),
                contextIdentifier.data()
            ));
        }

        m_actions.insert(
            {
                identifier,
                EditorAction(m_actionContexts[contextIdentifier]->GetWidget(), identifier, name, description, category, handler)
            }
        );
        return AZ::Success();
    }

    QAction* ActionManager::GetAction(AZStd::string_view actionIdentifier)
    {
        if (m_actions.contains(actionIdentifier))
        {
            return m_actions[actionIdentifier].GetAction();
        }

        return nullptr;
    }

    void ActionManager::ClearActionContextMap()
    {
        for (auto elem : m_actionContexts)
        {
            delete elem.second;
        }
    }

} // namespace AzToolsFramework
