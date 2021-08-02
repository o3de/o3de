/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QKeySequence>
#include <QAction>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include "MysticQtConfig.h"
#endif

class QKeyEvent;
class QSettings;

namespace MysticQt
{
    class MYSTICQT_API KeyboardShortcutManager
        : public QObject
    {
    public:
        struct Action
        {
            QAction*            m_qaction;
            QKeySequence        m_defaultKeySequence;
            bool                m_local;

            Action(QAction* qaction, bool local)
                : m_qaction(qaction)
                , m_defaultKeySequence(qaction->shortcut())
                , m_local(local)
            {
            }
        };

        class Group
        {
        public:
            Group(AZStd::string_view groupName)
                : m_name(groupName)
            {
            }

            void AddAction(AZStd::unique_ptr<Action> action)        { m_actions.emplace_back(AZStd::move(action)); }
            void RemoveAction(QAction* action, bool local);
            size_t GetNumActions() const                            { return m_actions.size(); }
            Action* GetAction(size_t index)                         { return m_actions[index].get(); }
            const AZStd::vector<AZStd::unique_ptr<Action>>& GetActions() const { return m_actions; }
            const AZStd::string& GetName() const                    { return m_name; }
            Action* FindActionByName(const QString& actionName, bool local) const;

        private:
            AZStd::string m_name;
            AZStd::vector<AZStd::unique_ptr<Action>> m_actions;
        };

        void RegisterKeyboardShortcut(QAction* qaction, AZStd::string_view groupName, bool local);
        void UnregisterKeyboardShortcut(QAction* qaction, AZStd::string_view groupName, bool local);
        Action* FindShortcut(QKeySequence keySequence, Group* group) const;
        Action* FindAction(const QString& actionName, AZStd::string_view groupName) const;
        Group* FindGroupForShortcut(Action* action) const;
        size_t GetNumGroups() const             { return m_groups.size(); }
        Group* GetGroup(size_t index) const     { return m_groups[index].get(); }

        void Save(QSettings* settings);
        void Load(QSettings* settings);

    private:
        AZStd::vector<AZStd::unique_ptr<Group>> m_groups;

        Group* FindGroupByName(AZStd::string_view groupName) const;
    };
} // namespace MysticQt
