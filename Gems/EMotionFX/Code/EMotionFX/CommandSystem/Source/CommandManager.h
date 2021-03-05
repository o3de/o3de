/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        MCORE_INLINE SelectionList& GetCurrentSelection()                                   { mCurrentSelection.MakeValid(); return mCurrentSelection; }

        /**
         * Set current selection.
         * @param selection The selection list containing all selected actors, motions and nodes.
         */
        MCORE_INLINE void SetCurrentSelection(SelectionList& selection)                     { mCurrentSelection.Clear(); mCurrentSelection.Add(selection); }

        MCORE_INLINE bool GetLockSelection() const                                          { return mLockSelection; }
        void SetLockSelection(bool lockSelection)                                           { mLockSelection = lockSelection; }

        void SetWorkspaceDirtyFlag(bool dirty)                                              { mWorkspaceDirtyFlag = dirty; }
        MCORE_INLINE bool GetWorkspaceDirtyFlag() const                                     { return mWorkspaceDirtyFlag; }

        // Only true when user create or open a workspace.
        void SetUserOpenedWorkspaceFlag(bool flag);
        bool GetUserOpenedWorkspaceFlag() const                                             { return m_userOpenedWorkspaceFlag; }

    private:
        SelectionList       mCurrentSelection;      /**< The current selected actors, motions and nodes. */
        bool                mLockSelection;
        bool                mWorkspaceDirtyFlag;
        bool                m_userOpenedWorkspaceFlag = false;
    };


    // get the command manager
    extern COMMANDSYSTEM_API CommandManager* GetCommandManager();
} // namespace CommandSystem
