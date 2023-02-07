/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <QtGui/private/qguiapplication_p.h>
#include <QShortcutEvent>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

namespace AzToolsFramework
{
    //! This class is used to install an event filter on each widget that the action contexts are registered to in order to properly handle
    //! ambiguous shorcuts.
    class ActionContextWidgetWatcher : public QObject
    {
    public:
        explicit ActionContextWidgetWatcher(QObject* parent)
            : QObject(parent)
        {
        }

        static bool TriggerActiveActionsWithShortcut(const QList<QAction*>& actions, const QKeySequence& shortcutKeySequence)
        {
            // Note: Triggering an action may change the enabled state of other actions.
            // As such, we first collect the actions that should be triggered, then trigger them in sequence.

            // Collect all actions that are enabled and match with the shortcut.
            QList<QAction*> matchingActions;
            for (QAction* action : actions)
            {
                if (action->isEnabled() && action->shortcut() == shortcutKeySequence)
                {
                    matchingActions.append(action);
                }
            }

            // Trigger all matching actions.
            for (QAction* action : matchingActions)
            {
                action->trigger();
            }

            // Return whether any action was triggered.
            return !matchingActions.isEmpty();
        }

        bool eventFilter(QObject* watched, QEvent* event) override
        {
            switch (event->type())
            {
            case QEvent::ShortcutOverride:
            {
                // QActions default "autoRepeat" to true, which is not an ideal user experience.
                // We globally disable that behavior here - in the unlikely event a shortcut needs to
                // replicate it, its owner can instead implement a keyEvent handler
                if (static_cast<QKeyEvent*>(event)->isAutoRepeat())
                {
                    event->accept();
                    return true;
                }

                auto keyEvent = static_cast<QKeyEvent*>(event);
                int keyCode = keyEvent->key();
                Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
                if (modifiers & Qt::ShiftModifier)
                {
                    keyCode += Qt::SHIFT;
                }
                if (modifiers & Qt::ControlModifier)
                {
                    keyCode += Qt::CTRL;
                }
                if (modifiers & Qt::AltModifier)
                {
                    keyCode += Qt::ALT;
                }
                if (modifiers & Qt::MetaModifier)
                {
                    keyCode += Qt::META;
                }

                QKeySequence keySequence(keyCode);

                auto globalShortcutMap = &QGuiApplicationPrivate::instance()->shortcutMap;
                bool isAmbiguous = globalShortcutMap->hasShortcutForKeySequence(keySequence);
                if (isAmbiguous)
                {
                    QWidget* watchedWidget = qobject_cast<QWidget*>(watched);

                    if (TriggerActiveActionsWithShortcut(watchedWidget->actions(), keySequence))
                    {
                        // We need to accept the event in addition to return true on this event filter
                        // to ensure the event doesn't get propagated to any parent widgets.
                        event->accept();
                        return true;
                    }
                }
                break;
            }
            case QEvent::Shortcut:
            {
                auto shortcutEvent = static_cast<QShortcutEvent*>(event);
                if (shortcutEvent->isAmbiguous())
                {
                    QWidget* watchedWidget = qobject_cast<QWidget*>(watched);

                    if (TriggerActiveActionsWithShortcut(watchedWidget->actions(), shortcutEvent->key()))
                    {
                        return true;
                    }
                }
                break;
            }
            default:
                break;
            }
            return false;
        }
    };

    ActionManager::ActionManager()
    {
        AZ::Interface<ActionManagerInterface>::Register(this);
        AZ::Interface<ActionManagerInternalInterface>::Register(this);

        EditorAction::Initialize();
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

        widget->installEventFilter(new ActionContextWidgetWatcher(widget));

        return AZ::Success();
    }

    bool ActionManager::IsActionContextRegistered(const AZStd::string& contextIdentifier) const
    {
        return m_actionContexts.contains(contextIdentifier);
    }

