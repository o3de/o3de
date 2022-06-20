/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManager.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>

namespace AzToolsFramework
{
    ToolBarManager::ToolBarManager()
    {
        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(m_actionManagerInterface, "ToolBarManager - Could not retrieve instance of ActionManagerInterface");

        AZ::Interface<ToolBarManagerInterface>::Register(this);

        EditorToolBar::Initialize();
    }

    ToolBarManager::~ToolBarManager()
    {
        AZ::Interface<ToolBarManagerInterface>::Unregister(this);
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

        QAction* action = m_actionManagerInterface->GetAction(actionIdentifier);
        if (!action)
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
        return AZ::Success();
    }

    ToolBarManagerOperationResult ToolBarManager::AddSeparatorToToolBar(
        const AZStd::string& toolBarIdentifier, int sortIndex)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return AZ::Failure(AZStd::string::format(
                "ToolBar Manager - Could not add separator - menu \"%s\" has not been registered.", toolBarIdentifier.c_str()));
        }

        toolBarIterator->second.AddSeparator(sortIndex);
        return AZ::Success();
    }

    QToolBar* ToolBarManager::GetToolBar(const AZStd::string& toolBarIdentifier)
    {
        auto toolBarIterator = m_toolBars.find(toolBarIdentifier);
        if (toolBarIterator == m_toolBars.end())
        {
            return nullptr;
        }

        return toolBarIterator->second.GetToolBar();
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

} // namespace AzToolsFramework
