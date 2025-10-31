/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManager.h>

#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>

#include <QWidget>

namespace AzToolsFramework
{
    ToolBarManager::ToolBarManager(QWidget* defaultParentWidget)
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "ToolBarManager - Could not retrieve instance of ActionManagerInterface");

        m_actionManagerInternalInterface = AZ::Interface<ActionManagerInternalInterface>::Get();
        AZ_Assert(m_actionManagerInternalInterface, "ToolBarManager - Could not retrieve instance of ActionManagerInternalInterface");

        AZ::Interface<ToolBarManagerInterface>::Register(this);
        AZ::Interface<ToolBarManagerInternalInterface>::Register(this);

        AZ::SystemTickBus::Handler::BusConnect();
        ActionManagerNotificationBus::Handler::BusConnect();

        EditorToolBar::Initialize(defaultParentWidget);
        ToolBarExpanderWatcher::Initialize();
        EditorToolBarArea::Initialize();
    }

    ToolBarManager::~ToolBarManager()
    {
        ActionManagerNotificationBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();

        AZ::Interface<ToolBarManagerInternalInterface>::Unregister(this);
        AZ::Interface<ToolBarManagerInterface>::Unregister(this);

        Reset();
    }

    void ToolBarManager::Reflect(AZ::ReflectContext* context)
    {
        EditorToolBar::Reflect(context);
    }
    
    ToolBarManagerOperationResult ToolBarManager::RegisterToolBar(const AZStd::string& toolBarIdentifier, const ToolBarProperties& properties)
    {
        if (m_toolBars.contains(toolBarIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("ToolBar Manager - Could not register toolbar \"%.s\" twice.", toolBarIdentifier.c_str()));
        }

        m_toolBars.insert(
            {
                toolBarIdentifier,
                EditorToolBar(properties.m_name)
            }
        );

        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::RegisterToolBarArea(
        const AZStd::string& toolBarAreaIdentifier, QMainWindow* mainWindow, Qt::ToolBarArea toolBarArea)
    {
        if (m_toolBarAreas.contains(toolBarAreaIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("ToolBar Manager - Could not register toolbar area \"%.s\" twice.", toolBarAreaIdentifier.c_str()));
        }

        m_toolBarAreas.insert(
            {
                toolBarAreaIdentifier,
                EditorToolBarArea(mainWindow, toolBarArea)
            }
        );

        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::AddActionToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, int sortIndex)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to toolbar \"%s\" - toolbar has not been registered.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to toolbar \"%s\" - action could not be found.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        if (toolBarIterator->second.ContainsAction(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to toolbar \"%s\" - toolbar already contains action.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        toolBarIterator->second.AddAction(sortIndex, actionIdentifier);
        m_actionsToToolBarsMap[actionIdentifier].insert(toolBarIdentifier);
        m_toolBarsToRefresh.insert(toolBarIdentifier);
        return AZ::Success();
    }
    
    ToolBarManagerOperationResult ToolBarManager::AddActionWithSubMenuToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to toolbar \"%s\" - toolbar has not been registered.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to toolbar \"%s\" - action could not be found.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        if (toolBarIterator->second.ContainsAction(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add action \"%s\" to toolbar \"%s\" - toolbar already contains action.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        toolBarIterator->second.AddActionWithSubMenu(sortIndex, actionIdentifier, subMenuIdentifier);
        m_actionsToToolBarsMap[actionIdentifier].insert(toolBarIdentifier);
        m_toolBarsToRefresh.insert(toolBarIdentifier);
        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::AddActionsToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add actions to toolbar \"%s\" - toolbar has not been registered.", toolBarIdentifier.c_str()));
        }

        AZStd::string errorMessage = AZStd::string::format(
            "ToolBar Manager - Errors on AddActionsToToolBar for toolbar \"%s\" - some actions were not added:", toolBarIdentifier.c_str());
        bool couldNotAddAction = false;

        for (const auto& pair : actions)
        {
            if (!m_actionManagerInterface->IsActionRegistered(pair.first))
            {
                errorMessage += AZStd::string(" ") + pair.first;
                couldNotAddAction = true;
                continue;
            }

            if (toolBarIterator->second.ContainsAction(pair.first))
            {
                errorMessage += AZStd::string(" ") + pair.first;
                couldNotAddAction = true;
                continue;
            }

            toolBarIterator->second.AddAction(pair.second, pair.first);
            m_actionsToToolBarsMap[pair.first].insert(toolBarIdentifier);
        }

        m_toolBarsToRefresh.insert(toolBarIdentifier);

        if (couldNotAddAction)
        {
            return AZ::Failure(errorMessage);
        }

        return AZ::Success();
    }
    
    ToolBarManagerOperationResult ToolBarManager::RemoveActionFromToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not remove action \"%s\" from toolbar \"%s\" - toolbar has not been registered.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not remove action \"%s\" from toolbar \"%s\" - action could not be found.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        if (!toolBarIterator->second.ContainsAction(actionIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not remove action \"%s\" from toolbar \"%s\" - toolbar does not contain action.", actionIdentifier.c_str(),
                toolBarIdentifier.c_str()));
        }

        toolBarIterator->second.RemoveAction(actionIdentifier);
        m_actionsToToolBarsMap[actionIdentifier].erase(toolBarIdentifier);

        m_toolBarsToRefresh.insert(toolBarIdentifier);
        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::RemoveActionsFromToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not remove actions from toolbar \"%s\" - toolbar has not been registered.", toolBarIdentifier.c_str()));
        }

        AZStd::string errorMessage = AZStd::string::format(
            "ToolBar Manager - Errors on RemoveActionsFromMenu for toolbar \"%s\" - some actions were not removed:", toolBarIdentifier.c_str());
        bool couldNotRemoveAction = false;

        for (const AZStd::string& actionIdentifier : actionIdentifiers)
        {
            if (!m_actionManagerInterface->IsActionRegistered(actionIdentifier))
            {
                errorMessage += AZStd::string(" ") + actionIdentifier;
                couldNotRemoveAction = true;
                continue;
            }

            if (!toolBarIterator->second.ContainsAction(actionIdentifier))
            {
                errorMessage += AZStd::string(" ") + actionIdentifier;
                couldNotRemoveAction = true;
                continue;
            }

            toolBarIterator->second.RemoveAction(actionIdentifier);
            m_actionsToToolBarsMap[actionIdentifier].erase(toolBarIdentifier);
        }

        m_toolBarsToRefresh.insert(toolBarIdentifier);

        if (couldNotRemoveAction)
        {
            return AZ::Failure(errorMessage);
        }

        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::AddSeparatorToToolBar(
        const AZStd::string& toolBarIdentifier, int sortIndex)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add separator - toolbar \"%s\" has not been registered.", toolBarIdentifier.c_str()));
        }

        toolBarIterator->second.AddSeparator(sortIndex);
        m_toolBarsToRefresh.insert(toolBarIdentifier);
        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::AddWidgetToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add widget - toolbar \"%s\" has not been registered.", toolBarIdentifier.c_str()));
        }

        if (!m_actionManagerInterface->IsWidgetActionRegistered(widgetActionIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format(
                    "ToolBar Manager - Could not add widget \"%s\" to toolbar \"%s\" - widget action was not registered.",
                    toolBarIdentifier.c_str(),
                    widgetActionIdentifier.c_str()
                )
            );
        }

        toolBarIterator->second.AddWidget(sortIndex, widgetActionIdentifier);
        m_toolBarsToRefresh.insert(toolBarIdentifier);

        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::AddToolBarToToolBarArea(
        const AZStd::string& toolBarAreaIdentifier, const AZStd::string& toolBarIdentifier, int sortIndex)
    {
        auto toolBarAreaIterator = m_toolBarAreas.find(toolBarAreaIdentifier);
        if (toolBarAreaIterator == m_toolBarAreas.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add toolbar \"%s\" to toolbar area \"%s\" - toolbar area has not been registered.",
                toolBarIdentifier.c_str(),
                toolBarAreaIdentifier.c_str()));
        }

        if (!m_toolBars.contains(toolBarIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add toolbar \"%s\" to toolbar area \"%s\" - toolbar has not been registered.",
                toolBarIdentifier.c_str(),
                toolBarAreaIdentifier.c_str()));
        }

        if (toolBarAreaIterator->second.ContainsToolBar(toolBarIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add toolbar \"%s\" to toolbar area \"%s\" - toolbar area already contains this toolbar.",
                toolBarIdentifier.c_str(),
                toolBarAreaIdentifier.c_str()));
        }

        toolBarAreaIterator->second.AddToolBar(sortIndex, toolBarIdentifier);
        m_toolBarAreasToRefresh.insert(toolBarAreaIdentifier);
        return AZ::Success();
    }

    QToolBar* ToolBarManager::GenerateToolBar(const AZStd::string& toolBarIdentifier)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return nullptr;
        }

        return toolBarIterator->second.GenerateToolBar();
    }
    
    ToolBarManagerIntegerResult ToolBarManager::GetSortKeyOfActionInToolBar(const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) const
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not get sort key of action \"%s\" in toolbar \"%s\" - toolbar has not been registered.", actionIdentifier.c_str(), toolBarIdentifier.c_str()));
        }

        auto sortKey = toolBarIterator->second.GetActionSortKey(actionIdentifier);
        if (!sortKey.has_value())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not get sort key of action \"%s\" in toolbar \"%s\" - action was not found in toolbar.", actionIdentifier.c_str(), toolBarIdentifier.c_str()));
        }

        return AZ::Success(sortKey.value());
    }

    ToolBarManagerIntegerResult ToolBarManager::GetSortKeyOfWidgetInToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier) const
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not get sort key of widget \"%s\" in toolbar \"%s\" - toolbar has not been registered.",
                widgetActionIdentifier.c_str(),
                toolBarIdentifier.c_str())
            );
        }

        auto sortKey = toolBarIterator->second.GetWidgetSortKey(widgetActionIdentifier);
        if (!sortKey.has_value())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not get sort key of widget \"%s\" in toolbar \"%s\" - widget was not found in toolbar.",
                widgetActionIdentifier.c_str(),
                toolBarIdentifier.c_str())
            );
        }

        return AZ::Success(sortKey.value());
    }

    ToolBarManagerOperationResult ToolBarManager::QueueToolBarRefresh(const AZStd::string& toolBarIdentifier)
    {
        if (!m_toolBars.contains(toolBarIdentifier))
        {
            return AZ::Failure(
                AZStd::string::format("ToolBar Manager - Could not refresh toolBar \"%.s\" as it is not registered.", toolBarIdentifier.c_str()));
        }

        m_toolBarsToRefresh.insert(toolBarIdentifier);
        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::QueueRefreshForToolBarsContainingAction(const AZStd::string& actionIdentifier)
    {
        auto actionIterator = m_actionsToToolBarsMap.find(actionIdentifier);

        if (actionIterator != m_actionsToToolBarsMap.end())
        {
            for (const AZStd::string& toolBarIdentifier : actionIterator->second)
            {
                m_toolBarsToRefresh.insert(toolBarIdentifier);
            }
        }

        return AZ::Success();
    }

    void ToolBarManager::RefreshToolBars()
    {
        for (const AZStd::string& toolBarIdentifier : m_toolBarsToRefresh)
        {
            auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
            if (toolBarIterator != m_toolBars.end())
            {
                toolBarIterator->second.RefreshToolBars();
            }
        }

        m_toolBarsToRefresh.clear();
    }

    void ToolBarManager::RefreshToolBarAreas()
    {
        for (const AZStd::string& toolBarAreaIdentifier : m_toolBarAreasToRefresh)
        {
            auto toolBarAreaIterator = m_toolBarAreas.find(toolBarAreaIdentifier);
            if (toolBarAreaIterator != m_toolBarAreas.end())
            {
                toolBarAreaIterator->second.RefreshToolBarArea();
            }
        }

        m_toolBarAreasToRefresh.clear();
    }

    ToolBarManagerStringResult ToolBarManager::SerializeToolBar(const AZStd::string& toolBarIdentifier)
    {
        if (!m_toolBars.contains(toolBarIdentifier))
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not serialize toolBar \"%.s\" as it is not registered.", toolBarIdentifier.c_str()));
        }

        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;

        // Generate toolbar dom using Json serialization system.
        AZ::JsonSerializationResult::ResultCode result =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), m_toolBars[toolBarIdentifier], settings);

        // Stringify dom to return it.
        if (result.HasDoneWork())
        {
            rapidjson::StringBuffer prefabBuffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabBuffer);
            document.Accept(writer);

            return AZ::Success(AZStd::string(prefabBuffer.GetString()));
        }

        return AZ::Failure(AZStd::string::format(
            "ToolBar Manager - Could not serialize toolbar \"%.s\" - serialization error.", toolBarIdentifier.c_str()));
    }

    void ToolBarManager::Reset()
    {
        // Reset all stored values that are registered by the environment after initialization.
        m_toolBars.clear();
        m_toolBarAreas.clear();

        m_actionsToToolBarsMap.clear();

        m_toolBarsToRefresh.clear();
        m_toolBarAreasToRefresh.clear();
    }

    void ToolBarManager::OnSystemTick()
    {
        RefreshToolBars();
        RefreshToolBarAreas();
    }

    void ToolBarManager::OnActionStateChanged(AZStd::string actionIdentifier)
    {
        // Only refresh the toolbar if the action state changing could result in the action being shown/hidden.
        if (m_actionManagerInternalInterface->GetActionToolBarVisibility(actionIdentifier) != ActionVisibility::AlwaysShow)
        {
            QueueRefreshForToolBarsContainingAction(actionIdentifier);
        }
    }

} // namespace AzToolsFramework
