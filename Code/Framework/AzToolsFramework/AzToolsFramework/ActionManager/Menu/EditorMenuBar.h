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

class QMenuBar;

namespace AzToolsFramework
{
    class MenuManagerInterface;
    class MenuManagerInternalInterface;

    class EditorMenuBar
    {
    public:
        EditorMenuBar();

        static void Initialize();

        void AddMenu(int sortKey, AZStd::string menuIdentifier);
        
        // Returns whether the menu queried is contained in this menu.
        bool ContainsMenu(const AZStd::string& menuIdentifier) const;

        // Returns the sort key for the queried menu, or 0 if it's not found.
        AZStd::optional<int> GetMenuSortKey(const AZStd::string& menuIdentifier) const;
        
        // Returns the pointer to the menu bar.
        QMenuBar* GetMenuBar();
        const QMenuBar* GetMenuBar() const;
        
        // Clears the menu bar and creates a new one from the EditorMenuBar information.
        void RefreshMenuBar();

    private:
        QMenuBar* m_menuBar = nullptr;
        AZStd::multimap<int, AZStd::string> m_menus;
        AZStd::map<AZStd::string, int> m_menuToSortKeyMap;

        inline static MenuManagerInterface* m_menuManagerInterface = nullptr;
        inline static MenuManagerInternalInterface* m_menuManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
