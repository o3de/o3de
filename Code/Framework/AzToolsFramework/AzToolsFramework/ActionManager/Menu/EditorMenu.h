/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
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
    class EditorMenu final
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorMenu, AZ::SystemAllocator);
        AZ_RTTI(EditorMenu, "{6B6F6802-C587-4734-A5DB-5732329EED03}");

        EditorMenu();
        EditorMenu(AZStd::string identifier, const AZStd::string& name);

        static void Initialize(QWidget* defaultParentWidget);
        static void Reflect(AZ::ReflectContext* context);

        // Add Menu Items
        void AddAction(int sortKey, AZStd::string actionIdentifier);
        void AddSubMenu(int sortKey, AZStd::string menuIdentifier);
        void AddWidget(int sortKey, AZStd::string widgetActionIdentifier);
        void AddSeparator(int sortKey);

        // Remove Menu Items
        void RemoveAction(AZStd::string actionIdentifier);
        void RemoveSubMenu(AZStd::string menuIdentifier);

        // Returns whether the action or menu queried is contained in this menu.
        bool ContainsAction(const AZStd::string& actionIdentifier) const;
        bool ContainsSubMenu(const AZStd::string& menuIdentifier) const;
        bool ContainsWidget(const AZStd::string& widgetActionIdentifier) const;

        // Returns the sort key for the queried action or menu, or 0 if it's not found.
        AZStd::optional<int> GetActionSortKey(const AZStd::string& actionIdentifier) const;
        AZStd::optional<int> GetSubMenuSortKey(const AZStd::string& menuIdentifier) const;
        AZStd::optional<int> GetWidgetSortKey(const AZStd::string& widgetActionIdentifier) const;
        
        // Returns the pointer to the menu.
        QMenu* GetMenu();
        const QMenu* GetMenu() const;

        // Displays the menu.
        void DisplayAtPosition(QPoint screenPosition) const;
        void DisplayUnderCursor() const;

        // Returns position of the menu.
        QPoint GetMenuPosition() const;

        // Returns whether the menu is currently visible.
        bool IsMenuVisible() const;

        // Clears the menu and creates a new one from the EditorMenu information.
        void RefreshMenu();

    private:
        enum class MenuItemType
        {
            Action = 0,
            SubMenu,
            Widget,
            Separator
        };

        struct MenuItem final
        {
            AZ_CLASS_ALLOCATOR(MenuItem, AZ::SystemAllocator);
            AZ_RTTI(MenuItem, "{1AB076C8-CF8F-42C1-98DB-856A067A4D21}");

            explicit MenuItem(
                MenuItemType type = MenuItemType::Separator,
                AZStd::string identifier = ""
            );

            MenuItemType m_type;

            AZStd::string m_identifier;

            QWidgetAction* m_widgetAction = nullptr;
        };

        // Record if this menu was empty at the last refresh.
        // Used to trigger a refresh on the parents when the situation changes.
        bool m_empty = true;

        QMenu* m_menu = nullptr;
        AZStd::string m_identifier;
        AZStd::map<int, AZStd::vector<MenuItem>> m_menuItems;
        AZStd::unordered_map<AZStd::string, int> m_actionToSortKeyMap;
        AZStd::unordered_map<AZStd::string, int> m_widgetToSortKeyMap;
        AZStd::unordered_map<AZStd::string, int> m_subMenuToSortKeyMap;

        inline static QWidget* s_defaultParentWidget = nullptr;

        inline static ActionManagerInterface* s_actionManagerInterface = nullptr;
        inline static ActionManagerInternalInterface* s_actionManagerInternalInterface = nullptr;
        inline static MenuManagerInterface* s_menuManagerInterface = nullptr;
        inline static MenuManagerInternalInterface* s_menuManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
