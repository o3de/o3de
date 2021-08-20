/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "KeyboardShortcutManager.h"
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Casting/numeric_cast.h>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <QtCore/QStringList>
#include <QtCore/QSettings>
#include <QtGui/QKeyEvent>

namespace MysticQt
{
    void KeyboardShortcutManager::Group::RemoveAction(QAction* qaction, bool local)
    {
        m_actions.erase(AZStd::find_if(begin(m_actions), end(m_actions), [&qaction, local](const AZStd::unique_ptr<Action>& action)
        {
            return action->m_local == local && action->m_qaction == qaction;
        }));
    }

    KeyboardShortcutManager::Action* KeyboardShortcutManager::Group::FindActionByName(const QString& actionName, bool local) const
    {
        const auto found = AZStd::find_if(begin(m_actions), end(m_actions), [&actionName, local](const AZStd::unique_ptr<Action>& action)
        {
            return action->m_local == local && action->m_qaction->text() == actionName;
        });

        return found != end(m_actions) ? found->get() : nullptr;
    }

    void KeyboardShortcutManager::RegisterKeyboardShortcut(QAction* qaction, AZStd::string_view groupName, bool local)
    {
        // find the group with the given name
        Group* group = FindGroupByName(groupName);

        // if there is no group with the given name, create it
        if (group == nullptr)
        {
            m_groups.emplace_back(AZStd::make_unique<Group>(groupName));
            group = m_groups.back().get();
        }

        // check if the action is already there to avoid adding it twice
        Action* action = group->FindActionByName(qaction->text(), local);
        if (action)
        {
            action->m_defaultKeySequence = qaction->shortcut();
            return;
        }

        // create the new action and add it to the group
        group->AddAction(AZStd::make_unique<Action>(qaction, local));

        QAction::connect(qaction, &QAction::destroyed, this, [this, groupName = AZStd::string(groupName), local](QObject* qaction)
        {
            UnregisterKeyboardShortcut(static_cast<QAction*>(qaction), groupName, local);
        });
    }

    void KeyboardShortcutManager::UnregisterKeyboardShortcut(QAction* qaction, AZStd::string_view groupName, bool local)
    {
        Group* group = FindGroupByName(groupName);

        if (!group)
        {
            return;
        }

        group->RemoveAction(qaction, local);
    }


    // find the action with the given name in the given group
    KeyboardShortcutManager::Action* KeyboardShortcutManager::FindAction(const QString& actionName, AZStd::string_view groupName) const
    {
        const Group* group = FindGroupByName(groupName);
        if (!group)
        {
            return nullptr;
        }

        Action* action = group->FindActionByName(actionName, false);
        if (action)
        {
            return action;
        }
        return group->FindActionByName(actionName, true);
    }


    // find a group by name
    KeyboardShortcutManager::Group* KeyboardShortcutManager::FindGroupByName(AZStd::string_view groupName) const
    {
        const auto found = AZStd::find_if(begin(m_groups), end(m_groups), [&groupName](const AZStd::unique_ptr<Group>& group)
        {
            return group->GetName() == groupName;
        });
        return found != end(m_groups) ? found->get() : nullptr;
    }


    // find the correspondng group for the given action
    KeyboardShortcutManager::Group* KeyboardShortcutManager::FindGroupForShortcut(Action* action) const
    {
        const auto foundGroup = AZStd::find_if(begin(m_groups), end(m_groups), [action](const AZStd::unique_ptr<Group>& group)
        {
            const auto foundAction = AZStd::find_if(begin(group->GetActions()), end(group->GetActions()), [action](const AZStd::unique_ptr<Action>& actionInGroup)
            {
                return action == actionInGroup.get();
            });
            return foundAction != end(group->GetActions()) ? foundAction->get() : nullptr;
        });
        return foundGroup != end(m_groups) ? foundGroup->get() : nullptr;
    }


    KeyboardShortcutManager::Action* KeyboardShortcutManager::FindShortcut(QKeySequence keySequence, Group* group) const
    {
        const auto findMatchingAction = [keySequence] (const Group* group, const bool local)
        {
            return AZStd::find_if(begin(group->GetActions()), end(group->GetActions()), [keySequence, local] (const AZStd::unique_ptr<Action>& action)
            {
                if (action->m_local != local)
                {
                    return false;
                }

                return action->m_qaction->shortcut().matches(keySequence) == QKeySequence::ExactMatch;
            });
        };

        // first check the global shortcuts
        const auto globalAction = findMatchingAction(group, false);
        if (globalAction != end(group->GetActions()))
        {
            return globalAction->get();
        }

        const auto localAction = findMatchingAction(group, true);
        if (localAction != end(group->GetActions()))
        {
            return localAction->get();
        }

        return nullptr;
    }


    void KeyboardShortcutManager::Save(QSettings* settings)
    {
        // clear the settings before saving new data
        settings->clear();

        // iterate through the groups and save all actions for them
        for (const AZStd::unique_ptr<Group>& group : m_groups)
        {
            settings->beginGroup(QString::fromUtf8(group->GetName().data(), aznumeric_caster(group->GetName().size())));

            // iterate through the actions and save them
            for (const AZStd::unique_ptr<Action>& action : group->GetActions())
            {
                settings->beginGroup(action->m_qaction->text());
                settings->setValue("Key", action->m_qaction->shortcut());
                settings->setValue("Local", action->m_local);
                settings->endGroup();
            }

            settings->endGroup();
        }
    }


    void KeyboardShortcutManager::Load(QSettings* settings)
    {
        // iterate through the groups and load all actions
        const QStringList groupNames = settings->childGroups();
        for (const QString& groupName : groupNames)
        {
            Group* group = FindGroupByName(FromQtString(groupName));
            if (!group)
            {
                continue;
            }

            settings->beginGroup(groupName);
            const QStringList actionNames = settings->childGroups();

            // iterate through the actions and save them
            for (const QString& actionName : actionNames)
            {
                settings->beginGroup(actionName);
                const bool local = settings->value("Local", false).toBool();

                Action* action = group->FindActionByName(actionName, local);
                if (!action)
                {
                    continue;
                }

                const QVariant keyValue = settings->value("Key", "");
                if (keyValue.canConvert<QKeySequence>())
                {
                    const QKeySequence key = keyValue.value<QKeySequence>();
                    action->m_qaction->setShortcut(key);
                }
                else if (keyValue.canConvert<int>())
                {
                    const int key = keyValue.value<int>();
                    const bool ctrlModifier = settings->value("Ctrl", false).value<bool>();
                    const bool altModifier = settings->value("Alt", false).value<bool>();
                    action->m_qaction->setShortcut(key | (ctrlModifier ? Qt::ControlModifier : 0) | (altModifier ? Qt::AltModifier : 0));
                }

                settings->endGroup();
            }

            settings->endGroup();
        }
    }
} // namespace MysticQt
