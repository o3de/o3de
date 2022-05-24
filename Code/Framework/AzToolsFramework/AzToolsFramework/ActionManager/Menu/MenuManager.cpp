/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/MenuManager.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>

namespace AzToolsFramework
{
    MenuManager::MenuManager()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "MenuManager - Could not retrieve instance of ActionManagerInterface");

        AZ::Interface<MenuManagerInterface>::Register(this);
    }

    MenuManager::~MenuManager()
    {
        AZ::Interface<MenuManagerInterface>::Unregister(this);
    }

    MenuManagerOperationResult MenuManager::RegisterMenu(const AZStd::string& identifier, const MenuProperties& properties)
    {
        if (m_menus.contains(identifier))
        {
            return AZ::Failure(
                AZStd::string::format("Menu Manager - Could not register menu \"%.s\" twice.", identifier.c_str()));
        }

        m_menus.insert(
            {
                identifier,
                EditorMenu(properties.m_name)
            }
        );

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddActionToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex)
    {
        if(!m_menus.contains(menuIdentifier))
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

        m_menus[menuIdentifier].AddAction(sortIndex, action);
        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddSeparatorToMenu(const AZStd::string& menuIdentifier, int sortIndex)
    {
        if (!m_menus.contains(menuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add separator - menu \"%s\" has not been registered.", menuIdentifier.c_str()));
        }

        m_menus[menuIdentifier].AddSeparator(sortIndex);

        return AZ::Success();
    }

    MenuManagerOperationResult MenuManager::AddSubMenuToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex)
    {
        if (!m_menus.contains(menuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add submenu \"%s\" to menu \"%s\" - menu has not been registered.", subMenuIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        if (!m_menus.contains(subMenuIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "Menu Manager - Could not add submenu \"%s\" to menu \"%s\" - submenu has not been registered.", subMenuIdentifier.c_str(),
                menuIdentifier.c_str()));
        }

        m_menus[menuIdentifier].AddSubMenu(sortIndex, m_menus[subMenuIdentifier].GetMenu());

        return AZ::Success();
    }

    QMenu* MenuManager::GetMenu(const AZStd::string& menuIdentifier)
    {
        if (!m_menus.contains(menuIdentifier))
        {
            return nullptr;
        }

        return m_menus[menuIdentifier].GetMenu();
    }

} // namespace AzToolsFramework
