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

    class EditorMenuBar
    {
    public:
        EditorMenuBar();

        static void Initialize();

        void AddMenu(int sortKey, AZStd::string menuIdentifier);
        
        // Returns the pointer to the menu bar.
        QMenuBar* GetMenuBar();
        const QMenuBar* GetMenuBar() const;

    private:
        void RefreshMenuBar();

        QMenuBar* m_menuBar = nullptr;
        AZStd::multimap<int, AZStd::string> m_menus;

        inline static MenuManagerInterface* m_menuManagerInterface = nullptr;
    };

} // namespace AzToolsFramework
