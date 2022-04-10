/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/lua/lua.h>

#include <ScriptCanvas/Execution/Interpreted/ExecutionStateInterpretedAPI.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        void ExecutionStatePush(lua_State* lua, ExecutionStateWeakPtr executionState)
        {
            lua_pushlightuserdata(lua, executionState);
        }

        ExecutionStateWeakPtr ExecutionStateRead(lua_State* lua, int index)
        {
            AZ_Assert(lua_islightuserdata(lua, index), "ExecutionStateRead: no lightuserdata at index: %d", index);
            void* lightuserdata = lua_touserdata(lua, index);
            AZ_Assert(lightuserdata != nullptr, "ExecutionStateRead: null lightuserdata at index: %d", index);
            ExecutionStateWeakPtr executionState = reinterpret_cast<ExecutionStateWeakPtr>(lightuserdata);
            AZ_Assert(executionState->m_lightUserDataMark == UserDataMark, "ExecutionStateRead: invalid state at index: %d", index);
            return executionState;
        }
    }
}
