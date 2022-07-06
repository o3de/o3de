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
        /**
        * Use this function instead of directly pushing the ExectionState onto the stack, to protect calling code from changes to the use
        * of the Lua runtime.
        */
        void ExecutionStatePush(lua_State* lua, ExecutionStateWeakPtr executionState);

        /**
        * Use this function instead of directly reading the ExectionState from the stack, to protect calling code from changes to the use of
        * the Lua runtime.
        */
        ExecutionStateWeakPtr ExecutionStateRead(lua_State* lua, int index);
    }
}
