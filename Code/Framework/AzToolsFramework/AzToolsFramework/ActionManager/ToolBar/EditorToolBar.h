/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
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
    class EditorToolBar
    {
    public:
        EditorToolBar();
        explicit EditorToolBar(const AZStd::string& name);

        static void Initialize(QWidget* defaultParentWidget);
        
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

        struct ToolBarItem
        {
            explicit ToolBarItem(
                QToolBar* toolBar,
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
        AZStd::multimap<int, ToolBarItem> m_toolBarItems;
        AZStd::map<AZStd::string, int> m_actionToSortKeyMap;
        AZStd::map<AZStd::string, int> m_widgetToSortKeyMap;

        inline static QWidget* m_defaultParentWidget;

        inline static ActionManagerInterface* m_actionManagerInterface = nullptr;
        inline static ActionManagerInternalInterface* m_actionManagerInternalInterface = nullptr;
        inline static MenuManagerInterface* m_menuManagerInterface = nullptr;
        inline static MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
        inline static ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
