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

#include <QToolBar>

class QMainWindow;
class QWidget;

namespace AzToolsFramework
{
    class ToolBarManagerInterface;
    class ToolBarManagerInternalInterface;

    class EditorToolBarArea final
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorToolBarArea, AZ::SystemAllocator);
        AZ_RTTI(EditorToolBarArea, "{7B55B739-B4E0-41C0-9E71-B526BD62C3FB}");

        EditorToolBarArea() = default;
        EditorToolBarArea(QMainWindow* mainWindow, Qt::ToolBarArea toolBarArea);

        static void Initialize();
        static void Reflect(AZ::ReflectContext* context);

        void AddToolBar(int sortKey, AZStd::string toolBarIdentifier);
        
        //! Returns whether the toolbar queried is contained in this toolbar area.
        bool ContainsToolBar(const AZStd::string& toolBarIdentifier) const;

        //! Returns the sort key for the queried toolbar, or AZStd::nullopt if it's not found.
        AZStd::optional<int> GenerateToolBarSortKey(const AZStd::string& toolBarIdentifier) const;
        
        //! Clears the toolbar area and creates a new one from the EditorToolBarArea information.
        void RefreshToolBarArea();

    private:
        QMainWindow* m_mainWindow = nullptr;
        Qt::ToolBarArea m_toolBarArea = Qt::TopToolBarArea;

        AZStd::map<int, AZStd::vector<AZStd::string>> m_toolBars;
        AZStd::unordered_map<AZStd::string, int> m_toolBarToSortKeyMap;
        AZStd::vector<QToolBar*> m_toolBarsCache;

        inline static ToolBarManagerInterface* m_toolBarManagerInterface = nullptr;
        inline static ToolBarManagerInternalInterface* m_toolBarManagerInternalInterface = nullptr;
    };

} // namespace AzToolsFramework
