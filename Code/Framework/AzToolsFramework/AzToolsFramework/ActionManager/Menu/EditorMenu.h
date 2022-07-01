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
    class ActionManagerInternalInterface;
    class MenuManagerInterface;
    class MenuManagerInternalInterface;
    
    //! Editor Menu class definitions.
    //! Wraps a QMenu and provides additional functionality to handle and sort its items.
    class EditorMenu
    {
    public:
        EditorMenu();
        explicit EditorMenu(const AZStd::string& name);

        static void Initialize();

        // Add Menu Items
        void AddAction(int sortKey, AZStd::string actionIdentifier);
        void AddSeparator(int sortKey);
        void AddSubMenu(int sortKey, AZStd::string menuIdentifier);
        void AddWidget(int sortKey, QWidget* widget);

        // Remove Menu Items
        void RemoveAction(AZStd::string actionIdentifier);

        // Returns whether the action or menu queried is contained in this menu.
        bool ContainsAction(const AZStd::string& actionIdentifier) const;
        bool ContainsSubMenu(const AZStd::string& menuIdentifier) const;

        // Returns the sort key for the queried action or menu, or 0 if it's not found.
        AZStd::optional<int> GetActionSortKey(const AZStd::string& actionIdentifier) const;
        AZStd::optional<int> GetSubMenuSortKey(const AZStd::string& menuIdentifier) const;
        
        // Returns the pointer to the menu.
        QMenu* GetMenu();
        const QMenu* GetMenu() const;

        // Clears the menu and creates a new one from the EditorMenu information.
        void RefreshMenu();

    private:
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
        AZStd::map<AZStd::string, int> m_actionToSortKeyMap;
        AZStd::map<AZStd::string, int> m_subMenuToSortKeyMap;

        inline static ActionManagerInterface* m_actionManagerInterface = nullptr;
        inline static ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
        inline static MenuManagerInterface* m_menuManagerInterface = nullptr;
        inline static MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
