/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

struct lua_State;

namespace ScriptCanvas
{
    namespace Execution
    {
        void ExecutionStatePush(lua_State* lua, ExecutionStateWeakPtr executionState);

        ExecutionStateWeakPtr ExecutionStateRead(lua_State* lua, int index);
    }
}
