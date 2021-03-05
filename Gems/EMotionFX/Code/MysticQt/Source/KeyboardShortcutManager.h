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

#ifndef __MYSTICQT_KEYBOARDSHORTCUTMANAGER_H
#define __MYSTICQT_KEYBOARDSHORTCUTMANAGER_H

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#endif

class QKeyEvent;
class QSettings;

namespace MysticQt
{
    class MYSTICQT_API KeyboardShortcutManager
    {
        MCORE_MEMORYOBJECTCATEGORY(KeyboardShortcutManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT);

    public:
        KeyboardShortcutManager();
        virtual ~KeyboardShortcutManager();

        struct Action
        {
            MCORE_MEMORYOBJECTCATEGORY(KeyboardShortcutManager::Action, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT);

            AZStd::string       mName;
            int                 mKey;
            bool                mCtrl;
            bool                mAlt;
            bool                mLocal;

            int                 mDefaultKey;
            bool                mDefaultCtrl;
            bool                mDefaultAlt;

            Action(const char* name, int defaultKey, bool defaultCtrl, bool defaultAlt, bool local)
            {
                mName           = name;
                mLocal          = local;

                mKey            = defaultKey;
                mCtrl           = defaultCtrl;
                mAlt            = defaultAlt;

                mDefaultKey     = defaultKey;
                mDefaultCtrl    = defaultCtrl;
                mDefaultAlt     = defaultAlt;
            }
        };

        class Group
        {
            MCORE_MEMORYOBJECTCATEGORY(KeyboardShortcutManager::Group, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT);
        public:
            Group(const char* groupName)                            { mName = groupName; }
            virtual ~Group()
            {
                const uint32 numActions = mActions.GetLength();
                for (uint32 i = 0; i < numActions; ++i)
                {
                    delete mActions[i];
                }
                mActions.Clear();
            }
            void AddAction(Action* action)                          { mActions.Add(action); }
            uint32 GetNumActions() const                            { return mActions.GetLength(); }
            Action* GetAction(uint32 index)                         { return mActions[index]; }
            const char* GetName() const                             { return mName.c_str(); }
            const AZStd::string& GetNameString() const              { return mName; }
            Action* FindActionByName(const char* actionName, bool local) const;

        private:
            AZStd::string           mName;
            MCore::Array<Action*>   mActions;
        };

        void RegisterKeyboardShortcut(const char* actionName, const char* groupName, int defaultKey, bool defaultCtrl, bool defaultAlt, bool local);
        bool Check(QKeyEvent* event, const char* actionName, const char* groupName);
        Action* FindShortcut(int key, bool ctrl, bool alt, Group* group);
        Action* FindAction(const char* actionName, const char* groupName);
        Group* FindGroupForShortcut(Action* action);
        uint32 GetNumGroups() const             { return mGroups.GetLength(); }
        Group* GetGroup(uint32 index) const     { return mGroups[index]; }
        void Clear();

        void Save(QSettings* settings);
        void Load(QSettings* settings);

    private:
        MCore::Array<Group*> mGroups;

        Group* FindGroupByName(const char* groupName) const;
    };
} // namespace MysticQt


#endif
