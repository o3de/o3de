/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/Menu/EditorMenu.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <QCursor>
#include <QMenu>

namespace AzToolsFramework
{
    EditorMenu::EditorMenu()
        : m_menu(new QMenu(""))
    {
    }

    EditorMenu::EditorMenu(AZStd::string identifier, const AZStd::string& name)
        : m_identifier(AZStd::move(identifier))
        , m_menu(new QMenu(name.c_str()))
    {
    }

    void EditorMenu::AddSeparator(int sortKey)
    {
        m_menuItems[sortKey].emplace_back();
    }

    void EditorMenu::AddAction(int sortKey, AZStd::string actionIdentifier)
    {
        if (ContainsAction(actionIdentifier))
        {
            return;
        }

        m_actionToSortKeyMap.insert(AZStd::make_pair(actionIdentifier, sortKey));
        m_menuItems[sortKey].emplace_back(MenuItemType::Action, AZStd::move(actionIdentifier));
    }

    void EditorMenu::RemoveAction(AZStd::string actionIdentifier)
    {
        auto sortKeyIterator = m_actionToSortKeyMap.find(actionIdentifier);
        if (sortKeyIterator == m_actionToSortKeyMap.end())
        {
            return;
        }

        int sortKey = sortKeyIterator->second;
        bool removed = false;

        AZStd::erase_if(
            m_menuItems[sortKey],
            [&](const MenuItem& item)
            {
                removed = true;
                return item.m_identifier == actionIdentifier;
            }
        );

        if (removed)
        {
            m_actionToSortKeyMap.erase(actionIdentifier);
        }
    }

    void EditorMenu::RemoveSubMenu(AZStd::string menuIdentifier)
    {
        auto sortKeyIterator = m_subMenuToSortKeyMap.find(menuIdentifier);
        if (sortKeyIterator == m_subMenuToSortKeyMap.end())
        {
            return;
        }

        int sortKey = sortKeyIterator->second;
        bool removed = false;

        AZStd::erase_if(
            m_menuItems[sortKey],
            [&](const MenuItem& item)
            {
                removed = true;
                return item.m_identifier == menuIdentifier;
            }
        );

        if (removed)
        {
            m_subMenuToSortKeyMap.erase(menuIdentifier);
        }
    }

    void EditorMenu::AddSubMenu(int sortKey, AZStd::string menuIdentifier)
    {
        if (ContainsSubMenu(menuIdentifier))
        {
            return;
        }

        m_subMenuToSortKeyMap.insert(AZStd::make_pair(menuIdentifier, sortKey));
        m_menuItems[sortKey].emplace_back(MenuItemType::SubMenu, AZStd::move(menuIdentifier));
    }

    void EditorMenu::AddWidget(int sortKey, AZStd::string widgetActionIdentifier)
    {
        if (!m_widgetToSortKeyMap.contains(widgetActionIdentifier))
        {
            m_widgetToSortKeyMap.insert(AZStd::make_pair(widgetActionIdentifier, sortKey));
            m_menuItems[sortKey].emplace_back(MenuItemType::Widget, AZStd::move(widgetActionIdentifier));
        }
    }

    bool EditorMenu::ContainsAction(const AZStd::string& actionIdentifier) const
    {
        return m_actionToSortKeyMap.contains(actionIdentifier);
    }

    bool EditorMenu::ContainsSubMenu(const AZStd::string& menuIdentifier) const
    {
        return m_subMenuToSortKeyMap.contains(menuIdentifier);
    }

    bool EditorMenu::ContainsWidget(const AZStd::string& widgetActionIdentifier) const
    {
        return m_widgetToSortKeyMap.contains(widgetActionIdentifier);
    }

