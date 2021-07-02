/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace MCore
{
    class CommandCallbackMock
        : public MCore::Command::Callback
    {
    public:
        CommandCallbackMock(bool executePreUndo, bool executePreCommand = false)
            : MCore::Command::Callback(executePreUndo, executePreCommand)
        {
        }

        MOCK_METHOD2(Execute, bool(MCore::Command*, const MCore::CommandLine&));
        MOCK_METHOD2(Undo, bool(MCore::Command*, const MCore::CommandLine&));
    };
} // namespace MCore
