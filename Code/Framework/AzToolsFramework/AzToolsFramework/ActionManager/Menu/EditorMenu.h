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

class QAction;
class QMenu;

namespace AzToolsFramework
{
    class EditorMenu
    {
    public:
        EditorMenu();
        explicit EditorMenu(const AZStd::string& name);

        void AddMenuItem(int sortKey, QAction* action);
        QMenu* GetMenu();

    private:
        void RefreshMenu();

        QMenu* m_menu = nullptr;
        AZStd::multimap<int, QAction*> m_menuItems;
    };

} // namespace AzToolsFramework
