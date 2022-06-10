/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>

#include <QWidgetAction>

class QAction;
class QMenu;
class QWidget;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;

    class EditorMenu
    {
    public:
        EditorMenu();
        explicit EditorMenu(const AZStd::string& name);

        static void Initialize();

        void AddSeparator(int sortKey);
        void AddAction(int sortKey, AZStd::string actionIdentifier);
        void AddSubMenu(int sortKey, AZStd::string menuIdentifier);
        void AddWidget(int sortKey, QWidget* widget);
        
        // Returns the pointer to the menu.
        QMenu* GetMenu();
        const QMenu* GetMenu() const;

    private:
        void RefreshMenu();

        enum class MenuItemType
        {
            Action = 0,
            Separator,
            SubMenu,
            Widget
        };

        struct MenuItem
        {
            explicit MenuItem(MenuItemType type = MenuItemType::Separator, AZStd::string identifier = "");
            explicit MenuItem(QWidget* widget);

            MenuItemType m_type;

            AZStd::string m_identifier;
            QWidgetAction* m_widgetAction = nullptr;
        };

        QMenu* m_menu = nullptr;
        AZStd::multimap<int, MenuItem> m_menuItems;

        inline static ActionManagerInterface* m_actionManagerInterface = nullptr;
        inline static MenuManagerInterface* m_menuManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
