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

    void MenuManager::AddActionToMenu(AZStd::string_view actionIdentifier, AZStd::string_view menuIdentifier, [[maybe_unused]]int sortIndex)
    {
        if(!m_menus.contains(menuIdentifier))
        {
            m_menus.insert({ menuIdentifier, EditorMenu() });
        }

        QAction* action = m_actionManagerInterface->GetAction(actionIdentifier);

        if (action)
        {
            m_menus[menuIdentifier].AddMenuItem(sortIndex, action);
        }
    }

    QMenu* MenuManager::GetMenu(AZStd::string_view menuIdentifier)
    {
        if (m_menus.contains(menuIdentifier))
        {
            // TODO - avoid reaching into the EditorMenu and actually implement a getter?
            return m_menus[menuIdentifier].m_menu;
        }

        return nullptr;
    }

} // namespace AzToolsFramework
