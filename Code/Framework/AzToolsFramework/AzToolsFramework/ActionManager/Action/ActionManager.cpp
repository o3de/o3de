/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>

#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

namespace AzToolsFramework
{
    ActionManager::ActionManager()
    {
        AZ::Interface<ActionManagerInterface>::Register(this);
        AZ::Interface<ActionManagerInternalInterface>::Register(this);
    }

    ActionManager::~ActionManager()
    {
        AZ::Interface<ActionManagerInternalInterface>::Unregister(this);
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
                    properties.m_iconPath,
                    properties.m_hideFromMenusWhenDisabled,
                    properties.m_hideFromToolBarsWhenDisabled,
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
        AZStd::function<bool()> checkStateCallback)
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
                    properties.m_iconPath,
                    properties.m_hideFromMenusWhenDisabled,
                    properties.m_hideFromToolBarsWhenDisabled,
                    handler,
                    checkStateCallback
                )
            }
        );

        return AZ::Success();
    }
    
    ActionManagerGetterResult ActionManager::GetActionName(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not get name of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        return AZ::Success(actionIterator->second.GetName());
    }

    ActionManagerOperationResult ActionManager::SetActionName(const AZStd::string& actionIdentifier, const AZStd::string& name)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not set name of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        actionIterator->second.SetName(name);
        return AZ::Success();
    }

    ActionManagerGetterResult ActionManager::GetActionDescription(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not get description of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        return AZ::Success(actionIterator->second.GetDescription());
    }

    ActionManagerOperationResult ActionManager::SetActionDescription(const AZStd::string& actionIdentifier, const AZStd::string& description)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not set description of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        actionIterator->second.SetDescription(description);
        return AZ::Success();
    }

    ActionManagerGetterResult ActionManager::GetActionCategory(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not get name of category \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        return AZ::Success(actionIterator->second.GetCategory());
    }

    ActionManagerOperationResult ActionManager::SetActionCategory(const AZStd::string& actionIdentifier, const AZStd::string& category)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not set name of category \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        actionIterator->second.SetCategory(category);
        return AZ::Success();
    }

    ActionManagerGetterResult ActionManager::GetActionIconPath(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not get icon path of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        return AZ::Success(actionIterator->second.GetIconPath());
    }

    ActionManagerOperationResult ActionManager::SetActionIconPath(const AZStd::string& actionIdentifier, const AZStd::string& iconPath)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not set icon path of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }
        
        actionIterator->second.SetIconPath(iconPath);
        return AZ::Success();
    }

    ActionManagerBooleanResult ActionManager::IsActionEnabled(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not retrieve enabled state of action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        return AZ::Success(actionIterator->second.IsEnabled());
    }

    ActionManagerOperationResult ActionManager::TriggerAction(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not trigger action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        actionIterator->second.GetAction()->trigger();
        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::InstallEnabledStateCallback(
        const AZStd::string& actionIdentifier, AZStd::function<bool()> enabledStateCallback)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not install enabled state callback on action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }

        if (actionIterator->second.HasEnabledStateCallback())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not install enabled state callback on action \"%s\" - action already has an enabled state callback installed.",
                actionIdentifier.c_str()));
        }

        actionIterator->second.SetEnabledStateCallback(AZStd::move(enabledStateCallback));
        return AZ::Success();
    }
    
    ActionManagerOperationResult ActionManager::UpdateAction(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not update action \"%s\" as no action with that identifier was registered.",
                actionIdentifier.c_str()));
        }
        
        actionIterator->second.Update();

        return AZ::Success();
    }
    
    ActionManagerOperationResult ActionManager::RegisterActionUpdater(const AZStd::string& actionUpdaterIdentifier)
    {
        if (m_actionUpdaters.contains(actionUpdaterIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not register action updater \"%s\" twice.",
                actionUpdaterIdentifier.c_str()
            ));
        }

        m_actionUpdaters.insert({ actionUpdaterIdentifier, {} });
        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::AddActionToUpdater(const AZStd::string& actionUpdaterIdentifier, const AZStd::string& actionIdentifier)
    {
        auto actionUpdaterIterator = m_actionUpdaters.find(actionUpdaterIdentifier);
        if (actionUpdaterIterator == m_actionUpdaters.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not add action \"%s\" to action updater \"%s\" - action updater has not been registered.",
                actionIdentifier.c_str(),
                actionUpdaterIdentifier.c_str()
            ));
        }
        
        if (!m_actions.contains(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to action updater \"%s\" - action could not be found.",
                actionIdentifier.c_str(),
                actionUpdaterIdentifier.c_str()
            ));
        }

        if (actionUpdaterIterator->second.contains(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not add action \"%s\" to action updater \"%s\" twice.",
                actionIdentifier.c_str(),
                actionUpdaterIdentifier.c_str()
            ));
        }

        actionUpdaterIterator->second.insert(actionIdentifier);
        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::TriggerActionUpdater(const AZStd::string& actionUpdaterIdentifier)
    {
        auto actionUpdaterIterator = m_actionUpdaters.find(actionUpdaterIdentifier);
        if (actionUpdaterIterator == m_actionUpdaters.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not trigger updates for action updater \"%s\" - action updater has not been registered.",
                actionUpdaterIdentifier.c_str()
            ));
        }

        for (const AZStd::string& actionIdentifier : actionUpdaterIterator->second)
        {
            UpdateAction(actionIdentifier);
        }

        return AZ::Success();
    }

    void ActionManager::ClearActionContextMap()
    {
        for (auto elem : m_actionContexts)
        {
            delete elem.second;
        }
    }

    QAction* ActionManager::GetAction(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return nullptr;
        }

        return actionIterator->second.GetAction();
    }

    const QAction* ActionManager::GetActionConst(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return nullptr;
        }

        return actionIterator->second.GetAction();
    }

    bool ActionManager::GetHideFromMenusWhenDisabled(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            // Return the default value.
            return true;
        }

        return actionIterator->second.GetHideFromMenusWhenDisabled();

    }

    bool ActionManager::GetHideFromToolBarsWhenDisabled(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            // Return the default value.
            return false;
        }

        return actionIterator->second.GetHideFromToolBarsWhenDisabled();
    }

} // namespace AzToolsFramework
