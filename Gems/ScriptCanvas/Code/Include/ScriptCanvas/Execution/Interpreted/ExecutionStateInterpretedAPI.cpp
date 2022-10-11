/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/lua/lua.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#if defined(SC_RUNTIME_CHECKS_ENABLED)
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#endif
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
#if defined(SC_RUNTIME_CHECKS_ENABLED)
            if (!lua_islightuserdata(lua, index))
            {
                SC_RUNTIME_CHECK(false, "ExecutionStateRead: no lightuserdata at index: %d", index);
                return nullptr;
            }

            void* lightuserdata = lua_touserdata(lua, index);
            if (!lightuserdata)
            {
                SC_RUNTIME_CHECK(false, "ExecutionStateRead: null lightuserdata at index: %d", index);
                return nullptr;
            }

            ExecutionStateWeakPtr executionState = reinterpret_cast<ExecutionStateWeakPtr>(lightuserdata);
            if (executionState->m_lightUserDataMark != UserDataMark)
            {
                SC_RUNTIME_CHECK(false, "ExecutionStateRead: invalid state at index : % d", index);
                return nullptr;
            }

            return executionState;
#else
            return reinterpret_cast<ExecutionStateWeakPtr>(lua_touserdata(lua, index));
#endif
        }
    }
}
