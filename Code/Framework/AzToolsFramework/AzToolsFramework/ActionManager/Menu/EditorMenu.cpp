/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>

#include <QMenu>

namespace AzToolsFramework
{
    EditorMenu::EditorMenu()
        : m_menu(new QMenu(""))
    {
    }

    EditorMenu::EditorMenu(const AZStd::string& name)
        : m_menu(new QMenu(name.c_str()))
    {
    }

    void EditorMenu::AddMenuItem(int sortKey, QAction* action)
    {
        m_menuItems.insert({ sortKey, action });
        RefreshMenu();
    }

    QMenu* EditorMenu::GetMenu()
    {
        return m_menu;
    }

    void EditorMenu::RefreshMenu()
    {
        m_menu->clear();

        for (auto elem : m_menuItems)
        {
            m_menu->addAction(elem.second);
        }
    }

} // namespace AzToolsFramework
