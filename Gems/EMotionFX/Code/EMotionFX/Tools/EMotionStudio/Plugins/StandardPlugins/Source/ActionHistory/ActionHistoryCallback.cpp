/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActionHistoryCallback.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/EventManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <QListWidget>
#include <QTextEdit>
#include <QApplication>
#include <QHBoxLayout>

namespace EMStudio
{
    ActionHistoryCallback::ActionHistoryCallback(QListWidget* list)
        : MCore::CommandManagerCallback()
    {
        m_list = list;
        m_index = 0;
        m_isRemoving = false;
        m_groupExecuting = false;
        m_executedGroup = nullptr;
        m_numGroupCommands = 0;
        m_currentCommandIndex = 0;
        m_darkenedBrush.setColor(QColor(110, 110, 110));
        m_brush.setColor(QColor(200, 200, 200));
    }

    // Before executing a command.
    void ActionHistoryCallback::OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(group);

        if (command)
        {
            if (MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
            {
                m_tempString = command->GetName();
                const size_t numParameters = commandLine.GetNumParameters();
                for (size_t i = 0; i < numParameters; ++i)
                {
                    m_tempString += " -";
                    m_tempString += commandLine.GetParameterName(i);
                    m_tempString += " ";
                    m_tempString += commandLine.GetParameterValue(i);
                }
                MCore::LogDebugMsg(m_tempString.c_str());
            }
        }
    }

    // After executing a command.
    void ActionHistoryCallback::OnPostExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine, bool wasSuccess, const AZStd::string& outResult)
    {
        MCORE_UNUSED(group);
        MCORE_UNUSED(commandLine);
        MCORE_UNUSED(outResult);
        if (m_groupExecuting && m_executedGroup)
        {
            m_currentCommandIndex++;
            if (m_currentCommandIndex % 32 == 0)
            {
                EMotionFX::GetEventManager().OnProgressValue(((float)m_currentCommandIndex / (m_numGroupCommands + 1)) * 100.0f);
            }
        }

        if (command && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            m_tempString = AZStd::string::format("%sExecution of command '%s' %s", wasSuccess ?  "    " : "*** ", command->GetName(), wasSuccess ? "completed successfully" : " FAILED");
            MCore::LogDebugMsg(m_tempString.c_str()); 
        }
    }

    // Before executing a command group.
    void ActionHistoryCallback::OnPreExecuteCommandGroup(MCore::CommandGroup* group, bool undo)
    {
        if (!m_groupExecuting && group->GetNumCommands() > 64)
        {
            m_groupExecuting = true;
            m_executedGroup = group;
            m_currentCommandIndex = 0;
            m_numGroupCommands = group->GetNumCommands();

            GetManager()->SetAvoidRendering(true);

            EMotionFX::GetEventManager().OnProgressStart();

            m_tempString = AZStd::string::format("%s%s", undo ? "Undo: " : "", group->GetGroupName());
            EMotionFX::GetEventManager().OnProgressText(m_tempString.c_str());
        }

        if (group && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            m_tempString = AZStd::string::format("Starting %s of command group '%s'", undo ? "undo" : "execution", group->GetGroupName());
            MCore::LogDebugMsg(m_tempString.c_str());
        }
    }

    // After executing a command group.
    void ActionHistoryCallback::OnPostExecuteCommandGroup(MCore::CommandGroup* group, bool wasSuccess)
    {
        if (m_executedGroup == group)
        {
            EMotionFX::GetEventManager().OnProgressEnd();

            m_groupExecuting      = false;
            m_executedGroup       = nullptr;
            m_numGroupCommands    = 0;
            m_currentCommandIndex = 0;

            GetManager()->SetAvoidRendering(false);
        }

        if (group && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            m_tempString = AZStd::string::format("%sExecution of command group '%s' %s", wasSuccess ?  "    " : "*** ", group->GetGroupName(), wasSuccess ? "completed successfully" : " FAILED");
            MCore::LogDebugMsg(m_tempString.c_str());
        }
    }

    // Add a new item to the history.
    void ActionHistoryCallback::OnAddCommandToHistory(size_t historyIndex, MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        m_tempString = MCore::CommandManager::CommandHistoryEntry::ToString(group, command, m_index++).c_str();

        m_list->insertItem(aznumeric_caster(historyIndex), new QListWidgetItem(m_tempString.c_str(), m_list));
        m_list->setCurrentRow(aznumeric_caster(historyIndex));
    }

    // Remove an item from the history.
    void ActionHistoryCallback::OnRemoveCommand(size_t historyIndex)
    {
        // Remove the item.
        m_isRemoving = true;
        delete m_list->takeItem(aznumeric_caster(historyIndex));
        m_isRemoving = false;
    }

    // Set the current command.
    void ActionHistoryCallback::OnSetCurrentCommand(size_t index)
    {
        if (m_isRemoving)
        {
            return;
        }

        if (index == InvalidIndex)
        {
            m_list->setCurrentRow(-1);

            // Darken all history items.
            const int numCommands = m_list->count();
            for (int i = 0; i < numCommands; ++i)
            {
                m_list->item(i)->setForeground(m_darkenedBrush);
            }
            return;
        }

        // get the list of selected items
        m_list->setCurrentRow(aznumeric_caster(index));

        // Get the current history index.
        const size_t historyIndex = GetCommandManager()->GetHistoryIndex();
        if (historyIndex == InvalidIndex)
        {
            AZStd::string outResult;
            const size_t numRedos = index + 1;
            for (size_t i = 0; i < numRedos; ++i)
            {
                outResult.clear();
                const bool result = GetCommandManager()->Redo(outResult);
                if (!outResult.empty())
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }
        else if (historyIndex > index) // if we need to perform undo's
        {
            AZStd::string outResult;
            const ptrdiff_t numUndos = historyIndex - index;
            for (ptrdiff_t i = 0; i < numUndos; ++i)
            {
                // try to undo
                outResult.clear();
                const bool result = GetCommandManager()->Undo(outResult);
                if (!outResult.empty())
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }
        else if (historyIndex < index) // if we need to redo commands
        {
            AZStd::string outResult;
            const ptrdiff_t numRedos = index - historyIndex;
            for (ptrdiff_t i = 0; i < numRedos; ++i)
            {
                outResult.clear();
                const bool result = GetCommandManager()->Redo(outResult);
                if (!outResult.empty())
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }

        const int numCommands = static_cast<int>(GetCommandManager()->GetNumHistoryItems());
        for (int i = aznumeric_caster(index); i < numCommands; ++i)
        {
            m_list->item(i)->setForeground(m_darkenedBrush);
        }

        // Color enabled ones.
        for (int i = 0; i <= static_cast<int>(index); ++i)
        {
            m_list->item(i)->setForeground(m_brush);
        }
    }
} // namespace EMStudio
