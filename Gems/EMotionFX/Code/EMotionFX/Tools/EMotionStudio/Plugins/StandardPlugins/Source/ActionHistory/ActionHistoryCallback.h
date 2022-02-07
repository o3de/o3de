/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/CommandManagerCallback.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/MCoreCommandManager.h>
#include <MCore/Source/CommandLine.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

#include <QBrush>
#include <QListWidget>
#include <QDialog>
#include <QTextEdit>
#endif


namespace EMStudio
{
    class ActionHistoryCallback
        : public MCore::CommandManagerCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(ActionHistoryCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        ActionHistoryCallback(QListWidget* list);

        void OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine) override;
        void OnPostExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine, bool wasSuccess, const AZStd::string& outResult) override;
        void OnAddCommandToHistory(size_t historyIndex, MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine) override;
        void OnPreExecuteCommandGroup(MCore::CommandGroup* group, bool undo) override;
        void OnPostExecuteCommandGroup(MCore::CommandGroup* group, bool wasSuccess) override;
        void OnRemoveCommand(size_t historyIndex) override;
        void OnSetCurrentCommand(size_t index) override;

    private:
        QListWidget* m_list;
        AZStd::string m_tempString;
        uint32 m_index;
        bool m_isRemoving;

        bool m_groupExecuting;
        MCore::CommandGroup* m_executedGroup;
        size_t m_numGroupCommands;
        uint32 m_currentCommandIndex;
        QBrush m_brush;
        QBrush m_darkenedBrush;
    };
} // namespace EMStudio
