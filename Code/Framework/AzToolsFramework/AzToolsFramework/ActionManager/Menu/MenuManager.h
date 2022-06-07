/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>
#include <AzToolsFramework/ActionManager/Menu/EditorMenuBar.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;

    class MenuManager
        : private MenuManagerInterface
    {
    public:
        MenuManager();
        virtual ~MenuManager();

    private:
        // MenuManagerInterface overrides ...
        MenuManagerOperationResult RegisterMenu(const AZStd::string& menuIdentifier, const MenuProperties& properties) override;
        MenuManagerOperationResult RegisterMenuBar(const AZStd::string& menuBarIdentifier) override;
        MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddSeparatorToMenu(const AZStd::string& menuIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddWidgetToMenu(
            const AZStd::string& menuIdentifier, QWidget* widget, int sortIndex) override;
        MenuManagerOperationResult AddMenuToMenuBar(
            const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex) override;
        QMenu* GetMenu(const AZStd::string& menuIdentifier) override;
        QMenuBar* GetMenuBar(const AZStd::string& menuBarIdentifier) override;

        AZStd::unordered_map<AZStd::string, EditorMenu> m_menus;
        AZStd::unordered_map<AZStd::string, EditorMenuBar> m_menuBars;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
