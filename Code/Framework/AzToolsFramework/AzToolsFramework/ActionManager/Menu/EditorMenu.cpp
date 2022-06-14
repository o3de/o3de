/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

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

    void EditorMenu::AddSeparator(int sortKey)
    {
        m_menuItems.insert({ sortKey, MenuItem() });
        RefreshMenu();
    }
    
    void EditorMenu::AddAction(int sortKey, AZStd::string actionIdentifier)
    {
        m_menuItems.insert({ sortKey, MenuItem(MenuItemType::Action, AZStd::move(actionIdentifier)) });
        RefreshMenu();
    }

    void EditorMenu::AddSubMenu(int sortKey, AZStd::string menuIdentifier)
    {
        m_menuItems.insert({ sortKey, MenuItem(MenuItemType::SubMenu, AZStd::move(menuIdentifier)) });
        RefreshMenu();
    }

    void EditorMenu::AddWidget(int sortKey, QWidget* widget)
    {
        m_menuItems.insert({ sortKey, MenuItem(widget) });
        RefreshMenu();
    }

    QMenu* EditorMenu::GetMenu()
    {
        return m_menu;
    }

    const QMenu* EditorMenu::GetMenu() const
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
                    if(QAction* action = m_actionManagerInterface->GetAction(elem.second.m_identifier))
                    {
                        m_menu->addAction(action);
                    }
                    break;
                }
            case MenuItemType::SubMenu:
                {
                    if(QMenu* menu = m_menuManagerInterface->GetMenu(elem.second.m_identifier))
                    {
                        m_menu->addMenu(menu);
                    }
                    break;
                }
            case MenuItemType::Separator:
                {
                    m_menu->addSeparator();
                    break;
                }
            case MenuItemType::Widget:
                {
                    m_menu->addAction(elem.second.m_widgetAction);
                    break;
                }
            default:
                break;
            }
        }
    }

    EditorMenu::MenuItem::MenuItem(MenuItemType type, AZStd::string identifier)
        : m_type(type)
    {
        if (type != MenuItemType::Separator)
        {
            m_identifier = AZStd::move(identifier);
        }
    }

    EditorMenu::MenuItem::MenuItem(QWidget* widget)
        : m_type(MenuItemType::Widget)
    {
        m_widgetAction = new QWidgetAction(widget->parent());
        m_widgetAction->setDefaultWidget(widget);
    }

    void EditorMenu::Initialize()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "EditorMenu - Could not retrieve instance of ActionManagerInterface");

        m_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(m_menuManagerInterface, "EditorMenu - Could not retrieve instance of MenuManagerInterface");
    }

} // namespace AzToolsFramework
