/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/MenuManager.h>

#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>

namespace AzToolsFramework
{
    MenuManager::MenuManager(QWidget* defaultParentWidget)
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "MenuManager - Could not retrieve instance of ActionManagerInterface");

        m_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(m_actionManagerInternalInterface, "MenuManager - Could not retrieve instance of ActionManagerInternalInterface");

        AZ::Interface<MenuManagerInterface>::Register(this);
        AZ::Interface<MenuManagerInternalInterface>::Register(this);

        AZ::SystemTickBus::Handler::BusConnect();
        ActionManagerNotificationBus::Handler::BusConnect();

        EditorMenu::Initialize(defaultParentWidget);
        EditorMenuBar::Initialize();
    }

    MenuManager::~MenuManager()
    {
        ActionManagerNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();

        AZ::Interface<MenuManagerInternalInterface>::Unregister(this);
        AZ::Interface<MenuManagerInterface>::Unregister(this);

        Reset();
    }
    
    void MenuManager::Reflect(AZ::ReflectContext* context)
    {
        EditorMenu::Reflect(context);
        EditorMenuBar::Reflect(context);
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
                menuIdentifier, EditorMenu(menuIdentifier, properties.m_name)
            }
        );

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::RegisterMenuBar(const AZStd::string& menuBarIdentifier, QMainWindow* mainWindow)
    {
        if (m_menuBars.contains(menuBarIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("Menu Manager - Could not register menu bar \"%.s\" twice.", menuBarIdentifier.c_str()));
        }

        m_menuBars.insert(
            {
                menuBarIdentifier,
                EditorMenuBar(mainWindow)
            }
        );

        return AZ::Success();
    }

    bool MenuManager::IsMenuRegistered(const AZStd::string& menuIdentifier) const
    {
        return m_menus.contains(menuIdentifier);
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

        if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add action \"%s\" to menu \"%s\" - action could not be found.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (menuIterator->second.ContainsAction(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add action \"%s\" to menu \"%s\" - menu already contains action.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        menuIterator->second.AddAction(sortIndex, actionIdentifier);
        m_actionsToMenusMap[actionIdentifier].insert(menuIdentifier);
        m_menusToRefresh.insert(menuIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddActionsToMenu(
        const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add actions to menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        AZStd::string errorMessage = AZStd::string::format(
            "Menu Manager - Errors on AddActionsToMenu for menu \"%s\" - some actions were not added:", menuIdentifier.c_str());
        bool couldNotAddAction = false;

        for (const auto& pair : actions)
        {
            if (!m_actionManagerInterface->IsActionRegistered(pair.first))
            {
                errorMessage += AZStd::string(" ") + pair.first;
                couldNotAddAction = true;
                continue;
            }

            if (menuIterator->second.ContainsAction(pair.first))
            {
                errorMessage += AZStd::string(" ") + pair.first;
                couldNotAddAction = true;
                continue;
            }

            menuIterator->second.AddAction(pair.second, pair.first);
            m_actionsToMenusMap[pair.first].insert(menuIdentifier);
        }

        m_menusToRefresh.insert(menuIdentifier);

        if (couldNotAddAction)
        {
            return AZ::Failure(errorMessage);
        }

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::RemoveActionFromMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove action \"%s\" from menu \"%s\" - menu has not been registered.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove action \"%s\" from menu \"%s\" - action could not be found.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (!menuIterator->second.ContainsAction(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove action \"%s\" from menu \"%s\" - menu does not contain action.", actionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        menuIterator->second.RemoveAction(actionIdentifier);
        m_actionsToMenusMap[actionIdentifier].erase(menuIdentifier);

        m_menusToRefresh.insert(menuIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::RemoveActionsFromMenu(
        const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove actions from menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        AZStd::string errorMessage = AZStd::string::format(
            "Menu Manager - Errors on RemoveActionsFromMenu for menu \"%s\" - some actions were not removed:", menuIdentifier.c_str());
        bool couldNotRemoveAction = false;

        for (const AZStd::string& actionIdentifier : actionIdentifiers)
        {
            if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
            {
                errorMessage += AZStd::string(" ") + actionIdentifier;
                couldNotRemoveAction = true;
                continue;
            }

            if (!menuIterator->second.ContainsAction(actionIdentifier))
            {
                errorMessage += AZStd::string(" ") + actionIdentifier;
                couldNotRemoveAction = true;
                continue;
            }

            menuIterator->second.RemoveAction(actionIdentifier);
            m_actionsToMenusMap[actionIdentifier].erase(menuIdentifier);
        }

        m_menusToRefresh.insert(menuIdentifier);

        if (couldNotRemoveAction)
        {
            return AZ::Failure(errorMessage);
        }

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
        m_menusToRefresh.insert(menuIdentifier);
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

        if (menuIdentifier == subMenuIdentifier)
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add sub-menu \"%s\" to menu \"%s\" - the two menus are the same.",
                subMenuIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (menuIterator->second.ContainsSubMenu(subMenuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add sub-menu \"%s\" to menu \"%s\" - menu already contains this sub-menu.",
                subMenuIdentifier.c_str(), menuIdentifier.c_str()));
        }

        if (WouldGenerateCircularDependency(menuIdentifier, subMenuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add sub-menu \"%s\" to menu \"%s\" as this would generate a circular dependency.",
                subMenuIdentifier.c_str(), menuIdentifier.c_str()));
        }

        menuIterator->second.AddSubMenu(sortIndex, subMenuIdentifier);
        m_subMenusToMenusMap[subMenuIdentifier].insert(menuIdentifier);
        m_menusToRefresh.insert(menuIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddSubMenusToMenu(const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& subMenus)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add sub-menus to menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        AZStd::string errorMessage = AZStd::string::format(
            "Menu Manager - Errors on AddSubMenusToMenu for menu \"%s\" - some sub-menus were not added:", menuIdentifier.c_str());
        bool couldNotAddSubMenu = false;

        for (const auto& pair : subMenus)
        {
            if (!IsMenuRegistered(pair.first))
            {
                errorMessage += AZStd::string(" ") + pair.first;
                couldNotAddSubMenu = true;
                continue;
            }

            if (menuIterator->second.ContainsSubMenu(pair.first))
            {
                errorMessage += AZStd::string(" ") + pair.first;
                couldNotAddSubMenu = true;
                continue;
            }

            menuIterator->second.AddSubMenu(pair.second, pair.first);
            m_subMenusToMenusMap[pair.first].insert(menuIdentifier);
        }

        m_menusToRefresh.insert(menuIdentifier);

        if (couldNotAddSubMenu)
        {
            return AZ::Failure(errorMessage);
        }

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::RemoveSubMenuFromMenu(const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove sub-menu \"%s\" from menu \"%s\" - menu has not been registered.",
                subMenuIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (!IsMenuRegistered(subMenuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove sub-menu \"%s\" from menu \"%s\" - sub-menu has not been registered.",
                subMenuIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (!menuIterator->second.ContainsSubMenu(subMenuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove sub-menu \"%s\" from menu \"%s\" - menu does not contain sub-menu.",
                subMenuIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        menuIterator->second.RemoveSubMenu(subMenuIdentifier);

        m_menusToRefresh.insert(menuIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::RemoveSubMenusFromMenu(const AZStd::string& menuIdentifier, const AZStd::vector<AZStd::string>& subMenuIdentifiers)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not remove sub-menus from menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        AZStd::string errorMessage = AZStd::string::format(
            "Menu Manager - Errors on RemoveSubMenusFromMenu for menu \"%s\" - some sub-menus were not removed:", menuIdentifier.c_str());
        bool couldNotRemoveSubMenus = false;

        for (const AZStd::string& subMenuIdentifier : subMenuIdentifiers)
        {
            if (!IsMenuRegistered(subMenuIdentifier))
            {
                errorMessage += AZStd::string(" ") + subMenuIdentifier;
                couldNotRemoveSubMenus = true;
                continue;
            }

            if (!menuIterator->second.ContainsSubMenu(subMenuIdentifier))
            {
                errorMessage += AZStd::string(" ") + subMenuIdentifier;
                couldNotRemoveSubMenus = true;
                continue;
            }

            menuIterator->second.RemoveSubMenu(subMenuIdentifier);
        }

        m_menusToRefresh.insert(menuIdentifier);

        if (couldNotRemoveSubMenus)
        {
            return AZ::Failure(errorMessage);
        }

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

        if (menuBarIterator->second.ContainsMenu(menuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add menu \"%s\" to menu bar \"%s\" - menu bar already contains this menu.",
                menuIdentifier.c_str(), menuBarIdentifier.c_str()));
        }

        menuBarIterator->second.AddMenu(sortIndex, menuIdentifier);
        m_menuBarsToRefresh.insert(menuBarIdentifier);
        return AZ::Success();
    }
    
    MenuManagerOperationResult MenuManager::AddWidgetToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add widget to menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        if (m_actionManagerInterface->IsWidgetActionRegistered(widgetActionIdentifier))
        {
            menuIterator->second.AddWidget(sortIndex, widgetActionIdentifier);
            m_menusToRefresh.insert(menuIdentifier);

            return AZ::Success();
        }

        return AZ::Failure(AZStd::string::format(
            "Menu Manager - Could not add widget \"%s\" to menu \"%s\" - widget has not been registered.",
            widgetActionIdentifier.c_str(),  menuIdentifier.c_str()));
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
    
    MenuManagerIntegerResult MenuManager::GetSortKeyOfActionInMenu(const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier) const
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of action \"%s\" in menu \"%s\" - menu has not been registered.", actionIdentifier.c_str(), menuIdentifier.c_str()));
        }
        
        auto sortKey = menuIterator->second.GetActionSortKey(actionIdentifier);
        if (!sortKey.has_value())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of action \"%s\" in menu \"%s\" - action was not found in menu.", actionIdentifier.c_str(), menuIdentifier.c_str()));
        }
        
        return AZ::Success(sortKey.value());
    }

    MenuManagerIntegerResult MenuManager::GetSortKeyOfSubMenuInMenu(const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier) const
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of sub-menu \"%s\" in menu \"%s\" - menu has not been registered.", subMenuIdentifier.c_str(), menuIdentifier.c_str()));
        }
        
        auto sortKey = menuIterator->second.GetSubMenuSortKey(subMenuIdentifier);
        if (!sortKey.has_value())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of sub-menu \"%s\" in menu \"%s\" - sub-menu was not found in menu.", subMenuIdentifier.c_str(), menuIdentifier.c_str()));
        }
        
        return AZ::Success(sortKey.value());
    }

    MenuManagerIntegerResult MenuManager::GetSortKeyOfWidgetInMenu(const AZStd::string& menuIdentifier, const AZStd::string& widgetActionIdentifier) const
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of widget \"%s\" in menu \"%s\" - menu has not been registered.",
                widgetActionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }
        
        auto sortKey = menuIterator->second.GetWidgetSortKey(widgetActionIdentifier);
        if (!sortKey.has_value())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of widget \"%s\" in menu \"%s\" - sub-menu was not found in menu.",
                widgetActionIdentifier.c_str(),
                menuIdentifier.c_str()));
        }
        
        return AZ::Success(sortKey.value());
    }

    MenuManagerIntegerResult MenuManager::GetSortKeyOfMenuInMenuBar(const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier) const
    {
        auto menuBarIterator = m_menuBars.find(menuBarIdentifier);
        if (menuBarIterator == m_menuBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of menu \"%s\" in menu bar \"%s\" - menu bar has not been registered.", menuIdentifier.c_str(),
                menuBarIdentifier.c_str()));
        }
        
        auto sortKey = menuBarIterator->second.GetMenuSortKey(menuIdentifier);
        if (!sortKey.has_value())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not get sort key of menu \"%s\" in menu bar \"%s\" - menu was not found in menu bar.", menuIdentifier.c_str(), menuBarIdentifier.c_str()));
        }
        
        return AZ::Success(sortKey.value());
    }

    MenuManagerOperationResult MenuManager::DisplayMenuAtScreenPosition(const AZStd::string& menuIdentifier, const QPoint& screenPosition)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not display menu \"%s\" - menu has not been registered.",
                menuIdentifier.c_str()));
        }

        m_lastDisplayedMenuIdentifier = menuIdentifier;
        menuIterator->second.DisplayAtPosition(screenPosition);

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::DisplayMenuUnderCursor(const AZStd::string& menuIdentifier)
    {
        auto menuIterator = m_menus.find(menuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not display menu \"%s\" - menu has not been registered.", menuIdentifier.c_str()));
        }

        m_lastDisplayedMenuIdentifier = menuIdentifier;
        menuIterator->second.DisplayUnderCursor();

        return AZ::Success();
    }

    MenuManagerPositionResult MenuManager::GetLastContextMenuPosition() const
    {
        if (m_lastDisplayedMenuIdentifier.empty())
        {
            return AZ::Failure("Menu Manager - Could not return last context menu position. No menu was displayed yet.");
        }

        auto menuIterator = m_menus.find(m_lastDisplayedMenuIdentifier);
        if (menuIterator == m_menus.end())
        {
            return AZ::Failure("Menu Manager - Could not return last context menu position. Menu could not be found.");
        }

        return menuIterator->second.GetMenuPosition();
    }

    MenuManagerOperationResult MenuManager::QueueRefreshForMenu(const AZStd::string& menuIdentifier)
    {
        if (!m_menus.contains(menuIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("Menu Manager - Could not refresh menu \"%.s\" as it is not registered.", menuIdentifier.c_str()));
        }

        m_menusToRefresh.insert(menuIdentifier);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::QueueRefreshForMenusContainingAction(const AZStd::string& actionIdentifier)
    {
        if (auto actionIterator = m_actionsToMenusMap.find(actionIdentifier); actionIterator != m_actionsToMenusMap.end())
        {
            for (const AZStd::string& menuIdentifier : actionIterator->second)
            {
                m_menusToRefresh.insert(menuIdentifier);
            }
        }

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::QueueRefreshForMenusContainingSubMenu(const AZStd::string& subMenuIdentifier)
    {
        if (auto menuIterator = m_subMenusToMenusMap.find(subMenuIdentifier); menuIterator != m_subMenusToMenusMap.end())
        {
            for (const AZStd::string& menuIdentifier : menuIterator->second)
            {
                m_menusToRefresh.insert(menuIdentifier);
            }
        }

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::QueueRefreshForMenuBar(const AZStd::string& menuBarIdentifier)
    {
        if (!m_menuBars.contains(menuBarIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not refresh menuBar \"%.s\" as it is not registered.", menuBarIdentifier.c_str()));
        }

        m_menuBarsToRefresh.insert(menuBarIdentifier);
        return AZ::Success();
    }

    void MenuManager::RefreshMenus()
    {
        // RefreshMenu can add more menus to the refresh queue.
        // Since it's unordered, we're making a copy of it to prevent issues with the iterator.
        auto menusToRefreshTemp = m_menusToRefresh;
        m_menusToRefresh.clear();

        for (const AZStd::string& menuIdentifier : menusToRefreshTemp)
        {
            auto menuIterator = m_menus.find(menuIdentifier);
            if (menuIterator != m_menus.end())
            {
                menuIterator->second.RefreshMenu();
            }
        }

        // If more menus were added during the refresh process, do this again.
        if (!m_menusToRefresh.empty())
        {
            RefreshMenus();
        }
    }

    bool MenuManager::WouldGenerateCircularDependency(const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier)
    {
        // Iterate through all menus that have menuIdentifier as their submenu.
        for (auto identifier : m_subMenusToMenusMap[menuIdentifier])
        {
            // Return true if the menu is the submenu we're trying to add, or keep checking with the ancestors.
            if (identifier == subMenuIdentifier || WouldGenerateCircularDependency(identifier, subMenuIdentifier))
            {
                return true;
            }
        }

        return false;
    }

    void MenuManager::RefreshMenuBars()
    {
        for (const AZStd::string& menuBarIdentifier : m_menuBarsToRefresh)
        {
            auto menuBarIterator = m_menuBars.find(menuBarIdentifier);
            if (menuBarIterator != m_menuBars.end())
            {
                menuBarIterator->second.RefreshMenuBar();
            }
        }

        m_menuBarsToRefresh.clear();
    }

    void MenuManager::RefreshLastDisplayedMenu()
    {
        if (!m_lastDisplayedMenuIdentifier.empty())
        {
            auto menuIterator = m_menus.find(m_lastDisplayedMenuIdentifier);
            if (menuIterator != m_menus.end() && !menuIterator->second.IsMenuVisible())
            {
                m_lastDisplayedMenuIdentifier.clear();
            }
        }
    }

    MenuManagerStringResult MenuManager::SerializeMenu(const AZStd::string& menuIdentifier)
    {
        if (!m_menus.contains(menuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not serialize menu \"%.s\" as it is not registered.", menuIdentifier.c_str()));
        }

        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;

        // Generate menu dom using Json serialization system.
        AZ::JsonSerializationResult::ResultCode result =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), m_menus[menuIdentifier], settings);

        // Stringify dom to return it.
        if (result.HasDoneWork())
        {
            rapidjson::StringBuffer prefabBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
            document.Accept(writer);

            return AZ::Success(AZStd::string(prefabBuffer.GetString()));
        }

        return AZ::Failure(
            AZStd::string::format("Menu Manager - Could not serialize menu \"%.s\" - serialization error.", menuIdentifier.c_str()));
    }

    MenuManagerStringResult MenuManager::SerializeMenuBar(const AZStd::string& menuBarIdentifier)
    {
        if (!m_menuBars.contains(menuBarIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not serialize menu bar \"%.s\" as it is not registered.", menuBarIdentifier.c_str()));
        }

        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;

        // Generate menu dom using Json serialization system.
        AZ::JsonSerializationResult::ResultCode result =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), m_menuBars[menuBarIdentifier], settings);

        // Stringify dom to return it.
        if (result.HasDoneWork())
        {
            rapidjson::StringBuffer prefabBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
            document.Accept(writer);

            return AZ::Success(AZStd::string(prefabBuffer.GetString()));
        }

        return AZ::Failure(
            AZStd::string::format("Menu Manager - Could not serialize menu bar \"%.s\" - serialization error.", menuBarIdentifier.c_str()));
    }

    void MenuManager::Reset()
    {
        // Reset all stored values that are registered by the environment after initialization.
        m_menus.clear();
        m_menuBars.clear();

        m_actionsToMenusMap.clear();
        m_subMenusToMenusMap.clear();

        m_menusToRefresh.clear();
        m_menuBarsToRefresh.clear();
    }

    void MenuManager::OnSystemTick()
    {
        RefreshMenus();
        RefreshMenuBars();
        RefreshLastDisplayedMenu();
    }

    void MenuManager::OnActionStateChanged(AZStd::string actionIdentifier)
    {
        // Only refresh the menu if the action state changing could result in the action being shown/hidden.
        if (m_actionManagerInternalInterface->GetActionMenuVisibility(actionIdentifier) != ActionVisibility::AlwaysShow)
        {
            QueueRefreshForMenusContainingAction(actionIdentifier);
        }
    }

} // namespace AzToolsFramework
