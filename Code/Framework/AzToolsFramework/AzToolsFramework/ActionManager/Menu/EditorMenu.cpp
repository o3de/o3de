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

    void EditorMenu::AddAction(int sortKey, QAction* action)
    {
        m_menuItems.insert({ sortKey, MenuItem(action) });
        RefreshMenu();
    }

    void EditorMenu::AddSeparator(int sortKey)
    {
        m_menuItems.insert({ sortKey, MenuItem() });
        RefreshMenu();
    }

    void EditorMenu::AddSubMenu(int sortKey, QMenu* submenu)
    {
        m_menuItems.insert({ sortKey, MenuItem(submenu) });
        RefreshMenu();
    }

    QMenu* EditorMenu::GetMenu()
    {
        return m_menu;
    }

    void EditorMenu::RefreshMenu()
    {
        m_menu->clear();

        for (const auto& elem : m_menuItems)
        {
            switch (elem.second.m_type)
            {
            case MenuItemType::Action:
                {
                    m_menu->addAction(AZStd::get<QAction*>(elem.second.m_value));
                    break;
                }
            case MenuItemType::SubMenu:
                {
                    m_menu->addMenu(AZStd::get<QMenu*>(elem.second.m_value));
                    break;
                }
            case MenuItemType::Separator:
                {
                    m_menu->addSeparator();
                    break;
                }
            default:
                break;
            }
        }
    }

    EditorMenu::MenuItem::MenuItem()
        : m_type(MenuItemType::Separator)
    {
    }

    EditorMenu::MenuItem::MenuItem(QAction* action)
        : m_type(MenuItemType::Action)
        , m_value(action)
    {
    }

    EditorMenu::MenuItem::MenuItem(QMenu* menu)
        : m_type(MenuItemType::SubMenu)
        , m_value(menu)
    {
    }

} // namespace AzToolsFramework
