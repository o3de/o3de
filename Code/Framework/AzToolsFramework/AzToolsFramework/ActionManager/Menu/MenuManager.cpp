/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/MenuManager.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>

namespace AzToolsFramework
{
    MenuManager::MenuManager()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "MenuManager - Could not retrieve instance of ActionManagerInterface");

        AZ::Interface<MenuManagerInterface>::Register(this);

        EditorMenu::Initialize();
        EditorMenuBar::Initialize();
    }

    MenuManager::~MenuManager()
    {
        AZ::Interface<MenuManagerInterface>::Unregister(this);
    }

    MenuManagerOperationResult MenuManager::RegisterMenu(const AZStd::string& menuIdentifier, const MenuProperties& properties)
    {
        if (m_menus.contains(menuIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("Menu Manager - Could not register menu \"%.s\" twice.", menuIdentifier.c_str()));
        }

        m_menus.insert(
            {
                menuIdentifier,
                EditorMenu(properties.m_name)
            }
        );

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::RegisterMenuBar(const AZStd::string& menuBarIdentifier)
    {
        if (m_menuBars.contains(menuBarIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("Menu Manager - Could not register menu bar \"%.s\" twice.", menuBarIdentifier.c_str()));
        }

        m_menuBars.insert(
            {
                menuBarIdentifier,
                EditorMenuBar()
            }
        );

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddActionToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add action \"%s\" to menu \"%s\" - menu has not been registered.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        QAction* action = m_actionManagerInterface->GetAction(actionIdentifier);
        if (!action)
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add action \"%s\" to menu \"%s\" - action could not be found.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        menuIterator->second.AddAction(sortIndex, actionIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddSeparatorToMenu(const AZStd::string& menuIdentifier, int sortIndex)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add separator - menu \"%s\" has not been registered.", menuIdentifier.c_str()));
        }

        menuIterator->second.AddSeparator(sortIndex);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddSubMenuToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add sub-menu \"%s\" to menu \"%s\" - menu has not been registered.",
                subMenuIdentifier.c_str(), menuIdentifier.c_str()));
        }

        if (!m_menus.contains(subMenuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add sub-menu \"%s\" to menu \"%s\" - sub-menu has not been registered.",
                subMenuIdentifier.c_str(), menuIdentifier.c_str()));
        }

        menuIterator->second.AddSubMenu(sortIndex, subMenuIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddMenuToMenuBar(const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex)
    {
        auto menuBarIterator = m_menuBars.find(menuBarIdentifier);
        if (menuBarIterator == m_menuBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add menu \"%s\" to menu bar \"%s\" - menu bar has not been registered.", menuIdentifier.c_str(),
                menuBarIdentifier.c_str()));
        }

        if (!m_menus.contains(menuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add menu \"%s\" to menu bar \"%s\" - menu has not been registered.", menuIdentifier.c_str(),
                menuBarIdentifier.c_str()));
        }

        menuBarIterator->second.AddMenu(sortIndex, menuIdentifier);
        return AZ::Success();
    }
    
    MenuManagerOperationResult MenuManager::AddWidgetToMenu(
        const AZStd::string& menuIdentifier, QWidget* widget, int sortIndex)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add widget to menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        if (!widget)
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add widget to menu \"%s\" - nullptr widget.", menuIdentifier.c_str()));
        }

        menuIterator->second.AddWidget(sortIndex, widget);

        return AZ::Success();
    }

    QMenu* MenuManager::GetMenu(const AZStd::string& menuIdentifier)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return nullptr;
        }

        return menuIterator->second.GetMenu();
    }

    QMenuBar* MenuManager::GetMenuBar(const AZStd::string& menuBarIdentifier)
    {
        auto menuBarIterator = m_menuBars.find(menuBarIdentifier);
        if (menuBarIterator == m_menuBars.end())
        {
            return nullptr;
        }

        return menuBarIterator->second.GetMenuBar();
    }

} // namespace AzToolsFramework
