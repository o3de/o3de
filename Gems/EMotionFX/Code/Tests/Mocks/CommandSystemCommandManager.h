/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace CommandSystem
{
    class CommandManager
        : public MCore::CommandManager
    {
    public:
        virtual ~CommandManager() = default;

        MOCK_METHOD0(GetCurrentSelection, SelectionList&());
        MOCK_METHOD1(SetCurrentSelection, void(SelectionList& selection));
        MOCK_CONST_METHOD0(GetLockSelection, bool());
        MOCK_METHOD1(SetLockSelection, void(bool lockSelection));
        MOCK_METHOD1(SetWorkspaceDirtyFlag, void(bool dirty));
        MOCK_CONST_METHOD0(GetWorkspaceDirtyFlag, bool());
    };

    CommandManager* GetCommandManager()
    {
        static CommandManager manager;
        return &manager;
    }
} // namespace CommandSystem