    ActionManagerOperationResult ActionManager::RegisterAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& actionIdentifier,
        const ActionProperties& properties,
        AZStd::function<void()> handler)
    {
        auto actionContextIterator = m_actionContexts.find(contextIdentifier);
        if (actionContextIterator == m_actionContexts.end())
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
                    actionContextIterator->second->GetWidget(),
                    actionContextIterator->first,
                    actionIdentifier,
                    properties.m_name,
                    properties.m_description,
                    properties.m_category,
                    properties.m_iconPath,
                    properties.m_menuVisibility,
                    properties.m_toolBarVisibility,
                    handler
                )
            }
        );
        actionContextIterator->second->AddAction(actionIdentifier);

        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::RegisterCheckableAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& actionIdentifier,
        const ActionProperties& properties,
        AZStd::function<void()> handler,
        AZStd::function<bool()> checkStateCallback)
    {
        auto actionContextIterator = m_actionContexts.find(contextIdentifier);
        if (actionContextIterator == m_actionContexts.end())
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
                    actionContextIterator->second->GetWidget(),
                    actionContextIterator->first,
                    actionIdentifier,
                    properties.m_name,
                    properties.m_description,
                    properties.m_category,
                    properties.m_iconPath,
                    properties.m_menuVisibility,
                    properties.m_toolBarVisibility,
                    handler,
                    checkStateCallback
                )
            }
        );
        actionContextIterator->second->AddAction(actionIdentifier);

        return AZ::Success();
    }

    bool ActionManager::IsActionRegistered(const AZStd::string& actionIdentifier) const
    {
        return m_actions.contains(actionIdentifier);
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

        actionIterator->second.AddEnabledStateCallback(AZStd::move(enabledStateCallback));
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

    ActionManagerOperationResult ActionManager::RegisterWidgetAction(
        const AZStd::string& widgetActionIdentifier, const WidgetActionProperties& properties, AZStd::function<QWidget*()> generator)
    {
        if (m_widgetActions.contains(widgetActionIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("Action Manager - Could not register widget action \"%.s\" twice.", widgetActionIdentifier.c_str()));
        }

        m_widgetActions.insert(
            {
                widgetActionIdentifier,
                EditorWidgetAction(
                  widgetActionIdentifier,
                  properties.m_name,
                  properties.m_category,
                  AZStd::move(generator)
                )
            }
        );

        return AZ::Success();
    }

    bool ActionManager::IsWidgetActionRegistered(const AZStd::string& widgetActionIdentifier) const
    {
        return m_widgetActions.contains(widgetActionIdentifier);
    }

    ActionManagerGetterResult ActionManager::GetWidgetActionName(const AZStd::string& widgetActionIdentifier)
    {
        auto widgetActionIterator = m_widgetActions.find(widgetActionIdentifier);
        if (widgetActionIterator == m_widgetActions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not get name of widget action \"%s\" as no widget action with that identifier was registered.",
                widgetActionIdentifier.c_str()));
        }

        return AZ::Success(widgetActionIterator->second.GetName());
    }

    ActionManagerOperationResult ActionManager::SetWidgetActionName(
        const AZStd::string& widgetActionIdentifier, const AZStd::string& name)
    {
        auto widgetActionIterator = m_widgetActions.find(widgetActionIdentifier);
        if (widgetActionIterator == m_widgetActions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not set name of widget action \"%s\" as no widget action with that identifier was registered.",
                widgetActionIdentifier.c_str()));
        }

        widgetActionIterator->second.SetName(name);
        return AZ::Success();
    }

    ActionManagerGetterResult ActionManager::GetWidgetActionCategory(const AZStd::string& widgetActionIdentifier)
    {
        auto widgetActionIterator = m_widgetActions.find(widgetActionIdentifier);
        if (widgetActionIterator == m_widgetActions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not get category of widget action \"%s\" as no widget action with that identifier was registered.",
                widgetActionIdentifier.c_str()));
        }

        return AZ::Success(widgetActionIterator->second.GetCategory());
    }

    ActionManagerOperationResult ActionManager::SetWidgetActionCategory(
        const AZStd::string& widgetActionIdentifier, const AZStd::string& category)
    {
        auto widgetActionIterator = m_widgetActions.find(widgetActionIdentifier);
        if (widgetActionIterator == m_widgetActions.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not set category of widget action \"%s\" as no widget action with that identifier was registered.",
                widgetActionIdentifier.c_str())
            );
        }

        widgetActionIterator->second.SetCategory(category);
        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::RegisterActionContextMode(
        const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier)
    {
        auto actionContextIterator = m_actionContexts.find(actionContextIdentifier);
        if (actionContextIterator == m_actionContexts.end())
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not register mode \"%s\" for action context \"%s\" as this context has not been registered.",
                    modeIdentifier.c_str(),
                    actionContextIdentifier.c_str()
                )
            );
        }

        if (actionContextIterator->second->HasMode(modeIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not register mode \"%s\" for action context \"%s\" - mode with the same identifier already exists.",
                    modeIdentifier.c_str(),
                    actionContextIdentifier.c_str()
                )
            );
        }

        actionContextIterator->second->AddMode(modeIdentifier);
        return AZ::Success();
    }

    ActionManagerOperationResult ActionManager::AssignModeToAction(
        const AZStd::string& modeIdentifier, const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not set mode \"%s\" to action \"%s\" as no action with that identifier was registered.",
                    modeIdentifier.c_str(),
                    actionIdentifier.c_str()
                )
            );
        }

        auto actionContextIterator = m_actionContexts.find(actionIterator->second.GetActionContextIdentifier());
        if (!actionContextIterator->second->HasMode(modeIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not set mode \"%s\" to action \"%s\" as no mode with that identifier was registered.",
                    modeIdentifier.c_str(),
                    actionIdentifier.c_str()
                )
            );
        }

        actionIterator->second.AssignToMode(modeIdentifier);
        return AZ::Success();
    }

    ActionManagerBooleanResult ActionManager::IsActionActiveInCurrentMode(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not retrieve whether action \"%s\" is active in current mode as no action with that identifier was registered.",
                    actionIdentifier.c_str()
                )
            );
        }

        return AZ::Success(actionIterator->second.IsActiveInCurrentMode());
    }

    ActionManagerOperationResult ActionManager::SetActiveActionContextMode(
        const AZStd::string& actionContextIdentifier, const AZStd::string& modeIdentifier)
    {
        auto actionContextIterator = m_actionContexts.find(actionContextIdentifier);
        if (actionContextIterator == m_actionContexts.end())
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not set active mode for action context \"%s\" to \"%s\" as the action context has not been registered.",
                    actionContextIdentifier.c_str(),
                    modeIdentifier.c_str()
                )
            );
        }

        if (!actionContextIterator->second->HasMode(modeIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format(
                    "Action Manager - Could not set active mode for action context \"%s\" to \"%s\" as the mode has not been registered.",
                    actionContextIdentifier.c_str(),
                    modeIdentifier.c_str()
                )
            );
        }

        if (actionContextIterator->second->SetActiveMode(modeIdentifier))
        {
            UpdateAllActionsInActionContext(actionContextIdentifier);
        }

        return AZ::Success();
    }

    ActionManagerGetterResult ActionManager::GetActiveActionContextMode(const AZStd::string& actionContextIdentifier) const
    {
        auto actionContextIterator = m_actionContexts.find(actionContextIdentifier);
        if (actionContextIterator == m_actionContexts.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Action Manager - Could not retrieve active mode for action context \"%s\" as it has not been registered.",
                actionContextIdentifier.c_str()));
        }

        return AZ::Success(AZStd::move(actionContextIterator->second->GetActiveMode()));
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

    EditorAction* ActionManager::GetEditorAction(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return nullptr;
        }

        return &actionIterator->second;
    }

    const EditorAction* ActionManager::GetEditorActionConst(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            return nullptr;
        }

        return &actionIterator->second;
    }

    ActionVisibility ActionManager::GetActionMenuVisibility(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            // Return the default value.
            return ActionVisibility::HideWhenDisabled;
        }

        return actionIterator->second.GetMenuVisibility();
    }

    ActionVisibility ActionManager::GetActionToolBarVisibility(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actions.find(actionIdentifier);
        if (actionIterator == m_actions.end())
        {
            // Return the default value.
            return ActionVisibility::OnlyInActiveMode;
        }

        return actionIterator->second.GetToolBarVisibility();
    }

    QWidget* ActionManager::GenerateWidgetFromWidgetAction(const AZStd::string& widgetActionIdentifier)
    {
        auto widgetActionIterator = m_widgetActions.find(widgetActionIdentifier);
        if (widgetActionIterator == m_widgetActions.end())
        {
            return nullptr;
        }

        return widgetActionIterator->second.GenerateWidget();
    }

    void ActionManager::UpdateAllActionsInActionContext(const AZStd::string& actionContextIdentifier)
    {
        auto actionContextIterator = m_actionContexts.find(actionContextIdentifier);
        if (actionContextIterator == m_actionContexts.end())
        {
            return;
        }

        actionContextIterator->second->IterateActionIdentifiers(
            [&](const AZStd::string& actionIdentifier)
            {
                UpdateAction(actionIdentifier);
                return true;
            }
        );
    }

} // namespace AzToolsFramework
