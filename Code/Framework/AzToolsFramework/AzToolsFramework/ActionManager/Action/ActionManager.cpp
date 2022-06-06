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
        const AZStd::string& parentContextIdentifier,
        const AZStd::string& contextIdentifier,
        const ActionContextProperties& properties,
        QWidget* widget)
    {
        if (!widget)
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action context \"%s\" to a null widget.", contextIdentifier.c_str())
            );
        }

        if (m_actionContexts.contains(contextIdentifier))
        {
            return AZ::Failure(AZStd::string::format("Action Manager - Could not register action context \"%.s\" twice.", contextIdentifier.c_str()));
        }

        m_actionContexts.insert(
            {
                contextIdentifier,
                new EditorActionContext(contextIdentifier, properties.m_name, parentContextIdentifier, widget)
            }
        );

        return AZ::Success();
    }

    
    ActionManagerOperationResult ActionManager::RegisterAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& actionIdentifier,
        const ActionProperties& properties,
        AZStd::function<void()> handler)
    {
        if (!m_actionContexts.contains(contextIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%s\" - context \"%s\" has not been registered.",
                actionIdentifier.c_str(), 
                contextIdentifier.c_str()
            ));
        }

        if (m_actions.contains(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%.s\" twice.",
                actionIdentifier.c_str()
            ));
        }

        m_actions.insert(
            {
                actionIdentifier,
                EditorAction(
                    m_actionContexts[contextIdentifier]->GetWidget(),
                    actionIdentifier,
                    properties.m_name,
                    properties.m_description,
                    properties.m_category,
                    handler
                )
            }
        );

        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::RegisterCheckableAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& actionIdentifier,
        const ActionProperties& properties,
        AZStd::function<void()> handler,
        AZStd::function<bool()> updateCallback)
    {
        if (!m_actionContexts.contains(contextIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%s\" - context \"%s\" has not been registered.",
                actionIdentifier.c_str(), 
                contextIdentifier.c_str()
            ));
        }

        if (m_actions.contains(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action \"%.s\" twice.",
                actionIdentifier.c_str()
            ));
        }

        m_actions.insert(
            {
                actionIdentifier,
                EditorAction(
                    m_actionContexts[contextIdentifier]->GetWidget(),
                    actionIdentifier,
                    properties.m_name,
                    properties.m_description,
                    properties.m_category,
                    handler,
                    updateCallback
                )
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
    
    ActionManagerOperationResult ActionManager::UpdateAction(const AZStd::string& actionIdentifier)
    {
        if (!m_actions.contains(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not update action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        if (!m_actions[actionIdentifier].IsCheckable())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not update action \"%s\" as it was not registered as Checkable.",
                actionIdentifier.c_str()));
        }
        
        m_actions[actionIdentifier].Update();

        return AZ::Success();
    }

    void ActionManager::ClearActionContextMap()
    {
        for (auto elem : m_actionContexts)
        {
            delete elem.second;
        }
    }

} // namespace AzToolsFramework
