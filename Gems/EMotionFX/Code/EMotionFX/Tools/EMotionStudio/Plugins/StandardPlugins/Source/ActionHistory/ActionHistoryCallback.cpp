/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mList = list;
        mIndex = 0;
        mIsRemoving = false;
        mGroupExecuting = false;
        mExecutedGroup = nullptr;
        mNumGroupCommands = 0;
        mCurrentCommandIndex = 0;
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
                mTempString = command->GetName();
                const uint32 numParameters = commandLine.GetNumParameters();
                for (uint32 i = 0; i < numParameters; ++i)
                {
                    mTempString += " -";
                    mTempString += commandLine.GetParameterName(i);
                    mTempString += " ";
                    mTempString += commandLine.GetParameterValue(i);
                }
                MCore::LogDebugMsg(mTempString.c_str());
            }
        }
    }

    // After executing a command.
    void ActionHistoryCallback::OnPostExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine, bool wasSuccess, const AZStd::string& outResult)
    {
        MCORE_UNUSED(group);
        MCORE_UNUSED(commandLine);
        MCORE_UNUSED(outResult);
        if (mGroupExecuting && mExecutedGroup)
        {
            mCurrentCommandIndex++;
            if (mCurrentCommandIndex % 32 == 0)
            {
                EMotionFX::GetEventManager().OnProgressValue(((float)mCurrentCommandIndex / (mNumGroupCommands + 1)) * 100.0f);
            }
        }

        if (command && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            mTempString = AZStd::string::format("%sExecution of command '%s' %s", wasSuccess ?  "    " : "*** ", command->GetName(), wasSuccess ? "completed successfully" : " FAILED");
            MCore::LogDebugMsg(mTempString.c_str()); 
        }
    }

    // Before executing a command group.
    void ActionHistoryCallback::OnPreExecuteCommandGroup(MCore::CommandGroup* group, bool undo)
    {
        if (!mGroupExecuting && group->GetNumCommands() > 64)
        {
            mGroupExecuting = true;
            mExecutedGroup = group;
            mCurrentCommandIndex = 0;
            mNumGroupCommands = group->GetNumCommands();

            GetManager()->SetAvoidRendering(true);

            EMotionFX::GetEventManager().OnProgressStart();

            mTempString = AZStd::string::format("%s%s", undo ? "Undo: " : "", group->GetGroupName());
            EMotionFX::GetEventManager().OnProgressText(mTempString.c_str());
        }

        if (group && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            mTempString = AZStd::string::format("Starting %s of command group '%s'", undo ? "undo" : "execution", group->GetGroupName());
            MCore::LogDebugMsg(mTempString.c_str());
        }
    }

    // After executing a command group.
    void ActionHistoryCallback::OnPostExecuteCommandGroup(MCore::CommandGroup* group, bool wasSuccess)
    {
        if (mExecutedGroup == group)
        {
            EMotionFX::GetEventManager().OnProgressEnd();

            mGroupExecuting      = false;
            mExecutedGroup       = nullptr;
            mNumGroupCommands    = 0;
            mCurrentCommandIndex = 0;

            GetManager()->SetAvoidRendering(false);
        }

        if (group && MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_DEBUG)
        {
            mTempString = AZStd::string::format("%sExecution of command group '%s' %s", wasSuccess ?  "    " : "*** ", group->GetGroupName(), wasSuccess ? "completed successfully" : " FAILED");
            MCore::LogDebugMsg(mTempString.c_str());
        }
    }

    // Add a new item to the history.
    void ActionHistoryCallback::OnAddCommandToHistory(uint32 historyIndex, MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        mTempString = MCore::CommandManager::CommandHistoryEntry::ToString(group, command, mIndex++).c_str();

        mList->insertItem(historyIndex, new QListWidgetItem(mTempString.c_str(), mList));
        mList->setCurrentRow(historyIndex);
    }

    // Remove an item from the history.
    void ActionHistoryCallback::OnRemoveCommand(uint32 historyIndex)
    {
        // Remove the item.
        mIsRemoving = true;
        delete mList->takeItem(historyIndex);
        mIsRemoving = false;
    }

    // Set the current command.
    void ActionHistoryCallback::OnSetCurrentCommand(uint32 index)
    {
        if (mIsRemoving)
        {
            return;
        }

        if (index == MCORE_INVALIDINDEX32)
        {
            mList->setCurrentRow(-1);

            // Darken all history items.
            const int numCommands = static_cast<int>(GetCommandManager()->GetNumHistoryItems());
            for (int i = 0; i < numCommands; ++i)
            {
                mList->item(i)->setForeground(m_darkenedBrush);
            }
            return;
        }

        // get the list of selected items
        mList->setCurrentRow(index);

        // Get the current history index.
        const uint32 historyIndex = GetCommandManager()->GetHistoryIndex();
        if (historyIndex == MCORE_INVALIDINDEX32)
        {
            AZStd::string outResult;
            const uint32 numRedos = index + 1;
            for (uint32 i = 0; i < numRedos; ++i)
            {
                outResult.clear();
                const bool result = GetCommandManager()->Redo(outResult);
                if (outResult.size() > 0)
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
            const int32 numUndos = historyIndex - index;
            for (int32 i = 0; i < numUndos; ++i)
            {
                // try to undo
                outResult.clear();
                const bool result = GetCommandManager()->Undo(outResult);
                if (outResult.size() > 0)
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
            const int32 numRedos = index - historyIndex;
            for (int32 i = 0; i < numRedos; ++i)
            {
                outResult.clear();
                const bool result = GetCommandManager()->Redo(outResult);
                if (outResult.size() > 0)
                {
                    if (!result)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                }
            }
        }

        // Darken disabled commands.
        const uint32 orgIndex = index;
        if (index == MCORE_INVALIDINDEX32)
        {
            index = 0;
        }

        const int numCommands = static_cast<int>(GetCommandManager()->GetNumHistoryItems());
        for (int i = index; i < numCommands; ++i)
        {
            mList->item(i)->setForeground(m_darkenedBrush);
        }

        // Color enabled ones.
        if (orgIndex != MCORE_INVALIDINDEX32)
        {
            for (int i = 0; i <= static_cast<int>(index); ++i)
            {
                mList->item(index)->setForeground(m_brush);
            }
        }
    }
} // namespace EMStudio
