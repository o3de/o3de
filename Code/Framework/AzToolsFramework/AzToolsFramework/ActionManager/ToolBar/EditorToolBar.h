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
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

#include <QWidgetAction>

class QAction;
class QToolBar;
class QWidget;

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class ActionManagerInternalInterface;
    class MenuManagerInterface;
    class MenuManagerInternalInterface;
    class ToolBarManagerInterface;

    //! Editor ToolBar class definitions.
    //! Wraps a QToolBar and provides additional functionality to handle and sort its items.
    class EditorToolBar final
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorToolBar, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorToolBar, "{A3862087-FEB3-466C-985B-92F9411BC2EF}");

        EditorToolBar();
        explicit EditorToolBar(const AZStd::string& name);

        static void Initialize(QWidget* defaultParentWidget);
        static void Reflect(AZ::ReflectContext* context);
        
        // Add Menu Items
        void AddAction(int sortKey, AZStd::string actionIdentifier);
        void AddActionWithSubMenu(int sortKey, AZStd::string actionIdentifier, const AZStd::string& subMenuIdentifier);
        void AddWidget(int sortKey, AZStd::string widgetActionIdentifier);
        void AddSeparator(int sortKey);

        // Remove Menu Items
        void RemoveAction(AZStd::string actionIdentifier);
        
        // Returns whether the action queried is contained in this toolbar.
        bool ContainsAction(const AZStd::string& actionIdentifier) const;
        bool ContainsWidget(const AZStd::string& widgetActionIdentifier) const;
        
        // Returns the sort key for the queried action, or 0 if it's not found.
        AZStd::optional<int> GetActionSortKey(const AZStd::string& actionIdentifier) const;
        AZStd::optional<int> GetWidgetSortKey(const AZStd::string& widgetActionIdentifier) const;

        // Returns the pointer to the ToolBar.
        QToolBar* GetToolBar();
        const QToolBar* GetToolBar() const;

        // Clears the menu and creates a new one from the EditorMenu information.
        void RefreshToolBar();

    private:
        enum class ToolBarItemType
        {
            Action = 0,
            ActionAndSubMenu,
            Widget,
            Separator
        };

        struct ToolBarItem final
        {
            AZ_CLASS_ALLOCATOR(ToolBarItem, AZ::SystemAllocator, 0);
            AZ_RTTI(ToolBarItem, "{B0DE0795-2C3F-4ABC-AAAB-1A68604EF33E}");

            explicit ToolBarItem(
                ToolBarItemType type = ToolBarItemType::Separator,
                AZStd::string identifier = AZStd::string(),
                AZStd::string subMenuIdentifier = AZStd::string()
            );

            ToolBarItemType m_type;

            AZStd::string m_identifier;
            AZStd::string m_subMenuIdentifier;

            QWidgetAction* m_widgetAction = nullptr;
        };

        QToolBar* m_toolBar = nullptr;
        AZStd::map<int, AZStd::vector<ToolBarItem>> m_toolBarItems;
        AZStd::unordered_map<AZStd::string, int> m_actionToSortKeyMap;
        AZStd::unordered_map<AZStd::string, int> m_widgetToSortKeyMap;

        inline static QWidget* s_defaultParentWidget = nullptr;

        inline static ActionManagerInterface* s_actionManagerInterface = nullptr;
        inline static ActionManagerInternalInterface* s_actionManagerInternalInterface = nullptr;
        inline static MenuManagerInterface* s_menuManagerInterface = nullptr;
        inline static MenuManagerInternalInterface* s_menuManagerInternalInterface = nullptr;
        inline static ToolBarManagerInterface* s_toolBarManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
