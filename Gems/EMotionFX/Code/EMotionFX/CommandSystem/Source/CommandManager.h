/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "SelectionList.h"
#include <MCore/Source/MCoreCommandManager.h>


namespace CommandSystem
{
    class COMMANDSYSTEM_API CommandManager
        : public MCore::CommandManager
    {
    public:
        /**
         * Default constructor.
         */
        CommandManager();

        /**
         * Destructor.
         */
        ~CommandManager();

        /**
         * Get current selection.
         * @return The selection list containing all selected actors, motions and nodes.
         */
        MCORE_INLINE SelectionList& GetCurrentSelection()                                   { m_currentSelection.MakeValid(); return m_currentSelection; }

        /**
         * Set current selection.
         * @param selection The selection list containing all selected actors, motions and nodes.
         */
        MCORE_INLINE void SetCurrentSelection(SelectionList& selection)                     { m_currentSelection.Clear(); m_currentSelection.Add(selection); }

        MCORE_INLINE bool GetLockSelection() const                                          { return m_lockSelection; }
        void SetLockSelection(bool lockSelection)                                           { m_lockSelection = lockSelection; }

        void SetWorkspaceDirtyFlag(bool dirty)                                              { m_workspaceDirtyFlag = dirty; }
        MCORE_INLINE bool GetWorkspaceDirtyFlag() const                                     { return m_workspaceDirtyFlag; }

        // Only true when user create or open a workspace.
        void SetUserOpenedWorkspaceFlag(bool flag);
        bool GetUserOpenedWorkspaceFlag() const                                             { return m_userOpenedWorkspaceFlag; }

    private:
        SelectionList       m_currentSelection;      /**< The current selected actors, motions and nodes. */
        bool                m_lockSelection;
        bool                m_workspaceDirtyFlag;
        bool                m_userOpenedWorkspaceFlag = false;
    };


    // get the command manager
    extern COMMANDSYSTEM_API CommandManager* GetCommandManager();
} // namespace CommandSystem
