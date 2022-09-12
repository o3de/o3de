/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ToolBar/EditorToolBarArea.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInternalInterface.h>

#include <QMainWindow>

namespace AzToolsFramework
{
    EditorToolBarArea::EditorToolBarArea(QMainWindow* mainWindow, Qt::ToolBarArea toolBarArea)
        : m_mainWindow(mainWindow)
        , m_toolBarArea(toolBarArea)
    {
    }
    
    void EditorToolBarArea::AddToolBar(int sortKey, AZStd::string toolBarIdentifier)
    {
        m_toolBarToSortKeyMap.insert(AZStd::make_pair(toolBarIdentifier, sortKey));
        m_toolBars[sortKey].push_back(AZStd::move(toolBarIdentifier));
    }
    
    bool EditorToolBarArea::ContainsToolBar(const AZStd::string& toolBarIdentifier) const
    {
        return m_toolBarToSortKeyMap.contains(toolBarIdentifier);
    }

    AZStd::optional<int> EditorToolBarArea::GetToolBarSortKey(const AZStd::string& toolBarIdentifier) const
    {
        auto toolBarIterator = m_toolBarToSortKeyMap.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBarToSortKeyMap.end())
        {
            return AZStd::nullopt;
        }

        return toolBarIterator->second;
    }

    void EditorToolBarArea::RefreshToolBarArea()
    {
        m_menuBar->clear();

        for (const auto& vectorIterator : m_menus)
        {
            for (const auto& menuIdentifier : vectorIterator.second)
            {
                if (QMenu* menu = m_menuManagerInternalInterface->GetMenu(menuIdentifier))
                {
                    m_menuBar->addMenu(menu);
                }
            }
        }
    }

    void EditorToolBarArea::Initialize(QWidget* defaultParentWidget)
    {
        m_defaultParentWidget = defaultParentWidget;

        m_toolBarManagerInterface = AZ::Interface<ToolBarManagerInterface>::Get();
        AZ_Assert(m_toolBarManagerInterface, "EditorMenuBar - Could not retrieve instance of ToolBarManagerInterface");

        m_toolBarManagerInternalInterface = AZ::Interface<ToolBarManagerInternalInterface>::Get();
        AZ_Assert(m_toolBarManagerInternalInterface, "EditorMenuBar - Could not retrieve instance of ToolBarManagerInternalInterface");
    }

    void EditorToolBarArea::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorToolBarArea>()->Field("ToolBars", &EditorToolBarArea::m_toolBars);
        }
    }

} // namespace AzToolsFramework
