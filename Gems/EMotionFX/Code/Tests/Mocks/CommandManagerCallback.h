/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace MCore
{
    class CommandManagerCallbackMock
        : public MCore::CommandManagerCallback
    {
    public:
        MOCK_METHOD3(OnPreExecuteCommand, void(MCore::CommandGroup*, MCore::Command*, const MCore::CommandLine&));
        MOCK_METHOD5(OnPostExecuteCommand, void(MCore::CommandGroup*, MCore::Command*, const MCore::CommandLine&, bool, const AZStd::string&));

        MOCK_METHOD2(OnPreUndoCommand, void(MCore::Command*, const MCore::CommandLine&));
        MOCK_METHOD2(OnPostUndoCommand, void(MCore::Command*, const MCore::CommandLine&));

        MOCK_METHOD2(OnPreExecuteCommandGroup, void(MCore::CommandGroup*, bool));
        MOCK_METHOD2(OnPostExecuteCommandGroup, void(MCore::CommandGroup*, bool));

        MOCK_METHOD4(OnAddCommandToHistory, void(size_t, MCore::CommandGroup*, MCore::Command*, const MCore::CommandLine&));
        MOCK_METHOD1(OnRemoveCommand, void(size_t));
        MOCK_METHOD1(OnSetCurrentCommand, void(size_t));
    };
} // namespace MCore
