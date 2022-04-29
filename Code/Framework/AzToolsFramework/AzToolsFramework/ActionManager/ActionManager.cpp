/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ActionManager.h>

namespace AzToolsFramework
{
    ActionManager::ActionManager()
    {
        AZ::Interface<ActionManagerInterface>::Register(this);
    }

    ActionManager::~ActionManager()
    {
        AZ::Interface<ActionManagerInterface>::Unregister(this);
    }

    void ActionManager::RegisterActionContext(
        [[maybe_unused]] QWidget* parentWidget,
        [[maybe_unused]] AZStd::string_view identifier,
        [[maybe_unused]] AZStd::string_view name,
        [[maybe_unused]] AZStd::string_view parentIdentifier)
    {
        EditorActionContext* actionContext = new EditorActionContext();

        // TODO - see if there's a better way to handle this stuff.
        actionContext->m_identifier = identifier;
        actionContext->m_name = name;
        actionContext->m_parentIdentifier = parentIdentifier;
        actionContext->m_parentWidget = parentWidget;

        m_actionContexts.insert({ identifier.data(), actionContext });
    }

    void ActionManager::RegisterAction(
        [[maybe_unused]] AZStd::string_view contextIdentifier,
        AZStd::string_view identifier,
        AZStd::string_view name,
        AZStd::string_view description,
        AZStd::string_view category,
        [[maybe_unused]] AZStd::string_view iconPath,
        AZStd::function<void()> handler)
    {
        if (!m_actionContexts.contains(contextIdentifier))
        {
            // TODO - Error?
            return;
        }

        // TODO - Handle context
        // TODO - Handle category
        // TODO - Add iconPath
        EditorAction action;

        // TODO - see if there's a better way to handle this stuff.
        action.m_identifier = identifier;
        action.m_name = name;
        action.m_description = description;
        action.m_category = category;
        action.m_action = new QAction(name.data(), nullptr);

        QObject::connect(
            action.m_action, &QAction::triggered, m_actionContexts[contextIdentifier]->m_parentWidget,
            [handler]()
            {
                handler();
            }
        );

        m_actions.insert({ identifier.data(), AZStd::move(action) });
    }

    
    QAction* ActionManager::GetAction(AZStd::string_view actionIdentifier)
    {
        if (m_actions.contains(actionIdentifier))
        {
            // TODO - avoid reaching into the EditorAction and actually implement a getter?
            return m_actions[actionIdentifier].m_action;
        }

        return nullptr;
    }

} // namespace AzToolsFramework
