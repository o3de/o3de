/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// include required headers
#include "KeyboardShortcutManager.h"

#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <QtCore/QStringList>
#include <QtCore/QSettings>
#include <QtGui/QKeyEvent>


namespace MysticQt
{
    // find action by name
    KeyboardShortcutManager::Action* KeyboardShortcutManager::Group::FindActionByName(const char* actionName, bool local) const
    {
        const uint32 numActions = mActions.GetLength();
        for (uint32 i = 0; i < numActions; ++i)
        {
            if (mActions[i]->mLocal == local && mActions[i]->mName == actionName)
            {
                return mActions[i];
            }
        }

        return nullptr;
    }

    // constructor
    KeyboardShortcutManager::KeyboardShortcutManager()
    {
    }


    // destructor
    KeyboardShortcutManager::~KeyboardShortcutManager()
    {
        Clear();
    }


    // get rid of all groups including their actions
    void KeyboardShortcutManager::Clear()
    {
        // get rid of the groups
        const uint32 numGroups = mGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            delete mGroups[i];
        }

        mGroups.Clear();
    }


    void KeyboardShortcutManager::RegisterKeyboardShortcut(const char* actionName, const char* groupName, int defaultKey, bool defaultCtrl, bool defaultAlt, bool local)
    {
        // find the group with the given name
        Group* group = FindGroupByName(groupName);

        // if there is no group with the given name, create it
        if (group == nullptr)
        {
            group = new Group(groupName);
            mGroups.Add(group);
        }

        // check if the action is already there to avoid adding it twice
        Action* action = group->FindActionByName(actionName, local);
        if (action)
        {
            action->mDefaultKey = defaultKey;
            action->mDefaultCtrl = defaultCtrl;
            action->mDefaultAlt = defaultAlt;
            return;
        }

        // create the new action and add it to the group
        action = new Action(actionName, defaultKey, defaultCtrl, defaultAlt, local);
        group->AddAction(action);
    }


    // find the action with the given name in the given group
    KeyboardShortcutManager::Action* KeyboardShortcutManager::FindAction(const char* actionName, const char* groupName)
    {
        const uint32 numGroups = mGroups.GetLength();

        // first search global shortcuts
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (mGroups[i]->GetNameString() == groupName)
            {
                Action* action = mGroups[i]->FindActionByName(actionName, false);
                if (action)
                {
                    return action;
                }
            }
        }

        // then local shortcuts
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (mGroups[i]->GetNameString() == groupName)
            {
                Action* action = mGroups[i]->FindActionByName(actionName, true);
                if (action)
                {
                    return action;
                }
            }
        }

        // failure, not found
        return nullptr;
    }


    // find a group by name
    KeyboardShortcutManager::Group* KeyboardShortcutManager::FindGroupByName(const char* groupName) const
    {
        // iterate through the groups and find the one with the given name
        const uint32 numGroups = mGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            if (mGroups[i]->GetNameString() == groupName)
            {
                return mGroups[i];
            }
        }

        // failure, a group with the given name hasn't been found
        return nullptr;
    }


    bool KeyboardShortcutManager::Check(QKeyEvent* event, const char* actionName, const char* groupName)
    {
        // find the corresponding action for the given strings
        Action* action = FindAction(actionName, groupName);
        if (action == nullptr)
        {
            //MCore::LogError("Action named '%s' in group '%s' not registered. Please register the shortcut before using it.", actionName, groupName);
            return false;
        }

        const bool ctrlPressed  = event->modifiers() & Qt::ControlModifier;
        //const bool shiftPressed   = event->modifiers() & Qt::ShiftModifier;
        const bool altPressed   = event->modifiers() & Qt::AltModifier;

        Group* group = FindGroupByName(groupName);
        Action* conflictAction = FindShortcut(event->key(), ctrlPressed, altPressed, group);

        // check if they are equal, if yes this means they match
        if (action == conflictAction)
        {
            return true;
        }

        // check if the action and the key event are the same shortcut
        /*if (event->key() == action->mKey &&
            ctrlPressed  == action->mCtrl &&
            altPressed   == action->mAlt)
            return true;*/

        return false;
    }


    // find the correspondng group for the given action
    KeyboardShortcutManager::Group* KeyboardShortcutManager::FindGroupForShortcut(Action* action)
    {
        // get the number of available groups
        const uint32 numGroups = mGroups.GetLength();

        // first check the global shortcuts
        for (uint32 i = 0; i < numGroups; ++i)
        {
            Group* group = mGroups[i];

            // iterate through the actions and save them
            const uint32 numActions = group->GetNumActions();
            for (uint32 j = 0; j < numActions; ++j)
            {
                if (group->GetAction(j) == action)
                {
                    return group;
                }
            }
        }

        // failure, not found
        return nullptr;
    }


    KeyboardShortcutManager::Action* KeyboardShortcutManager::FindShortcut(int key, bool ctrl, bool alt, Group* group)
    {
        // get the number of available groups
        const uint32 numGroups = mGroups.GetLength();

        // first check the global shortcuts
        for (uint32 i = 0; i < numGroups; ++i)
        {
            Group* currentGroup = mGroups[i];

            // iterate through the actions and save them
            const uint32 numActions = currentGroup->GetNumActions();
            for (uint32 j = 0; j < numActions; ++j)
            {
                // get the shortcut action
                KeyboardShortcutManager::Action* action = currentGroup->GetAction(j);
                if (action->mLocal)
                {
                    continue;
                }

                // check if the action and shortcut are the same
                if (key     == action->mKey &&
                    ctrl    == action->mCtrl &&
                    alt     == action->mAlt)
                {
                    return action;
                }
            }
        }

        // iterate through the actions and save them
        const uint32 numActions = group->GetNumActions();
        for (uint32 j = 0; j < numActions; ++j)
        {
            // get the shortcut action
            KeyboardShortcutManager::Action* action = group->GetAction(j);

            // check if the action and shortcut are the same
            if (key     == action->mKey &&
                ctrl    == action->mCtrl &&
                alt     == action->mAlt)
            {
                return action;
            }
        }

        // failure, shortcut not found
        return nullptr;
    }


    void KeyboardShortcutManager::Save(QSettings* settings)
    {
        // clear the settings before saving new data
        settings->clear();

        // iterate through the groups and save all actions for them
        const uint32 numGroups = mGroups.GetLength();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            Group* group = mGroups[i];
            settings->beginGroup(group->GetName());

            // iterate through the actions and save them
            const uint32 numActions = group->GetNumActions();
            for (uint32 j = 0; j < numActions; ++j)
            {
                // get the shortcut action
                KeyboardShortcutManager::Action* action = group->GetAction(j);

                settings->beginGroup(action->mName.c_str());
                settings->setValue("Key",   action->mKey);
                settings->setValue("Ctrl",  action->mCtrl);
                settings->setValue("Alt",   action->mAlt);
                settings->setValue("Local", action->mLocal);
                settings->endGroup();
            }

            settings->endGroup();
        }
    }


    void KeyboardShortcutManager::Load(QSettings* settings)
    {
        // clear the shortcut manager before loading
        Clear();

        // iterate through the groups and load all actions
        QStringList groupNames = settings->childGroups();
        const uint32 numGroups = groupNames.count();
        for (uint32 i = 0; i < numGroups; ++i)
        {
            QString groupName = groupNames[i];
            settings->beginGroup(groupNames[i]);
            QStringList actionNames = settings->childGroups();

            // iterate through the actions and save them
            const uint32 numActions = actionNames.count();
            for (uint32 j = 0; j < numActions; ++j)
            {
                QString actionName = actionNames[j];
                settings->beginGroup(actionName);
                int     key         =  settings->value("Key", "").toInt();
                bool    ctrlPressed =  settings->value("Ctrl", false).toBool();
                bool    altPressed  =  settings->value("Alt", false).toBool();
                bool    local       =  settings->value("Local", false).toBool();
                RegisterKeyboardShortcut(FromQtString(actionName).c_str(), FromQtString(groupName).c_str(), key, ctrlPressed, altPressed, local);
                settings->endGroup();
            }

            settings->endGroup();
        }
    }
}   // namespace MysticQt