    AZStd::optional<int> EditorMenu::GetActionSortKey(const AZStd::string& actionIdentifier) const
    {
        auto actionIterator = m_actionToSortKeyMap.find(actionIdentifier);
        if (actionIterator == m_actionToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return actionIterator->second;
    }

    AZStd::optional<int> EditorMenu::GetSubMenuSortKey(const AZStd::string& menuIdentifier) const
    {
        auto menuIterator = m_subMenuToSortKeyMap.find(menuIdentifier);
        if (menuIterator == m_subMenuToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return menuIterator->second;
    }

    AZStd::optional<int> EditorMenu::GetWidgetSortKey(const AZStd::string& widgetActionIdentifier) const
    {
        auto widgetIterator = m_widgetToSortKeyMap.find(widgetActionIdentifier);
        if (widgetIterator == m_widgetToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return widgetIterator->second;
    }

    QMenu* EditorMenu::GetMenu()
    {
        return m_menu;
    }

    const QMenu* EditorMenu::GetMenu() const
    {
        return m_menu;
    }

    void EditorMenu::DisplayAtPosition(QPoint screenPosition) const
    {
        m_menu->popup(screenPosition);
    }

    void EditorMenu::DisplayUnderCursor() const
    {
        DisplayAtPosition(QCursor::pos());
    }

    QPoint EditorMenu::GetMenuPosition() const
    {
        return m_menu->pos();
    }

    bool EditorMenu::IsMenuVisible() const
    {
        return m_menu->isVisible();
    }

    void EditorMenu::RefreshMenu()
    {
        m_menu->clear();

        for (const auto& vectorIterator : m_menuItems)
        {
            for (const auto& menuItem : vectorIterator.second)
            {
                switch (menuItem.m_type)
                {
                case MenuItemType::Action:
                    {
                        if (QAction* action = s_actionManagerInternalInterface->GetAction(menuItem.m_identifier))
                        {
                            auto outcome = s_actionManagerInterface->IsActionActiveInCurrentMode(menuItem.m_identifier);
                            bool isActiveInCurrentMode = outcome.IsSuccess() && outcome.GetValue();

                            if (
                                !IsActionVisible(
                                    s_actionManagerInternalInterface->GetActionMenuVisibility(menuItem.m_identifier),
                                    isActiveInCurrentMode,
                                    action->isEnabled()
                                )
                            )
                            {
                                continue;
                            }

                            m_menu->addAction(action);
                        }
                        break;
                    }
                case MenuItemType::SubMenu:
                    {
                        if (QMenu* menu = s_menuManagerInternalInterface->GetMenu(menuItem.m_identifier); menu && !menu->isEmpty())
                        {
                            m_menu->addMenu(menu);
                        }
                        break;
                    }
                case MenuItemType::Separator:
                    {
                        m_menu->addSeparator();
                        break;
                    }
                case MenuItemType::Widget:
                    {
                        m_menu->addAction(menuItem.m_widgetAction);
                        break;
                    }
                default:
                    break;
                }
            }
        }

        // If the menu contents changed from empty to full or viceversa, refresh all the menus containing this menu.
        if (m_menu->isEmpty() != m_empty)
        {
            s_menuManagerInternalInterface->QueueRefreshForMenusContainingSubMenu(m_identifier);
            m_empty = m_menu->isEmpty();
        }
    }

    EditorMenu::MenuItem::MenuItem(MenuItemType type, AZStd::string identifier)
        : m_type(type)
        , m_identifier(AZStd::move(identifier))
    {
        if (type == MenuItemType::Widget)
        {
            if (QWidget* widget = s_actionManagerInternalInterface->GenerateWidgetFromWidgetAction(m_identifier))
            {
                m_widgetAction = new QWidgetAction(s_defaultParentWidget);
                m_widgetAction->setDefaultWidget(widget);
            }
        }
    }

    void EditorMenu::Initialize(QWidget* defaultParentWidget)
    {
        s_defaultParentWidget = defaultParentWidget;

        s_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(s_actionManagerInterface, "EditorMenu - Could not retrieve instance of ActionManagerInterface");

        s_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(s_actionManagerInternalInterface, "EditorMenu - Could not retrieve instance of ActionManagerInternalInterface");

        s_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(s_menuManagerInterface, "EditorMenu - Could not retrieve instance of MenuManagerInterface");

        s_menuManagerInternalInterface = AZ::Interface<MenuManagerInternalInterface>::Get();
        AZ_Assert(s_menuManagerInternalInterface, "EditorMenu - Could not retrieve instance of MenuManagerInternalInterface");
    }

    void EditorMenu::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MenuItem>()
                ->Field("Type", &MenuItem::m_type)
                ->Field("Identifier", &MenuItem::m_identifier);

            serializeContext->Class<EditorMenu>()
                ->Field("Items", &EditorMenu::m_menuItems);
        }
    }

} // namespace AzToolsFramework
