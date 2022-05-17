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
        MenuManagerOperationResult RegisterMenu(const AZStd::string& identifier, const MenuProperties& properties) override;
        MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) override;
        MenuManagerOperationResult AddSeparatorToMenu(const AZStd::string& menuIdentifier, int sortIndex) override;
        QMenu* GetMenu(const AZStd::string& menuIdentifier) override;
        MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) override;

        AZStd::unordered_map<AZStd::string, EditorMenu> m_menus;

        ActionManagerInterface* m_actionManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
