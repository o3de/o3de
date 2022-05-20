/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/string/string.h>

class QAction;
class QMenu;

namespace AzToolsFramework
{
    class EditorMenu
    {
    public:
        EditorMenu();
        explicit EditorMenu(const AZStd::string& name);

        void AddAction(int sortKey, QAction* action);
        void AddSeparator(int sortKey);
        void AddSubMenu(int sortKey, QMenu* submenu);
        QMenu* GetMenu();

    private:
        void RefreshMenu();

        enum class MenuItemType
        {
            Action = 0,
            Separator,
            SubMenu
        };

        struct MenuItem
        {
            MenuItem();
            MenuItem(QAction* action);
            MenuItem(QMenu* menu);

            MenuItemType m_type;

            AZStd::variant<QAction*, QMenu*> m_value;
        };

        QMenu* m_menu = nullptr;
        AZStd::multimap<int, MenuItem> m_menuItems;
    };

} // namespace AzToolsFramework
